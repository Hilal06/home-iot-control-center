use axum::{
    Json,
    extract::{Path, State},
    http::StatusCode,
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
    Path(device_id): Path<String>,
    Json(payload): Json<ControlCommandPayload>,
) -> Result<Json<serde_json::Value>, (StatusCode, String)> {
    // 1. Check Smart Lock status on target device
    let device_opt = get_device_by_id(&state.db, &device_id)
        .await
        .map_err(|e| (StatusCode::INTERNAL_SERVER_ERROR, e.to_string()))?;

    if let Some(dev) = device_opt {
        if dev.is_locked {
            let is_automated = matches!(
                payload.source.to_uppercase().as_str(),
                "SCHEDULE" | "AUTOMATION" | "DEVICE_TRIGGER" | "EMERGENCY"
            );

            if !is_automated && !payload.force {
                return Err((
                    StatusCode::LOCKED,
                    format!("Device '{}' is LOCKED. Manual commands rejected.", dev.name),
                ));
            }
        }
    }

    // 2. Validate command constraints
    if let Err(err) = payload.command.validate() {
        return Err((StatusCode::BAD_REQUEST, format!("Invalid command: {}", err)));
    }

    let mut mqtt_json = payload.command.to_mqtt_payload();
    if let Some(obj) = mqtt_json.as_object_mut() {
        obj.insert("device_id".to_string(), json!(device_id));
        obj.insert("source".to_string(), json!(payload.source));
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
        command_name, device_id, payload.source
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
        if is_success { &payload.source } else { "FAILED" },
        is_success,
    ).await;

    if is_success {
        let mut resp = json!({
            "status": "success",
            "channel": channel,
            "device_id": device_id,
            "command": command_name,
            "source": payload.source,
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
