use anyhow::Result;
use std::time::Duration;
use tokio::net::TcpListener;
use tracing::{info, warn};

mod api;
mod config;
mod db;
mod fallback;
mod models;
mod mqtt;
mod state;

use api::routes::create_router;
use config::Config;
use db::repository::{create_db_pool, run_migrations};
use mqtt::client::create_mqtt_client;
use mqtt::handler::start_mqtt_listener;
use state::AppState;

#[tokio::main]
async fn main() -> Result<()> {
    // Initialize structured logging
    tracing_subscriber::fmt()
        .with_env_filter(
            tracing_subscriber::EnvFilter::try_from_default_env()
                .unwrap_or_else(|_| "home_iot_backend=info,tower_http=debug".into()),
        )
        .init();

    info!("Starting Home IoT Service Backend (Rust + Axum + rumqttc)...");

    // Load configuration
    let config = Config::from_env();
    info!("Configuration loaded: Server port {}", config.server_port);

    // 1. Connect to Database with Retry (useful for Docker Compose startup order)
    let pool = {
        let mut connected_pool = None;
        for attempt in 1..=10 {
            match create_db_pool(&config.database_url).await {
                Ok(p) => {
                    info!("Database connected successfully on attempt {}", attempt);
                    if let Err(e) = run_migrations(&p).await {
                        warn!("Migration warning: {:?}", e);
                    }
                    connected_pool = Some(p);
                    break;
                }
                Err(e) => {
                    warn!(
                        "Database connection attempt {}/10 failed: {:?}. Retrying in 2s...",
                        attempt, e
                    );
                    tokio::time::sleep(Duration::from_secs(2)).await;
                }
            }
        }
        connected_pool.expect("Failed to connect to PostgreSQL/TimescaleDB after 10 attempts")
    };

    // 2. Setup MQTT Client
    let (mqtt_client, eventloop) = create_mqtt_client(
        &config.mqtt_client_id,
        &config.mqtt_broker_host,
        config.mqtt_broker_port,
    )?;

    // 3. Build App State
    let (state, _) = AppState::new(pool, mqtt_client, config.clone()).await;

    // 4. Spawn Background MQTT Listener
    tokio::spawn(start_mqtt_listener(eventloop, state.clone()));

    // 4b. Spawn Device Timeout & Health Watchdog Task (Checks every 3s)
    let watchdog_db = state.db.clone();
    tokio::spawn(async move {
        let mut interval = tokio::time::interval(Duration::from_secs(3));
        loop {
            interval.tick().await;
            let res = sqlx::query(
                r#"
                UPDATE devices
                SET status = 'OFFLINE'
                WHERE status = 'ONLINE' AND (last_seen IS NULL OR last_seen < NOW() - INTERVAL '5 seconds')
                RETURNING id
                "#
            )
            .fetch_all(&watchdog_db)
            .await;

            if let Ok(rows) = res {
                for r in rows {
                    use sqlx::Row;
                    let dev_id: String = r.get("id");
                    warn!("Device '{}' heartbeat timeout (> 5s). Marked as OFFLINE.", dev_id);
                }
            }
        }
    });

    // 5. Build Axum Router
    let app = create_router(state);

    // 6. Bind HTTP Server
    let bind_addr = format!("{}:{}", config.server_host, config.server_port);
    info!("Binding HTTP and WebSocket server on {}", bind_addr);
    let listener = TcpListener::bind(&bind_addr).await?;

    info!("Home IoT Backend is live and ready on http://{}", bind_addr);
    info!("Health check: http://{}/health", bind_addr);
    info!("WebSocket stream: ws://{}/api/ws", bind_addr);

    // Run Axum server with graceful shutdown
    axum::serve(listener, app)
        .with_graceful_shutdown(shutdown_signal())
        .await?;

    info!("Service Backend stopped gracefully.");
    Ok(())
}

async fn shutdown_signal() {
    let ctrl_c = async {
        tokio::signal::ctrl_c()
            .await
            .expect("failed to install Ctrl+C handler");
    };

    #[cfg(unix)]
    let terminate = async {
        tokio::signal::unix::signal(tokio::signal::unix::SignalKind::terminate())
            .expect("failed to install signal handler")
            .recv()
            .await;
    };

    #[cfg(not(unix))]
    let terminate = std::future::pending::<()>();

    tokio::select! {
        _ = ctrl_c => {},
        _ = terminate => {},
    }

    info!("Signal received, initiating graceful shutdown...");
}
