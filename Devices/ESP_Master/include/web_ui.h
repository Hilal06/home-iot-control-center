#ifndef WEB_UI_H
#define WEB_UI_H

#include <Arduino.h>

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 Master Node</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      background: #090d16;
      color: #e2e8f0;
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, monospace;
      padding: 16px;
      line-height: 1.4;
    }
    .container { max-width: 1200px; margin: 0 auto; display: flex; flex-direction: column; gap: 16px; }
    
    /* Boxy Header */
    .header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      flex-wrap: wrap;
      gap: 12px;
      background: #111827;
      border: 2px solid #374151;
      border-radius: 4px;
      padding: 14px 20px;
    }
    .header h1 {
      font-size: 1.2rem;
      font-weight: 700;
      letter-spacing: 0.05em;
      font-family: monospace;
      display: flex;
      align-items: center;
      gap: 10px;
    }
    .badges { display: flex; gap: 8px; align-items: center; flex-wrap: wrap; }
    .badge {
      display: inline-block;
      font-size: 0.72rem;
      font-weight: 700;
      padding: 3px 8px;
      border-radius: 3px;
      text-transform: uppercase;
      font-family: monospace;
      border: 1px solid currentColor;
    }
    .badge-ok { color: #34d399; background: #064e3b44; }
    .badge-warn { color: #fbbf24; background: #78350f44; }
    .badge-err { color: #f87171; background: #7f1d1d44; }
    .badge-info { color: #38bdf8; background: #0c4a6e44; }

    /* Grid Layout */
    .grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(340px, 1fr));
      gap: 16px;
    }

    /* Boxy Card */
    .card {
      background: #111827;
      border: 2px solid #374151;
      border-radius: 4px;
      display: flex;
      flex-direction: column;
    }
    .card-full { grid-column: 1 / -1; }
    .card-header {
      background: #1f2937;
      border-bottom: 2px solid #374151;
      padding: 10px 14px;
      font-size: 0.85rem;
      font-weight: 700;
      letter-spacing: 0.05em;
      text-transform: uppercase;
      font-family: monospace;
      display: flex;
      justify-content: space-between;
      align-items: center;
    }
    .card-body { padding: 14px; display: flex; flex-direction: column; gap: 10px; flex: 1; }

    /* Data rows */
    .row { display: flex; justify-content: space-between; align-items: center; font-size: 0.85rem; }
    .label { color: #9ca3af; font-size: 0.8rem; }
    .val { font-family: monospace; font-weight: 600; }

    /* Buttons & Controls */
    button {
      background: #1f2937;
      color: #f3f4f6;
      border: 2px solid #4b5563;
      border-radius: 3px;
      padding: 6px 12px;
      font-size: 0.82rem;
      font-family: monospace;
      font-weight: 600;
      cursor: pointer;
      transition: all 0.1s;
    }
    button:hover { background: #374151; border-color: #9ca3af; }
    button:active { transform: translateY(1px); }
    .btn-toggle { min-width: 78px; text-align: center; }
    .btn-on { background: #065f46; border-color: #10b981; color: #a7f3d0; }
    .btn-off { background: #374151; border-color: #4b5563; color: #9ca3af; }
    .btn-danger { background: #7f1d1d; border-color: #ef4444; color: #fecaca; }
    .btn-danger:hover { background: #991b1b; }
    .btn-primary { background: #1e3a8a; border-color: #3b82f6; color: #bfdbfe; }
    .btn-xs { padding: 3px 8px; font-size: 0.72rem; }

    .control-group {
      display: flex;
      align-items: center;
      justify-content: space-between;
      padding: 6px 0;
      border-bottom: 1px solid #1f2937;
    }
    .control-group:last-child { border-bottom: none; }

    .num-ctrl { display: flex; align-items: center; gap: 6px; font-family: monospace; }
    .num-ctrl button { padding: 4px 8px; }
    input[type="number"] {
      width: 75px;
      background: #030712;
      border: 1px solid #4b5563;
      border-radius: 3px;
      color: #f3f4f6;
      padding: 4px 6px;
      font-family: monospace;
      font-size: 0.82rem;
    }

    /* Lighting Subgrid */
    .lamps-grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(240px, 1fr));
      gap: 10px;
    }
    .lamp-item {
      background: #0d131f;
      border: 1px solid #2d3748;
      border-radius: 3px;
      padding: 8px 12px;
      display: flex;
      justify-content: space-between;
      align-items: center;
    }
    .section-title {
      font-family: monospace;
      font-size: 0.76rem;
      color: #94a3b8;
      text-transform: uppercase;
      letter-spacing: 0.05em;
      margin: 6px 0 2px 0;
      padding-bottom: 2px;
      border-bottom: 1px dashed #334155;
    }

    /* Terminal Console */
    .console-card { grid-column: 1 / -1; }
    .terminal {
      background: #030712;
      border: 2px solid #1f2937;
      border-radius: 3px;
      height: 260px;
      overflow-y: auto;
      padding: 10px 12px;
      font-family: "Courier New", Courier, monospace;
      font-size: 0.8rem;
      line-height: 1.45;
      display: flex;
      flex-direction: column;
      gap: 3px;
    }
    .log-line { display: flex; gap: 8px; word-break: break-all; align-items: baseline; }
    .log-time { color: #6b7280; flex-shrink: 0; font-size: 0.75rem; }
    .log-tag {
      font-weight: 700;
      flex-shrink: 0;
      width: 58px;
      text-align: center;
      border-radius: 2px;
      font-size: 0.68rem;
      padding: 1px 3px;
      border: 1px solid currentColor;
    }
    .log-tag-INFO { color: #38bdf8; border-color: #0284c7; }
    .log-tag-WARN { color: #fbbf24; border-color: #d97706; }
    .log-tag-ERROR { color: #f87171; border-color: #dc2626; }
    .log-tag-ACTION { color: #4ade80; border-color: #16a34a; }
    .log-msg { color: #e5e7eb; }
    
    .console-bar {
      display: flex;
      justify-content: space-between;
      align-items: center;
      gap: 8px;
      flex-wrap: wrap;
    }
  </style>
</head>
<body>
  <div class="container">
    <!-- Header -->
    <div class="header">
      <div>
        <h1>ESP32 MASTER NODE</h1>
        <div style="font-size: 0.75rem; color: #9ca3af; font-family: monospace;">Edge IoT Controller & Gateway</div>
      </div>
      <div class="badges">
        <span id="badge-wifi" class="badge badge-warn">WIFI: ...</span>
        <span id="badge-mqtt" class="badge badge-warn">MQTT: ...</span>
        <span id="badge-uptime" class="badge badge-info">UP: 0s</span>
        <button onclick="restartESP()" class="btn-danger btn-xs">REBOOT</button>
      </div>
    </div>

    <!-- Main Grid -->
    <div class="grid">
      <!-- Network & Broker Card -->
      <div class="card">
        <div class="card-header">
          <span>Connection Status</span>
          <span id="conn-ping" class="badge badge-info">-</span>
        </div>
        <div class="card-body">
          <div class="row"><span class="label">WiFi SSID</span><span class="val" id="val-ssid">-</span></div>
          <div class="row"><span class="label">IP Address</span><span class="val" id="val-ip">-</span></div>
          <div class="row"><span class="label">Signal (RSSI)</span><span class="val" id="val-rssi">-</span></div>
          <div class="row"><span class="label">MQTT Broker</span><span class="val" id="val-mqtt-server">-</span></div>
          <div class="row"><span class="label">MQTT State</span><span class="val" id="val-mqtt-state">-</span></div>
          <div class="row"><span class="label">Last Reconnect Attempt</span><span class="val" id="val-mqtt-retry">-</span></div>
          <div class="row"><span class="label">Free Heap</span><span class="val" id="val-heap">-</span></div>
        </div>
      </div>

      <!-- General Actuators Card -->
      <div class="card">
        <div class="card-header">Access & Climate Control</div>
        <div class="card-body">
          <div class="control-group">
            <div>
              <div class="val">Garage Door Opener</div>
              <div class="label">Motor Relay</div>
            </div>
            <button id="btn-garage" onclick="toggleGarage()" class="btn-toggle btn-off">CLOSED</button>
          </div>
          <div class="control-group">
            <div>
              <div class="val">Kitchen Breaker</div>
              <div class="label">Main Circuit Relay</div>
            </div>
            <button id="btn-breaker" onclick="toggleBreaker()" class="btn-toggle btn-on">ON</button>
          </div>
          <div class="control-group">
            <div>
              <div class="val">HVAC Climate</div>
              <div class="label">AC Power & Target</div>
            </div>
            <div style="display: flex; gap: 6px; align-items: center;">
              <button id="btn-hvac" onclick="toggleHVAC()" class="btn-toggle btn-off" style="min-width: 55px;">OFF</button>
              <div class="num-ctrl">
                <button onclick="adjustTemp(-1)">-</button>
                <span id="val-target-temp" style="font-weight: bold; width: 35px; text-align: center;">22°C</span>
                <button onclick="adjustTemp(1)">+</button>
              </div>
            </div>
          </div>
        </div>
      </div>

      <!-- Solar & Energy Card -->
      <div class="card">
        <div class="card-header">Solar & Config</div>
        <div class="card-body">
          <div class="control-group">
            <div>
              <div class="val">Solar Charger</div>
              <div class="label">MPPT Relay Status</div>
            </div>
            <button id="btn-solar" onclick="toggleSolar()" class="btn-toggle btn-on">ENABLED</button>
          </div>
          <div class="control-group">
            <div>
              <div class="val">Inverter Max Output</div>
              <div class="label">Grid-Tie Cap (W)</div>
            </div>
            <div class="num-ctrl">
              <input type="number" id="input-max-watts" value="3000" min="500" max="10000" step="100">
              <button onclick="saveMaxWatts()" class="btn-primary">SET</button>
            </div>
          </div>
          <div class="control-group">
            <div>
              <div class="val">Telemetry Interval</div>
              <div class="label">Reporting cycle (ms)</div>
            </div>
            <div class="num-ctrl">
              <input type="number" id="input-interval" value="2000" min="500" max="60000" step="500">
              <button onclick="saveInterval()" class="btn-primary">SET</button>
            </div>
          </div>
          <div class="row" style="margin-top: 6px;"><span class="label">Last Telemetry Sent</span><span class="val" id="val-last-telem">-</span></div>
        </div>
      </div>

      <!-- Live Sensors Snapshot Card -->
      <div class="card">
        <div class="card-header">Live Telemetry Snapshot</div>
        <div class="card-body">
          <div class="row"><span class="label">Solar Power</span><span class="val" id="telem-solar-w">- W</span></div>
          <div class="row"><span class="label">PV Array V / I</span><span class="val" id="telem-pv-vi">- V / - A</span></div>
          <div class="row"><span class="label">House Consumption</span><span class="val" id="telem-house-w">- W</span></div>
          <div class="row"><span class="label">Mains Voltage</span><span class="val" id="telem-mains-v">- V</span></div>
          <div class="row"><span class="label">UPS Battery / Load</span><span class="val" id="telem-ups">- % / - %</span></div>
          <div class="row"><span class="label">Rack / Outdoor Temp</span><span class="val" id="telem-temp">-°C / -°C</span></div>
          <div class="row"><span class="label">Humidity</span><span class="val" id="telem-hum">- %</span></div>
        </div>
      </div>

      <!-- Lighting Switches Card (11 Zones) -->
      <div class="card card-full">
        <div class="card-header">
          <div>
            <span>Lighting & Lamp Switches (11 Zones)</span>
            <span id="lamps-active-count" class="badge badge-info" style="margin-left: 8px;">0 / 11 ON</span>
          </div>
          <div style="display: flex; gap: 6px;">
            <button onclick="setAllLamps(true)" class="btn-primary btn-xs">ALL ON</button>
            <button onclick="setAllLamps(false)" class="btn-danger btn-xs">ALL OFF</button>
          </div>
        </div>
        <div class="card-body">
          <div class="section-title">Indoor Living & Rooms</div>
          <div class="lamps-grid">
            <div class="lamp-item">
              <div><div class="val">Living Room</div><div class="label">living_room</div></div>
              <button id="btn-lamp-living_room" onclick="toggleLampZone('living_room')" class="btn-toggle btn-off">OFF</button>
            </div>
            <div class="lamp-item">
              <div><div class="val">Main Room</div><div class="label">main_room</div></div>
              <button id="btn-lamp-main_room" onclick="toggleLampZone('main_room')" class="btn-toggle btn-off">OFF</button>
            </div>
            <div class="lamp-item">
              <div><div class="val">Kitchen</div><div class="label">kitchen</div></div>
              <button id="btn-lamp-kitchen" onclick="toggleLampZone('kitchen')" class="btn-toggle btn-off">OFF</button>
            </div>
            <div class="lamp-item">
              <div><div class="val">Garage Lamp</div><div class="label">garage</div></div>
              <button id="btn-lamp-garage" onclick="toggleLampZone('garage')" class="btn-toggle btn-off">OFF</button>
            </div>
          </div>

          <div class="section-title">Stairs & Hallways</div>
          <div class="lamps-grid">
            <div class="lamp-item">
              <div><div class="val">Stair Garage</div><div class="label">stair_garage</div></div>
              <button id="btn-lamp-stair_garage" onclick="toggleLampZone('stair_garage')" class="btn-toggle btn-off">OFF</button>
            </div>
            <div class="lamp-item">
              <div><div class="val">Stair Lt 2</div><div class="label">stair_fl2</div></div>
              <button id="btn-lamp-stair_fl2" onclick="toggleLampZone('stair_fl2')" class="btn-toggle btn-off">OFF</button>
            </div>
          </div>

          <div class="section-title">Outdoor & Yards</div>
          <div class="lamps-grid">
            <div class="lamp-item">
              <div><div class="val">Halaman Depan</div><div class="label">yard_front</div></div>
              <button id="btn-lamp-yard_front" onclick="toggleLampZone('yard_front')" class="btn-toggle btn-off">OFF</button>
            </div>
            <div class="lamp-item">
              <div><div class="val">Halaman Samping</div><div class="label">yard_side</div></div>
              <button id="btn-lamp-yard_side" onclick="toggleLampZone('yard_side')" class="btn-toggle btn-off">OFF</button>
            </div>
            <div class="lamp-item">
              <div><div class="val">Halaman Belakang</div><div class="label">yard_back</div></div>
              <button id="btn-lamp-yard_back" onclick="toggleLampZone('yard_back')" class="btn-toggle btn-off">OFF</button>
            </div>
          </div>

          <div class="section-title">Utility & Technical Rooms</div>
          <div class="lamps-grid">
            <div class="lamp-item">
              <div><div class="val">Server Room</div><div class="label">server_room</div></div>
              <button id="btn-lamp-server_room" onclick="toggleLampZone('server_room')" class="btn-toggle btn-off">OFF</button>
            </div>
            <div class="lamp-item">
              <div><div class="val">Electrical Room</div><div class="label">electrical_room</div></div>
              <button id="btn-lamp-electrical_room" onclick="toggleLampZone('electrical_room')" class="btn-toggle btn-off">OFF</button>
            </div>
          </div>
        </div>
      </div>

      <!-- Boxy Console Log Card -->
      <div class="card console-card">
        <div class="card-header console-bar">
          <div>
            <span>>_ SYSTEM CONSOLE LOGS</span>
            <span id="log-count" class="badge badge-info" style="margin-left: 8px;">0 ENTRIES</span>
          </div>
          <div style="display: flex; gap: 8px; align-items: center;">
            <button id="btn-autoscroll" onclick="toggleAutoScroll()" class="btn-on btn-xs">AUTO-SCROLL: ON</button>
            <button onclick="clearConsoleView()" class="btn-xs">CLEAR VIEW</button>
            <button onclick="fetchLogs()" class="btn-xs">REFRESH</button>
          </div>
        </div>
        <div class="card-body" style="padding: 10px;">
          <div id="terminal" class="terminal">
            <div class="log-line">
              <span class="log-time">[00:00:00]</span>
              <span class="log-tag log-tag-INFO">INFO</span>
              <span class="log-msg">Web console initialized. Waiting for ESP32 logs...</span>
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>

  <script>
    let state = {};
    let autoScroll = true;

    function formatUptime(seconds) {
      const h = Math.floor(seconds / 3600);
      const m = Math.floor((seconds % 3600) / 60);
      const s = seconds % 60;
      return `${h}h ${m}m ${s}s`;
    }

    async function sendCommand(cmd, payload = {}) {
      try {
        const body = Object.assign({ command: cmd }, payload);
        const res = await fetch('/api/control', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify(body)
        });
        const data = await res.json();
        fetchStatus();
        fetchLogs();
        return data;
      } catch (err) {
        console.error("Control failed:", err);
      }
    }

    function toggleLampZone(room) {
      const cur = (state.lamps && state.lamps[room]) || false;
      sendCommand("set_lamp", { room: room, state: !cur });
    }

    function setAllLamps(targetState) {
      sendCommand("set_lamp", { room: "all", state: targetState });
    }

    function toggleGarage() {
      sendCommand("set_garage", { state: !state.garage_status });
    }

    function toggleBreaker() {
      sendCommand("set_breaker", { state: !state.kitchen_breaker });
    }

    function toggleHVAC() {
      sendCommand("set_hvac", { power: !state.hvac_power });
    }

    function adjustTemp(delta) {
      const target = (state.hvac_target_temp || 22) + delta;
      if (target >= 16 && target <= 32) {
        sendCommand("set_hvac", { target_temp: target });
      }
    }

    function toggleSolar() {
      sendCommand("set_solar", { charge_enabled: !state.solar_charge_enabled });
    }

    function saveMaxWatts() {
      const watts = parseInt(document.getElementById('input-max-watts').value);
      if (watts > 0) sendCommand("set_solar", { max_watts: watts });
    }

    function saveInterval() {
      const ms = parseInt(document.getElementById('input-interval').value);
      if (ms >= 500) sendCommand("set_config", { telemetry_interval_ms: ms });
    }

    async function restartESP() {
      if (confirm("Restart ESP32 Master node now?")) {
        await fetch('/api/restart', { method: 'POST' });
        alert("Restart signal sent. Node is rebooting...");
      }
    }

    function toggleAutoScroll() {
      autoScroll = !autoScroll;
      const btn = document.getElementById('btn-autoscroll');
      btn.textContent = `AUTO-SCROLL: ${autoScroll ? 'ON' : 'OFF'}`;
      btn.className = autoScroll ? 'btn-on' : 'btn-off';
    }

    function clearConsoleView() {
      document.getElementById('terminal').innerHTML = '';
      document.getElementById('log-count').textContent = '0 ENTRIES';
    }

    async function fetchStatus() {
      try {
        const res = await fetch('/api/status');
        if (!res.ok) return;
        state = await res.json();

        // Update Header Badges
        const wifiBadge = document.getElementById('badge-wifi');
        wifiBadge.textContent = state.wifi_connected ? 'WIFI: OK' : 'WIFI: ERR';
        wifiBadge.className = `badge ${state.wifi_connected ? 'badge-ok' : 'badge-err'}`;

        const mqttBadge = document.getElementById('badge-mqtt');
        mqttBadge.textContent = state.mqtt_connected ? 'MQTT: OK' : 'MQTT: FAIL';
        mqttBadge.className = `badge ${state.mqtt_connected ? 'badge-ok' : 'badge-err'}`;

        document.getElementById('badge-uptime').textContent = `UP: ${formatUptime(state.uptime_sec || 0)}`;

        // Network Info
        document.getElementById('val-ssid').textContent = state.wifi_ssid || '-';
        document.getElementById('val-ip').textContent = state.ip_address || '-';
        document.getElementById('val-rssi').textContent = `${state.wifi_rssi || 0} dBm`;
        document.getElementById('val-mqtt-server').textContent = `${state.mqtt_server}:${state.mqtt_port}`;
        document.getElementById('val-mqtt-state').textContent = state.mqtt_state_desc || '-';
        document.getElementById('val-mqtt-retry').textContent = `${state.mqtt_retries || 0} attempts`;
        document.getElementById('val-heap').textContent = `${state.free_heap_kb || 0} KB`;

        // Actuators
        const btnGarage = document.getElementById('btn-garage');
        btnGarage.textContent = state.garage_status ? 'OPEN' : 'CLOSED';
        btnGarage.className = `btn-toggle ${state.garage_status ? 'btn-on' : 'btn-off'}`;

        const btnBreaker = document.getElementById('btn-breaker');
        btnBreaker.textContent = state.kitchen_breaker ? 'ON' : 'OFF';
        btnBreaker.className = `btn-toggle ${state.kitchen_breaker ? 'btn-on' : 'btn-off'}`;

        const btnHVAC = document.getElementById('btn-hvac');
        btnHVAC.textContent = state.hvac_power ? 'ON' : 'OFF';
        btnHVAC.className = `btn-toggle ${state.hvac_power ? 'btn-on' : 'btn-off'}`;
        document.getElementById('val-target-temp').textContent = `${state.hvac_target_temp}°C`;

        // Lamps (11 zones)
        if (state.lamps) {
          let activeCount = 0;
          let totalCount = 0;
          for (const [room, isOn] of Object.entries(state.lamps)) {
            totalCount++;
            if (isOn) activeCount++;
            const btn = document.getElementById('btn-lamp-' + room);
            if (btn) {
              btn.textContent = isOn ? 'ON' : 'OFF';
              btn.className = `btn-toggle ${isOn ? 'btn-on' : 'btn-off'}`;
            }
          }
          const counterEl = document.getElementById('lamps-active-count');
          if (counterEl) counterEl.textContent = `${activeCount} / ${totalCount} ON`;
        }

        // Solar
        const btnSolar = document.getElementById('btn-solar');
        btnSolar.textContent = state.solar_charge_enabled ? 'ENABLED' : 'DISABLED';
        btnSolar.className = `btn-toggle ${state.solar_charge_enabled ? 'btn-on' : 'btn-off'}`;
        
        if (document.activeElement !== document.getElementById('input-max-watts')) {
          document.getElementById('input-max-watts').value = state.solar_max_watts || 2400;
        }
        if (document.activeElement !== document.getElementById('input-interval')) {
          document.getElementById('input-interval').value = state.telemetry_interval_ms || 1000;
        }

        // Telemetry snapshot
        if (state.last_telemetry) {
          const t = state.last_telemetry;
          document.getElementById('val-last-telem').textContent = `${state.last_telemetry_age_s || 0}s ago`;
          document.getElementById('telem-solar-w').textContent = `${(t.pv_power_watts || 0).toFixed(1)} W`;
          document.getElementById('telem-pv-vi').textContent = `${(t.pv_array_voltage || 0).toFixed(1)} V / ${(t.pv_array_current || 0).toFixed(1)} A`;
          document.getElementById('telem-house-w').textContent = `${(t.house_power_w || 0).toFixed(1)} W`;
          document.getElementById('telem-mains-v').textContent = `${(t.mains_voltage || 0).toFixed(1)} V`;
          document.getElementById('telem-ups').textContent = `${(t.ups_battery_pct || 0).toFixed(0)}% / ${(t.ups_load_pct || 0).toFixed(0)}%`;
          document.getElementById('telem-temp').textContent = `${(t.rack_temp_c || 0).toFixed(1)}°C / ${(t.temp_outdoor_c || 0).toFixed(1)}°C`;
          document.getElementById('telem-hum').textContent = `${(t.humidity_pct || 0).toFixed(0)}%`;
        }
      } catch (err) {
        console.error("fetchStatus error:", err);
      }
    }

    async function fetchLogs() {
      try {
        const res = await fetch('/api/logs');
        if (!res.ok) return;
        const logs = await res.json();
        
        const term = document.getElementById('terminal');
        document.getElementById('log-count').textContent = `${logs.length} ENTRIES`;
        
        term.innerHTML = logs.map(l => {
          const tagClass = `log-tag-${l.level}`;
          return `<div class="log-line">
            <span class="log-time">[${l.time}]</span>
            <span class="log-tag ${tagClass}">${l.level}</span>
            <span class="log-msg">${escapeHtml(l.msg)}</span>
          </div>`;
        }).join('');

        if (autoScroll) {
          term.scrollTop = term.scrollHeight;
        }
      } catch (err) {
        console.error("fetchLogs error:", err);
      }
    }

    function escapeHtml(str) {
      if (!str) return '';
      return str.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;');
    }

    // Initial and periodic fetch
    fetchStatus();
    fetchLogs();
    setInterval(fetchStatus, 2000);
    setInterval(fetchLogs, 1500);
  </script>
</body>
</html>
)rawliteral";

#endif
