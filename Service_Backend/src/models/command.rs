use serde::{Deserialize, Serialize};

fn default_command_source() -> String {
    "MANUAL".to_string()
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ControlCommandPayload {
    #[serde(flatten)]
    pub command: DeviceCommand,
    #[serde(default = "default_command_source")]
    pub source: String, // "MANUAL", "SCHEDULE", "AUTOMATION", "DEVICE_TRIGGER", "EMERGENCY"
    #[serde(default)]
    pub force: bool,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(tag = "command", rename_all = "snake_case")]
pub enum DeviceCommand {
    SetLamp {
        #[serde(default)]
        room: Option<String>,
        state: bool,
    },
    SetGarage {
        state: GarageStateInput,
    },
    SetBreaker {
        state: bool,
    },
    SetHvac {
        power: bool,
        target_temp: i32,
    },
    SetSolar {
        charge_enabled: bool,
        max_watts: i32,
    },
    SetConfig {
        telemetry_interval_ms: u32,
    },
    SetRelay {
        channel: u8,
        state: bool,
    },
    SetValve {
        valve_id: String,
        state: bool,
    },
    TriggerEmergency {
        alarm: bool,
        cutoff: bool,
        #[serde(default)]
        reason: Option<String>,
    },
    Custom {
        action: String,
        #[serde(default)]
        target: Option<String>,
        #[serde(default)]
        payload: Option<serde_json::Value>,
    },
    RebootEsp,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(untagged)]
pub enum GarageStateInput {
    Bool(bool),
    Text(String),
}

impl DeviceCommand {
    /// Validates command parameters according to firmware and device constraints
    pub fn validate(&self) -> Result<(), String> {
        match self {
            DeviceCommand::SetLamp { room, .. } => {
                if let Some(r) = room {
                    let valid_rooms = [
                        "all",
                        "living_room",
                        "main_room",
                        "garage",
                        "kitchen",
                        "stair_garage",
                        "stair_fl2",
                        "stair_lt2",
                        "stair2",
                        "yard_front",
                        "front_yard",
                        "yard_side",
                        "side_yard",
                        "yard_back",
                        "back_yard",
                        "backyard",
                        "server_room",
                        "electrical_room",
                        "zone_1",
                        "zone_2",
                        "zone_3",
                        "zone_4",
                        "zone_5",
                        "zone_6",
                        "zone_7",
                        "zone_8",
                    ];
                    if !valid_rooms.iter().any(|v| v.eq_ignore_ascii_case(r)) {
                        return Err(format!(
                            "Invalid room '{}'. Allowed rooms: living_room, main_room, garage, kitchen, stair_garage, stair_fl2, yard_front, yard_side, yard_back, server_room, electrical_room, zone_1..zone_8, all",
                            r
                        ));
                    }
                }
                Ok(())
            }
            DeviceCommand::SetGarage { state } => match state {
                GarageStateInput::Bool(_) => Ok(()),
                GarageStateInput::Text(t) => {
                    let t_lower = t.to_lowercase();
                    if t_lower == "open" || t_lower == "closed" || t_lower == "true" || t_lower == "false" {
                        Ok(())
                    } else {
                        Err("Garage state string must be 'open', 'closed', 'true', or 'false'".to_string())
                    }
                }
            },
            DeviceCommand::SetBreaker { .. } => Ok(()),
            DeviceCommand::SetHvac { target_temp, .. } => {
                if *target_temp < 16 || *target_temp > 32 {
                    Err(format!(
                        "HVAC target_temp must be between 16 and 32 Celsius, got {}",
                        target_temp
                    ))
                } else {
                    Ok(())
                }
            }
            DeviceCommand::SetSolar { max_watts, .. } => {
                if *max_watts < 100 || *max_watts > 15000 {
                    Err(format!(
                        "Solar max_watts must be between 100 and 15000 W, got {}",
                        max_watts
                    ))
                } else {
                    Ok(())
                }
            }
            DeviceCommand::SetConfig {
                telemetry_interval_ms,
            } => {
                if *telemetry_interval_ms < 500 || *telemetry_interval_ms > 60000 {
                    Err(format!(
                        "telemetry_interval_ms must be between 500 and 60000 ms, got {}",
                        telemetry_interval_ms
                    ))
                } else {
                    Ok(())
                }
            }
            DeviceCommand::SetRelay { channel, .. } => {
                if *channel < 1 || *channel > 32 {
                    Err(format!("Relay channel must be between 1 and 32, got {}", channel))
                } else {
                    Ok(())
                }
            }
            DeviceCommand::SetValve { valve_id, .. } => {
                if valve_id.trim().is_empty() {
                    Err("valve_id cannot be empty".to_string())
                } else {
                    Ok(())
                }
            }
            DeviceCommand::TriggerEmergency { .. } => Ok(()),
            DeviceCommand::Custom { action, .. } => {
                if action.trim().is_empty() {
                    Err("Custom action name cannot be empty".to_string())
                } else {
                    Ok(())
                }
            }
            DeviceCommand::RebootEsp => Ok(()),
        }
    }

    /// Convert to JSON Value for MQTT transmission
    pub fn to_mqtt_payload(&self) -> serde_json::Value {
        serde_json::to_value(self).unwrap_or(serde_json::json!({}))
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_set_lamp_valid_rooms() {
        let cmd = DeviceCommand::SetLamp {
            room: Some("living_room".to_string()),
            state: true,
        };
        assert!(cmd.validate().is_ok());

        let cmd_all = DeviceCommand::SetLamp {
            room: Some("all".to_string()),
            state: false,
        };
        assert!(cmd_all.validate().is_ok());

        let cmd_zone = DeviceCommand::SetLamp {
            room: Some("zone_1".to_string()),
            state: true,
        };
        assert!(cmd_zone.validate().is_ok());
    }

    #[test]
    fn test_set_relay() {
        let cmd_valid = DeviceCommand::SetRelay { channel: 2, state: true };
        assert!(cmd_valid.validate().is_ok());

        let cmd_invalid = DeviceCommand::SetRelay { channel: 0, state: true };
        assert!(cmd_invalid.validate().is_err());
    }

    #[test]
    fn test_set_valve() {
        let cmd_valid = DeviceCommand::SetValve { valve_id: "overflow_drain".to_string(), state: true };
        assert!(cmd_valid.validate().is_ok());

        let cmd_invalid = DeviceCommand::SetValve { valve_id: "   ".to_string(), state: true };
        assert!(cmd_invalid.validate().is_err());
    }

    #[test]
    fn test_trigger_emergency() {
        let cmd = DeviceCommand::TriggerEmergency { alarm: true, cutoff: true, reason: Some("Gas leak detected".to_string()) };
        assert!(cmd.validate().is_ok());
    }

    #[test]
    fn test_control_payload_deserialization() {
        let json_str = r#"{
            "command": "set_lamp",
            "room": "living_room",
            "state": true,
            "source": "SCHEDULE"
        }"#;
        let payload: ControlCommandPayload = serde_json::from_str(json_str).unwrap();
        assert_eq!(payload.source, "SCHEDULE");
        assert!(!payload.force);
        assert!(matches!(payload.command, DeviceCommand::SetLamp { .. }));
    }
}
