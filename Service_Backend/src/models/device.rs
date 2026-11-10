use chrono::{DateTime, Utc};
use serde::{Deserialize, Serialize};

fn default_role() -> String {
    "Edge Gateway".to_string()
}

fn default_true() -> bool {
    true
}

fn default_empty_array() -> serde_json::Value {
    serde_json::json!([])
}

#[derive(Debug, Clone, Serialize, Deserialize, sqlx::FromRow)]
pub struct Device {
    pub id: String,
    pub name: String,
    pub ip_address: Option<String>,
    pub status: String,
    pub role: String,
    pub is_locked: bool,
    pub allow_automation_when_locked: bool,
    pub description: Option<String>,
    pub buttons: serde_json::Value,
    pub automation_rules: serde_json::Value,
    pub last_seen: Option<DateTime<Utc>>,
    pub created_at: DateTime<Utc>,
}

#[derive(Debug, Clone, Deserialize)]
pub struct CreateDeviceRequest {
    pub id: String,
    pub name: String,
    pub ip_address: Option<String>,
    #[serde(default = "default_role")]
    pub role: String,
    #[serde(default)]
    pub is_locked: bool,
    #[serde(default = "default_true")]
    pub allow_automation_when_locked: bool,
    pub description: Option<String>,
    #[serde(default = "default_empty_array")]
    pub buttons: serde_json::Value,
    #[serde(default = "default_empty_array")]
    pub automation_rules: serde_json::Value,
}

#[derive(Debug, Clone, Deserialize)]
pub struct UpdateDeviceRequest {
    pub name: Option<String>,
    pub ip_address: Option<String>,
    pub role: Option<String>,
    pub status: Option<String>,
    pub description: Option<String>,
    pub allow_automation_when_locked: Option<bool>,
}

#[derive(Debug, Clone, Deserialize)]
pub struct ToggleLockRequest {
    pub is_locked: Option<bool>,
}

#[derive(Debug, Clone, Deserialize)]
pub struct UpdateDeviceButtonsRequest {
    pub buttons: serde_json::Value,
}

#[derive(Debug, Clone, Deserialize)]
pub struct UpdateDeviceRulesRequest {
    pub automation_rules: serde_json::Value,
}

#[derive(Debug, Clone, Serialize, Deserialize, sqlx::FromRow)]
pub struct DeviceState {
    pub device_id: String,
    pub lamp_states: serde_json::Value,
    pub garage_status: bool,
    pub kitchen_breaker: bool,
    pub hvac_power: bool,
    pub hvac_target_temp: i32,
    pub solar_charge_enabled: bool,
    pub solar_max_watts: i32,
    pub telemetry_interval_ms: i32,
    pub updated_at: DateTime<Utc>,
}

#[derive(Debug, Clone, Serialize, Deserialize, sqlx::FromRow)]
pub struct TelemetryRecord {
    pub recorded_at: DateTime<Utc>,
    pub device_id: String,
    pub pv_array_voltage: Option<f32>,
    pub pv_array_current: Option<f32>,
    pub pv_power_watts: Option<f32>,
    pub solar_yield_kwh: Option<f32>,
    pub panel_temp_c: Option<f32>,
    pub mains_voltage: Option<f32>,
    pub house_power_w: Option<f32>,
    pub ups_battery_pct: Option<f32>,
    pub ups_load_pct: Option<f32>,
    pub rack_temp_c: Option<f32>,
    pub temp_outdoor_c: Option<f32>,
    pub humidity_pct: Option<f32>,
    pub water_leak: Option<bool>,
    pub ping_gateway_ms: Option<f32>,
    pub rssi_dbm: Option<i32>,
    pub esp_free_heap_kb: Option<i32>,
}

#[derive(Debug, Clone, Serialize, Deserialize, sqlx::FromRow)]
pub struct ActuatorEvent {
    pub id: i64,
    pub device_id: String,
    pub command: String,
    pub target: Option<String>,
    pub payload: serde_json::Value,
    pub source: String,
    pub success: bool,
    pub executed_at: DateTime<Utc>,
}
