<script>
  import SceneControlBar from './SceneControlBar.svelte';
  import Zap from '@lucide/svelte/icons/zap';
  import Sun from '@lucide/svelte/icons/sun';
  import Battery from '@lucide/svelte/icons/battery';
  import BatteryCharging from '@lucide/svelte/icons/battery-charging';
  import Wind from '@lucide/svelte/icons/wind';
  import Lock from '@lucide/svelte/icons/lock';
  import Unlock from '@lucide/svelte/icons/unlock';
  import Warehouse from '@lucide/svelte/icons/warehouse';
  import Lightbulb from '@lucide/svelte/icons/lightbulb';
  import Flame from '@lucide/svelte/icons/flame';
  import ChevronDown from '@lucide/svelte/icons/chevron-down';
  import ChevronUp from '@lucide/svelte/icons/chevron-up';

  let {
    telemetry = {},
    activeDevice = {},
    isDeviceOnline = true,
    onCommand = () => {},
    onToggleLock = () => {},
    onSceneChange = () => {},
  } = $props();

  // HVAC state
  let targetTemp = $state(22);
  let hvacMode = $state('COOL'); // 'AUTO' | 'COOL' | 'FAN'
  let isHvacBusy = $state(false);
  let isGarageBusy = $state(false);
  let isAlarmBusy = $state(false);
  let showAllLamps = $state(false);

  $effect(() => {
    if (telemetry.hvac_target_temp !== undefined) {
      targetTemp = telemetry.hvac_target_temp;
    }
  });

  // Power metrics
  let houseWatts = $derived(Number(telemetry.house_power_w ?? 0));
  let plnLimitWatts = 900;
  let plnPct = $derived(Math.min(100, Math.max(0, (houseWatts / plnLimitWatts) * 100)));
  let solarWatts = $derived(Number(telemetry.pv_power_watts ?? 0));
  let batteryPct = $derived(Number(telemetry.ups_battery_pct ?? 98));
  let isSolarSurplus = $derived(solarWatts > houseWatts);

  // HVAC actions
  async function handleHvacPower(newPower) {
    isHvacBusy = true;
    try {
      await onCommand({
        command: 'set_hvac',
        power: newPower,
        target_temp: targetTemp,
      });
    } finally {
      isHvacBusy = false;
    }
  }

  async function handleTempChange(newTemp) {
    const val = Math.max(16, Math.min(30, newTemp));
    targetTemp = val;
    isHvacBusy = true;
    try {
      await onCommand({
        command: 'set_hvac',
        power: telemetry.hvac_power ?? true,
        target_temp: val,
      });
    } finally {
      isHvacBusy = false;
    }
  }

  // Garage Door action
  async function handleGarage(targetState) {
    isGarageBusy = true;
    try {
      await onCommand({
        command: 'set_garage',
        state: targetState,
      });
    } finally {
      isGarageBusy = false;
    }
  }

  // Emergency Alarm action
  async function handleAlarm(targetState) {
    isAlarmBusy = true;
    try {
      await onCommand({
        command: 'set_emergency_alarm',
        state: targetState,
      });
    } finally {
      isAlarmBusy = false;
    }
  }

  // Lamp definitions: 4 Main, 7 Additional
  const MAIN_LAMPS = [
    { id: 'living_room', name: 'Ruang Tamu', zone: 'Utama' },
    { id: 'yard_front', name: 'Teras Depan', zone: 'Luar' },
    { id: 'kitchen', name: 'Dapur', zone: 'Servis' },
    { id: 'garage', name: 'Garasi Mobil', zone: 'Parkir' },
  ];

  const EXTRA_LAMPS = [
    { id: 'main_room', name: 'Kamar Utama', zone: 'Lantai 1' },
    { id: 'stair_garage', name: 'Tangga Garasi', zone: 'Lantai 1' },
    { id: 'stair_fl2', name: 'Tangga Lantai 2', zone: 'Lantai 2' },
    { id: 'yard_side', name: 'Halaman Samping', zone: 'Luar' },
    { id: 'yard_back', name: 'Halaman Belakang', zone: 'Luar' },
    { id: 'server_room', name: 'Ruang Server', zone: 'Fasilitas' },
    { id: 'electrical_room', name: 'Ruang Panel', zone: 'Fasilitas' },
  ];

  async function toggleLamp(lampId, currentState) {
    await onCommand({
      command: 'set_lamp',
      room: lampId,
      state: !currentState,
    });
  }

  async function setAllLamps(state) {
    await onCommand({
      command: 'set_lamp',
      room: 'all',
      state: state,
    });
  }

  let activeLampsCount = $derived(
    [...MAIN_LAMPS, ...EXTRA_LAMPS].filter((l) => !!telemetry.lamps?.[l.id]).length
  );
</script>

<div class="home-view-container">
  <!-- 1. SCENE OTOMASI RUMAH -->
  <SceneControlBar {onSceneChange} />

  <!-- 2. GRID UTAMA PERALATAN & DAYA (PRELINE SMART HOME CARDS) -->
  <div class="home-appliances-grid">
    <!-- KARTU 1: AC RUANGAN (DENGAN TOGGLE & SLIDER SUHU) -->
    <div class="panel appliance-card">
      <div class="card-header">
        <div class="header-title">
          <span class="icon-wrap text-cyan"><Wind size={15} /></span>
          <div>
            <h3>AC RUANG TAMU</h3>
            <span class="card-subtitle">Living Room &bull; Low Watt (~330W)</span>
          </div>
        </div>
        <label class="toggle-switch" title={telemetry.hvac_power ? 'Matikan AC' : 'Nyalakan AC'}>
          <input
            type="checkbox"
            checked={!!telemetry.hvac_power}
            disabled={isHvacBusy}
            onchange={(e) => handleHvacPower(e.target.checked)}
          />
          <span class="toggle-track"></span>
        </label>
      </div>

      <div class="card-body">
        <div class="temp-hero">
          <div class="temp-display">
            <span class="temp-big">{targetTemp}</span>
            <span class="temp-unit">°C</span>
          </div>
          <span class="badge {telemetry.hvac_power ? 'active' : 'inactive'}">
            {telemetry.hvac_power ? 'MENYALA (~330W)' : 'MATI (0W)'}
          </span>
        </div>

        <!-- Slider Range Pengatur Suhu Interaktif -->
        <div class="slider-control-group">
          <div class="slider-labels">
            <span>16°C</span>
            <span class="slider-lbl-mid">Geser Suhu Target</span>
            <span>30°C</span>
          </div>
          <input
            type="range"
            class="range-slider"
            min="16"
            max="30"
            step="1"
            value={targetTemp}
            disabled={isHvacBusy || !telemetry.hvac_power}
            onchange={(e) => handleTempChange(Number(e.target.value))}
          />
        </div>

        <!-- Quick Mode Pills -->
        <div class="hvac-modes">
          {#each ['AUTO', 'COOL', 'FAN'] as mode}
            <button
              type="button"
              class="mode-btn {hvacMode === mode ? 'active' : ''}"
              disabled={isHvacBusy || !telemetry.hvac_power}
              onclick={() => (hvacMode = mode)}
            >
              {mode}
            </button>
          {/each}
        </div>
      </div>
    </div>

    <!-- KARTU 2: KONSUMSI LISTRIK RUMAH (BEBAN VS PLN 900W) -->
    <div class="panel appliance-card">
      <div class="card-header">
        <div class="header-title">
          <span class="icon-wrap text-amber"><Zap size={15} /></span>
          <div>
            <h3>KONSUMSI LISTRIK</h3>
            <span class="card-subtitle">Batas Daya PLN 900W (MCB 4A)</span>
          </div>
        </div>
        <span class="badge {plnPct > 88 ? 'alert' : 'active'}">
          {houseWatts.toFixed(0)} / {plnLimitWatts} W
        </span>
      </div>

      <div class="card-body">
        <div class="metric-meter">
          <div class="meter-info">
            <span class="meter-lbl">Beban Rumah Saat Ini</span>
            <span class="meter-val">{plnPct.toFixed(0)}% Kuota</span>
          </div>
          <div class="meter-track">
            <div
              class="meter-fill {plnPct > 88 ? 'fill-alert' : plnPct > 68 ? 'fill-warn' : 'fill-cyan'}"
              style="width: {plnPct}%;"
            ></div>
          </div>
        </div>

        <div class="quick-stats-row">
          <div class="quick-stat">
            <span class="qs-lbl flex-center"><Sun size={11} class="text-green" /> Solar Yield</span>
            <span class="qs-val text-green">{telemetry.solar_yield_kwh?.toFixed(2) ?? '0.00'} kWh</span>
          </div>
          <div class="quick-stat">
            <span class="qs-lbl flex-center">
              {#if isSolarSurplus}
                <BatteryCharging size={11} class="text-amber" />
              {:else}
                <Battery size={11} class="text-amber" />
              {/if}
              Baterai UPS
            </span>
            <span class="qs-val text-amber">{batteryPct.toFixed(0)}%</span>
          </div>
          <div class="quick-stat">
            <span class="qs-lbl">Suhu Luar</span>
            <span class="qs-val">{telemetry.temp_outdoor_c ?? 28.5}°C</span>
          </div>
        </div>
      </div>
    </div>

    <!-- KARTU 3: KEAMANAN & AKSES PINTU GARASI -->
    <div class="panel appliance-card">
      <div class="card-header">
        <div class="header-title">
          <span class="icon-wrap text-green"><Lock size={15} /></span>
          <div>
            <h3>KEAMANAN & AKSES</h3>
            <span class="card-subtitle">Kunci Smart Lock & Pintu Garasi</span>
          </div>
        </div>
        <span class="badge {activeDevice.is_locked ? 'active' : 'warning'}">
          {activeDevice.is_locked ? 'TERKUNCI' : 'BEBAS'}
        </span>
      </div>

      <div class="card-body">
        <div class="access-rows">
          <!-- Item 1: Smart Lock Node -->
          <div class="access-item">
            <div class="access-info">
              <span class="access-icon">{activeDevice.is_locked ? '🔒' : '🔓'}</span>
              <div>
                <span class="access-title">Kunci Pengaman Node</span>
                <span class="access-sub">{activeDevice.is_locked ? 'Proteksi konfigurasi aktif' : 'Konfigurasi dapat diedit'}</span>
              </div>
            </div>
            <label class="toggle-switch" title="Kunci atau buka proteksi perangkat">
              <input
                type="checkbox"
                checked={!!activeDevice.is_locked}
                onchange={() => onToggleLock(activeDevice)}
              />
              <span class="toggle-track"></span>
            </label>
          </div>

          <!-- Item 2: Pintu Garasi Mobil -->
          <div class="access-item">
            <div class="access-info">
              <span class="access-icon"><Warehouse size={14} /></span>
              <div>
                <span class="access-title">Gerbang Garasi Mobil</span>
                <span class="access-sub">{telemetry.garage_open ? 'Pintu posisi terbuka' : 'Pintu tertutup rapat'}</span>
              </div>
            </div>
            <button
              type="button"
              class="btn-sm {telemetry.garage_open ? 'btn-danger' : 'btn-success'}"
              disabled={isGarageBusy}
              onclick={() => handleGarage(!telemetry.garage_open)}
            >
              {telemetry.garage_open ? 'TUTUP PINTU' : 'BUKA PINTU'}
            </button>
          </div>
        </div>
      </div>
    </div>

    <!-- KARTU 4: BAHAYA & ALARM GAS -->
    <div class="panel appliance-card">
      <div class="card-header">
        <div class="header-title">
          <span class="icon-wrap text-alert"><Flame size={15} /></span>
          <div>
            <h3>SISTEM GAS & ALARM</h3>
            <span class="card-subtitle">Detektor Gas Dapur & Peringatan Dini</span>
          </div>
        </div>
        <span class="badge {telemetry.gas_ppm > 400 ? 'alert' : 'active'}">
          {telemetry.gas_ppm > 400 ? 'BAHAYA KEBOCORAN' : 'KONDISI AMAN'}
        </span>
      </div>

      <div class="card-body">
        <div class="gas-summary">
          <div class="gas-metric">
            <span class="gas-lbl">Konsentrasi Gas Saat Ini</span>
            <span class="gas-val {telemetry.gas_ppm > 400 ? 'text-alert' : 'text-primary'}">
              {telemetry.gas_ppm ?? 145} <span class="gas-unit">PPM</span>
            </span>
          </div>
          <span class="gas-threshold">Ambang bahaya: &gt; 400 PPM</span>
        </div>

        <button
          type="button"
          class="btn-full {telemetry.emergency_alarm ? 'btn-danger' : 'btn-secondary'}"
          disabled={isAlarmBusy}
          onclick={() => handleAlarm(!telemetry.emergency_alarm)}
        >
          <Flame size={13} />
          <span>{telemetry.emergency_alarm ? 'MATIKAN SIRENE DARURAT' : 'UJI SIRENE ALARM'}</span>
        </button>
      </div>
    </div>
  </div>

  <!-- 3. KONTROL LAMPU UTAMA (PRELINE SMART LAMP CARDS) -->
  <div class="panel lamps-section-panel">
    <div class="panel-header">
      <div class="title-with-bar">
        <span class="amber-bar"></span>
        <h2>SAKLAR LAMPU RUMAH</h2>
        <span class="badge amber active-counter">{activeLampsCount} / 11 Menyala</span>
      </div>

      <div class="lamps-header-actions">
        <button
          type="button"
          class="btn-sm btn-success"
          onclick={() => setAllLamps(true)}
        >
          NYALAKAN SEMUA
        </button>
        <button
          type="button"
          class="btn-sm btn-danger"
          onclick={() => setAllLamps(false)}
        >
          MATIKAN SEMUA
        </button>
      </div>
    </div>

    <!-- Grid 4 Lampu Utama Rumah -->
    <div class="lamps-preline-grid">
      {#each MAIN_LAMPS as lamp}
        {@const isOn = !!telemetry.lamps?.[lamp.id]}
        <div class="lamp-card {isOn ? 'lamp-on' : 'lamp-off'}">
          <div class="lamp-card-top">
            <div class="lamp-icon-wrap {isOn ? 'icon-on' : 'icon-off'}">
              <Lightbulb size={18} />
            </div>
            <label class="toggle-switch" title="Saklar {lamp.name}">
              <input
                type="checkbox"
                checked={isOn}
                onchange={() => toggleLamp(lamp.id, isOn)}
              />
              <span class="toggle-track"></span>
            </label>
          </div>

          <div class="lamp-card-bottom">
            <span class="lamp-title">{lamp.name}</span>
            <span class="lamp-meta">Zona {lamp.zone} &bull; {isOn ? '~9W' : '0W'}</span>
          </div>
        </div>
      {/each}
    </div>

    <!-- Tombol Ekspansi 7 Ruangan Tambahan -->
    <div class="lamps-expander-wrapper">
      <button
        type="button"
        class="btn-expander"
        onclick={() => (showAllLamps = !showAllLamps)}
      >
        {#if showAllLamps}
          <ChevronUp size={13} />
          <span>Sembunyikan 7 Ruangan Tambahan</span>
        {:else}
          <ChevronDown size={13} />
          <span>Tampilkan 7 Ruangan Tambahan (Lantai 2, Kamar, Luar)</span>
        {/if}
      </button>
    </div>

    <!-- Grid 7 Ruangan Tambahan (Collapsible) -->
    {#if showAllLamps}
      <div class="lamps-preline-grid extra-grid">
        {#each EXTRA_LAMPS as lamp}
          {@const isOn = !!telemetry.lamps?.[lamp.id]}
          <div class="lamp-card {isOn ? 'lamp-on' : 'lamp-off'}">
            <div class="lamp-card-top">
              <div class="lamp-icon-wrap {isOn ? 'icon-on' : 'icon-off'}">
                <Lightbulb size={18} />
              </div>
              <label class="toggle-switch" title="Saklar {lamp.name}">
                <input
                  type="checkbox"
                  checked={isOn}
                  onchange={() => toggleLamp(lamp.id, isOn)}
                />
                <span class="toggle-track"></span>
              </label>
            </div>

            <div class="lamp-card-bottom">
              <span class="lamp-title">{lamp.name}</span>
              <span class="lamp-meta">Zona {lamp.zone} &bull; {isOn ? '~9W' : '0W'}</span>
            </div>
          </div>
        {/each}
      </div>
    {/if}
  </div>
</div>

<style>
  .home-view-container {
    display: flex;
    flex-direction: column;
    gap: 14px;
    padding-bottom: 24px;
    font-family: var(--font-mono);
  }

  .home-appliances-grid {
    display: grid;
    grid-template-columns: repeat(2, 1fr);
    gap: 14px;
  }

  @media (max-width: 1024px) {
    .home-appliances-grid {
      grid-template-columns: 1fr;
    }
  }

  .appliance-card {
    display: flex;
    flex-direction: column;
    background-color: var(--bg-panel);
    border: 1px solid var(--border-medium);
    border-radius: var(--radius-sm);
  }

  .card-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 10px 14px;
    background-color: var(--bg-panel-header);
    border-bottom: 1px solid var(--border-medium);
  }

  .header-title {
    display: flex;
    align-items: center;
    gap: 8px;
  }

  .header-title h3 {
    margin: 0;
    font-size: 12px;
    font-weight: 700;
    color: var(--text-primary);
  }

  .card-subtitle {
    font-size: 10px;
    color: var(--text-muted);
  }

  .icon-wrap {
    display: inline-flex;
    align-items: center;
  }

  .card-body {
    padding: 14px;
    display: flex;
    flex-direction: column;
    gap: 12px;
  }

  /* AC Hero Temperature */
  .temp-hero {
    display: flex;
    justify-content: space-between;
    align-items: baseline;
  }

  .temp-display {
    display: flex;
    align-items: baseline;
    gap: 2px;
  }

  .temp-big {
    font-size: 32px;
    font-weight: 700;
    color: var(--accent-cyan);
    line-height: 1;
  }

  .temp-unit {
    font-size: 15px;
    font-weight: 700;
    color: var(--text-secondary);
  }

  .slider-control-group {
    display: flex;
    flex-direction: column;
    gap: 2px;
  }

  .slider-labels {
    display: flex;
    justify-content: space-between;
    font-size: 10px;
    color: var(--text-muted);
  }

  .slider-lbl-mid {
    font-size: 9px;
    color: var(--text-secondary);
  }

  .hvac-modes {
    display: grid;
    grid-template-columns: repeat(3, 1fr);
    gap: 6px;
  }

  .mode-btn {
    padding: 6px;
    font-size: 10px;
    font-weight: 700;
    background-color: var(--bg-card);
    border: 1px solid var(--border-medium);
    border-radius: var(--radius-sm);
    color: var(--text-secondary);
    cursor: pointer;
    font-family: var(--font-mono);
  }
  .mode-btn:hover:not(:disabled) {
    color: var(--text-primary);
    border-color: var(--border-heavy);
  }
  .mode-btn.active {
    background-color: #f0f9ff;
    border-color: var(--accent-cyan);
    color: var(--accent-cyan);
  }
  .mode-btn:disabled {
    opacity: 0.5;
    cursor: not-allowed;
  }

  /* Meter / Progress */
  .metric-meter {
    display: flex;
    flex-direction: column;
    gap: 5px;
  }

  .meter-info {
    display: flex;
    justify-content: space-between;
    font-size: 10px;
    color: var(--text-secondary);
  }

  .meter-val {
    font-weight: 700;
    color: var(--text-primary);
  }

  .meter-track {
    height: 7px;
    background-color: var(--bg-hover);
    border: 1px solid var(--border-medium);
    border-radius: var(--radius-sm);
    overflow: hidden;
  }

  .meter-fill {
    height: 100%;
    transition: width 0.3s ease;
  }

  .fill-cyan { background-color: var(--pln-cyan); }
  .fill-warn { background-color: var(--accent-amber); }
  .fill-alert { background-color: var(--status-alert); }

  /* Quick stats row */
  .quick-stats-row {
    display: grid;
    grid-template-columns: repeat(3, 1fr);
    gap: 8px;
    padding-top: 6px;
    border-top: 1px solid var(--border-light);
  }

  .quick-stat {
    display: flex;
    flex-direction: column;
    gap: 2px;
    background: var(--bg-panel-header);
    padding: 6px 10px;
    border: 1px solid var(--border-medium);
    border-radius: var(--radius-sm);
  }

  .qs-lbl {
    font-size: 10px;
    color: var(--text-muted);
  }

  .qs-val {
    font-size: 13px;
    font-weight: 700;
    color: var(--text-primary);
  }

  .flex-center {
    display: inline-flex;
    align-items: center;
    gap: 4px;
  }

  /* Access & Security Items */
  .access-rows {
    display: flex;
    flex-direction: column;
    gap: 8px;
  }

  .access-item {
    display: flex;
    justify-content: space-between;
    align-items: center;
    background-color: var(--bg-panel-header);
    padding: 8px 12px;
    border: 1px solid var(--border-medium);
    border-radius: var(--radius-sm);
  }

  .access-info {
    display: flex;
    align-items: center;
    gap: 10px;
  }

  .access-icon {
    font-size: 14px;
    display: inline-flex;
    align-items: center;
    color: var(--text-secondary);
  }

  .access-title {
    font-size: 11px;
    font-weight: 700;
    color: var(--text-primary);
    display: block;
  }

  .access-sub {
    font-size: 9px;
    color: var(--text-muted);
    display: block;
  }

  /* Gas Summary */
  .gas-summary {
    display: flex;
    justify-content: space-between;
    align-items: center;
    background-color: var(--bg-panel-header);
    padding: 8px 12px;
    border: 1px solid var(--border-medium);
    border-radius: var(--radius-sm);
  }

  .gas-metric {
    display: flex;
    flex-direction: column;
    gap: 1px;
  }

  .gas-lbl {
    font-size: 10px;
    color: var(--text-muted);
  }

  .gas-val {
    font-size: 16px;
    font-weight: 700;
  }

  .gas-unit {
    font-size: 11px;
    color: var(--text-secondary);
  }

  .gas-threshold {
    font-size: 9px;
    color: var(--text-muted);
  }

  .btn-full {
    width: 100%;
    padding: 8px 12px;
    display: inline-flex;
    align-items: center;
    justify-content: center;
    gap: 6px;
    font-family: var(--font-mono);
    font-size: 11px;
    font-weight: 700;
    cursor: pointer;
    border-radius: var(--radius-sm);
  }

  /* Lamp Section & Preline Cards */
  .lamps-section-panel {
    background-color: var(--bg-panel);
    border: 1px solid var(--border-medium);
    border-radius: var(--radius-sm);
  }

  .title-with-bar {
    display: flex;
    align-items: center;
    gap: 8px;
    flex-wrap: wrap;
  }

  .amber-bar {
    width: 3px;
    height: 14px;
    background-color: var(--accent-amber);
    border-radius: var(--radius-sm);
  }

  .active-counter {
    font-size: 10px;
    padding: 2px 6px;
  }

  .lamps-header-actions {
    display: flex;
    gap: 6px;
  }

  .lamps-preline-grid {
    display: grid;
    grid-template-columns: repeat(4, 1fr);
    gap: 12px;
    padding: 14px;
  }

  .extra-grid {
    padding-top: 0;
    border-top: 1px solid var(--border-light);
  }

  @media (max-width: 1024px) {
    .lamps-preline-grid {
      grid-template-columns: repeat(2, 1fr);
    }
  }

  @media (max-width: 600px) {
    .lamps-preline-grid {
      grid-template-columns: 1fr;
    }
  }

  .lamp-card {
    display: flex;
    flex-direction: column;
    justify-content: space-between;
    gap: 12px;
    background-color: var(--bg-card);
    border: 1px solid var(--border-medium);
    border-radius: var(--radius-sm);
    padding: 12px;
    transition: all 0.15s ease;
  }

  .lamp-card.lamp-on {
    border-color: var(--accent-amber);
    background-color: #fffbeb;
  }

  .lamp-card-top {
    display: flex;
    justify-content: space-between;
    align-items: center;
  }

  .lamp-icon-wrap {
    width: 32px;
    height: 32px;
    display: inline-flex;
    align-items: center;
    justify-content: center;
    border: 1px solid var(--border-medium);
    border-radius: var(--radius-sm);
    background-color: var(--bg-panel-header);
  }

  .lamp-icon-wrap.icon-on {
    background-color: #fef3c7;
    border-color: var(--accent-amber);
    color: var(--accent-amber);
  }

  .lamp-icon-wrap.icon-off {
    color: var(--text-muted);
  }

  .lamp-card-bottom {
    display: flex;
    flex-direction: column;
    gap: 2px;
  }

  .lamp-title {
    font-size: 12px;
    font-weight: 700;
    color: var(--text-primary);
  }

  .lamp-meta {
    font-size: 10px;
    color: var(--text-muted);
  }

  .lamps-expander-wrapper {
    display: flex;
    justify-content: center;
    padding: 6px 14px 12px 14px;
  }

  .btn-expander {
    display: inline-flex;
    align-items: center;
    gap: 6px;
    padding: 6px 14px;
    font-family: var(--font-mono);
    font-size: 11px;
    font-weight: 700;
    background-color: var(--bg-card);
    border: 1px solid var(--border-medium);
    border-radius: var(--radius-sm);
    color: var(--text-secondary);
    cursor: pointer;
  }

  .btn-expander:hover {
    background-color: var(--bg-hover);
    color: var(--text-primary);
    border-color: var(--border-heavy);
  }
</style>
