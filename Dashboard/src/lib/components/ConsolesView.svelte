<script>
  let {
    telemetryLogs = [],
    controlLogs = [],
    onClearTelemetry = () => {},
    onClearControl = () => {},
    wsStatus = 'DISCONNECTED',
  } = $props();

  // Control pause/freeze for real-time telemetry stream
  let isStreamPaused = $state(false);
  let pausedTelemetrySnapshot = $state([]);

  function togglePauseStream() {
    if (!isStreamPaused) {
      pausedTelemetrySnapshot = [...telemetryLogs];
      isStreamPaused = true;
    } else {
      isStreamPaused = false;
      pausedTelemetrySnapshot = [];
    }
  }

  let displayedTelemetry = $derived(
    isStreamPaused ? pausedTelemetrySnapshot : telemetryLogs
  );

  // Filter for control feedback log
  let activeFilter = $state('ALL'); // 'ALL' | 'LAMPU' | 'AKTUATOR' | 'SECURITY' | 'SYSTEM'

  let filteredControlLogs = $derived(
    activeFilter === 'ALL'
      ? controlLogs
      : controlLogs.filter((log) => {
          if (activeFilter === 'LAMPU') return log.category === 'LAMPU' || log.action?.toLowerCase().includes('lamp');
          if (activeFilter === 'AKTUATOR') return log.category === 'AKTUATOR' || log.action?.toLowerCase().includes('ac') || log.action?.toLowerCase().includes('garasi') || log.action?.toLowerCase().includes('pompa') || log.action?.toLowerCase().includes('breaker') || log.action?.toLowerCase().includes('valve');
          if (activeFilter === 'SECURITY') return log.category === 'SECURITY' || log.action?.toLowerCase().includes('lock') || log.action?.toLowerCase().includes('emergency');
          if (activeFilter === 'SYSTEM') return log.category === 'SYSTEM' || log.action?.toLowerCase().includes('ws') || log.action?.toLowerCase().includes('node');
          return true;
        })
  );
</script>

<div class="consoles-layout">
  <!-- CONSOLE 1: TELEMETRI STREAM REAL-TIME (DATA SENSOR INTERVAL 1 DETIK) -->
  <section class="panel console-panel">
    <div class="panel-header">
      <div class="header-title-group">
        <span class="amber-bar"></span>
        <div>
          <h2>KONSOL TELEMETRI REAL-TIME</h2>
          <span class="panel-sub">Stream sensor & daya listrik masuk setiap 1 detik</span>
        </div>
      </div>

      <div class="header-actions">
        <span class="status-pill {isStreamPaused ? 'pill-paused' : 'pill-live'}">
          <span class="status-dot {isStreamPaused ? 'dot-paused' : 'dot-live'}"></span>
          {isStreamPaused ? 'STREAM DIJEDA' : 'LIVE STREAM'}
        </span>

        <button
          type="button"
          class="btn-action {isStreamPaused ? 'btn-resume' : 'btn-pause'}"
          onclick={togglePauseStream}
          title={isStreamPaused ? 'Lanjutkan stream data real-time' : 'Jeda stream untuk membaca data'}
        >
          {isStreamPaused ? '▶ LANJUTKAN' : '⏸ JEDA'}
        </button>

        <button
          type="button"
          class="btn-action btn-clear"
          onclick={onClearTelemetry}
          title="Bersihkan riwayat telemetri"
        >
          BERSIHKAN
        </button>
      </div>
    </div>

    <div class="console-body">
      {#if displayedTelemetry.length === 0}
        <div class="empty-state">
          <span class="empty-icon">⏱</span>
          <p class="empty-title">Menunggu Data Telemetri...</p>
          <p class="empty-desc">Data dari mikrokontroler ESP32 akan tampil di sini secara real-time setiap detik.</p>
        </div>
      {:else}
        <div class="table-wrapper">
          <table class="telemetry-table">
            <thead>
              <tr>
                <th style="width: 85px;">WAKTU</th>
                <th style="width: 100px;">SOLAR (W)</th>
                <th style="width: 100px;">BEBAN (W)</th>
                <th style="width: 80px;">PLN (V)</th>
                <th style="width: 85px;">BATERAI</th>
                <th style="width: 80px;">GAS (PPM)</th>
                <th style="width: 80px;">AIR (%)</th>
                <th style="width: 75px;">WI-FI</th>
              </tr>
            </thead>
            <tbody>
              {#each displayedTelemetry as row}
                <tr>
                  <td class="col-time">{row.time}</td>
                  <td class="col-solar">
                    <strong>{Number(row.solar_w ?? 0).toFixed(0)}</strong> W
                  </td>
                  <td class="col-house">
                    <strong>{Number(row.house_w ?? 0).toFixed(0)}</strong> W
                  </td>
                  <td class="col-mains">
                    {Number(row.mains_v ?? 0).toFixed(1)} V
                  </td>
                  <td class="col-battery">
                    <span class="bat-badge {row.battery_pct < 25 ? 'bat-low' : 'bat-ok'}">
                      {Number(row.battery_pct ?? 0).toFixed(0)}%
                    </span>
                  </td>
                  <td class="col-gas">
                    <span class="{row.gas_ppm >= 400 ? 'text-alert' : 'text-slate'}">
                      {row.gas_ppm ?? '--'}
                    </span>
                  </td>
                  <td class="col-water">
                    {row.water_pct !== undefined ? row.water_pct + '%' : '--'}
                  </td>
                  <td class="col-wifi">
                    {row.rssi !== undefined ? row.rssi + ' dBm' : '--'}
                  </td>
                </tr>
              {/each}
            </tbody>
          </table>
        </div>
      {/if}
    </div>

    <div class="console-footer">
      <span class="footer-meta">
        Total Catatan: <strong>{displayedTelemetry.length}</strong> baris {isStreamPaused ? '(Freeze View)' : '(Maks 100)'}
      </span>
      <span class="footer-ws {wsStatus === 'CONNECTED' ? 'ws-ok' : 'ws-err'}">
        Koneksi Server: <strong>{wsStatus}</strong>
      </span>
    </div>
  </section>

  <!-- CONSOLE 2: FEEDBACK & KONTROL USER (TOGGLE LAMPU, SAKLAR, LOCK, ERROR) -->
  <section class="panel console-panel">
    <div class="panel-header">
      <div class="header-title-group">
        <span class="amber-bar"></span>
        <div>
          <h2>KONSOL FEEDBACK & KONTROL</h2>
          <span class="panel-sub">Respon balik tombol saklar, perintah kontrol, & keamanan</span>
        </div>
      </div>

      <div class="header-actions">
        <button
          type="button"
          class="btn-action btn-clear"
          onclick={onClearControl}
          title="Bersihkan riwayat feedback kontrol"
        >
          BERSIHKAN
        </button>
      </div>
    </div>

    <!-- Filter Buttons Bar -->
    <div class="filter-bar">
      <span class="filter-lbl">FILTER:</span>
      <button
        type="button"
        class="filter-btn {activeFilter === 'ALL' ? 'active' : ''}"
        onclick={() => (activeFilter = 'ALL')}
      >
        SEMUA ({controlLogs.length})
      </button>
      <button
        type="button"
        class="filter-btn {activeFilter === 'LAMPU' ? 'active' : ''}"
        onclick={() => (activeFilter = 'LAMPU')}
      >
        LAMPU
      </button>
      <button
        type="button"
        class="filter-btn {activeFilter === 'AKTUATOR' ? 'active' : ''}"
        onclick={() => (activeFilter = 'AKTUATOR')}
      >
        PERALATAN
      </button>
      <button
        type="button"
        class="filter-btn {activeFilter === 'SECURITY' ? 'active' : ''}"
        onclick={() => (activeFilter = 'SECURITY')}
      >
        KEAMANAN
      </button>
      <button
        type="button"
        class="filter-btn {activeFilter === 'SYSTEM' ? 'active' : ''}"
        onclick={() => (activeFilter = 'SYSTEM')}
      >
        SISTEM
      </button>
    </div>

    <div class="console-body">
      {#if filteredControlLogs.length === 0}
        <div class="empty-state">
          <span class="empty-icon">✓</span>
          <p class="empty-title">Belum Ada Feedback Kontrol</p>
          <p class="empty-desc">
            Klik tombol saklar lampu, saklar AC, kunci hardware, atau perintah lainnya pada Dashboard untuk melihat respon balik sistem di sini.
          </p>
        </div>
      {:else}
        <div class="table-wrapper">
          <table class="control-table">
            <thead>
              <tr>
                <th style="width: 85px;">WAKTU</th>
                <th style="width: 170px;">AKSI / TARGET</th>
                <th>DETAIL PARAMETER</th>
                <th style="width: 110px; text-align: right;">STATUS</th>
              </tr>
            </thead>
            <tbody>
              {#each filteredControlLogs as log}
                <tr class="control-row status-{log.status?.toLowerCase() || 'info'}">
                  <td class="col-time">{log.time}</td>
                  <td class="col-action">
                    <span class="action-tag">{log.category || 'KONTROL'}</span>
                    <strong class="action-name">{log.action}</strong>
                  </td>
                  <td class="col-details">
                    <span class="details-text" title={log.details}>{log.details}</span>
                  </td>
                  <td class="col-status">
                    <span
                      class="status-badge {log.status === 'SUCCESS' ? 'badge-ok' : log.status === 'WARNING' ? 'badge-warn' : 'badge-err'}"
                    >
                      {#if log.status === 'SUCCESS'}
                        ✓ BERHASIL
                      {:else if log.status === 'WARNING'}
                        ⚠ DITOLAK
                      {:else}
                        ✕ GAGAL
                      {/if}
                    </span>
                  </td>
                </tr>
              {/each}
            </tbody>
          </table>
        </div>
      {/if}
    </div>

    <div class="console-footer">
      <span class="footer-meta">
        Menampilkan <strong>{filteredControlLogs.length}</strong> dari <strong>{controlLogs.length}</strong> respon kontrol
      </span>
    </div>
  </section>
</div>

<style>
  .consoles-layout {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 16px;
    height: 100%;
    min-height: 0;
  }

  .console-panel {
    display: flex;
    flex-direction: column;
    min-height: 600px;
    background-color: var(--bg-panel);
    border: 1px solid var(--border-medium);
  }

  .panel-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 12px 14px;
    background-color: var(--bg-panel-header);
    border-bottom: 1px solid var(--border-medium);
    gap: 10px;
    flex-wrap: wrap;
  }

  .header-title-group {
    display: flex;
    align-items: center;
    gap: 10px;
  }

  .amber-bar {
    width: 3px;
    height: 28px;
    background-color: var(--accent-amber);
    flex-shrink: 0;
  }

  h2 {
    font-size: 13px;
    font-weight: 700;
    color: var(--text-primary);
    letter-spacing: 0.04em;
    margin: 0;
  }

  .panel-sub {
    font-size: 11px;
    color: var(--text-secondary);
    display: block;
    margin-top: 2px;
  }

  .header-actions {
    display: flex;
    align-items: center;
    gap: 8px;
  }

  .status-pill {
    display: inline-flex;
    align-items: center;
    gap: 5px;
    font-family: var(--font-mono);
    font-size: 10px;
    font-weight: 700;
    padding: 4px 8px;
    border: 1px solid transparent;
  }

  .pill-live {
    background-color: rgba(16, 185, 129, 0.12);
    color: var(--solar-green-text);
    border-color: rgba(16, 185, 129, 0.35);
  }

  .pill-paused {
    background-color: rgba(245, 158, 11, 0.12);
    color: var(--text-amber);
    border-color: rgba(245, 158, 11, 0.35);
  }

  .status-dot {
    width: 6px;
    height: 6px;
    flex-shrink: 0;
  }

  .dot-live {
    background-color: var(--solar-green);
  }

  .dot-paused {
    background-color: var(--accent-amber);
  }

  .btn-action {
    background-color: var(--bg-card);
    border: 1px solid var(--border-medium);
    color: var(--text-secondary);
    font-family: var(--font-mono);
    font-size: 11px;
    font-weight: 600;
    padding: 5px 10px;
    cursor: pointer;
  }

  .btn-action:hover {
    background-color: var(--bg-hover);
    color: var(--text-primary);
    border-color: var(--border-amber);
  }

  .btn-pause {
    color: var(--text-amber);
    border-color: rgba(245, 158, 11, 0.4);
  }

  .btn-resume {
    color: var(--solar-green-text);
    border-color: rgba(16, 185, 129, 0.4);
  }

  .btn-clear {
    color: var(--text-secondary);
  }

  /* Filter Bar */
  .filter-bar {
    display: flex;
    align-items: center;
    gap: 6px;
    padding: 8px 14px;
    background-color: var(--bg-card);
    border-bottom: 1px solid var(--border-light);
    flex-wrap: wrap;
  }

  .filter-lbl {
    font-family: var(--font-mono);
    font-size: 10px;
    color: var(--text-muted);
    font-weight: 700;
    margin-right: 2px;
  }

  .filter-btn {
    background: transparent;
    border: 1px solid var(--border-medium);
    color: var(--text-secondary);
    font-family: var(--font-mono);
    font-size: 10px;
    padding: 3px 8px;
    cursor: pointer;
  }

  .filter-btn:hover {
    background-color: var(--bg-hover);
    color: var(--text-primary);
  }

  .filter-btn.active {
    background-color: var(--accent-amber-bg);
    border-color: var(--accent-amber);
    color: var(--text-amber);
    font-weight: 700;
  }

  /* Body & Tables */
  .console-body {
    flex: 1;
    overflow-y: auto;
    position: relative;
    background-color: #0b0e13;
  }

  .table-wrapper {
    width: 100%;
  }

  table {
    width: 100%;
    border-collapse: collapse;
    font-family: var(--font-mono);
    font-size: 11px;
    text-align: left;
  }

  th {
    background-color: #121720;
    color: var(--text-secondary);
    font-weight: 700;
    font-size: 10px;
    letter-spacing: 0.05em;
    padding: 8px 10px;
    border-bottom: 1px solid var(--border-medium);
    position: sticky;
    top: 0;
    z-index: 2;
  }

  td {
    padding: 7px 10px;
    border-bottom: 1px solid #19212c;
    color: var(--text-primary);
    white-space: nowrap;
  }

  tbody tr:hover {
    background-color: #161d28;
  }

  /* Column Accents */
  .col-time {
    color: var(--text-muted);
    font-size: 10px;
  }

  .col-solar {
    color: var(--solar-green-text);
  }

  .col-house {
    color: var(--text-amber);
  }

  .col-mains {
    color: var(--pln-cyan-text);
  }

  .bat-badge {
    padding: 1px 5px;
    font-size: 10px;
    font-weight: 700;
  }

  .bat-ok {
    background-color: rgba(132, 204, 22, 0.15);
    color: var(--battery-lime-text);
  }

  .bat-low {
    background-color: rgba(239, 68, 68, 0.2);
    color: #fca5a5;
  }

  .col-action {
    display: flex;
    flex-direction: column;
    gap: 2px;
  }

  .action-tag {
    font-size: 9px;
    color: var(--text-muted);
    letter-spacing: 0.05em;
  }

  .action-name {
    color: var(--text-primary);
    font-size: 11px;
  }

  .col-details {
    color: var(--text-secondary);
    max-width: 260px;
    overflow: hidden;
    text-overflow: ellipsis;
  }

  .details-text {
    font-size: 11px;
  }

  .col-status {
    text-align: right;
  }

  .status-badge {
    display: inline-block;
    padding: 3px 7px;
    font-size: 10px;
    font-weight: 700;
    letter-spacing: 0.03em;
  }

  .badge-ok {
    background-color: var(--status-active-bg);
    color: var(--status-active-text);
    border: 1px solid var(--status-active-border);
  }

  .badge-warn {
    background-color: var(--status-warning-bg);
    color: var(--status-warning-text);
    border: 1px solid var(--status-warning-border);
  }

  .badge-err {
    background-color: var(--status-alert-bg);
    color: var(--status-alert-text);
    border: 1px solid var(--status-alert-border);
  }

  .text-alert {
    color: #f87171;
    font-weight: 700;
  }

  .text-slate {
    color: var(--text-secondary);
  }

  /* Empty State */
  .empty-state {
    padding: 48px 24px;
    text-align: center;
    color: var(--text-secondary);
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
  }

  .empty-icon {
    font-size: 28px;
    margin-bottom: 12px;
    color: var(--text-muted);
  }

  .empty-title {
    font-size: 13px;
    font-weight: 700;
    color: var(--text-primary);
    margin-bottom: 4px;
  }

  .empty-desc {
    font-size: 11px;
    color: var(--text-muted);
    max-width: 320px;
    line-height: 1.4;
  }

  /* Footer */
  .console-footer {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 8px 14px;
    background-color: var(--bg-panel-header);
    border-top: 1px solid var(--border-medium);
    font-family: var(--font-mono);
    font-size: 10px;
    color: var(--text-muted);
  }

  .footer-meta strong {
    color: var(--text-secondary);
  }

  .ws-ok strong {
    color: var(--solar-green-text);
  }

  .ws-err strong {
    color: #f87171;
  }

  @media (max-width: 900px) {
    .consoles-layout {
      grid-template-columns: 1fr;
    }
  }
</style>

