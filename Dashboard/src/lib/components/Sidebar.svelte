<script>
  let {
    currentView = 'monitor',
    onViewChange = () => {},
    devices = [],
    activeDeviceId = 'esp32_master',
    onDeviceSelect = () => {},
    telemetry = {},
    wsStatus = 'DISCONNECTED',
    device = {},
    onToggleLock = () => {},
    isDeviceOnline = true,
  } = $props();

  // Collapsible state (persisted in localStorage)
  let isCollapsed = $state(
    typeof localStorage !== 'undefined' ? localStorage.getItem('home_iot_sidebar_collapsed') === 'true' : false
  );

  function toggleSidebar() {
    isCollapsed = !isCollapsed;
    if (typeof localStorage !== 'undefined') {
      localStorage.setItem('home_iot_sidebar_collapsed', String(isCollapsed));
    }
  }
</script>

<aside class="sidebar {isCollapsed ? 'collapsed' : 'expanded'}">
  <!-- 1. SIDEBAR BRAND / HEADER -->
  <div class="sidebar-header">
    {#if !isCollapsed}
      <div class="brand">
        <span class="brand-badge">SCADA</span>
        <span class="brand-text">HOME-IOT</span>
      </div>
    {:else}
      <div class="brand-mini" title="Home IoT SCADA Gateway">
        <span class="brand-badge">S</span>
      </div>
    {/if}

    <button
      type="button"
      class="toggle-btn"
      onclick={toggleSidebar}
      title={isCollapsed ? 'Perluas Navigasi (Expand)' : 'Ciutkan Navigasi (Collapse)'}
      aria-label="Toggle Navigation"
    >
      {#if isCollapsed}
        <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
          <polyline points="9 18 15 12 9 6"></polyline>
        </svg>
      {:else}
        <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
          <polyline points="15 18 9 12 15 6"></polyline>
        </svg>
      {/if}
    </button>
  </div>

  <!-- 2. NAVIGATION LINKS -->
  <nav class="sidebar-nav">
    <!-- Item 1: MONITOR / DASHBOARD -->
    <button
      type="button"
      class="nav-item {currentView === 'monitor' ? 'active' : ''}"
      onclick={() => onViewChange('monitor')}
      title="Dashboard & Live Monitor"
    >
      <span class="nav-icon">
        <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8">
          <rect x="3" y="3" width="7" height="9"></rect>
          <rect x="14" y="3" width="7" height="5"></rect>
          <rect x="14" y="12" width="7" height="9"></rect>
          <rect x="3" y="16" width="7" height="5"></rect>
        </svg>
      </span>
      {#if !isCollapsed}
        <span class="nav-label">DASHBOARD</span>
      {/if}
    </button>

    <!-- Item 2: DEVICES / PERANGKAT -->
    <button
      type="button"
      class="nav-item {currentView === 'settings' || currentView === 'devices' ? 'active' : ''}"
      onclick={() => onViewChange('settings')}
      title="Manajemen Devices & Perangkat"
    >
      <span class="nav-icon">
        <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8">
          <rect x="2" y="4" width="20" height="8" rx="0"></rect>
          <rect x="2" y="14" width="20" height="6" rx="0"></rect>
          <line x1="6" y1="8" x2="6" y2="8.01"></line>
          <line x1="10" y1="8" x2="10" y2="8.01"></line>
        </svg>
      </span>
      {#if !isCollapsed}
        <span class="nav-label">DEVICES</span>
        {#if devices && devices.length > 0}
          <span class="nav-pill">{devices.length}</span>
        {/if}
      {/if}
    </button>

    <!-- Item 3: CONSOLES / KONSOL LOG -->
    <button
      type="button"
      class="nav-item {currentView === 'consoles' ? 'active' : ''}"
      onclick={() => onViewChange('consoles')}
      title="Konsol Telemetri & Feedback Kontrol"
    >
      <span class="nav-icon">
        <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8">
          <polyline points="4 17 10 11 4 5"></polyline>
          <line x1="12" y1="19" x2="20" y2="19"></line>
        </svg>
      </span>
      {#if !isCollapsed}
        <span class="nav-label">CONSOLES</span>
      {/if}
    </button>
  </nav>

  <!-- 3. ACTIVE NODE SELECTOR & STATUS -->
  <div class="sidebar-section">
    {#if !isCollapsed}
      <div class="section-label">NODE AKTIF</div>
      <div class="node-control-card">
        <div class="select-row">
          <select
            class="node-select"
            value={activeDeviceId}
            onchange={(e) => onDeviceSelect(e.target.value)}
          >
            {#if devices && devices.length > 0}
              {#each devices as dev}
                <option value={dev.id}>
                  {dev.name || dev.id}
                </option>
              {/each}
            {:else}
              <option value="esp32_master">ESP32 Master Node</option>
            {/if}
          </select>
        </div>

        <div class="node-meta">
          <span class="badge {isDeviceOnline ? 'active' : 'alert'} node-status-pill">
            {isDeviceOnline ? 'ONLINE' : 'OFFLINE'}
          </span>
          <span class="node-ip">{device.ip_address || '192.168.100.76'}</span>
          <button
            type="button"
            class="lock-btn {device.is_locked ? 'is-locked' : 'is-unlocked'}"
            onclick={() => onToggleLock(activeDeviceId)}
            title={device.is_locked ? 'Device Terkunci (Klik untuk Unlock)' : 'Device Terbuka (Klik untuk Lock)'}
          >
            {#if device.is_locked}
              <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <rect x="3" y="11" width="18" height="11"></rect>
                <path d="M7 11V7a5 5 0 0 1 10 0v4"></path>
              </svg>
              <span>LOCKED</span>
            {:else}
              <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <rect x="3" y="11" width="18" height="11"></rect>
                <path d="M7 11V7a5 5 0 0 1 9.9-1"></path>
              </svg>
              <span>UNLOCKED</span>
            {/if}
          </button>
        </div>
      </div>
    {:else}
      <div class="collapsed-node-indicator" title="Node: {device.name || activeDeviceId} ({isDeviceOnline ? 'ONLINE' : 'OFFLINE'})">
        <span class="status-dot {isDeviceOnline ? 'online' : 'offline'}"></span>
      </div>
    {/if}
  </div>

  <!-- 4. HARDWARE DIAGNOSTICS & SYSTEM FOOTER -->
  <div class="sidebar-footer">
    {#if !isCollapsed}
      <div class="section-label">DIAGNOSTIK GATEWAY</div>
      <div class="diag-card">
        <div class="diag-row">
          <span class="diag-lbl">DEVICE STATE</span>
          <span class="badge {isDeviceOnline ? 'active' : 'alert'}">
            {isDeviceOnline ? 'ONLINE' : 'MATI / OFFLINE'}
          </span>
        </div>
        <div class="diag-row">
          <span class="diag-lbl">WI-FI RSSI</span>
          <span class="diag-val">{isDeviceOnline && telemetry.rssi_dbm !== undefined ? telemetry.rssi_dbm + ' dBm' : '--'}</span>
        </div>
        <div class="diag-row">
          <span class="diag-lbl">LATENCY PING</span>
          <span class="diag-val">{isDeviceOnline && telemetry.ping_gateway_ms !== undefined ? Number(telemetry.ping_gateway_ms).toFixed(1) + ' ms' : '--'}</span>
        </div>
        <div class="diag-row">
          <span class="diag-lbl">FREE HEAP</span>
          <span class="diag-val">{isDeviceOnline && telemetry.esp_free_heap_kb !== undefined ? telemetry.esp_free_heap_kb + ' KB' : '--'}</span>
        </div>
        <div class="diag-row ws-row">
          <span class="diag-lbl">WS STREAM</span>
          <span class="badge {wsStatus === 'CONNECTED' ? 'active' : wsStatus === 'CONNECTING' ? 'warning' : 'alert'}">
            {wsStatus}
          </span>
        </div>
      </div>
    {:else}
      <div class="collapsed-footer" title="WS: {wsStatus} | RSSI: {telemetry.rssi_dbm ?? '--'} dBm">
        <span class="badge {wsStatus === 'CONNECTED' ? 'active' : 'alert'} ws-mini">WS</span>
      </div>
    {/if}
  </div>
</aside>

<style>
  .sidebar {
    height: 100vh;
    background-color: var(--bg-sidebar);
    border-right: 1px solid var(--border-medium);
    display: flex;
    flex-direction: column;
    flex-shrink: 0;
    user-select: none;
    z-index: 50;
  }

  .sidebar.expanded {
    width: var(--sidebar-w-expanded);
  }

  .sidebar.collapsed {
    width: var(--sidebar-w-collapsed);
  }

  /* 1. Header */
  .sidebar-header {
    height: 52px;
    padding: 0 12px;
    display: flex;
    align-items: center;
    justify-content: space-between;
    border-bottom: 1px solid var(--border-medium);
    background-color: var(--bg-panel-header);
  }

  .brand {
    display: flex;
    align-items: center;
    gap: 8px;
  }

  .brand-badge {
    background-color: var(--accent-amber);
    color: #000000;
    font-family: var(--font-mono);
    font-size: 10px;
    font-weight: 800;
    padding: 2px 5px;
    letter-spacing: 0.08em;
  }

  .brand-text {
    font-family: var(--font-mono);
    font-size: 12px;
    font-weight: 700;
    letter-spacing: 0.08em;
    color: var(--text-primary);
  }

  .brand-mini {
    display: flex;
    align-items: center;
    justify-content: center;
    width: 100%;
  }

  .toggle-btn {
    background: transparent;
    border: 1px solid var(--border-medium);
    color: var(--text-secondary);
    padding: 4px;
    width: 26px;
    height: 26px;
    display: flex;
    align-items: center;
    justify-content: center;
  }

  .toggle-btn:hover {
    border-color: var(--border-amber);
    color: var(--text-amber);
    background-color: var(--bg-hover);
  }

  .collapsed .sidebar-header {
    padding: 0 8px;
    flex-direction: column;
    justify-content: center;
    gap: 6px;
    height: 64px;
  }

  /* 2. Nav items */
  .sidebar-nav {
    display: flex;
    flex-direction: column;
    padding: 8px 0;
    border-bottom: 1px solid var(--border-light);
  }

  .nav-item {
    display: flex;
    align-items: center;
    gap: 12px;
    padding: 10px 14px;
    background: transparent;
    border: none;
    border-left: 3px solid transparent;
    color: var(--text-secondary);
    font-family: var(--font-mono);
    font-size: 11px;
    font-weight: 600;
    letter-spacing: 0.06em;
    text-align: left;
    width: 100%;
    cursor: pointer;
    transition: background-color 150ms ease, color 150ms ease, border-left-color 150ms ease;
  }

  .nav-item:hover {
    background-color: var(--bg-hover);
    color: var(--text-primary);
  }

  .nav-item.active {
    background-color: var(--bg-active);
    color: var(--text-amber);
    border-left-color: var(--accent-amber);
    font-weight: 700;
  }

  .nav-icon {
    display: flex;
    align-items: center;
    justify-content: center;
    width: 20px;
    height: 20px;
  }

  .nav-label {
    flex: 1;
  }

  .nav-pill {
    background-color: var(--border-medium);
    color: var(--text-primary);
    font-size: 9px;
    padding: 1px 5px;
    font-weight: 700;
  }

  .collapsed .nav-item {
    justify-content: center;
    padding: 12px 0;
  }

  /* 3. Section Labels & Nodes */
  .sidebar-section {
    padding: 12px;
    border-bottom: 1px solid var(--border-light);
  }

  .section-label {
    font-family: var(--font-mono);
    font-size: 9px;
    font-weight: 700;
    letter-spacing: 0.08em;
    color: var(--text-muted);
    margin-bottom: 6px;
    text-transform: uppercase;
  }

  .node-control-card {
    background-color: var(--bg-card);
    border: 1px solid var(--border-medium);
    padding: 8px;
    display: flex;
    flex-direction: column;
    gap: 8px;
  }

  .node-select {
    width: 100%;
    padding: 5px 8px;
    font-size: 11px;
  }

  .node-meta {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 6px;
  }

  .node-ip {
    font-family: var(--font-mono);
    font-size: 10px;
    color: var(--text-muted);
  }

  .lock-btn {
    display: inline-flex;
    align-items: center;
    gap: 4px;
    font-size: 9px;
    padding: 3px 6px;
  }

  .lock-btn.is-locked {
    background-color: var(--status-warning-bg);
    color: var(--status-warning-text);
    border-color: var(--status-warning-border);
  }

  .lock-btn.is-unlocked {
    background-color: var(--bg-hover);
    color: var(--text-secondary);
    border-color: var(--border-medium);
  }

  .collapsed-node-indicator {
    display: flex;
    justify-content: center;
    padding: 8px 0;
  }

  .status-dot {
    width: 10px;
    height: 10px;
    border: 1px solid #000;
  }

  .status-dot.online {
    background-color: var(--solar-green);
  }

  .status-dot.offline {
    background-color: var(--status-alert-border);
  }

  /* 4. Footer & Diagnostics */
  .sidebar-footer {
    margin-top: auto;
    padding: 12px;
    background-color: var(--bg-panel-header);
    border-top: 1px solid var(--border-medium);
  }

  .diag-card {
    display: flex;
    flex-direction: column;
    gap: 5px;
    font-family: var(--font-mono);
    font-size: 10px;
  }

  .diag-row {
    display: flex;
    justify-content: space-between;
    align-items: center;
  }

  .diag-lbl {
    color: var(--text-muted);
    font-size: 9px;
    letter-spacing: 0.05em;
  }

  .diag-val {
    color: var(--text-secondary);
    font-weight: 600;
  }

  .ws-row {
    margin-top: 4px;
    padding-top: 4px;
    border-top: 1px solid var(--border-light);
  }

  .collapsed-footer {
    display: flex;
    justify-content: center;
    padding: 6px 0;
  }

  .ws-mini {
    font-size: 9px;
    padding: 2px 4px;
  }
</style>
