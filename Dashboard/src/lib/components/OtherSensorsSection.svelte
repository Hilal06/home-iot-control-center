<script>
  let { telemetry = {}, devices = [], onCommand } = $props();

  // Find devices by id or role
  let gasDevice = $derived(
    devices.find((d) => d.id?.includes('gas') || d.role?.toLowerCase().includes('gas')) || {
      id: 'esp32_gas_safety',
      name: 'ESP32 Gas & Fire Safety Unit',
      status: 'ONLINE',
      ip_address: '192.168.100.85',
      is_locked: false,
    }
  );

  let waterDevice = $derived(
    devices.find((d) => d.id?.includes('water') || d.role?.toLowerCase().includes('water')) || {
      id: 'esp32_water_tank',
      name: 'ESP32 Water Tank & Pump Unit',
      status: 'ONLINE',
      ip_address: '192.168.100.88',
      is_locked: false,
    }
  );

  // Gas dynamic derived status
  let gasPpm = $derived(Number(telemetry.gas_ppm ?? 145));
  let isGasDanger = $derived(gasPpm >= 400);
  let gasValveOpen = $state(true);
  let sirenActive = $state(false);

  // Water dynamic derived status
  let waterLevelPct = $derived(Number(telemetry.water_level_pct ?? 82));
  let waterVolumeL = $derived(Math.round((waterLevelPct / 100) * 1000));
  let ultrasonicDist = $derived(Number(telemetry.ultrasonic_distance_cm ?? 18.4));
  let pumpRunning = $state(true);
  let divertValveOpen = $derived(waterLevelPct >= 100);

  function toggleGasValve() {
    gasValveOpen = !gasValveOpen;
    if (onCommand) {
      onCommand({
        command: 'set_valve',
        valve_id: 'main_gas',
        state: gasValveOpen,
        source: 'MANUAL',
      });
    }
  }

  function triggerEmergencyAlarm() {
    sirenActive = !sirenActive;
    if (onCommand) {
      onCommand({
        command: 'trigger_emergency',
        alarm: sirenActive,
        cutoff: sirenActive,
        reason: 'Manual Emergency Test from Dashboard',
        source: 'EMERGENCY',
      });
    }
  }

  function togglePump() {
    pumpRunning = !pumpRunning;
    if (onCommand) {
      onCommand({
        command: 'set_relay',
        channel: 1,
        state: pumpRunning,
        source: 'MANUAL',
      });
    }
  }

  function pingSensor(deviceId) {
    if (onCommand) {
      onCommand({
        command: 'ping',
        source: 'MANUAL',
      });
    }
  }
</script>

<div class="panel">
  <div class="panel-header">
    <div class="title-with-bar">
      <span class="amber-bar"></span>
      <h2>SENSOR KEAMANAN (GAS & TANGKI AIR)</h2>
    </div>
  </div>

  <div class="sensors-grid">
    <!-- Card 1: Gas & Fire Safety Unit -->
    <div class="card">
      <div class="card-header">
        <span class="card-title">DETEKTOR GAS & KEBAKARAN</span>
        <span class="badge {isGasDanger ? 'alert' : 'active'}">
          {isGasDanger ? 'BAHAYA KEBOCORAN' : 'KONDISI AMAN'}
        </span>
      </div>

      <div class="sensor-body">
        <!-- Gas Concentration Readout -->
        <div class="metric-highlight {isGasDanger ? 'highlight-alert' : 'highlight-normal'}">
          <div class="metric-info">
            <span class="lbl">KONSENTRASI GAS LPG / ASAP:</span>
            <span class="val-large {isGasDanger ? 'text-alert' : 'text-green'}">
              <strong>{gasPpm}</strong> <span class="unit">PPM</span>
            </span>
          </div>
          <div class="metric-note">
            <span>Batas Aman: &lt; 400 PPM</span>
          </div>
        </div>

        <!-- Meter Progress Bar -->
        <div class="meter-bar-container">
          <div
            class="meter-bar"
            style="width: {Math.min(100, (gasPpm / 600) * 100)}%; background-color: {isGasDanger ? '#dc2626' : '#15803d'};"
          ></div>
        </div>

        <!-- Hardware Actuators / Solenoid Status -->
        <div class="actuator-grid">
          <div class="actuator-item">
            <span class="lbl">Keran Pipa Gas Utama:</span>
            <button
              type="button"
              class="btn-act {gasValveOpen ? 'btn-active' : 'btn-danger'}"
              onclick={toggleGasValve}
            >
              {gasValveOpen ? 'KERAN TERBUKA (ON)' : 'KERAN DITUTUP (OFF)'}
            </button>
          </div>

          <div class="actuator-item">
            <span class="lbl">Sirine Alarm Darurat:</span>
            <button
              type="button"
              class="btn-act {sirenActive ? 'btn-danger' : 'btn-standby'}"
              onclick={triggerEmergencyAlarm}
            >
              {sirenActive ? 'ALARM BUNYI (ON)' : 'STANDBY (HENING)'}
            </button>
          </div>
        </div>

        <!-- Safety Note -->
        <div class="rule-box">
          <span class="rule-title">SISTEM PENGAMAN OTOMATIS:</span>
          <p class="rule-desc">
            Bila kadar gas &gt; 400 PPM, keran gas otomatis menutup dan sirine berbunyi demi keselamatan.
          </p>
        </div>
      </div>
    </div>

    <!-- Card 2: Water Tank & Reservoir Unit -->
    <div class="card">
      <div class="card-header">
        <span class="card-title">TANGKI AIR & POMPA RUMAH</span>
        <span class="badge active">LEVEL: {waterLevelPct}%</span>
      </div>

      <div class="sensor-body">
        <!-- Water Level Readout -->
        <div class="metric-highlight highlight-water">
          <div class="metric-info">
            <span class="lbl">KAPASITAS TANGKI AIR TANDON:</span>
            <span class="val-large text-cyan">
              <strong>{waterLevelPct}%</strong>
              <span class="unit">({waterVolumeL} L / 1000 L)</span>
            </span>
          </div>
          <div class="metric-note">
            <span>Jarak Sensor: <strong>{ultrasonicDist} cm</strong></span>
          </div>
        </div>

        <!-- Water Level Visual Progress Bar -->
        <div class="meter-bar-container">
          <div
            class="meter-bar"
            style="width: {waterLevelPct}%; background-color: #0284c7;"
          ></div>
        </div>

        <!-- Water Actuators / Pump Status -->
        <div class="actuator-grid">
          <div class="actuator-item">
            <span class="lbl">Pompa Air Tandon:</span>
            <button
              type="button"
              class="btn-act {pumpRunning ? 'btn-active' : 'btn-standby'}"
              onclick={togglePump}
            >
              {pumpRunning ? 'POMPA MENYALA' : 'POMPA MATI'}
            </button>
          </div>

          <div class="actuator-item">
            <span class="lbl">Saluran Limpasan:</span>
            <span class="badge {divertValveOpen ? 'active' : 'inactive'}">
              {divertValveOpen ? 'DIALIHKAN KE TAMAN' : 'NORMAL / TERTUTUP'}
            </span>
          </div>
        </div>

        <!-- Safety Note -->
        <div class="rule-box rule-box-water">
          <span class="rule-title">PENGISIAN OTOMATIS:</span>
          <p class="rule-desc">
            Jika tandon penuh 100%, limpasan otomatis dialihkan ke penyiraman tanaman.
          </p>
        </div>
      </div>
    </div>
  </div>
</div>

<style>
  .title-with-bar {
    display: flex;
    align-items: center;
    gap: 8px;
  }

  .amber-bar {
    width: 3px;
    height: 14px;
    background-color: var(--accent-amber);
  }

  .sensors-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
    gap: 12px;
    padding: 12px;
  }

  .card {
    background-color: var(--bg-card);
    border: 1px solid var(--border-medium);
    border-radius: var(--radius-sm);
    display: flex;
    flex-direction: column;
  }

  .card-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    background-color: var(--bg-panel-header);
    border-bottom: 1px solid var(--border-medium);
    padding: 7px 12px;
    font-family: var(--font-mono);
    font-size: 11px;
  }

  .card-title {
    font-weight: 700;
    color: var(--text-primary);
  }

  .sensor-body {
    padding: 12px;
    display: flex;
    flex-direction: column;
    gap: 10px;
  }

  /* Metric Highlight Box */
  .metric-highlight {
    background: var(--bg-panel-header);
    border: 1px solid var(--border-medium);
    border-radius: var(--radius-sm);
    padding: 10px 12px;
    display: flex;
    justify-content: space-between;
    align-items: center;
  }

  .highlight-alert {
    border-color: #fca5a5;
    background: #fee2e2;
  }

  .highlight-water {
    border-color: #bae6fd;
    background: #f0f9ff;
  }

  .metric-info {
    display: flex;
    flex-direction: column;
    gap: 2px;
  }

  .lbl {
    font-family: var(--font-mono);
    font-size: 10px;
    color: var(--text-muted);
    letter-spacing: 0.04em;
  }

  .val-large {
    font-family: var(--font-mono);
    font-size: 22px;
    line-height: 1;
  }

  .val-large .unit {
    font-size: 12px;
    color: var(--text-secondary);
  }

  .text-green { color: #15803d; }
  .text-alert { color: #dc2626; }
  .text-cyan { color: #0284c7; }

  .metric-note {
    font-family: var(--font-mono);
    font-size: 11px;
    color: var(--text-secondary);
    display: flex;
    flex-direction: column;
    align-items: flex-end;
    gap: 4px;
  }

  /* Meter Bar */
  .meter-bar-container {
    height: 6px;
    background: var(--bg-hover);
    border: 1px solid var(--border-medium);
    border-radius: var(--radius-sm);
    overflow: hidden;
  }

  .meter-bar {
    height: 100%;
    border-radius: var(--radius-sm);
  }

  /* Actuator Grid */
  .actuator-grid {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 8px;
    padding-top: 2px;
  }

  .actuator-item {
    background: var(--bg-panel-header);
    border: 1px solid var(--border-medium);
    border-radius: var(--radius-sm);
    padding: 8px 10px;
    display: flex;
    flex-direction: column;
    gap: 6px;
  }

  .actuator-item .lbl {
    font-size: 10px;
    color: var(--text-secondary);
    font-family: var(--font-mono);
  }

  .btn-act {
    font-size: 10px;
    padding: 5px 8px;
    font-weight: 700;
    border-radius: var(--radius-sm);
  }

  .btn-active {
    background: #dcfce7;
    color: #15803d;
    border: 1px solid #86efac;
  }

  .btn-standby {
    background: var(--bg-card);
    color: var(--text-secondary);
    border: 1px solid var(--border-medium);
  }

  .btn-danger {
    background: #fee2e2;
    color: #b91c1c;
    border: 1px solid #fca5a5;
  }

  /* Rule Box */
  .rule-box {
    background: #fffbeb;
    border: 1px solid rgba(245, 158, 11, 0.4);
    border-radius: var(--radius-sm);
    padding: 8px 10px;
    display: flex;
    flex-direction: column;
    gap: 2px;
  }

  .rule-box-water {
    background: #f0f9ff;
    border-color: rgba(2, 132, 199, 0.4);
  }

  .rule-title {
    font-family: var(--font-mono);
    font-size: 10px;
    font-weight: 700;
    color: #d97706;
  }

  .rule-box-water .rule-title {
    color: #0284c7;
  }

  .rule-desc {
    font-size: 11px;
    color: var(--text-secondary);
    line-height: 1.3;
    margin: 0;
  }

  @media (max-width: 768px) {
    .sensors-grid {
      grid-template-columns: 1fr;
    }
    .actuator-grid {
      grid-template-columns: 1fr;
    }
  }
</style>
