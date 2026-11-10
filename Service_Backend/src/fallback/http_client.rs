use anyhow::Result;
use reqwest::Client;
use serde_json::Value;
use std::time::Duration;
use tracing::{info, warn};

pub struct EspHttpClient {
    client: Client,
    base_url: Option<String>,
}

impl EspHttpClient {
    pub fn new(base_url: Option<String>) -> Self {
        let client = Client::builder()
            .timeout(Duration::from_secs(3))
            .build()
            .unwrap_or_default();

        Self { client, base_url }
    }

    pub async fn send_command(&self, payload: &Value) -> Result<Value> {
        let base = self
            .base_url
            .as_deref()
            .ok_or_else(|| anyhow::anyhow!("ESP32 direct HTTP URL is not configured"))?;

        let url = format!("{}/api/control", base.trim_end_matches('/'));
        info!("Sending direct HTTP fallback command to {}", url);

        let response = self
            .client
            .post(&url)
            .json(payload)
            .send()
            .await?;

        if !response.status().is_success() {
            let status = response.status();
            let text = response.text().await.unwrap_or_default();
            warn!("ESP32 direct HTTP returned error {}: {}", status, text);
            return Err(anyhow::anyhow!("ESP32 HTTP error {}: {}", status, text));
        }

        let json_resp: Value = response.json().await.unwrap_or(serde_json::json!({
            "status": "success",
            "message": "Command dispatched via direct HTTP"
        }));

        Ok(json_resp)
    }

    #[allow(dead_code)]
    pub async fn get_status(&self) -> Result<Value> {
        let base = self
            .base_url
            .as_deref()
            .ok_or_else(|| anyhow::anyhow!("ESP32 direct HTTP URL is not configured"))?;

        let url = format!("{}/api/status", base.trim_end_matches('/'));
        let response = self.client.get(&url).send().await?.json().await?;
        Ok(response)
    }
}
