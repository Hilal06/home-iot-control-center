<script>
  let {
    telemetry = {},
    isDeviceOnline = true,
    device = {},
  } = $props();
</script>

<div class="panel">
  <div class="panel-header">
    <div class="title-with-bar">
      <span class="amber-bar"></span>
      <h2>KONDISI LINGKUNGAN & JARINGAN</h2>
    </div>
  </div>

  <div class="env-grid">
    <!-- Environmental Readings -->
    <div class="card">
      <div class="card-header">
        <span class="card-title">SUHU & KELEMBABAN</span>
        <span class="badge {telemetry.water_leak ? 'alert' : 'active'}">
          {telemetry.water_leak ? 'BAHAYA BOCOR' : 'NORMAL / KERING'}
        </span>
      </div>

      <div class="card-body">
        <div class="data-row">
          <span class="data-label">Suhu Luar Ruangan:</span>
          <span class="data-val"><strong>{telemetry.temp_outdoor_c?.toFixed(1) ?? '--'}</strong> °C</span>
        </div>
        <div class="data-row">
          <span class="data-label">Kelembaban Udara:</span>
          <span class="data-val"><strong>{telemetry.humidity_pct?.toFixed(1) ?? '--'}</strong> %</span>
        </div>
        <div class="data-row">
          <span class="data-label">Suhu Ruang Inverter:</span>
          <span class="data-val"><strong>{telemetry.rack_temp_c?.toFixed(1) ?? '--'}</strong> °C</span>
        </div>
        <div class="data-row">
          <span class="data-label">Sensor Kebocoran Air:</span>
          <span class="data-val">
            <strong class={telemetry.water_leak ? 'text-alert' : 'text-green'}>
              {telemetry.water_leak ? 'Terdeteksi Bocor' : 'Kering (Aman)'}
            </strong>
          </span>
        </div>
      </div>
    </div>

    <!-- Edge Gateway Diagnostics -->
    <div class="card {isDeviceOnline ? '' : 'card-offline'}">
      <div class="card-header">
        <span class="card-title">KONEKSI & PERANGKAT</span>
        <span class="badge {isDeviceOnline ? 'active' : 'alert'}">
          {isDeviceOnline ? 'TERHUBUNG' : 'TERPUTUS / MATI'}
        </span>
      </div>

      <div class="card-body">
        {#if !isDeviceOnline}
          <div class="offline-warning-box">
            <span class="warning-title">⚠ PERANGKAT MATI / TIDAK TERDETEKSI</span>
            <p class="warning-desc">
              Tidak ada data telemetri dari {device.name || 'ESP32 Master'} (&gt; 3.5 detik). Periksa kabel daya atau koneksi Wi-Fi perangkat.
            </p>
          </div>
        {/if}

        <div class="data-row">
          <span class="data-label">Status Koneksi:</span>
          <span class="data-val">
            <strong class="{isDeviceOnline ? 'text-green' : 'text-alert'}">
              {isDeviceOnline ? 'ONLINE' : 'OFFLINE (MATI)'}
            </strong>
          </span>
        </div>
        <div class="data-row">
          <span class="data-label">Kekuatan Sinyal Wi-Fi:</span>
          <span class="data-val">
            <strong>{isDeviceOnline && telemetry.rssi_dbm !== undefined ? telemetry.rssi_dbm + ' dBm' : '-- (Mati)'}</strong>
          </span>
        </div>
        <div class="data-row">
          <span class="data-label">Kecepatan Ping:</span>
          <span class="data-val">
            <strong>{isDeviceOnline && telemetry.ping_gateway_ms !== undefined ? Number(telemetry.ping_gateway_ms).toFixed(1) + ' ms' : '--'}</strong>
          </span>
        </div>
        <div class="data-row">
          <span class="data-label">Memori Bebas (RAM):</span>
          <span class="data-val">
            <strong>{isDeviceOnline && telemetry.esp_free_heap_kb !== undefined ? telemetry.esp_free_heap_kb + ' KB' : '--'}</strong>
          </span>
        </div>
        <div class="data-row">
          <span class="data-label">Perangkat Pengontrol:</span>
          <span class="data-val">{device.name || 'ESP32 Master Gateway'}</span>
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

  .env-grid {
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
  }

  .card-body {
    padding: 12px;
    display: flex;
    flex-direction: column;
    gap: 6px;
    font-family: var(--font-mono);
  }

  .data-row {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 4px 6px;
    font-size: 11px;
    border-bottom: 1px solid var(--border-light);
  }

  .data-label {
    color: var(--text-secondary);
  }

  .data-val strong {
    color: var(--text-primary);
  }

  .card-offline {
    border-color: #dc2626;
  }

  .offline-warning-box {
    background-color: #fee2e2;
    border: 1px solid #fca5a5;
    border-radius: var(--radius-sm);
    padding: 8px 10px;
    margin-bottom: 6px;
    font-family: var(--font-mono);
  }

  .warning-title {
    color: #b91c1c;
    font-size: 11px;
    font-weight: 700;
    display: block;
    margin-bottom: 3px;
  }

  .warning-desc {
    color: #991b1b;
    font-size: 10px;
    line-height: 1.35;
    margin: 0;
  }

  .text-green {
    color: var(--solar-green-text);
  }

  .text-alert {
    color: #dc2626;
  }
</style>
