use std::env;

#[derive(Clone, Debug)]
pub struct Config {
    pub server_port: u16,
    pub server_host: String,
    pub database_url: String,
    pub mqtt_broker_host: String,
    pub mqtt_broker_port: u16,
    pub mqtt_client_id: String,
    pub mqtt_telemetry_topic: String,
    pub mqtt_command_topic: String,
    pub esp32_direct_http_url: Option<String>,
    pub redis_url: String,
}

impl Config {
    pub fn from_env() -> Self {
        dotenvy::dotenv().ok();

        let server_port = env::var("PORT")
            .ok()
            .and_then(|p| p.parse().ok())
            .unwrap_or(8080);

        let server_host = env::var("HOST").unwrap_or_else(|_| "0.0.0.0".to_string());

        let database_url = env::var("DATABASE_URL").unwrap_or_else(|_| {
            "postgres://iot_admin:iot_secret_password@localhost:5432/home_iot_db".to_string()
        });

        let mqtt_broker_host = env::var("MQTT_BROKER_HOST").unwrap_or_else(|_| "localhost".to_string());

        let mqtt_broker_port = env::var("MQTT_BROKER_PORT")
            .ok()
            .and_then(|p| p.parse().ok())
            .unwrap_or(1883);

        let mqtt_client_id = env::var("MQTT_CLIENT_ID")
            .unwrap_or_else(|_| "home_iot_backend_service".to_string());

        let mqtt_telemetry_topic = env::var("MQTT_TELEMETRY_TOPIC")
            .unwrap_or_else(|_| "home/sensors/telemetry".to_string());

        let mqtt_command_topic = env::var("MQTT_COMMAND_TOPIC")
            .unwrap_or_else(|_| "home/actuators/control".to_string());

        let esp32_direct_http_url = env::var("ESP_DIRECT_HTTP_URL").ok();
        let redis_url = env::var("REDIS_URL").unwrap_or_else(|_| "redis://localhost:6379".to_string());

        Self {
            server_port,
            server_host,
            database_url,
            mqtt_broker_host,
            mqtt_broker_port,
            mqtt_client_id,
            mqtt_telemetry_topic,
            mqtt_command_topic,
            esp32_direct_http_url,
            redis_url,
        }
    }
}
