<script>
  import { onMount } from 'svelte';
  import { fetchPlnTokenStatus, rechargePlnTokenApi } from '../api.js';

  let tokenData = $state({
    remaining_kwh: 50.0,
    remaining_idr: 67600.0,
    tariff_per_kwh: 1352.0,
    daily_burn_rate_kwh: 4.5,
    estimated_days_remaining: 11.1,
    is_low_balance: false,
    low_balance_threshold_kwh: 15.0
  });

  let isLoading = $state(false);
  let showRechargeDrawer = $state(false);
  let customAmount = $state('');
  let isSubmitting = $state(false);
  let statusMessage = $state('');

  async function loadTokenStatus() {
    try {
      const data = await fetchPlnTokenStatus();
      if (data) {
        tokenData = data;
      }
    } catch (e) {
      console.warn('Could not fetch PLN token status:', e);
    }
  }

  async function handleRecharge(amountIdr) {
    if (!amountIdr || amountIdr <= 0 || isSubmitting) return;
    isSubmitting = true;
    statusMessage = 'Memproses isi ulang token...';
    try {
      const updated = await rechargePlnTokenApi({ amount_idr: Number(amountIdr) });
      tokenData = updated;
      statusMessage = `Berhasil menambah pulsa Rp ${Number(amountIdr).toLocaleString('id-ID')}!`;
      customAmount = '';
      setTimeout(() => {
        showRechargeDrawer = false;
        statusMessage = '';
      }, 1800);
    } catch (err) {
      console.error('Recharge error:', err);
      statusMessage = `Gagal: ${err.message}`;
    } finally {
      isSubmitting = false;
    }
  }

  onMount(() => {
    loadTokenStatus();
    // Poll every 30s to stay in sync with telemetries
    const interval = setInterval(loadTokenStatus, 30000);
    return () => clearInterval(interval);
  });
</script>

<div class="card pln-token-card">
  <div class="card-header">
    <div class="header-title">
      <span class="card-title">PULSA LISTRIK TOKEN PLN (PRABAYAR)</span>
      <span class="sub-text">TARIF R-1/900VA: Rp {tokenData.tariff_per_kwh.toLocaleString('id-ID')} / kWh</span>
    </div>
    <span class="badge {tokenData.is_low_balance ? 'alert pulse' : 'active'}">
      {tokenData.is_low_balance ? '⚠️ PULSA KRITIS' : 'PULSA AMAN'}
    </span>
  </div>

  <div class="card-body">
    <!-- Primary Balance Display -->
    <div class="balance-hero">
      <div class="hero-label">SISA SALDO TOKEN:</div>
      <div class="hero-val-wrap">
        <span class="hero-digits {tokenData.is_low_balance ? 'text-alert' : 'text-amber'}">
          {tokenData.remaining_kwh.toFixed(2)}
        </span>
        <span class="hero-unit">kWh</span>
      </div>
      <div class="hero-idr">
        Setara: <strong>Rp {Math.round(tokenData.remaining_idr).toLocaleString('id-ID')}</strong>
      </div>
    </div>

    <!-- Telemetry Details -->
    <div class="data-grid">
      <div class="data-row">
        <span class="data-label">Laju Pemakaian:</span>
        <span class="data-val">
          <strong>{tokenData.daily_burn_rate_kwh.toFixed(2)}</strong> kWh/hari
        </span>
      </div>

      <div class="data-row">
        <span class="data-label">Estimasi Ketahanan:</span>
        <span class="data-val {tokenData.estimated_days_remaining < 3 ? 'text-alert' : 'text-lime'}">
          <strong>~{tokenData.estimated_days_remaining.toFixed(1)}</strong> Hari
        </span>
      </div>

      <div class="data-row">
        <span class="data-label">Batas Peringatan:</span>
        <span class="data-val">
          &lt; {tokenData.low_balance_threshold_kwh.toFixed(0)} kWh (Bot Alert)
        </span>
      </div>
    </div>

    <!-- Burn Gauge Bar -->
    <div class="meter-bar-wrapper">
      <div class="meter-bar-label">
        <span>Tingkat Ketersediaan:</span>
        <strong>{Math.min(100, (tokenData.remaining_kwh / 100) * 100).toFixed(0)}%</strong>
      </div>
      <div class="meter-bar-track">
        <div
          class="meter-bar-fill {tokenData.is_low_balance ? 'fill-alert' : 'fill-amber'}"
          style="width: {Math.min(100, (tokenData.remaining_kwh / 100) * 100)}%;"
        ></div>
      </div>
    </div>

    <!-- Action Drawer Toggle -->
    <div class="action-row">
      <button
        type="button"
        class="btn-action"
        onclick={() => (showRechargeDrawer = !showRechargeDrawer)}
      >
        {showRechargeDrawer ? '▲ TUTUP FORM ISI ULANG' : '+ ISI ULANG TOKEN PLN'}
      </button>
    </div>

    <!-- Inline Recharge Drawer -->
    {#if showRechargeDrawer}
      <div class="recharge-drawer">
        <div class="drawer-title">PILIH NOMINAL TOP-UP PLN:</div>
        
        <div class="preset-pills">
          <button type="button" class="pill-btn" disabled={isSubmitting} onclick={() => handleRecharge(20000)}>
            Rp 20.000
          </button>
          <button type="button" class="pill-btn" disabled={isSubmitting} onclick={() => handleRecharge(50000)}>
            Rp 50.000
          </button>
          <button type="button" class="pill-btn" disabled={isSubmitting} onclick={() => handleRecharge(100000)}>
            Rp 100.000
          </button>
          <button type="button" class="pill-btn" disabled={isSubmitting} onclick={() => handleRecharge(200000)}>
            Rp 200.000
          </button>
        </div>

        <form
          class="custom-form"
          onsubmit={(e) => {
            e.preventDefault();
            handleRecharge(customAmount);
          }}
        >
          <input
            type="number"
            class="input-custom"
            placeholder="Nominal kustom (cth: 75000)"
            bind:value={customAmount}
            min="10000"
            step="1000"
            disabled={isSubmitting}
          />
          <button type="submit" class="btn-submit" disabled={isSubmitting || !customAmount}>
            {isSubmitting ? 'MEMPROSES...' : 'TOP-UP'}
          </button>
        </form>

        {#if statusMessage}
          <div class="status-msg">{statusMessage}</div>
        {/if}
      </div>
    {/if}
  </div>
</div>

<style>
  .card {
    background-color: var(--bg-card);
    border: 1px solid var(--border-medium);
    border-radius: 0;
    display: flex;
    flex-direction: column;
    font-family: var(--font-mono);
  }

  .card-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 10px 14px;
    background-color: var(--bg-card-header);
    border-bottom: 1px solid var(--border-medium);
  }

  .header-title {
    display: flex;
    flex-direction: column;
    gap: 2px;
  }

  .card-title {
    font-size: 11px;
    font-weight: 700;
    letter-spacing: 0.05em;
    color: var(--text-primary);
  }

  .sub-text {
    font-size: 9px;
    color: var(--text-muted);
  }

  .badge {
    font-size: 10px;
    font-weight: 700;
    padding: 3px 8px;
    border-radius: 0;
    letter-spacing: 0.05em;
  }
  .badge.active {
    background: rgba(16, 185, 129, 0.15);
    color: var(--accent-green);
    border: 1px solid var(--accent-green);
  }
  .badge.alert {
    background: rgba(239, 68, 68, 0.2);
    color: var(--accent-red);
    border: 1px solid var(--accent-red);
  }
  .badge.pulse {
    animation: blink 1s infinite alternate;
  }

  @keyframes blink {
    from { opacity: 1; }
    to { opacity: 0.4; }
  }

  .card-body {
    padding: 12px 14px;
    display: flex;
    flex-direction: column;
    gap: 12px;
  }

  /* Hero Balance */
  .balance-hero {
    background: rgba(0, 0, 0, 0.25);
    border: 1px solid var(--border-subtle);
    padding: 10px 12px;
    display: flex;
    flex-direction: column;
    gap: 4px;
  }

  .hero-label {
    font-size: 10px;
    color: var(--text-muted);
    letter-spacing: 0.05em;
  }

  .hero-val-wrap {
    display: flex;
    align-items: baseline;
    gap: 6px;
  }

  .hero-digits {
    font-size: 28px;
    font-weight: 800;
    line-height: 1;
    letter-spacing: -0.02em;
  }

  .hero-unit {
    font-size: 14px;
    font-weight: 700;
    color: var(--text-secondary);
  }

  .hero-idr {
    font-size: 11px;
    color: var(--text-secondary);
  }

  .hero-idr strong {
    color: var(--text-primary);
  }

  .data-grid {
    display: flex;
    flex-direction: column;
    gap: 6px;
  }

  .data-row {
    display: flex;
    justify-content: space-between;
    font-size: 11px;
  }

  .data-label {
    color: var(--text-muted);
  }

  .data-val {
    color: var(--text-primary);
  }

  .text-amber { color: var(--accent-amber); }
  .text-alert { color: var(--accent-red); }
  .text-lime { color: var(--accent-green); }

  /* Meter Bar */
  .meter-bar-wrapper {
    display: flex;
    flex-direction: column;
    gap: 4px;
  }

  .meter-bar-label {
    display: flex;
    justify-content: space-between;
    font-size: 10px;
    color: var(--text-muted);
  }

  .meter-bar-track {
    width: 100%;
    height: 6px;
    background: var(--bg-panel);
    border: 1px solid var(--border-subtle);
    border-radius: 0;
    overflow: hidden;
  }

  .meter-bar-fill {
    height: 100%;
    border-radius: 0;
    transition: width 0.3s ease;
  }
  .fill-amber { background: var(--accent-amber); }
  .fill-alert { background: var(--accent-red); }

  /* Action & Drawer */
  .action-row {
    margin-top: 4px;
  }

  .btn-action {
    width: 100%;
    padding: 7px;
    background: var(--bg-panel);
    border: 1px solid var(--border-medium);
    border-radius: 0;
    color: var(--accent-amber);
    font-family: var(--font-mono);
    font-size: 11px;
    font-weight: 700;
    letter-spacing: 0.05em;
    cursor: pointer;
    transition: all 0.15s ease;
  }

  .btn-action:hover {
    background: rgba(245, 158, 11, 0.1);
    border-color: var(--accent-amber);
  }

  .recharge-drawer {
    background: rgba(0, 0, 0, 0.3);
    border: 1px dashed var(--border-medium);
    padding: 10px;
    display: flex;
    flex-direction: column;
    gap: 8px;
  }

  .drawer-title {
    font-size: 10px;
    font-weight: 700;
    color: var(--text-secondary);
  }

  .preset-pills {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 6px;
  }

  .pill-btn {
    background: var(--bg-card);
    border: 1px solid var(--border-subtle);
    border-radius: 0;
    color: var(--text-primary);
    font-family: var(--font-mono);
    font-size: 10px;
    font-weight: 600;
    padding: 6px;
    cursor: pointer;
  }

  .pill-btn:hover:not(:disabled) {
    border-color: var(--accent-amber);
    color: var(--accent-amber);
  }

  .custom-form {
    display: flex;
    gap: 6px;
  }

  .input-custom {
    flex: 1;
    background: var(--bg-panel);
    border: 1px solid var(--border-medium);
    border-radius: 0;
    color: var(--text-primary);
    font-family: var(--font-mono);
    font-size: 11px;
    padding: 6px 8px;
  }

  .input-custom:focus {
    outline: none;
    border-color: var(--accent-amber);
  }

  .btn-submit {
    background: var(--accent-amber);
    color: #000;
    border: none;
    border-radius: 0;
    font-family: var(--font-mono);
    font-size: 10px;
    font-weight: 800;
    padding: 0 12px;
    cursor: pointer;
  }

  .btn-submit:disabled {
    opacity: 0.5;
    cursor: not-allowed;
  }

  .status-msg {
    font-size: 10px;
    color: var(--accent-amber);
    text-align: center;
  }
</style>

