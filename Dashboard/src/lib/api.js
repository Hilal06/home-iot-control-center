// API & WebSocket Communication Layer for Home IoT Service

export async function fetchHealth() {
  const res = await fetch('/health');
  if (!res.ok) throw new Error(`Health error: ${res.status}`);
  return res.json();
}

export async function fetchDevices() {
  const res = await fetch('/api/devices');
  if (!res.ok) throw new Error(`Devices error: ${res.status}`);
  return res.json();
}

export async function createDeviceApi(deviceData) {
  try {
    const res = await fetch('/api/devices', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(deviceData),
    });
    if (res.ok) return await res.json();
  } catch (e) {
    console.info('Backend direct device creation not yet implemented or unreachable, saved locally:', e);
  }
  return { status: 'saved_locally', device: deviceData };
}

export async function updateDeviceApi(deviceId, updateData) {
  try {
    const res = await fetch(`/api/devices/${deviceId}`, {
      method: 'PUT',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(updateData),
    });
    if (res.ok) return await res.json();
  } catch (e) {
    console.info('Backend direct device update not yet implemented or unreachable, saved locally:', e);
  }
  return { status: 'saved_locally', deviceId, updateData };
}

export async function deleteDeviceApi(deviceId) {
  try {
    const res = await fetch(`/api/devices/${deviceId}`, {
      method: 'DELETE',
    });
    if (res.ok) return await res.json();
  } catch (e) {
    console.info('Backend direct device deletion not yet implemented or unreachable, deleted locally:', e);
  }
  return { status: 'deleted_locally', deviceId };
}

export async function toggleDeviceLockApi(deviceId, isLocked = null) {
  try {
    const res = await fetch(`/api/devices/${deviceId}/lock`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(isLocked !== null ? { is_locked: isLocked } : {}),
    });
    if (res.ok) return await res.json();
  } catch (e) {
    console.info('Backend lock sync not yet implemented or unreachable, saved locally:', e);
  }
  return { status: 'saved_locally', deviceId, is_locked: isLocked };
}

export async function updateDeviceButtonsApi(deviceId, buttons) {
  try {
    const res = await fetch(`/api/devices/${deviceId}/buttons`, {
      method: 'PUT',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ buttons }),
    });
    if (res.ok) return await res.json();
  } catch (e) {
    console.info('Backend buttons sync not yet implemented or unreachable, saved locally:', e);
  }
  return { status: 'saved_locally', deviceId, buttons };
}

export async function updateDeviceRulesApi(deviceId, rules) {
  try {
    const res = await fetch(`/api/devices/${deviceId}/rules`, {
      method: 'PUT',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ automation_rules: rules }),
    });
    if (res.ok) return await res.json();
  } catch (e) {
    console.info('Backend rules sync not yet implemented or unreachable, saved locally:', e);
  }
  return { status: 'saved_locally', deviceId, rules };
}



export async function fetchDeviceStatus(deviceId = 'esp32_master') {
  const res = await fetch(`/api/devices/${deviceId}/status`);
  if (!res.ok) throw new Error(`Status error: ${res.status}`);
  return res.json();
}

export async function fetchTelemetryLatest(deviceId = 'esp32_master') {
  const res = await fetch(`/api/devices/${deviceId}/telemetry/latest`);
  if (!res.ok) throw new Error(`Telemetry error: ${res.status}`);
  return res.json();
}

export async function fetchTelemetryHistory(deviceId = 'esp32_master', limit = 60) {
  const res = await fetch(`/api/devices/${deviceId}/telemetry/history?limit=${limit}`);
  if (!res.ok) throw new Error(`Telemetry history error: ${res.status}`);
  return res.json();
}

export async function fetchDeviceEvents(deviceId = 'esp32_master', limit = 20) {
  const res = await fetch(`/api/devices/${deviceId}/events?limit=${limit}`);
  if (!res.ok) throw new Error(`Events error: ${res.status}`);
  return res.json();
}

export async function sendControlCommand(deviceId = 'esp32_master', command) {
  const res = await fetch(`/api/devices/${deviceId}/control`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(command),
  });
  if (!res.ok) {
    const errText = await res.text();
    throw new Error(`Command failed: ${errText || res.statusText}`);
  }
  return res.json();
}

export function initWebSocket(onTelemetry, onStatusChange) {
  let ws = null;
  let retryTimer = null;
  let retryDelay = 2000;
  let isClosedExplicitly = false;

  function connect() {
    if (isClosedExplicitly) return;

    onStatusChange('CONNECTING');
    const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
    const wsUrl = `${protocol}//${window.location.host}/api/ws`;

    try {
      ws = new WebSocket(wsUrl);

      ws.onopen = () => {
        onStatusChange('CONNECTED');
        retryDelay = 2000;
      };

      ws.onmessage = (event) => {
        try {
          const data = JSON.parse(event.data);
          onTelemetry(data);
        } catch (e) {
          console.warn('Failed to parse WS JSON:', e, event.data);
        }
      };

      ws.onclose = () => {
        onStatusChange('DISCONNECTED');
        scheduleReconnect();
      };

      ws.onerror = () => {
        ws.close();
      };
    } catch (e) {
      onStatusChange('DISCONNECTED');
      scheduleReconnect();
    }
  }

  function scheduleReconnect() {
    if (isClosedExplicitly) return;
    clearTimeout(retryTimer);
    retryTimer = setTimeout(() => {
      retryDelay = Math.min(retryDelay * 1.5, 10000);
      connect();
    }, retryDelay);
  }

  connect();

  return () => {
    isClosedExplicitly = true;
    clearTimeout(retryTimer);
    if (ws) ws.close();
  };
}

// --- Business Engine API (Scenes & PLN Token) ---

export async function fetchScenes() {
  const res = await fetch('/api/v1/business/scenes');
  if (!res.ok) throw new Error(`Fetch scenes error: ${res.status}`);
  return res.json();
}

export async function activateSceneApi(sceneName) {
  const res = await fetch(`/api/v1/business/scenes/${sceneName}/activate`, {
    method: 'POST',
  });
  if (!res.ok) throw new Error(`Activate scene error: ${res.status}`);
  return res.json();
}

export async function fetchPlnTokenStatus() {
  const res = await fetch('/api/v1/business/token/status');
  if (!res.ok) throw new Error(`Fetch PLN token error: ${res.status}`);
  return res.json();
}

export async function rechargePlnTokenApi(payload) {
  const res = await fetch('/api/v1/business/token/recharge', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(payload),
  });
  if (!res.ok) throw new Error(`Recharge PLN token error: ${res.status}`);
  return res.json();
}
