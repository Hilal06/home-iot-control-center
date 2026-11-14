use axum::{
    extract::{
        State,
        ws::{Message, WebSocket, WebSocketUpgrade},
    },
    response::IntoResponse,
};
use futures_util::{SinkExt, StreamExt};
use tracing::{debug, info};

use crate::state::AppState;

pub async fn ws_handler(
    ws: WebSocketUpgrade,
    State(state): State<AppState>,
) -> impl IntoResponse {
    ws.max_frame_size(64 * 1024)
        .max_message_size(64 * 1024)
        .on_upgrade(|socket| handle_socket(socket, state))
}

async fn handle_socket(socket: WebSocket, state: AppState) {
    let (mut sender, mut receiver) = socket.split();
    let mut broadcast_rx = state.broadcast_tx.subscribe();

    info!("New WebSocket client connected to live telemetry stream");

    // Send immediate snapshot if available
    if let Some(snapshot) = state.latest_telemetry.read().await.clone() {
        if let Ok(json_str) = serde_json::to_string(&snapshot) {
            let _ = sender.send(Message::Text(json_str.into())).await;
        }
    }

    // Bidirectional loop
    loop {
        tokio::select! {
            // Outgoing telemetry broadcast from MQTT
            telemetry_result = broadcast_rx.recv() => {
                match telemetry_result {
                    Ok(telemetry) => {
                        if let Ok(json_str) = serde_json::to_string(&telemetry) {
                            if sender.send(Message::Text(json_str.into())).await.is_err() {
                                debug!("Client disconnected during telemetry send");
                                break;
                            }
                        }
                    }
                    Err(tokio::sync::broadcast::error::RecvError::Lagged(skipped)) => {
                        debug!("WebSocket receiver lagged behind by {} messages", skipped);
                    }
                    Err(tokio::sync::broadcast::error::RecvError::Closed) => {
                        break;
                    }
                }
            }

            // Incoming messages from client (Ping, Close, etc.)
            client_msg = receiver.next() => {
                match client_msg {
                    Some(Ok(Message::Close(_))) | None => {
                        debug!("WebSocket client requested close or disconnected");
                        break;
                    }
                    Some(Ok(Message::Ping(payload))) => {
                        if sender.send(Message::Pong(payload)).await.is_err() {
                            break;
                        }
                    }
                    Some(Ok(_)) => {
                        // Ignore arbitrary text messages from client for now
                    }
                    Some(Err(_)) => {
                        break;
                    }
                }
            }
        }
    }

    info!("WebSocket client disconnected");
}
