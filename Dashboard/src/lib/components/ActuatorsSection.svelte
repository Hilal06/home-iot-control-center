<script>
  let { telemetry = {}, onCommand = () => {} } = $props();

  let isGarageBusy = $state(false);
  let isHvacBusy = $state(false);
  let isBreakerBusy = $state(false);

  // Local temperature adjustment buffer
  let targetTemp = $state(22);

  $effect(() => {
    if (telemetry.hvac_target_temp !== undefined) {
      targetTemp = telemetry.hvac_target_temp;
    }
  });

  async function handleGarage(targetState) {
    isGarageBusy = true;
    try {
      await onCommand({
        command: 'set_garage',
        state: targetState
      });
    } finally {
      isGarageBusy = false;
    }
  }

  async function handleHvacPower(newPower) {
    isHvacBusy = true;
    try {
      await onCommand({
        command: 'set_hvac',
        power: newPower,
        target_temp: targetTemp
      });
    } finally {
      isHvacBusy = false;
    }
  }

  async function adjustTemp(delta) {
    const next = Math.max(16, Math.min(32, targetTemp + delta));
    targetTemp = next;
    isHvacBusy = true;
    try {
      await onCommand({
        command: 'set_hvac',
        power: telemetry.hvac_power ?? true,
        target_temp: next
      });
    } finally {
      isHvacBusy = false;
    }
  }

  async function handleBreaker(targetState) {
    isBreakerBusy = true;
    try {
      await onCommand({
        command: 'set_breaker',
        state: targetState
      });
    } finally {
      isBreakerBusy = false;
    }
  }
</script>

<div class="panel">
  <div class="panel-header">
    <div class="title-with-bar">
      <span class="amber-bar"></span>
      <h2>PERALATAN UTAMA RUMAH</h2>
    </div>
  </div>

  <div class="actuator-grid">
    <!-- 1. GARAGE DOOR -->
    <div class="actuator-card">
      <div class="actuator-top">
        <span class="actuator-title">PINTU GARASI</span>
        <span class="badge {telemetry.garage_status ? 'warning' : 'inactive'}">
          {telemetry.garage_status ? 'TERBUKA' : 'TERTUTUP'}
        </span>
      </div>

      <div class="actuator-body">
        <p class="desc">Kontrol buka tutup pintu gerbang garasi mobil.</p>
        <div class="button-group">
          <button
            type="button"
            class="btn-warning"
            disabled={isGarageBusy || telemetry.garage_status}
            onclick={() => handleGarage(true)}
          >
            BUKA PINTU
          </button>
          <button
            type="button"
            class="btn-danger"
            disabled={isGarageBusy || !telemetry.garage_status}
            onclick={() => handleGarage(false)}
          >
            TUTUP PINTU
          </button>
        </div>
      </div>
    </div>

    <!-- 2. HVAC (AC 0.5 PK LOW WATT) -->
    <div class="actuator-card">
      <div class="actuator-top">
        <span class="actuator-title">AC RUANGAN (0.5 PK)</span>
        <span class="badge {telemetry.hvac_power ? 'active' : 'inactive'}">
          {telemetry.hvac_power ? 'MENYALA (~330W)' : 'MATI (0W)'}
        </span>
      </div>

      <div class="actuator-body">
        <div class="temp-control-row">
          <span class="temp-display">SUHU TARGET: <strong>{targetTemp} °C</strong></span>
          <div class="temp-btns">
            <button type="button" disabled={isHvacBusy || targetTemp <= 16} onclick={() => adjustTemp(-1)}>- 1°C</button>
            <button type="button" disabled={isHvacBusy || targetTemp >= 32} onclick={() => adjustTemp(1)}>+ 1°C</button>
          </div>
        </div>

        <div class="button-group">
          <button
            type="button"
            class={telemetry.hvac_power ? 'btn-danger' : 'btn-success'}
            disabled={isHvacBusy}
            onclick={() => handleHvacPower(!telemetry.hvac_power)}
          >
            {telemetry.hvac_power ? 'MATIKAN AC' : 'HIDUPKAN AC'}
          </button>
        </div>
      </div>
    </div>

    <!-- 3. KITCHEN BREAKER -->
    <div class="actuator-card">
      <div class="actuator-top">
        <span class="actuator-title">MCB LISTRIK DAPUR</span>
        <span class="badge {telemetry.kitchen_breaker ? 'active' : 'alert'}">
          {telemetry.kitchen_breaker ? 'NORMAL / ON' : 'MATI / OFF'}
        </span>
      </div>

      <div class="actuator-body">
        <p class="desc">Saklar pengaman jalur stop kontak dapur.</p>
        <div class="button-group">
          <button
            type="button"
            class="btn-danger"
            disabled={isBreakerBusy || !telemetry.kitchen_breaker}
            onclick={() => handleBreaker(false)}
          >
            MATIKAN MCB
          </button>
          <button
            type="button"
            class="btn-success"
            disabled={isBreakerBusy || telemetry.kitchen_breaker}
            onclick={() => handleBreaker(true)}
          >
            NYALAKAN MCB
          </button>
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

  .actuator-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
    gap: 12px;
    padding: 12px;
  }

  .actuator-card {
    background-color: var(--bg-card);
    border: 1px solid var(--border-medium);
    display: flex;
    flex-direction: column;
  }

  .actuator-top {
    display: flex;
    justify-content: space-between;
    align-items: center;
    background-color: var(--bg-panel-header);
    border-bottom: 1px solid var(--border-medium);
    padding: 7px 12px;
    font-family: var(--font-mono);
    font-size: 11px;
  }

  .actuator-title {
    font-weight: 700;
    color: var(--text-primary);
  }

  .actuator-body {
    padding: 12px;
    display: flex;
    flex-direction: column;
    gap: 10px;
    font-family: var(--font-mono);
  }

  .desc {
    color: var(--text-secondary);
    font-size: 11px;
    line-height: 1.4;
  }

  .temp-control-row {
    display: flex;
    justify-content: space-between;
    align-items: center;
    background-color: var(--bg-app);
    padding: 7px 10px;
    border: 1px solid var(--border-medium);
  }

  .temp-display {
    color: var(--text-secondary);
  }

  .temp-display strong {
    font-size: 14px;
    color: var(--text-amber);
  }

  .temp-btns {
    display: flex;
    gap: 4px;
  }

  .button-group {
    display: flex;
    gap: 8px;
  }

  .button-group button {
    flex: 1;
  }
</style>
