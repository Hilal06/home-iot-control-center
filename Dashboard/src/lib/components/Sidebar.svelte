<script>
  import ChevronLeft from '@lucide/svelte/icons/chevron-left';
  import ChevronRight from '@lucide/svelte/icons/chevron-right';
  import Home from '@lucide/svelte/icons/home';
  import Activity from '@lucide/svelte/icons/activity';
  import Cpu from '@lucide/svelte/icons/cpu';
  import Terminal from '@lucide/svelte/icons/terminal';
  import Lock from '@lucide/svelte/icons/lock';
  import Unlock from '@lucide/svelte/icons/unlock';
  import Radio from '@lucide/svelte/icons/radio';
  import Wifi from '@lucide/svelte/icons/wifi';

  let {
    currentView = 'home',
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

  function getWifiBars(rssi) {
    if (rssi === undefined || rssi === null) return 0;
    if (rssi >= -60) return 4;
    if (rssi >= -70) return 3;
    if (rssi >= -80) return 2;
    return 1;
  }

  function getWifiQualityText(rssi) {
    if (rssi === undefined || rssi === null) return '--';
    if (rssi >= -60) return 'EXCELLENT';
    if (rssi >= -70) return 'GOOD';
    if (rssi >= -80) return 'FAIR';
    return 'WEAK';
  }

  let wifiBars = $derived(getWifiBars(telemetry?.rssi_dbm));
</script>

<aside class="sidebar {isCollapsed ? 'collapsed' : 'expanded'}">
  <!-- 1. SIDEBAR BRAND / HEADER (ICON INDUSTRIAL REPLACES TEXT SCADA) -->
  <div class="sidebar-header">
    {#if !isCollapsed}
      <div class="brand">
        <div class="brand-icon-box" title="Home IoT Edge Gateway">
          <Radio size={18} strokeWidth={2.2} />
        </div>
        <div class="brand-text-wrap">
          <span class="brand-title">HOME·IOT</span>
          <span class="brand-subtitle">EDGE GATEWAY</span>
        </div>
      </div>
    {:else}
      <div class="brand-mini" title="Home IoT Edge Gateway">
        <div class="brand-icon-box">
          <Radio size={18} strokeWidth={2.2} />
        </div>
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
        <ChevronRight size={14} />
      {:else}
        <ChevronLeft size={14} />
      {/if}
    </button>
  </div>

  <!-- 2. NAVIGATION LINKS (PRELINE MODULAR CARDS) -->
  <nav class="sidebar-nav">
    <!-- Item 1: HOME (SMART HOME OVERVIEW) -->
    <button
      type="button"
      class="nav-item {currentView === 'home' ? 'active' : ''}"
      onclick={() => onViewChange('home')}
      title="Smart Home Overview & Kontrol Peralatan"
    >
      <span class="nav-icon-box">
        <Home size={16} strokeWidth={2} />
      </span>
      {#if !isCollapsed}
        <span class="nav-label">HOME</span>
      {/if}
    </button>

    <!-- Item 2: STATS (TELEMETRI & ANALISIS) -->
    <button
      type="button"
      class="nav-item {currentView === 'stats' ? 'active' : ''}"
      onclick={() => onViewChange('stats')}
      title="Grafik Telemetri, Konsumsi Daya & Sensor"
    >
      <span class="nav-icon-box">
        <Activity size={16} strokeWidth={2} />
      </span>
      {#if !isCollapsed}
        <span class="nav-label">STATS</span>
      {/if}
    </button>

    <!-- Item 3: DEVICES (MANAJEMEN PERANGKAT) -->
    <button
      type="button"
      class="nav-item {currentView === 'settings' || currentView === 'devices' ? 'active' : ''}"
      onclick={() => onViewChange('devices')}
      title="Manajemen Devices & Perangkat"
    >
      <span class="nav-icon-box">
        <Cpu size={16} strokeWidth={2} />
      </span>
      {#if !isCollapsed}
        <span class="nav-label">DEVICES</span>
        {#if devices && devices.length > 0}
          <span class="nav-pill">{devices.length}</span>
        {/if}
      {/if}
    </button>

    <!-- Item 4: CONSOLES (KONSOL LOG & MQTT) -->
    <button
      type="button"
      class="nav-item {currentView === 'consoles' ? 'active' : ''}"
      onclick={() => onViewChange('consoles')}
      title="Konsol Telemetri & Feedback Kontrol"
    >
      <span class="nav-icon-box">
        <Terminal size={16} strokeWidth={2} />
      </span>
      {#if !isCollapsed}
        <span class="nav-label">CONSOLES</span>
      {/if}
    </button>
  </nav>

  <!-- 3. ACTIVE NODE SELECTOR CARD -->
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
          <div class="node-status-wrap">
            <span class="status-indicator-dot {isDeviceOnline ? 'dot-online' : 'dot-offline'}"></span>
            <span class="node-ip mono">{device?.ip_address || '192.168.100.76'}</span>
          </div>

          <button
            type="button"
            class="lock-btn {device?.is_locked ? 'is-locked' : 'is-unlocked'}"
            onclick={() => onToggleLock(activeDeviceId)}
            title={device?.is_locked ? 'Perangkat Terkunci (Klik untuk Buka)' : 'Perangkat Bebas (Klik untuk Kunci)'}
          >
            {#if device?.is_locked}
              <Lock size={11} strokeWidth={2.2} />
              <span>TERKUNCI</span>
            {:else}
              <Unlock size={11} strokeWidth={2.2} />
              <span>BEBAS</span>
            {/if}
          </button>
        </div>
      </div>
    {:else}
      <div class="collapsed-node-indicator" title="Node: {device?.name || activeDeviceId} ({isDeviceOnline ? 'ONLINE' : 'OFFLINE'})">
        <span class="status-dot {isDeviceOnline ? 'online' : 'offline'}"></span>
      </div>
    {/if}
  </div>

  <!-- 4. HARDWARE & CONNECTION TELEMETRY CARD (BOTTOM) -->
  <div class="sidebar-footer">
    {#if !isCollapsed}
      <div class="section-label">JARINGAN & KONEKSI</div>
      <div class="connection-card">
        <!-- Row 1: WebSocket Stream Status -->
        <div class="conn-row">
          <span class="conn-lbl">WS STREAM</span>
          <div class="conn-status-tag">
            <span class="pulse-dot {wsStatus === 'CONNECTED' ? 'pulse-green' : wsStatus === 'CONNECTING' ? 'pulse-amber' : 'pulse-red'}"></span>
            <span class="conn-val mono status-{wsStatus.toLowerCase()}">{wsStatus}</span>
          </div>
        </div>

        <!-- Row 2: Wi-Fi RSSI with Signal Meter -->
        <div class="conn-row">
          <span class="conn-lbl">WI-FI RSSI</span>
          <div class="wifi-info-wrap">
            <div class="wifi-meter" title="Wi-Fi Signal: {telemetry?.rssi_dbm ?? '--'} dBm ({getWifiQualityText(telemetry?.rssi_dbm)})">
              <span class="bar bar-1 {wifiBars >= 1 ? (wifiBars <= 2 ? 'bar-amber' : 'bar-green') : ''}"></span>
              <span class="bar bar-2 {wifiBars >= 2 ? (wifiBars <= 2 ? 'bar-amber' : 'bar-green') : ''}"></span>
              <span class="bar bar-3 {wifiBars >= 3 ? 'bar-green' : ''}"></span>
              <span class="bar bar-4 {wifiBars >= 4 ? 'bar-green' : ''}"></span>
            </div>
            <span class="conn-val mono">
              {isDeviceOnline && telemetry?.rssi_dbm !== undefined ? `${telemetry.rssi_dbm} dBm` : '--'}
            </span>
          </div>
        </div>

        <!-- Row 3: Gateway Ping Latency -->
        <div class="conn-row">
          <span class="conn-lbl">PING GATEWAY</span>
          <span class="conn-val mono">
            {isDeviceOnline && telemetry?.ping_gateway_ms !== undefined ? `${telemetry.ping_gateway_ms} ms` : '3.4 ms'}
          </span>
        </div>

        <!-- Row 4: Node Host IP -->
        <div class="conn-row">
          <span class="conn-lbl">NODE HOST</span>
          <span class="conn-val mono">{device?.ip_address || '192.168.100.76'}</span>
        </div>
      </div>
    {:else}
      <!-- Collapsed Footer Mode -->
      <div class="collapsed-footer" title="WS: {wsStatus} | Wi-Fi: {telemetry?.rssi_dbm ?? '--'} dBm | Ping: {telemetry?.ping_gateway_ms ?? '--'} ms">
        <div class="collapsed-conn-pill {wsStatus === 'CONNECTED' ? 'conn-active' : 'conn-alert'}">
          <Wifi size={14} />
        </div>
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
    transition: width 200ms ease;
  }

  .sidebar.expanded {
    width: var(--sidebar-w-expanded);
  }

  .sidebar.collapsed {
    width: var(--sidebar-w-collapsed);
  }

  /* 1. Header (Solid Brand Icon & Industrial Title) */
  .sidebar-header {
    height: 56px;
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
    gap: 10px;
  }

  .brand-icon-box {
    width: 34px;
    height: 34px;
    display: inline-flex;
    align-items: center;
    justify-content: center;
    background-color: #fffbeb;
    border: 1px solid var(--accent-amber);
    border-radius: var(--radius-sm);
    color: var(--accent-amber);
    flex-shrink: 0;
  }

  .brand-text-wrap {
    display: flex;
    flex-direction: column;
  }

  .brand-title {
    font-family: var(--font-mono);
    font-size: 13px;
    font-weight: 800;
    letter-spacing: 0.08em;
    color: var(--text-primary);
    line-height: 1.1;
  }

  .brand-subtitle {
    font-family: var(--font-mono);
    font-size: 9px;
    font-weight: 700;
    color: var(--text-muted);
    letter-spacing: 0.06em;
    margin-top: 1px;
  }

  .brand-mini {
    display: flex;
    align-items: center;
    justify-content: center;
  }

  .toggle-btn {
    background: transparent;
    border: 1px solid var(--border-medium);
    border-radius: var(--radius-sm);
    color: var(--text-secondary);
    padding: 4px;
    width: 26px;
    height: 26px;
    display: flex;
    align-items: center;
    justify-content: center;
    cursor: pointer;
    transition: all 150ms ease;
  }

  .toggle-btn:hover {
    border-color: var(--accent-amber);
    color: var(--accent-amber);
    background-color: var(--bg-hover);
  }

  .collapsed .sidebar-header {
    padding: 8px 0;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    gap: 8px;
    height: 84px;
  }

  /* 2. Navigation items */
  .sidebar-nav {
    display: flex;
    flex-direction: column;
    gap: 4px;
    padding: 10px 8px;
    border-bottom: 1px solid var(--border-light);
  }

  .nav-item {
    display: flex;
    align-items: center;
    gap: 10px;
    padding: 6px 8px;
    background: transparent;
    border: 1px solid transparent;
    border-radius: var(--radius-sm);
    color: var(--text-secondary);
    font-family: var(--font-mono);
    font-size: 11px;
    font-weight: 600;
    letter-spacing: 0.05em;
    text-align: left;
    width: 100%;
    cursor: pointer;
    transition: all 150ms ease;
  }

  .nav-icon-box {
    width: 30px;
    height: 30px;
    display: inline-flex;
    align-items: center;
    justify-content: center;
    border: 1px solid var(--border-medium);
    border-radius: var(--radius-sm);
    background-color: var(--bg-panel-header);
    color: var(--text-secondary);
    flex-shrink: 0;
    transition: all 150ms ease;
  }

  .nav-item:hover {
    background-color: var(--bg-hover);
    color: var(--text-primary);
  }

  .nav-item:hover .nav-icon-box {
    border-color: var(--border-heavy);
    color: var(--text-primary);
  }

  .nav-item.active {
    background-color: #fffbeb;
    border-color: #fde68a;
    color: var(--accent-amber);
    font-weight: 700;
  }

  .nav-item.active .nav-icon-box {
    background-color: #fef3c7;
    border-color: var(--accent-amber);
    color: var(--accent-amber);
  }

  .nav-label {
    flex: 1;
  }

  .nav-pill {
    background-color: var(--bg-hover);
    border: 1px solid var(--border-medium);
    color: var(--text-primary);
    font-size: 10px;
    padding: 1px 6px;
    font-weight: 700;
    border-radius: var(--radius-sm);
  }

  .collapsed .nav-item {
    justify-content: center;
    padding: 6px 0;
  }

  /* 3. Section Labels & Node Card */
  .sidebar-section {
    padding: 10px 10px;
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
    border-radius: var(--radius-sm);
    padding: 8px;
    display: flex;
    flex-direction: column;
    gap: 8px;
  }

  .node-select {
    width: 100%;
    padding: 5px 8px;
    font-size: 11px;
    font-family: var(--font-mono);
    border: 1px solid var(--border-medium);
    border-radius: var(--radius-sm);
    background-color: var(--bg-panel-header);
    color: var(--text-primary);
  }

  .node-meta {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 6px;
  }

  .node-status-wrap {
    display: flex;
    align-items: center;
    gap: 6px;
  }

  .status-indicator-dot {
    width: 8px;
    height: 8px;
    border-radius: 0;
  }

  .dot-online {
    background-color: #16a34a;
  }

  .dot-offline {
    background-color: #dc2626;
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
    font-weight: 700;
    padding: 3px 6px;
    font-family: var(--font-mono);
    border-radius: var(--radius-sm);
    cursor: pointer;
  }

  .lock-btn.is-locked {
    background-color: #fef3c7;
    color: #b45309;
    border: 1px solid #f59e0b;
  }

  .lock-btn.is-unlocked {
    background-color: var(--bg-hover);
    color: var(--text-secondary);
    border: 1px solid var(--border-medium);
  }

  .lock-btn:hover {
    filter: brightness(0.95);
  }

  .collapsed-node-indicator {
    display: flex;
    justify-content: center;
    padding: 8px 0;
  }

  .status-dot {
    width: 10px;
    height: 10px;
    border: 1px solid var(--border-heavy);
  }

  .status-dot.online {
    background-color: #16a34a;
  }

  .status-dot.offline {
    background-color: #dc2626;
  }

  /* 4. Connection & Diagnostics Footer */
  .sidebar-footer {
    margin-top: auto;
    padding: 10px;
    background-color: var(--bg-panel-header);
    border-top: 1px solid var(--border-medium);
  }

  .connection-card {
    background-color: var(--bg-card);
    border: 1px solid var(--border-medium);
    border-radius: var(--radius-sm);
    padding: 8px 10px;
    display: flex;
    flex-direction: column;
    gap: 6px;
    font-family: var(--font-mono);
    font-size: 10px;
  }

  .conn-row {
    display: flex;
    justify-content: space-between;
    align-items: center;
  }

  .conn-lbl {
    color: var(--text-muted);
    font-size: 9px;
    font-weight: 700;
    letter-spacing: 0.05em;
  }

  .conn-val {
    color: var(--text-secondary);
    font-weight: 600;
  }

  .conn-status-tag {
    display: flex;
    align-items: center;
    gap: 6px;
  }

  .pulse-dot {
    width: 6px;
    height: 6px;
    border-radius: 0;
    display: inline-block;
  }

  .pulse-green {
    background-color: #16a34a;
  }

  .pulse-amber {
    background-color: #d97706;
  }

  .pulse-red {
    background-color: #dc2626;
  }

  .status-connected {
    color: #15803d;
    font-weight: 700;
  }

  .status-connecting {
    color: #b45309;
    font-weight: 700;
  }

  .status-disconnected {
    color: #b91c1c;
    font-weight: 700;
  }

  /* Wi-Fi Signal Meter */
  .wifi-info-wrap {
    display: flex;
    align-items: center;
    gap: 6px;
  }

  .wifi-meter {
    display: inline-flex;
    align-items: flex-end;
    gap: 2px;
    height: 11px;
    width: 14px;
  }

  .wifi-meter .bar {
    width: 2px;
    background-color: var(--border-medium);
    border-radius: 0;
  }

  .wifi-meter .bar-1 { height: 3px; }
  .wifi-meter .bar-2 { height: 5px; }
  .wifi-meter .bar-3 { height: 8px; }
  .wifi-meter .bar-4 { height: 11px; }

  .wifi-meter .bar.bar-green {
    background-color: #16a34a;
  }

  .wifi-meter .bar.bar-amber {
    background-color: #d97706;
  }

  /* Collapsed Footer */
  .collapsed-footer {
    display: flex;
    justify-content: center;
    padding: 6px 0;
  }

  .collapsed-conn-pill {
    width: 32px;
    height: 32px;
    display: flex;
    align-items: center;
    justify-content: center;
    border: 1px solid var(--border-medium);
    border-radius: var(--radius-sm);
  }

  .collapsed-conn-pill.conn-active {
    background-color: #f0fdf4;
    color: #16a34a;
    border-color: #bbf7d0;
  }

  .collapsed-conn-pill.conn-alert {
    background-color: #fef2f2;
    color: #dc2626;
    border-color: #fecaca;
  }

  .mono {
    font-family: var(--font-mono);
  }
</style>
