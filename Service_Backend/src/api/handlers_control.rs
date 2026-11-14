use axum::{
    Json,
    extract::{Path, State},
    http::{HeaderMap, StatusCode},
};
use rumqttc::QoS;
use serde_json::json;
use tracing::{error, info, warn};

use crate::{
    db::repository::{get_device_by_id, save_actuator_event},
    models::command::ControlCommandPayload,
    state::AppState,
};

pub async fn control_device_handler(
    State(state): State<AppState>,
    headers: HeaderMap,
    Path(device_id): Path<String>,
    Json(payload): Json<ControlCommandPayload>,
) -> Result<Json<serde_json::Value>, (StatusCode, String)> {
    // 0. Validate device_id format to prevent MQTT topic path traversal or wildcard injection
    if !device_id.chars().all(|c| c.is_ascii_alphanumeric() || c == '_' || c == '-') {
        return Err((StatusCode::BAD_REQUEST, "Invalid device_id format".into()));
    }

    // Verify whether request carries trusted internal service token or admin authorization
    let expected_key = std::env::var("IOT_ADMIN_KEY").unwrap_or_else(|_| "iot_admin_secret_key_2026".to_string());
    let internal_auth = headers
        .get("x-internal-token")
        .or_else(|| headers.get("x-admin-key"))
        .and_then(|v| v.to_str().ok())
        .map(|token| token == expected_key)
        .unwrap_or(false);

    // 1. Check Smart Lock status on target device
    let device_opt = get_device_by_id(&state.db, &device_id)
        .await
        .map_err(|e| (StatusCode::INTERNAL_SERVER_ERROR, e.to_string()))?;

    if let Some(dev) = device_opt {
        if dev.is_locked {
            // Untrusted external HTTP callers cannot bypass lock by forging source=AUTOMATION or force=true
            let is_automated = internal_auth && matches!(
                payload.source.to_uppercase().as_str(),
                "SCHEDULE" | "AUTOMATION" | "DEVICE_TRIGGER" | "EMERGENCY"
            );

            if !is_automated && !(internal_auth && payload.force) {
                return Err((
                    StatusCode::LOCKED,
                    format!("Device '{}' is LOCKED. Manual/unauthorized commands rejected.", dev.name),
                ));
            }
        }
    }

    // 2. Validate command constraints
    if let Err(err) = payload.command.validate() {
        return Err((StatusCode::BAD_REQUEST, format!("Invalid command: {}", err)));
    }

    let verified_source = if internal_auth {
        payload.source
    } else {
        "MANUAL".to_string()
    };

    let mut mqtt_json = payload.command.to_mqtt_payload();
    if let Some(obj) = mqtt_json.as_object_mut() {
        obj.insert("device_id".to_string(), json!(device_id));
        obj.insert("source".to_string(), json!(verified_source));
    }

    let command_name = mqtt_json
        .get("command")
        .and_then(|v| v.as_str())
        .unwrap_or("unknown")
        .to_string();

    let target = mqtt_json
        .get("room")
        .or_else(|| mqtt_json.get("valve_id"))
        .or_else(|| mqtt_json.get("channel"))
        .map(|v| v.to_string());

    info!(
        "Received control command '{}' for device '{}' [Source: {}]",
        command_name, device_id, verified_source
    );

    // 3. Determine topic routing
    let target_topic = if device_id == "esp32_master" {
        state.config.mqtt_command_topic.clone()
    } else {
        format!("home/actuators/{}/control", device_id)
    };

    let payload_bytes = serde_json::to_vec(&mqtt_json).unwrap_or_default();
    let mqtt_result = state
        .mqtt_client
        .publish(&target_topic, QoS::AtLeastOnce, false, payload_bytes.clone())
        .await;

    // Cross-publish peripheral commands to global command topic for centralized audit listeners
    if device_id != "esp32_master" && mqtt_result.is_ok() {
        let _ = state
            .mqtt_client
            .publish(&state.config.mqtt_command_topic, QoS::AtLeastOnce, false, payload_bytes)
            .await;
    }

    // 4. Resolve delivery channel (MQTT or HTTP fallback)
    let (channel, is_success, device_response) = match mqtt_result {
        Ok(_) => ("MQTT", true, None),
        Err(mqtt_err) if device_id == "esp32_master" => {
            warn!("MQTT dispatch failed ({:?}), trying HTTP fallback...", mqtt_err);
            match state.http_fallback.send_command(&mqtt_json).await {
                Ok(resp) => ("HTTP_FALLBACK", true, Some(resp)),
                Err(http_err) => {
                    error!("Both MQTT and HTTP failed for '{}': {:?}", command_name, http_err);
                    ("FAILED", false, None)
                }
            }
        }
        Err(mqtt_err) => {
            warn!("MQTT dispatch failed for peripheral device '{}': {:?}", device_id, mqtt_err);
            ("FAILED", false, None)
        }
    };

    // 5. Single audit log call (Ponytail deduplication)
    let _ = save_actuator_event(
        &state.db,
        &device_id,
        &command_name,
        target.as_deref(),
        &mqtt_json,
        if is_success { &verified_source } else { "FAILED" },
        is_success,
    ).await;

    if is_success {
        let mut resp = json!({
            "status": "success",
            "channel": channel,
            "device_id": device_id,
            "command": command_name,
            "source": verified_source,
            "payload": mqtt_json
        });
        if let Some(r) = device_response {
            resp["device_response"] = r;
        }
        Ok(Json(resp))
    } else {
        Err((
            StatusCode::BAD_GATEWAY,
            format!("Failed to deliver command '{}' to device '{}'", command_name, device_id),
        ))
    }
}
