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

<div class="other-sensors-container">
  <!-- Card 1: Gas & Fire Safety Unit -->
  <div class="panel sensor-card">
    <div class="panel-header">
      <div class="hdr-title">
        <div class="title-with-bar">
          <span class="amber-bar"></span>
          <h2>DETEKTOR GAS & KEBAKARAN</h2>
        </div>
      </div>
      <div class="hdr-badges">
        <span class="badge {isGasDanger ? 'alert' : 'active'}">
          {isGasDanger ? 'BAHAYA KEBOCORAN' : 'KONDISI AMAN'}
        </span>
      </div>
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
          <span class="badge {isGasDanger ? 'alert' : 'active'}">
            {isGasDanger ? 'BAHAYA' : 'NORMAL'}
          </span>
        </div>
      </div>

      <!-- Meter Progress Bar -->
      <div class="meter-bar-container">
        <div
          class="meter-bar"
          style="width: {Math.min(100, (gasPpm / 600) * 100)}%; background-color: {isGasDanger ? '#ef4444' : '#22c55e'};"
        ></div>
      </div>

      <!-- Hardware Actuators / Solenoid Status -->
      <div class="actuator-grid">
        <div class="actuator-item">
          <span class="lbl">Keran Pipa Gas Utama:</span>
          <button
            class="btn-act {gasValveOpen ? 'btn-active' : 'btn-danger'}"
            onclick={toggleGasValve}
          >
            {gasValveOpen ? 'KERAN TERBUKA (ON)' : 'KERAN DITUTUP (OFF)'}
          </button>
        </div>

        <div class="actuator-item">
          <span class="lbl">Sirine Alarm Darurat:</span>
          <button
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
  <div class="panel sensor-card">
    <div class="panel-header">
      <div class="hdr-title">
        <div class="title-with-bar">
          <span class="amber-bar"></span>
          <h2>TANGKI AIR & POMPA RUMAH</h2>
        </div>
      </div>
      <div class="hdr-badges">
        <span class="badge active">LEVEL: {waterLevelPct}%</span>
      </div>
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
          <span>Jarak Sensor Air: <strong>{ultrasonicDist} cm</strong></span>
        </div>
      </div>

      <!-- Water Level Visual Progress Bar -->
      <div class="meter-bar-container">
        <div
          class="meter-bar"
          style="width: {waterLevelPct}%; background-color: #38bdf8;"
        ></div>
      </div>

      <!-- Water Actuators / Pump Status -->
      <div class="actuator-grid">
        <div class="actuator-item">
          <span class="lbl">Pompa Air Tandon:</span>
          <button
            class="btn-act {pumpRunning ? 'btn-active' : 'btn-standby'}"
            onclick={togglePump}
          >
            {pumpRunning ? 'POMPA MENYALA' : 'POMPA MATI'}
          </button>
        </div>

        <div class="actuator-item">
          <span class="lbl">Saluran Limpasan (Overflow):</span>
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

<style>
  .other-sensors-container {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(480px, 1fr));
    gap: 16px;
  }

  .sensor-card {
    background-color: var(--bg-panel);
    border: 1px solid var(--border-medium);
    display: flex;
    flex-direction: column;
  }

  .hdr-title {
    display: flex;
    flex-direction: column;
    gap: 2px;
  }

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

  .hdr-badges {
    display: flex;
    gap: 6px;
    align-items: center;
  }

  .sensor-body {
    padding: 14px;
    display: flex;
    flex-direction: column;
    gap: 12px;
  }

  /* Metric Highlight Box */
  .metric-highlight {
    background: #0f141c;
    border: 1px solid #1f2937;
    padding: 10px 14px;
    display: flex;
    justify-content: space-between;
    align-items: center;
  }

  .highlight-alert {
    border-color: #dc2626;
    background: #450a0a;
  }

  .highlight-water {
    border-color: #0369a1;
    background: #082f49;
  }

  .metric-info {
    display: flex;
    flex-direction: column;
    gap: 2px;
  }

  .val-large {
    font-family: var(--font-mono);
    font-size: 24px;
    line-height: 1;
  }

  .val-large .unit {
    font-size: 13px;
    color: var(--text-secondary);
  }

  .text-green { color: #4ade80; }
  .text-alert { color: #f87171; }
  .text-cyan { color: #38bdf8; }

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
    height: 8px;
    background: #181818;
    border: 1px solid #333333;
    overflow: hidden;
  }

  .meter-bar {
    height: 100%;
  }

  /* Actuator Grid */
  .actuator-grid {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 10px;
    padding-top: 4px;
  }

  .actuator-item {
    background: #181818;
    border: 1px solid var(--border-light);
    padding: 8px 10px;
    display: flex;
    flex-direction: column;
    gap: 6px;
  }

  .actuator-item .lbl {
    font-size: 11px;
    color: var(--text-secondary);
    font-family: var(--font-mono);
  }

  .btn-act {
    font-size: 10px;
    padding: 6px 10px;
    font-weight: 700;
  }

  .btn-active {
    background: #166534;
    color: #4ade80;
    border-color: #22c55e;
  }

  .btn-standby {
    background: #262626;
    color: #9ca3af;
    border-color: #4b5563;
  }

  .btn-danger {
    background: #991b1b;
    color: #fca5a5;
    border-color: #ef4444;
  }

  /* Rule Box */
  .rule-box {
    background: var(--bg-card);
    border: 1px solid rgba(245, 158, 11, 0.35);
    padding: 8px 10px;
    display: flex;
    flex-direction: column;
    gap: 2px;
  }

  .rule-box-water {
    border-color: rgba(56, 189, 248, 0.35);
  }

  .rule-title {
    font-family: var(--font-mono);
    font-size: 10px;
    font-weight: 700;
    color: #fde047;
  }

  .rule-box-water .rule-title {
    color: #38bdf8;
  }

  .rule-desc {
    font-size: 11px;
    color: #a3a3a3;
    line-height: 1.3;
  }

  @media (max-width: 768px) {
    .other-sensors-container {
      grid-template-columns: 1fr;
    }
    .actuator-grid {
      grid-template-columns: 1fr;
    }
  }
</style>
