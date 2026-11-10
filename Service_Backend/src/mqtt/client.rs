use anyhow::Result;
use rumqttc::{AsyncClient, MqttOptions, QoS};
use std::time::Duration;
use tracing::info;

pub fn create_mqtt_client(
    client_id: &str,
    host: &str,
    port: u16,
) -> Result<(AsyncClient, rumqttc::EventLoop)> {
    info!(
        "Configuring MQTT client '{}' connecting to {}:{}",
        client_id, host, port
    );
    let mut mqttoptions = MqttOptions::new(client_id, host, port);
    mqttoptions.set_keep_alive(Duration::from_secs(15));
    mqttoptions.set_clean_session(true);

    let (client, eventloop) = AsyncClient::new(mqttoptions, 50);
    Ok((client, eventloop))
}

#[allow(dead_code)]
pub async fn subscribe_telemetry(client: &AsyncClient, topic: &str) -> Result<()> {
    info!("Subscribing to telemetry topic '{}'", topic);
    client.subscribe(topic, QoS::AtLeastOnce).await?;
    Ok(())
}
