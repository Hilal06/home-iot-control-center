<script>
  let {
    telemetry = {},
    device = {},
    isDeviceOnline = true,
  } = $props();

  let timeString = $state(new Date().toLocaleTimeString());

  $effect(() => {
    const timer = setInterval(() => {
      timeString = new Date().toLocaleTimeString();
    }, 1000);
    return () => clearInterval(timer);
  });

  // Calculations for 900W PLN limit and 2400Wp Solar
  let houseWatts = $derived(Number(telemetry.house_power_w ?? 0));
  let plnLimitWatts = 900;
  let plnPct = $derived(Math.min(100, Math.max(0, (houseWatts / plnLimitWatts) * 100)));
  let plnRemainingWatts = $derived(Math.max(0, plnLimitWatts - houseWatts));

  let solarWatts = $derived(Number(telemetry.pv_power_watts ?? 0));
  let solarMaxWatts = 2400;
  let solarPct = $derived(Math.min(100, Math.max(0, (solarWatts / solarMaxWatts) * 100)));

  let batteryPct = $derived(Number(telemetry.ups_battery_pct ?? 98));
  let isSolarSurplus = $derived(solarWatts > houseWatts);
</script>

<header class="top-glance-bar">
  <!-- 1. PLN 900W Meter Bar -->
  <div class="glance-item pln-item">
    <div class="item-header">
      <span class="item-title">LISTRIK PLN (MAKS 900W)</span>
      <span class="item-val">
        <strong>{houseWatts.toFixed(0)}</strong> / {plnLimitWatts} W
        <span class="sub-val">({plnPct.toFixed(0)}%)</span>
      </span>
    </div>
    <div class="meter-track">
      <div
        class="meter-fill {plnPct > 88 ? 'fill-alert' : plnPct > 68 ? 'fill-warn' : 'fill-cyan'}"
        style="width: {plnPct}%;"
      ></div>
    </div>
    <div class="item-sub">
      Sisa Kuota Daya: <strong>{plnRemainingWatts.toFixed(0)} W</strong>
    </div>
  </div>

  <!-- 2. Solar Panel 2400Wp (12.6m2) Meter Bar -->
  <div class="glance-item solar-item">
    <div class="item-header">
      <span class="item-title">PANEL SURYA (2400Wp)</span>
      <span class="item-val text-green">
        <strong>{solarWatts.toFixed(0)}</strong> W
        <span class="sub-val">({solarPct.toFixed(0)}%)</span>
      </span>
    </div>
    <div class="meter-track">
      <div class="meter-fill fill-green" style="width: {solarPct}%;"></div>
    </div>
    <div class="item-sub">
      Panen Hari Ini: <strong>{telemetry.solar_yield_kwh?.toFixed(2) ?? '0.00'} kWh</strong>
    </div>
  </div>

  <!-- 3. Battery Storage Status -->
  <div class="glance-item battery-item">
    <div class="item-header">
      <span class="item-title">BATERAI RUMAH (4.8 kWh)</span>
      <span class="item-val text-amber">
        <strong>{batteryPct.toFixed(0)}%</strong>
      </span>
    </div>
    <div class="meter-track">
      <div
        class="meter-fill {batteryPct < 25 ? 'fill-alert' : batteryPct < 50 ? 'fill-warn' : 'fill-lime'}"
        style="width: {batteryPct}%;"
      ></div>
    </div>
    <div class="item-sub">
      Status: <span class="badge {isSolarSurplus ? 'active' : 'amber'}">{isSolarSurplus ? 'MENGISI BATERAI' : 'MEMAKAI BATERAI'}</span>
    </div>
  </div>

  <!-- 4. Clock & Node Mini Status -->
  <div class="glance-meta">
    <div class="time-display">{timeString}</div>
    <div class="node-badge">
      {#if !isDeviceOnline}
        <span class="badge alert">
          ● PERANGKAT OFFLINE
        </span>
      {:else}
        <span class="badge {device.is_locked ? 'warning' : 'active'}">
          {device.is_locked ? 'PERANGKAT TERKUNCI' : 'PERANGKAT ONLINE'}
        </span>
      {/if}
    </div>
  </div>
</header>

<style>
  .top-glance-bar {
    display: grid;
    grid-template-columns: 1fr 1fr 1fr auto;
    gap: 12px;
    background-color: var(--bg-panel);
    border: 1px solid var(--border-medium);
    padding: 10px 14px;
    align-items: center;
    font-family: var(--font-mono);
  }

  .glance-item {
    display: flex;
    flex-direction: column;
    gap: 4px;
    background-color: var(--bg-card);
    border: 1px solid var(--border-light);
    padding: 8px 10px;
  }

  .item-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    font-size: 11px;
    letter-spacing: 0.04em;
  }

  .item-title {
    color: var(--text-secondary);
    font-weight: 700;
  }

  .item-val {
    color: var(--text-primary);
    font-size: 12px;
  }

  .item-val strong {
    font-size: 14px;
    font-weight: 800;
  }

  .item-val.text-green {
    color: var(--solar-green-text);
  }

  .item-val.text-amber {
    color: var(--text-amber);
  }

  .sub-val {
    color: var(--text-muted);
    font-size: 11px;
  }

  .meter-track {
    width: 100%;
    height: 5px;
    background-color: #080a0e;
    border: 1px solid var(--border-light);
    overflow: hidden;
  }

  .meter-fill {
    height: 100%;
  }

  .fill-cyan {
    background-color: var(--pln-cyan);
  }

  .fill-warn {
    background-color: var(--accent-amber);
  }

  .fill-alert {
    background-color: var(--status-alert-border);
  }

  .fill-green {
    background-color: var(--solar-green);
  }

  .fill-lime {
    background-color: var(--battery-lime);
  }

  .item-sub {
    font-size: 11px;
    color: var(--text-secondary);
    display: flex;
    align-items: center;
    gap: 6px;
  }

  .item-sub strong {
    color: var(--text-primary);
  }

  .glance-meta {
    display: flex;
    flex-direction: column;
    align-items: flex-end;
    justify-content: center;
    gap: 4px;
    padding-left: 8px;
    border-left: 1px solid var(--border-medium);
  }

  .time-display {
    font-size: 14px;
    font-weight: 700;
    color: var(--text-amber);
    letter-spacing: 0.08em;
  }

  .node-badge {
    display: flex;
    align-items: center;
  }

  @media (max-width: 1024px) {
    .top-glance-bar {
      grid-template-columns: 1fr 1fr;
    }
    .glance-meta {
      grid-column: 1 / -1;
      flex-direction: row;
      justify-content: space-between;
      border-left: none;
      border-top: 1px solid var(--border-medium);
      padding-top: 8px;
    }
  }

  @media (max-width: 640px) {
    .top-glance-bar {
      grid-template-columns: 1fr;
    }
  }
</style>
