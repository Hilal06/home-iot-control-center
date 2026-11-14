use axum::{
    Json,
    extract::{Path, Query, State},
    http::StatusCode,
};
use serde::Deserialize;
use serde_json::json;

use crate::{
    db::repository::{
        create_device, delete_device, get_actuator_events, get_device_by_id,
        get_device_state, get_telemetry_history, list_devices, set_device_lock,
        update_device, update_device_buttons, update_device_rules,
    },
    models::device::{
        CreateDeviceRequest, Device, ToggleLockRequest, UpdateDeviceButtonsRequest,
        UpdateDeviceRequest, UpdateDeviceRulesRequest,
    },
    state::AppState,
};

#[derive(Deserialize)]
pub struct PaginationQuery {
    pub limit: Option<i64>,
}

// Reusable Ponytail helper: fetch device or 404
async fn fetch_device(db: &sqlx::PgPool, device_id: &str) -> Result<Device, (StatusCode, String)> {
    get_device_by_id(db, device_id)
        .await
        .map_err(|e| (StatusCode::INTERNAL_SERVER_ERROR, e.to_string()))?
        .ok_or_else(|| (StatusCode::NOT_FOUND, format!("Device '{}' not found", device_id)))
}

// Reusable Ponytail helper: fetch device ensuring it is not locked
async fn fetch_unlocked_device(db: &sqlx::PgPool, device_id: &str) -> Result<Device, (StatusCode, String)> {
    let dev = fetch_device(db, device_id).await?;
    if dev.is_locked {
        return Err((
            StatusCode::LOCKED,
            format!("Device '{}' is LOCKED. Unlock device before modifying.", dev.name),
        ));
    }
    Ok(dev)
}

pub async fn health_check(State(state): State<AppState>) -> Json<serde_json::Value> {
    let uptime = state.start_time.elapsed().as_secs();
    Json(json!({
        "status": "healthy",
        "service": "Home-IOT-Backend",
        "uptime_seconds": uptime,
        "mqtt_broker": format!("{}:{}", state.config.mqtt_broker_host, state.config.mqtt_broker_port),
        "telemetry_topic": state.config.mqtt_telemetry_topic,
        "command_topic": state.config.mqtt_command_topic,
    }))
}

pub async fn get_devices(
    State(state): State<AppState>,
) -> Result<Json<serde_json::Value>, (StatusCode, String)> {
    let devices = list_devices(&state.db)
        .await
        .map_err(|e| (StatusCode::INTERNAL_SERVER_ERROR, e.to_string()))?;

    Ok(Json(json!({ "devices": devices, "count": devices.len() })))
}

pub async fn get_device_handler(
    State(state): State<AppState>,
    Path(device_id): Path<String>,
) -> Result<Json<serde_json::Value>, (StatusCode, String)> {
    let dev = fetch_device(&state.db, &device_id).await?;
    Ok(Json(json!({ "device": dev })))
}

pub async fn create_device_handler(
    State(state): State<AppState>,
    Json(req): Json<CreateDeviceRequest>,
) -> Result<(StatusCode, Json<serde_json::Value>), (StatusCode, String)> {
    let clean_id = req.id.trim().to_lowercase().replace(' ', "_");
    if clean_id.is_empty() || !clean_id.chars().all(|c| c.is_ascii_alphanumeric() || c == '_' || c == '-') {
        return Err((
            StatusCode::BAD_REQUEST,
            "Device ID must contain only lowercase alphanumeric characters, underscores, and dashes (no slashes or wildcards)".into(),
        ));
    }

    if get_device_by_id(&state.db, &clean_id).await.map_err(|e| (StatusCode::INTERNAL_SERVER_ERROR, e.to_string()))?.is_some() {
        return Err((StatusCode::CONFLICT, format!("Device with ID '{}' already exists", clean_id)));
    }

    let mut clean_req = req;
    clean_req.id = clean_id;

    let created = create_device(&state.db, &clean_req)
        .await
        .map_err(|e| (StatusCode::INTERNAL_SERVER_ERROR, e.to_string()))?;

    Ok((StatusCode::CREATED, Json(json!({ "status": "created", "device": created }))))
}

pub async fn update_device_handler(
    State(state): State<AppState>,
    Path(device_id): Path<String>,
    Json(req): Json<UpdateDeviceRequest>,
) -> Result<Json<serde_json::Value>, (StatusCode, String)> {
    fetch_unlocked_device(&state.db, &device_id).await?;

    let updated = update_device(&state.db, &device_id, &req)
        .await
        .map_err(|e| (StatusCode::INTERNAL_SERVER_ERROR, e.to_string()))?;

    Ok(Json(json!({ "status": "updated", "device": updated })))
}

pub async fn delete_device_handler(
    State(state): State<AppState>,
    Path(device_id): Path<String>,
) -> Result<Json<serde_json::Value>, (StatusCode, String)> {
    if device_id == "esp32_master" {
        return Err((StatusCode::FORBIDDEN, "Cannot delete primary Edge Gateway node 'esp32_master'".into()));
    }

    fetch_unlocked_device(&state.db, &device_id).await?;

    delete_device(&state.db, &device_id)
        .await
        .map_err(|e| (StatusCode::INTERNAL_SERVER_ERROR, e.to_string()))?;

    Ok(Json(json!({ "status": "deleted", "device_id": device_id })))
}

pub async fn toggle_device_lock_handler(
    State(state): State<AppState>,
    Path(device_id): Path<String>,
    Json(req): Json<ToggleLockRequest>,
) -> Result<Json<serde_json::Value>, (StatusCode, String)> {
    let dev = fetch_device(&state.db, &device_id).await?;
    let target_lock = req.is_locked.unwrap_or(!dev.is_locked);

    set_device_lock(&state.db, &device_id, target_lock)
        .await
        .map_err(|e| (StatusCode::INTERNAL_SERVER_ERROR, e.to_string()))?;

    Ok(Json(json!({
        "device_id": device_id,
        "is_locked": target_lock,
        "allow_automation_when_locked": dev.allow_automation_when_locked,
        "message": if target_lock { "Device is now LOCKED." } else { "Device is now UNLOCKED." }
    })))
}

pub async fn update_device_buttons_handler(
    State(state): State<AppState>,
    Path(device_id): Path<String>,
    Json(req): Json<UpdateDeviceButtonsRequest>,
) -> Result<Json<serde_json::Value>, (StatusCode, String)> {
    fetch_unlocked_device(&state.db, &device_id).await?;

    update_device_buttons(&state.db, &device_id, &req.buttons)
        .await
        .map_err(|e| (StatusCode::INTERNAL_SERVER_ERROR, e.to_string()))?;

    Ok(Json(json!({ "device_id": device_id, "status": "buttons_updated", "buttons": req.buttons })))
}

pub async fn update_device_rules_handler(
    State(state): State<AppState>,
    Path(device_id): Path<String>,
    Json(req): Json<UpdateDeviceRulesRequest>,
) -> Result<Json<serde_json::Value>, (StatusCode, String)> {
    fetch_unlocked_device(&state.db, &device_id).await?;

    update_device_rules(&state.db, &device_id, &req.automation_rules)
        .await
        .map_err(|e| (StatusCode::INTERNAL_SERVER_ERROR, e.to_string()))?;

    Ok(Json(json!({ "device_id": device_id, "status": "rules_updated", "automation_rules": req.automation_rules })))
}

pub async fn get_device_status(
    State(state): State<AppState>,
    Path(device_id): Path<String>,
) -> Result<Json<serde_json::Value>, (StatusCode, String)> {
    let db_state = get_device_state(&state.db, &device_id)
        .await
        .map_err(|e| (StatusCode::INTERNAL_SERVER_ERROR, e.to_string()))?;

    let latest_telemetry = state.latest_telemetry.read().await.clone();

    Ok(Json(json!({ "device_id": device_id, "state": db_state, "latest_telemetry": latest_telemetry })))
}

pub async fn get_device_telemetry_latest(
    State(state): State<AppState>,
    Path(device_id): Path<String>,
) -> Result<Json<serde_json::Value>, (StatusCode, String)> {
    if let Some(telem) = state.latest_telemetry.read().await.clone() {
        return Ok(Json(json!({ "device_id": device_id, "source": "in_memory_cache", "telemetry": telem })));
    }

    let history = get_telemetry_history(&state.db, &device_id, 1)
        .await
        .map_err(|e| (StatusCode::INTERNAL_SERVER_ERROR, e.to_string()))?;

    if let Some(record) = history.into_iter().next() {
        Ok(Json(json!({ "device_id": device_id, "source": "database_history", "telemetry": record })))
    } else {
        Err((StatusCode::NOT_FOUND, format!("No telemetry available for device '{}'", device_id)))
    }
}

pub async fn get_device_telemetry_history_handler(
    State(state): State<AppState>,
    Path(device_id): Path<String>,
    Query(query): Query<PaginationQuery>,
) -> Result<Json<serde_json::Value>, (StatusCode, String)> {
    let limit = query.limit.unwrap_or(50).clamp(1, 1000);
    let records = get_telemetry_history(&state.db, &device_id, limit)
        .await
        .map_err(|e| (StatusCode::INTERNAL_SERVER_ERROR, e.to_string()))?;

    Ok(Json(json!({ "device_id": device_id, "count": records.len(), "limit": limit, "records": records })))
}

pub async fn get_device_events_handler(
    State(state): State<AppState>,
    Path(device_id): Path<String>,
    Query(query): Query<PaginationQuery>,
) -> Result<Json<serde_json::Value>, (StatusCode, String)> {
    let limit = query.limit.unwrap_or(50).clamp(1, 500);
    let events = get_actuator_events(&state.db, &device_id, limit)
        .await
        .map_err(|e| (StatusCode::INTERNAL_SERVER_ERROR, e.to_string()))?;

    Ok(Json(json!({ "device_id": device_id, "count": events.len(), "limit": limit, "events": events })))
}
