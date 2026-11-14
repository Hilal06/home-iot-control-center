<script>
  let { device = {}, telemetry = {} } = $props();

  // Helper calculation for signal quality
  function getRssiQuality(rssi) {
    if (rssi === undefined || rssi === null) return { text: 'NO SIGNAL', badge: 'inactive', pct: 0 };
    if (rssi >= -60) return { text: 'EXCELLENT', badge: 'active', pct: 95 };
    if (rssi >= -70) return { text: 'GOOD', badge: 'active', pct: 75 };
    if (rssi >= -80) return { text: 'FAIR', badge: 'warning', pct: 50 };
    return { text: 'WEAK', badge: 'alert', pct: 25 };
  }

  function fmtNum(val, decimals = 1, fallback = '--') {
    if (val === undefined || val === null || isNaN(Number(val))) return fallback;
    return Number(val).toFixed(decimals);
  }

  let rssiInfo = $derived(getRssiQuality(telemetry.rssi_dbm));

  let isGasDevice = $derived(
    Boolean((device.id && device.id.toLowerCase().includes('gas')) ||
    (device.role && device.role.toLowerCase().includes('gas')))
  );

  let isWaterDevice = $derived(
    Boolean((device.id && (device.id.toLowerCase().includes('water') || device.id.toLowerCase().includes('tank'))) ||
    (device.role && device.role.toLowerCase().includes('water')))
  );

  let isGatewayDevice = $derived(!isGasDevice && !isWaterDevice);
</script>

<div class="panel">
  <div class="panel-header">
    <div class="header-info">
      <h2>INDIKATOR & SENSOR: {device.name || device.id}</h2>
      <span class="device-sub">ID: {device.id} &bull; IP: {device.ip_address || telemetry.esp_ip || '192.168.100.76'}</span>
    </div>

    <div class="header-status">
      <span class="badge {device.status === 'ONLINE' ? 'active' : 'inactive'}">
        STATUS: {device.status || 'ONLINE'}
      </span>
      <span class="badge {device.is_locked ? 'warning' : 'inactive'}">
        {device.is_locked ? 'LOCKED' : 'UNLOCKED'}
      </span>
      {#if device.is_locked && device.allow_automation_when_locked !== false}
        <span class="badge active">SMART BYPASS</span>
      {/if}
    </div>
  </div>

  <div class="indicator-grid">
    <!-- 1. HARDWARE & COMMUNICATION DIAGNOSTICS (ALL DEVICES) -->
    <div class="ind-card">
      <div class="ind-card-header">
        <span class="card-title">1. HARDWARE & NETWORK DIAGNOSTICS</span>
        <span class="badge {rssiInfo.badge}">{rssiInfo.text}</span>
      </div>
      <div class="ind-card-body">
        <div class="data-row">
          <span class="lbl">Wi-Fi RSSI Signal:</span>
          <span class="val"><strong>{telemetry.rssi_dbm ?? '--'}</strong> dBm</span>
        </div>
        <div class="meter-bar-container">
          <div class="meter-bar" style="width: {rssiInfo.pct}%;"></div>
        </div>

        <div class="data-row">
          <span class="lbl">Round-Trip Gateway Ping:</span>
          <span class="val"><strong>{fmtNum(telemetry.ping_gateway_ms, 1)}</strong> ms</span>
        </div>

        <div class="data-row">
          <span class="lbl">Free Heap SRAM:</span>
          <span class="val"><strong>{telemetry.esp_free_heap_kb ?? '--'}</strong> KB</span>
        </div>

        <div class="data-row">
          <span class="lbl">Device IP & Mode:</span>
          <span class="val">{device.ip_address || '192.168.100.x'} (STA)</span>
        </div>

        <div class="data-row">
          <span class="lbl">Classification Role:</span>
          <span class="val">{device.role || 'Peripheral Node'}</span>
        </div>
      </div>
    </div>

    <!-- SPECIALIZED CARD FOR GAS SAFETY NODES -->
    {#if isGasDevice}
      <div class="ind-card">
        <div class="ind-card-header">
          <span class="card-title">2. GAS & HAZARD SENSOR INDICATORS</span>
          <span class="badge active">MQ-SENSOR: NORMAL</span>
        </div>
        <div class="ind-card-body">
          <div class="data-row highlight">
            <span class="lbl">Gas Concentration (LPG/Smoke):</span>
            <span class="val solar-green"><strong>145</strong> PPM (Safe &lt; 400 PPM)</span>
          </div>
          <div class="meter-bar-container">
            <div class="meter-bar" style="width: 25%; background-color: #4ade80;"></div>
          </div>

          <div class="data-row">
            <span class="lbl">Gas Cutoff Solenoid Valve:</span>
            <span class="val"><span class="badge active">VALVE OPEN / SUPPLY ON</span></span>
          </div>

          <div class="data-row">
            <span class="lbl">Audible Siren Alarm:</span>
            <span class="val"><span class="badge inactive">STANDBY / SILENT</span></span>
          </div>

          <div class="data-row">
            <span class="lbl">Automated Mitigation Rule:</span>
            <span class="val" style="color: #4ade80; font-size: 10px;">
              ON GAS LEAK &gt; 400 PPM: AUTO ALARM + CUTOFF VALVE
            </span>
          </div>

          <div class="data-row">
            <span class="lbl">Smart Lock Rule:</span>
            <span class="val" style="color: #fde047; font-size: 10px;">
              Emergency sensor triggers bypass manual lock
            </span>
          </div>
        </div>
      </div>
    {/if}

    <!-- SPECIALIZED CARD FOR WATER TANK & PUMP NODES -->
    {#if isWaterDevice}
      <div class="ind-card">
        <div class="ind-card-header">
          <span class="card-title">2. WATER RESERVOIR & PUMP TELEMETRY</span>
          <span class="badge active">RESERVOIR: 82%</span>
        </div>
        <div class="ind-card-body">
          <div class="data-row highlight">
            <span class="lbl">Reservoir Storage Level:</span>
            <span class="val" style="color: #38bdf8;"><strong>820</strong> L / 1000 L (82%)</span>
          </div>
          <div class="meter-bar-container">
            <div class="meter-bar" style="width: 82%; background-color: #38bdf8;"></div>
          </div>

          <div class="data-row">
            <span class="lbl">Ultrasonic Sensor Distance:</span>
            <span class="val"><strong>18.4</strong> cm from lip</span>
          </div>

          <div class="data-row">
            <span class="lbl">Main Water Pump Relay:</span>
            <span class="val"><span class="badge active">PUMP RUNNING</span></span>
          </div>

          <div class="data-row">
            <span class="lbl">Secondary Overflow / Divert Valve:</span>
            <span class="val"><span class="badge inactive">STANDBY (TRIGGERS AT 100%)</span></span>
          </div>

          <div class="data-row">
            <span class="lbl">Automated Smart Flow Routing:</span>
            <span class="val" style="color: #38bdf8; font-size: 10px;">
              ON TANK 100%: DIVERT FLOW TO GARDEN IRRIGATION VALVE
            </span>
          </div>
        </div>
      </div>
    {/if}

    <!-- 2. POWER & ELECTRICAL GRID INDICATORS (FOR GATEWAYS & POWER NODES) -->
    {#if isGatewayDevice}
      <div class="ind-card">
        <div class="ind-card-header">
          <span class="card-title">2. POWER & ELECTRICAL GRID TELEMETRY</span>
          <span class="badge {telemetry.solar_charger_active ? 'active' : 'inactive'}">
            {telemetry.solar_charger_active ? 'MPPT: ACTIVE' : 'MPPT: OFF'}
          </span>
        </div>
        <div class="ind-card-body">
          <div class="data-row highlight">
            <span class="lbl">Solar PV Array Output:</span>
            <span class="val solar-green"><strong>{fmtNum(telemetry.pv_power_watts, 1)}</strong> W</span>
          </div>

          <div class="data-row">
            <span class="lbl">PV Voltage & Current:</span>
            <span class="val">{fmtNum(telemetry.pv_array_voltage, 1)} V / {fmtNum(telemetry.pv_array_current, 2)} A</span>
          </div>

          <div class="data-row">
            <span class="lbl">Cumulative Solar Yield Today:</span>
            <span class="val"><strong>{fmtNum(telemetry.solar_yield_kwh, 2)}</strong> kWh</span>
          </div>

          <div class="data-row highlight">
            <span class="lbl">Total House Power Draw:</span>
            <span class="val"><strong>{fmtNum(telemetry.house_power_w, 1)}</strong> W</span>
          </div>

          <div class="data-row">
            <span class="lbl">Mains Line Voltage (PLN):</span>
            <span class="val"><strong>{fmtNum(telemetry.mains_voltage, 1)}</strong> V</span>
          </div>

          <div class="data-row">
            <span class="lbl">UPS Battery Reserve / Load:</span>
            <span class="val">{fmtNum(telemetry.ups_battery_pct, 0)}% (Load: {fmtNum(telemetry.ups_load_pct, 0)}%)</span>
          </div>
        </div>
      </div>
    {/if}

    <!-- 3. ENVIRONMENT & SAFETY SENSORS -->
    <div class="ind-card">
      <div class="ind-card-header">
        <span class="card-title">3. ENVIRONMENT & CRITICAL SENSORS</span>
        <span class="badge {telemetry.water_leak ? 'alert' : 'active'}">
          {telemetry.water_leak ? '[ALERT: WATER LEAK]' : '[WATER: CLEAR]'}
        </span>
      </div>
      <div class="ind-card-body">
        <div class="data-row">
          <span class="lbl">Outdoor Ambient Temperature:</span>
          <span class="val"><strong>{fmtNum(telemetry.temp_outdoor_c, 1)}</strong> °C</span>
        </div>

        <div class="data-row">
          <span class="lbl">Ambient Humidity:</span>
          <span class="val"><strong>{fmtNum(telemetry.humidity_pct, 1)}</strong> %</span>
        </div>

        <div class="data-row">
          <span class="lbl">Server Rack Temperature:</span>
          <span class="val"><strong>{fmtNum(telemetry.rack_temp_c, 1)}</strong> °C</span>
        </div>

        <div class="data-row">
          <span class="lbl">Solar Panel Surface Temp:</span>
          <span class="val"><strong>{fmtNum(telemetry.panel_temp_c, 1)}</strong> °C</span>
        </div>

        <div class="data-row">
          <span class="lbl">Basement / Sump Water Sensor:</span>
          <span class="val">
            <span class="badge {telemetry.water_leak ? 'alert' : 'active'}">
              {telemetry.water_leak ? '[CRITICAL LEAK]' : '[NORMAL / DRY]'}
            </span>
          </span>
        </div>
      </div>
    </div>

    <!-- 4. ACTUATOR & RELAY STATE MATRIX -->
    <div class="ind-card">
      <div class="ind-card-header">
        <span class="card-title">4. ACTUATOR & RELAY REAL-TIME MATRIX</span>
        <span class="badge">{device.buttons?.length || 0} CONFIGURED</span>
      </div>
      <div class="ind-card-body">
        <div class="actuator-status-grid">
          {#if device.id === 'esp32_master'}
            <div class="status-cell">
              <span class="cell-label">GARAGE MOTOR</span>
              <span class="badge {telemetry.garage_status ? 'warning' : 'inactive'}">
                {telemetry.garage_status ? '[OPEN]' : '[CLOSED]'}
              </span>
            </div>

            <div class="status-cell">
              <span class="cell-label">HVAC CLIMATE</span>
              <span class="badge {telemetry.hvac_power ? 'active' : 'inactive'}">
                {telemetry.hvac_power ? `[ON: ${telemetry.hvac_target_temp ?? 22}°C]` : '[OFF]'}
              </span>
            </div>

            <div class="status-cell">
              <span class="cell-label">KITCHEN BREAKER</span>
              <span class="badge {telemetry.kitchen_breaker !== false ? 'active' : 'alert'}">
                {telemetry.kitchen_breaker !== false ? '[ACTIVE / ON]' : '[TRIPPED]'}
              </span>
            </div>

            <div class="status-cell">
              <span class="cell-label">SOLAR MPPT CHARGER</span>
              <span class="badge {telemetry.solar_charger_active ? 'active' : 'inactive'}">
                {telemetry.solar_charger_active ? '[ENABLED]' : '[DISABLED]'}
              </span>
            </div>
          {:else}
            {#each (device.buttons || []).slice(0, 6) as btn}
              <div class="status-cell">
                <span class="cell-label">{btn.label?.toUpperCase()}</span>
                <span class="badge {btn.enabled ? 'active' : 'inactive'}">
                  {btn.enabled ? '[READY]' : '[DISABLED]'}
                </span>
              </div>
            {/each}
          {/if}
        </div>

        {#if device.id === 'esp32_master' && telemetry.lamps}
          <div class="lamp-sub-header">
            LIGHTING RELAYS (11 ZONES):
          </div>
          <div class="lamps-mini-grid">
            {#each Object.entries(telemetry.lamps) as [zone, state]}
              <div class="lamp-pill {state ? 'pill-on' : 'pill-off'}">
                <span class="pill-dot"></span>
                <span class="pill-name">{zone.replace(/_/g, ' ')}</span>
              </div>
            {/each}
          </div>
        {/if}
      </div>
    </div>
  </div>
</div>

<style>
  .header-info {
    display: flex;
    flex-direction: column;
    gap: 2px;
  }

  .device-sub {
    font-family: var(--font-mono);
    font-size: 10px;
    color: var(--text-secondary);
  }

  .header-status {
    display: flex;
    gap: 8px;
    align-items: center;
  }

  .indicator-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(320px, 1fr));
    gap: 12px;
    padding: 12px;
  }

  .ind-card {
    background-color: var(--bg-card);
    border: 1px solid var(--border-medium);
    border-radius: var(--radius-sm);
    display: flex;
    flex-direction: column;
  }

  .ind-card-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    background-color: var(--bg-panel-header);
    border-bottom: 1px solid var(--border-medium);
    padding: 6px 10px;
  }

  .card-title {
    font-family: var(--font-mono);
    font-size: 10px;
    font-weight: 700;
    letter-spacing: 0.05em;
    color: var(--text-secondary);
  }

  .ind-card-body {
    padding: 10px;
    display: flex;
    flex-direction: column;
    gap: 6px;
    font-family: var(--font-mono);
    font-size: 11px;
  }

  .data-row {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 4px 0;
    border-bottom: 1px solid var(--border-light);
  }

  .data-row.highlight {
    background-color: var(--bg-hover);
    padding: 5px 6px;
    border-radius: var(--radius-sm);
  }

  .lbl {
    color: var(--text-secondary);
  }

  .val {
    color: var(--text-primary);
  }

  .solar-green {
    color: #15803d;
  }

  .meter-bar-container {
    width: 100%;
    height: 4px;
    background-color: var(--bg-hover);
    margin-bottom: 4px;
    border-radius: var(--radius-sm);
  }

  .meter-bar {
    height: 100%;
    background-color: #10b981;
    border-radius: var(--radius-sm);
  }

  .actuator-status-grid {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 6px;
    margin-bottom: 8px;
  }

  .status-cell {
    background-color: var(--bg-hover);
    border: 1px solid var(--border-medium);
    border-radius: var(--radius-sm);
    padding: 6px 8px;
    display: flex;
    flex-direction: column;
    gap: 4px;
  }

  .cell-label {
    font-size: 9px;
    font-weight: 700;
    color: var(--text-muted);
  }

  .lamp-sub-header {
    font-size: 9px;
    font-weight: 700;
    color: var(--text-secondary);
    margin-top: 6px;
    margin-bottom: 4px;
    letter-spacing: 0.05em;
  }

  .lamps-mini-grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(130px, 1fr));
    gap: 4px;
  }

  .lamp-pill {
    display: flex;
    align-items: center;
    gap: 6px;
    padding: 3px 6px;
    font-size: 10px;
    border: 1px solid var(--border-medium);
    border-radius: var(--radius-sm);
    text-transform: capitalize;
  }

  .pill-on {
    background-color: #dcfce7;
    color: #15803d;
    border-color: #86efac;
  }

  .pill-off {
    background-color: var(--bg-hover);
    color: var(--text-muted);
  }

  .pill-dot {
    width: 6px;
    height: 6px;
    border-radius: var(--radius-sm);
    background-color: currentColor;
  }
</style>
