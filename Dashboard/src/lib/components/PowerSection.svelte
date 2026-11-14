<script>
  import PlnTokenCard from './PlnTokenCard.svelte';
  let { telemetry = {}, onCommand = () => {} } = $props();

  let isTogglingSolar = $state(false);

  async function handleToggleSolar() {
    isTogglingSolar = true;
    const currentState = telemetry.solar_charger_active ?? true;
    try {
      await onCommand({
        command: 'set_solar',
        charge_enabled: !currentState,
        max_watts: 2400
      });
    } finally {
      isTogglingSolar = false;
    }
  }

  let houseWatts = $derived(Number(telemetry.house_power_w ?? 0));
  let solarWatts = $derived(Number(telemetry.pv_power_watts ?? 0));
  let batteryPct = $derived(Number(telemetry.ups_battery_pct ?? 98));
</script>

<div class="panel">
  <div class="panel-header">
    <div class="title-with-bar">
      <span class="amber-bar"></span>
      <h2>KELISTRIKAN RUMAH (PLN 900W & SOLAR 2400W)</h2>
    </div>
    <span class="badge amber">SOLAR & BATERAI OFF-GRID</span>
  </div>

  <div class="grid-3">
    <!-- 1. SOLAR PV ARRAY (2400Wp) -->
    <div class="card">
      <div class="card-header">
        <span class="card-title">PANEL SURYA (2400W)</span>
        <span class="badge {telemetry.solar_charger_active ? 'active' : 'inactive'}">
          {telemetry.solar_charger_active ? 'CHARGER: AKTIF' : 'CHARGER: MATI'}
        </span>
      </div>

      <div class="card-body">
        <div class="data-row highlight-green">
          <span class="data-label">Daya Output Solar:</span>
          <span class="data-val text-green"><strong>{solarWatts.toFixed(0)}</strong> W</span>
        </div>
        <div class="data-row">
          <span class="data-label">Tegangan Panel:</span>
          <span class="data-val"><strong>{telemetry.pv_array_voltage?.toFixed(1) ?? '--'}</strong> V</span>
        </div>
        <div class="data-row">
          <span class="data-label">Arus Panel:</span>
          <span class="data-val"><strong>{telemetry.pv_array_current?.toFixed(2) ?? '--'}</strong> A</span>
        </div>
        <div class="data-row">
          <span class="data-label">Panen Hari Ini:</span>
          <span class="data-val"><strong>{telemetry.solar_yield_kwh?.toFixed(2) ?? '--'}</strong> kWh</span>
        </div>
        <div class="data-row">
          <span class="data-label">Suhu Panel:</span>
          <span class="data-val"><strong>{telemetry.panel_temp_c?.toFixed(1) ?? '--'}</strong> °C</span>
        </div>

        <div class="action-row">
          <button
            type="button"
            class={telemetry.solar_charger_active ? 'btn-danger' : 'btn-success'}
            disabled={isTogglingSolar}
            onclick={handleToggleSolar}
          >
            {isTogglingSolar ? 'MEMPROSES...' : telemetry.solar_charger_active ? 'MATIKAN CHARGER' : 'NYALAKAN CHARGER'}
          </button>
        </div>
      </div>
    </div>

    <!-- 2. MAINS POWER (PLN 900W) -->
    <div class="card">
      <div class="card-header">
        <span class="card-title">LISTRIK PLN (900W)</span>
        <span class="badge active">PLN: NORMAL</span>
      </div>

      <div class="card-body">
        <div class="data-row highlight-cyan">
          <span class="data-label">Beban Rumah Saat Ini:</span>
          <span class="data-val text-cyan"><strong>{houseWatts.toFixed(0)}</strong> W</span>
        </div>
        <div class="data-row">
          <span class="data-label">Batas Kuota PLN:</span>
          <span class="data-val"><strong>900</strong> W (MCB 4A)</span>
        </div>
        <div class="data-row">
          <span class="data-label">Tegangan Jaringan PLN:</span>
          <span class="data-val"><strong>{telemetry.mains_voltage?.toFixed(1) ?? '--'}</strong> V</span>
        </div>
        <div class="data-row">
          <span class="data-label">Saklar MCB Dapur:</span>
          <span class="data-val">
            <button
              type="button"
              class="btn-sm {telemetry.kitchen_breaker ? 'btn-danger' : 'btn-success'}"
              onclick={() => onCommand({ command: 'set_breaker', state: !telemetry.kitchen_breaker })}
            >
              {telemetry.kitchen_breaker ? 'TRIP (MATIKAN)' : 'HUBUNGKAN'}
            </button>
          </span>
        </div>
        <div class="meter-bar-wrapper">
          <div class="meter-bar-label">Pemakaian dari Batas 900W: <strong>{((houseWatts / 900) * 100).toFixed(0)}%</strong></div>
          <div class="meter-bar-track">
            <div class="meter-bar-fill" style="width: {Math.min(100, (houseWatts / 900) * 100)}%;"></div>
          </div>
        </div>
      </div>
    </div>

    <!-- 3. PLN PREPAID TOKEN & TARIFF (TOKEN PULSA LISTRIK) -->
    <PlnTokenCard />

    <!-- 4. BACKUP POWER (BATERAI RUMAH) -->
    <div class="card">
      <div class="card-header">
        <span class="card-title">BATERAI CADANGAN (4.8 kWh)</span>
        <span class="badge {batteryPct > 20 ? 'active' : 'alert'}">
          {batteryPct > 20 ? 'BATERAI: BAIK' : 'BATERAI: LEMAH'}
        </span>
      </div>

      <div class="card-body">
        <div class="data-row highlight-amber">
          <span class="data-label">Sisa Daya Baterai:</span>
          <span class="data-val text-amber"><strong>{batteryPct.toFixed(0)}</strong> %</span>
        </div>
        <div class="data-row">
          <span class="data-label">Beban Inverter:</span>
          <span class="data-val"><strong>{telemetry.ups_load_pct?.toFixed(0) ?? '--'}</strong> %</span>
        </div>
        <div class="data-row">
          <span class="data-label">Estimasi Ketahanan:</span>
          <span class="data-val">~10 - 14 Jam</span>
        </div>
        <div class="data-row">
          <span class="data-label">Status Pengisian:</span>
          <span class="data-val">{solarWatts > houseWatts ? 'Mengisi Baterai' : 'Memakai Baterai'}</span>
        </div>
        <div class="meter-bar-wrapper">
          <div class="meter-bar-label">Kapasitas Tersedia: <strong>{batteryPct.toFixed(0)}%</strong></div>
          <div class="meter-bar-track">
            <div class="meter-bar-fill fill-battery" style="width: {batteryPct}%;"></div>
          </div>
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

  .grid-3 {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
    gap: 12px;
    padding: 12px;
  }

  .card {
    background-color: var(--bg-card);
    border: 1px solid var(--border-medium);
    border-radius: var(--radius-sm);
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
    letter-spacing: 0.04em;
  }

  .card-body {
    padding: 12px;
    display: flex;
    flex-direction: column;
    gap: 6px;
  }

  .data-row {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 4px 6px;
    font-family: var(--font-mono);
    font-size: 11px;
    border-bottom: 1px solid var(--border-light);
  }

  .data-row.highlight-green {
    background-color: var(--solar-green-dim);
    border-color: var(--solar-green);
  }

  .data-row.highlight-cyan {
    background-color: var(--pln-cyan-dim);
    border-color: var(--pln-cyan);
  }

  .data-row.highlight-amber {
    background-color: var(--accent-amber-bg);
    border-color: var(--accent-amber);
  }

  .text-green strong {
    color: var(--solar-green-text);
  }

  .text-cyan strong {
    color: var(--pln-cyan-text);
  }

  .text-amber strong {
    color: var(--text-amber);
  }

  .data-label {
    color: var(--text-secondary);
    font-size: 12px;
  }

  .data-val {
    font-size: 13px;
  }

  .data-val strong {
    color: var(--text-primary);
    font-size: 14px;
    font-weight: 700;
  }

  .highlight-green .data-val strong,
  .highlight-cyan .data-val strong,
  .highlight-amber .data-val strong {
    font-size: 17px;
  }

  .action-row {
    margin-top: 8px;
    display: flex;
    justify-content: flex-end;
  }

  .meter-bar-wrapper {
    margin-top: 8px;
    padding-top: 6px;
    border-top: 1px solid var(--border-light);
    display: flex;
    flex-direction: column;
    gap: 4px;
    font-family: var(--font-mono);
    font-size: 10px;
  }

  .meter-bar-label {
    color: var(--text-secondary);
    display: flex;
    justify-content: space-between;
  }

  .meter-bar-label strong {
    color: var(--text-amber);
  }

  .meter-bar-track {
    width: 100%;
    height: 6px;
    background-color: var(--bg-hover);
    border: 1px solid var(--border-medium);
    border-radius: var(--radius-sm);
  }

  .meter-bar-fill {
    height: 100%;
    background-color: var(--pln-cyan);
    border-radius: var(--radius-sm);
  }

  .meter-bar-fill.fill-battery {
    background-color: var(--battery-lime);
  }
</style>
