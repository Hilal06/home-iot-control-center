use axum::{
    Router,
    routing::{get, post, put},
};
use tower_http::cors::{Any, CorsLayer};
use tower_http::trace::TraceLayer;

use super::{
    handlers_control::control_device_handler,
    handlers_devices::{
        create_device_handler, delete_device_handler, get_device_events_handler,
        get_device_handler, get_device_status, get_device_telemetry_history_handler,
        get_device_telemetry_latest, get_devices, health_check,
        toggle_device_lock_handler, update_device_buttons_handler,
        update_device_handler, update_device_rules_handler,
    },
    ws::ws_handler,
};
use crate::state::AppState;

pub fn create_router(state: AppState) -> Router {
    let cors = CorsLayer::new()
        .allow_origin(Any)
        .allow_methods(Any)
        .allow_headers(Any);

    Router::new()
        .route("/health", get(health_check))
        .route("/api/devices", get(get_devices).post(create_device_handler))
        .route(
            "/api/devices/{id}",
            get(get_device_handler)
                .put(update_device_handler)
                .delete(delete_device_handler),
        )
        .route("/api/devices/{id}/lock", post(toggle_device_lock_handler))
        .route("/api/devices/{id}/buttons", put(update_device_buttons_handler))
        .route("/api/devices/{id}/rules", put(update_device_rules_handler))
        .route("/api/devices/{id}/status", get(get_device_status))
        .route("/api/devices/{id}/control", post(control_device_handler))
        .route(
            "/api/devices/{id}/telemetry/latest",
            get(get_device_telemetry_latest),
        )
        .route(
            "/api/devices/{id}/telemetry/history",
            get(get_device_telemetry_history_handler),
        )
        .route("/api/devices/{id}/events", get(get_device_events_handler))
        .route("/api/ws", get(ws_handler))
        .layer(cors)
        .layer(TraceLayer::new_for_http())
        .with_state(state)
}
