use redis::AsyncCommands;
use rumqttc::{Event, EventLoop, Packet, QoS};
use std::time::Duration;
use tracing::{error, info, warn};

use crate::db::repository::save_telemetry;
use crate::models::telemetry::TelemetryPayload;
use crate::state::AppState;

pub async fn start_mqtt_listener(mut eventloop: EventLoop, state: AppState) {
    info!("Starting MQTT eventloop listener task...");

    loop {
        match eventloop.poll().await {
            Ok(Event::Incoming(Packet::ConnAck(connack))) => {
                info!("MQTT broker connected! Code: {:?}", connack.code);

                let client = state.mqtt_client.clone();
                let base_topic = state.config.mqtt_telemetry_topic.clone();

                tokio::spawn(async move {
                    for topic in &[
                        base_topic.as_str(),
                        "home/sensors/+/telemetry",
                        "home/+/telemetry",
                        "home/+/status",
                    ] {
                        if let Err(e) = client.subscribe(*topic, QoS::AtLeastOnce).await {
                            warn!("Subscription note for topic '{}': {:?}", topic, e);
                        } else {
                            info!("Successfully subscribed to topic '{}'", topic);
                        }
                    }
                });
            }
            Ok(Event::Incoming(Packet::Publish(publish))) => {
                let topic = &publish.topic;
                let is_telemetry_topic = topic == &state.config.mqtt_telemetry_topic
                    || (topic.starts_with("home/sensors/") && topic.ends_with("/telemetry"))
                    || (topic.starts_with("home/") && topic.ends_with("/telemetry"));

                let is_status_topic = topic.ends_with("/status");

                if is_status_topic {
                    if let Ok(val) = serde_json::from_slice::<serde_json::Value>(&publish.payload) {
                        let status = val.get("status").and_then(|v| v.as_str()).unwrap_or("OFFLINE");
                        let dev_id = val.get("device_id").and_then(|v| v.as_str()).unwrap_or("esp32_master");
                        info!("MQTT status update received: device '{}' is {}", dev_id, status);

                        let db_pool = state.db.clone();
                        let d_id = dev_id.to_string();
                        let st = status.to_string();
                        tokio::spawn(async move {
                            let _ = sqlx::query(
                                "UPDATE devices SET status = $1, last_seen = CASE WHEN $1 = 'ONLINE' THEN NOW() ELSE last_seen END WHERE id = $2"
                            )
                            .bind(&st)
                            .bind(&d_id)
                            .execute(&db_pool)
                            .await;
                        });

                        // Forward status event to Redis Pub/Sub for Spring Boot Business Engine
                        if let Some(mut conn) = state.redis_conn.clone() {
                            let event_json = serde_json::json!({
                                "device_id": dev_id,
                                "status": status,
                                "timestamp": chrono::Utc::now().timestamp_millis()
                            });
                            let _ = conn.publish::<_, _, ()>("iot:device:status", event_json.to_string()).await;
                        }
                    }
                } else if is_telemetry_topic {
                    match serde_json::from_slice::<TelemetryPayload>(&publish.payload) {
                        Ok(mut telem) => {
                            // Extract device_id without heap vector allocations
                            let device_id = telem.device_id.clone().unwrap_or_else(|| {
                                topic.strip_prefix("home/sensors/")
                                    .and_then(|s| s.strip_suffix("/telemetry"))
                                    .or_else(|| topic.strip_prefix("home/").and_then(|s| s.strip_suffix("/telemetry")))
                                    .unwrap_or("esp32_master")
                                    .to_string()
                            });

                            telem.device_id = Some(device_id.clone());

                            // 1. Update in-memory latest snapshot
                            {
                                let mut lock = state.latest_telemetry.write().await;
                                *lock = Some(telem.clone());
                            }

                            // 2. Broadcast to active WebSocket clients
                            let _ = state.broadcast_tx.send(telem.clone());

                            // 2b. Forward telemetry to Redis Pub/Sub for Spring Boot Business Engine
                            if let Some(mut conn) = state.redis_conn.clone() {
                                if let Ok(json_str) = serde_json::to_string(&telem) {
                                    let _ = conn.publish::<_, _, ()>("iot:telemetry:stream", json_str).await;
                                }
                            }

                            // 3. Persist to TimescaleDB asynchronously
                            let db_pool = state.db.clone();
                            tokio::spawn(async move {
                                if let Err(e) = save_telemetry(&db_pool, &device_id, &telem).await {
                                    error!("Failed to persist telemetry for device '{}': {:?}", device_id, e);
                                }
                            });
                        }
                        Err(e) => {
                            warn!(
                                "Received unparsable telemetry JSON on topic {}: {:?}",
                                topic, e
                            );
                        }
                    }
                }
            }
            Ok(_) => {}
            Err(e) => {
                warn!("MQTT connection issue: {}. Reconnecting automatically...", e);
                tokio::time::sleep(Duration::from_secs(3)).await;
            }
        }
    }
}
