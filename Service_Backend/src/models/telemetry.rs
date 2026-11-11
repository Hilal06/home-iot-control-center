use serde::{Deserialize, Serialize};
use std::collections::HashMap;

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct TelemetryPayload {
    #[serde(default)]
    pub device_id: Option<String>,
    #[serde(default)]
    pub pv_array_voltage: Option<f32>,
    #[serde(default)]
    pub pv_array_current: Option<f32>,
    #[serde(default)]
    pub pv_power_watts: Option<f32>,
    #[serde(default)]
    pub solar_yield_kwh: Option<f32>,
    #[serde(default)]
    pub panel_temp_c: Option<f32>,
    #[serde(default)]
    pub solar_charger_active: Option<bool>,
    #[serde(default)]
    pub mains_voltage: Option<f32>,
    #[serde(default)]
    pub house_power_w: Option<f32>,
    #[serde(default)]
    pub ups_battery_pct: Option<f32>,
    #[serde(default)]
    pub ups_load_pct: Option<f32>,
    #[serde(default)]
    pub rack_temp_c: Option<f32>,
    #[serde(default)]
    pub temp_outdoor_c: Option<f32>,
    #[serde(default)]
    pub humidity_pct: Option<f32>,
    #[serde(default)]
    pub water_leak: Option<bool>,
    #[serde(default)]
    pub ping_gateway_ms: Option<f32>,
    #[serde(default)]
    pub rssi_dbm: Option<i32>,
    #[serde(default)]
    pub esp_free_heap_kb: Option<i32>,
    #[serde(default)]
    pub lamps: Option<HashMap<String, bool>>,
    #[serde(default)]
    pub lamp_status: Option<bool>,
    #[serde(default)]
    pub garage_status: Option<bool>,
    #[serde(default)]
    pub kitchen_breaker: Option<bool>,
    #[serde(default)]
    pub emergency_alarm: Option<bool>,
    #[serde(default)]
    pub emergency_cutoff: Option<bool>,
    #[serde(default)]
    pub hvac_power: Option<bool>,
    #[serde(default)]
    pub hvac_target_temp: Option<i32>,
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_deserialize_esp32_architecture_payload() {
        let sample_json = r#"
        {
          "pv_array_voltage": 42.5,
          "pv_array_current": 4.1,
          "pv_power_watts": 174.25,
          "solar_yield_kwh": 12.4,
          "panel_temp_c": 38.6,
          "solar_charger_active": true,
          "mains_voltage": 224.8,
          "house_power_w": 1420.5,
          "ups_battery_pct": 98.0,
          "ups_load_pct": 28.0,
          "rack_temp_c": 24.3,
          "temp_outdoor_c": 29.1,
          "humidity_pct": 58.0,
          "water_leak": false,
          "ping_gateway_ms": 3.4,
          "rssi_dbm": -52,
          "esp_free_heap_kb": 226,
          "lamps": {
            "living_room": true,
            "main_room": false,
            "garage": false,
            "kitchen": true,
            "stair_garage": false,
            "stair_fl2": false,
            "yard_front": true,
            "yard_side": false,
            "yard_back": false,
            "server_room": true,
            "electrical_room": false
          },
          "lamp_status": true,
          "garage_status": false,
          "hvac_power": true,
          "hvac_target_temp": 22
        }
        "#;

        let telem: Result<TelemetryPayload, _> = serde_json::from_str(sample_json);
        assert!(telem.is_ok(), "Failed to deserialize ESP32 JSON: {:?}", telem.err());
        let t = telem.unwrap();
        assert_eq!(t.pv_array_voltage, Some(42.5));
        assert_eq!(t.solar_charger_active, Some(true));
        assert_eq!(t.house_power_w, Some(1420.5));
        assert_eq!(t.water_leak, Some(false));
        assert_eq!(t.hvac_target_temp, Some(22));

        let lamps = t.lamps.unwrap();
        assert_eq!(lamps.get("living_room"), Some(&true));
        assert_eq!(lamps.get("main_room"), Some(&false));
    }
}
