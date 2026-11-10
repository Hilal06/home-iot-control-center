use sqlx::PgPool;
use std::sync::Arc;
use std::time::Instant;
use tokio::sync::{RwLock, broadcast};

use crate::config::Config;
use crate::fallback::http_client::EspHttpClient;
use crate::models::telemetry::TelemetryPayload;

#[derive(Clone)]
pub struct AppState {
    pub db: PgPool,
    pub mqtt_client: rumqttc::AsyncClient,
    pub broadcast_tx: broadcast::Sender<TelemetryPayload>,
    pub latest_telemetry: Arc<RwLock<Option<TelemetryPayload>>>,
    pub config: Config,
    pub http_fallback: Arc<EspHttpClient>,
    pub start_time: Instant,
    pub redis_conn: Option<redis::aio::MultiplexedConnection>,
}

impl AppState {
    pub async fn new(
        db: PgPool,
        mqtt_client: rumqttc::AsyncClient,
        config: Config,
    ) -> (Self, broadcast::Receiver<TelemetryPayload>) {
        let (broadcast_tx, broadcast_rx) = broadcast::channel(100);
        let http_fallback = Arc::new(EspHttpClient::new(config.esp32_direct_http_url.clone()));

        let redis_conn = match redis::Client::open(config.redis_url.clone()) {
            Ok(client) => client.get_multiplexed_tokio_connection().await.ok(),
            Err(_) => None,
        };

        let state = Self {
            db,
            mqtt_client,
            broadcast_tx,
            latest_telemetry: Arc::new(RwLock::new(None)),
            config,
            http_fallback,
            start_time: Instant::now(),
            redis_conn,
        };

        (state, broadcast_rx)
    }
}
