<script>
  import { deviceStore } from '../deviceStore.js';

  let { device = {}, onCommand = () => {} } = $props();

  let isModalOpen = $state(false);
  let editMode = $state('add'); // 'add' | 'edit'
  let currentBtnId = $state('');

  // Form states
  let formId = $state('');
  let formLabel = $state('');
  let formCommand = $state('set_lamp');
  let formRoom = $state('');
  let formChannel = $state('');
  let formValveId = $state('');
  let formType = $state('toggle');
  let formDanger = $state(false);
  let formError = $state('');

  // Live Test feedback & smart lock testing
  let testLoadingId = $state(null);
  let testResult = $state(null);
  let testSource = $state('MANUAL'); // 'MANUAL' | 'SCHEDULE' | 'AUTOMATION' | 'DEVICE_TRIGGER' | 'EMERGENCY'

  const KNOWN_COMMANDS = [
    { cmd: 'set_lamp', label: 'set_lamp (Lighting Switch)', hasRoom: true },
    { cmd: 'set_valve', label: 'set_valve (Solenoid / Divert Valve)', hasValve: true },
    { cmd: 'set_relay', label: 'set_relay (General Relay Channel)', hasChannel: true },
    { cmd: 'trigger_emergency', label: 'trigger_emergency (Siren Alarm & Cutoff)', isEmergency: true },
    { cmd: 'set_garage', label: 'set_garage (Garage Actuator)', hasRoom: false },
    { cmd: 'set_hvac', label: 'set_hvac (HVAC / AC Controller)', hasRoom: false },
    { cmd: 'set_breaker', label: 'set_breaker (Circuit Breaker Relay)', hasRoom: false },
    { cmd: 'set_solar', label: 'set_solar (Solar MPPT Inverter)', hasRoom: false },
    { cmd: 'ping', label: 'ping (Telemetry & Sensor Ping)', hasRoom: false },
    { cmd: 'reboot_esp', label: 'reboot_esp (Restart Device MCU)', hasRoom: false },
    { cmd: 'custom', label: 'Custom Command...', isCustom: true },
  ];

  function openAddModal() {
    if (device.is_locked) return;
    editMode = 'add';
    currentBtnId = '';
    formId = `btn_${Date.now().toString(36)}`;
    formLabel = '';
    formCommand = 'set_lamp';
    formRoom = 'living_room';
    formChannel = '';
    formValveId = '';
    formType = 'toggle';
    formDanger = false;
    formError = '';
    isModalOpen = true;
  }

  function openEditModal(btn) {
    if (device.is_locked) return;
    editMode = 'edit';
    currentBtnId = btn.id;
    formId = btn.id;
    formLabel = btn.label;
    formCommand = btn.command;
    formRoom = btn.room || '';
    formChannel = btn.channel !== undefined ? String(btn.channel) : '';
    formValveId = btn.valve_id || '';
    formType = btn.type || 'toggle';
    formDanger = !!btn.danger;
    formError = '';
    isModalOpen = true;
  }

  function handleSaveButton(e) {
    e.preventDefault();
    formError = '';
    try {
      if (editMode === 'add') {
        deviceStore.addButton(device.id, {
          id: formId,
          label: formLabel,
          command: formCommand,
          room: formRoom || undefined,
          channel: formChannel ? Number(formChannel) : undefined,
          valve_id: formValveId ? formValveId.trim() : undefined,
          type: formType,
          danger: formDanger,
        });
      } else {
        deviceStore.updateButton(device.id, currentBtnId, {
          label: formLabel,
          command: formCommand,
          room: formRoom || undefined,
          channel: formChannel ? Number(formChannel) : undefined,
          valve_id: formValveId ? formValveId.trim() : undefined,
          type: formType,
          danger: formDanger,
        });
      }
      isModalOpen = false;
    } catch (err) {
      formError = err.message;
    }
  }

  function handleToggleEnabled(btn) {
    if (device.is_locked) return;
    try {
      deviceStore.toggleButton(device.id, btn.id);
    } catch (err) {
      alert(err.message);
    }
  }

  function handleDeleteButton(btn) {
    if (device.is_locked) return;
    const confirmDelete = confirm(`Hapus tombol fungsi '${btn.label}' (ID: ${btn.id}) dari perangkat ${device.name}?`);
    if (confirmDelete) {
      try {
        deviceStore.deleteButton(device.id, btn.id);
      } catch (err) {
        alert(err.message);
      }
    }
  }

  async function handleTestDispatch(btn) {
    testLoadingId = btn.id;
    testResult = null;

    let payload = { command: btn.command };
    if (btn.room) payload.room = btn.room;
    if (btn.channel !== undefined) payload.channel = btn.channel;
    if (btn.valve_id) payload.valve_id = btn.valve_id;

    if (btn.command === 'set_lamp') {
      payload.state = true;
    } else if (btn.command === 'set_garage') {
      payload.state = 'open';
    } else if (btn.command === 'set_hvac') {
      payload.power = true;
      payload.target_temp = 22;
    } else if (btn.command === 'set_solar') {
      payload.charge_enabled = true;
      payload.max_watts = 3000;
    } else if (btn.command === 'set_breaker') {
      payload.state = true;
    } else if (btn.command === 'set_relay') {
      payload.channel = btn.channel || 1;
      payload.state = true;
    } else if (btn.command === 'set_valve') {
      payload.valve_id = btn.valve_id || btn.room || 'main_valve';
      payload.state = true;
    } else if (btn.command === 'trigger_emergency') {
      payload.alarm = true;
      payload.cutoff = true;
      payload.reason = 'Manual Test from Button Manager';
    }

    // Attach simulated source to test smart lock routing
    payload.source = testSource;

    try {
      const res = await onCommand(payload);
      testResult = {
        btnId: btn.id,
        success: true,
        time: new Date().toLocaleTimeString(),
        message: `Command '${btn.command}' [Source: ${testSource}] sukses diproses node: ${JSON.stringify(res || payload)}`,
      };
    } catch (err) {
      testResult = {
        btnId: btn.id,
        success: false,
        time: new Date().toLocaleTimeString(),
        message: `Response dari backend untuk '${btn.command}' [Source: ${testSource}]: ${err.message}`,
      };
    } finally {
      testLoadingId = null;
    }
  }
</script>

<div class="panel">
  <div class="panel-header">
    <div class="header-info">
      <h2>[KELOLA PERFUNGSI TOMBOL: {device.name || device.id}]</h2>
      <span class="device-sub">ID: {device.id} | TOTAL: {device.buttons?.length || 0} TOMBOL</span>
    </div>

    <div class="header-actions">
      <button
        class="btn-success"
        onclick={openAddModal}
        disabled={device.is_locked}
        title={device.is_locked ? 'Buka kunci perangkat untuk menambah tombol' : ''}
      >
        [+ TAMBAH TOMBOL FUNGSI]
      </button>
    </div>
  </div>

  {#if device.is_locked}
    <div class="lock-notice">
      [STATUS: DEVICE LOCKED] - Seluruh konfigurasi tombol pada perangkat ini terkunci dan diproteksi dari pengubahan atau penghapusan tidak disengaja.
    </div>
  {/if}

  {#if testResult}
    <div class="test-feedback {testResult.success ? 'feedback-success' : 'feedback-error'}">
      <div class="feedback-meta">
        <span>[{testResult.success ? 'SUCCESS' : 'ERROR'}] TEST DISPATCH ({testResult.time})</span>
        <button onclick={() => (testResult = null)} class="btn-sm">[TUTUP]</button>
      </div>
      <div class="feedback-msg">{testResult.message}</div>
    </div>
  {/if}

  <div class="panel-body">
    <!-- Test Bench & Smart Lock Simulation Controls -->
    <div class="test-controls-bar">
      <span class="test-ctrl-label">[TEST BENCH SIMULASI]:</span>
      <div class="test-select-group">
        <label for="test_source_select" class="test-source-label">Simulasi Source:</label>
        <select id="test_source_select" bind:value={testSource} class="test-select">
          <option value="MANUAL">MANUAL (Perintah Pengguna - Ditolak jika Locked)</option>
          <option value="SCHEDULE">SCHEDULE (Timer Terjadwal - Bypass Lock)</option>
          <option value="AUTOMATION">AUTOMATION (Event Mesin / Tabung Penuh - Bypass Lock)</option>
          <option value="DEVICE_TRIGGER">DEVICE_TRIGGER (Sensor Gas / Kebocoran - Bypass Lock)</option>
          <option value="EMERGENCY">EMERGENCY (Prosedur Darurat Bencana - Bypass Lock)</option>
        </select>
      </div>
      <div class="test-hint">
        {#if device.is_locked}
          <span class="badge {testSource === 'MANUAL' ? 'alert' : 'active'}">
            {testSource === 'MANUAL' ? '[LOCK TEST: REJECTED (423)]' : '[LOCK TEST: BYPASS ALLOWED]'}
          </span>
        {:else}
          <span class="badge">[UNLOCKED: ALL PASS]</span>
        {/if}
      </div>
    </div>

    {#if !device.buttons || device.buttons.length === 0}
      <div class="empty-state">
        <span class="empty-title">[BELUM ADA TOMBOL FUNGSI TERDAFTAR]</span>
        <p class="empty-desc">Perangkat ini belum memiliki konfigurasi tombol kontrol. Klik tombol di atas untuk menambahkan fungsi tombol pertama.</p>
      </div>
    {:else}
      <div class="buttons-table-wrapper">
        <table class="buttons-table">
          <thead>
            <tr>
              <th style="width: 130px;">STATUS</th>
              <th>NAMA TOMBOL</th>
              <th>COMMAND & TARGET</th>
              <th>TIPE</th>
              <th style="text-align: right;">AKSI KONTROL</th>
            </tr>
          </thead>
          <tbody>
            {#each device.buttons as btn}
              <tr class={btn.enabled ? '' : 'row-disabled'}>
                <td>
                  <span class="badge {btn.enabled ? 'active' : 'inactive'}">
                    {btn.enabled ? '[ENABLED]' : '[DISABLED]'}
                  </span>
                  {#if btn.danger}
                    <span class="badge alert">[CRITICAL]</span>
                  {/if}
                </td>

                <td>
                  <strong class="btn-name">{btn.label}</strong>
                  <div class="btn-sub-id">ID: {btn.id}</div>
                </td>

                <td>
                  <span class="mono-code">{btn.command}</span>
                  {#if btn.room}
                    <span class="sub-target">room: <strong>{btn.room}</strong></span>
                  {/if}
                  {#if btn.channel !== undefined}
                    <span class="sub-target">ch: <strong>{btn.channel}</strong></span>
                  {/if}
                  {#if btn.valve_id}
                    <span class="sub-target">valve: <strong>{btn.valve_id}</strong></span>
                  {/if}
                </td>

                <td>
                  <span class="badge">{btn.type?.toUpperCase() || 'TOGGLE'}</span>
                </td>

                <td style="text-align: right;">
                  <div class="btn-actions-inline">
                    <!-- Live test dispatch -->
                    <button
                      class="btn-sm"
                      disabled={!btn.enabled || testLoadingId === btn.id}
                      onclick={() => handleTestDispatch(btn)}
                      title="Uji coba eksekusi perintah ke node perangkat"
                    >
                      {testLoadingId === btn.id ? 'TESTING...' : '[TEST DISPATCH]'}
                    </button>

                    <!-- Toggle enabled -->
                    <button
                      class="btn-sm"
                      disabled={device.is_locked}
                      onclick={() => handleToggleEnabled(btn)}
                    >
                      {btn.enabled ? '[NONAKTIFKAN]' : '[AKTIFKAN]'}
                    </button>

                    <!-- Edit -->
                    <button
                      class="btn-sm"
                      disabled={device.is_locked}
                      onclick={() => openEditModal(btn)}
                    >
                      [EDIT]
                    </button>

                    <!-- Delete -->
                    <button
                      class="btn-sm btn-danger"
                      disabled={device.is_locked}
                      onclick={() => handleDeleteButton(btn)}
                    >
                      [HAPUS]
                    </button>
                  </div>
                </td>
              </tr>
            {/each}
          </tbody>
        </table>
      </div>
    {/if}
  </div>
</div>

<!-- Modal Tambah/Edit Tombol -->
{#if isModalOpen}
  <!-- svelte-ignore a11y_click_events_have_key_events a11y_no_noninteractive_element_interactions -->
  <div class="modal-backdrop" onclick={() => (isModalOpen = false)} onkeydown={(e) => e.key === 'Escape' && (isModalOpen = false)} role="dialog" aria-modal="true" tabindex="-1">
    <!-- svelte-ignore a11y_no_noninteractive_element_interactions -->
    <div class="modal-window" onclick={(e) => e.stopPropagation()} role="document">
      <div class="panel-header">
        <h2>{editMode === 'add' ? '[+ TAMBAH TOMBOL FUNGSI]' : '[EDIT TOMBOL FUNGSI]'}</h2>
        <button onclick={() => (isModalOpen = false)} class="btn-sm">[X]</button>
      </div>

      {#if formError}
        <div class="lock-notice alert-red">[ERROR]: {formError}</div>
      {/if}

      <form onsubmit={handleSaveButton} class="panel-body">
        <div class="form-group">
          <label for="btn_id_input">Button Identifier (ID):</label>
          <input
            id="btn_id_input"
            type="text"
            bind:value={formId}
            disabled={editMode === 'edit'}
            required
            placeholder="misal: lamp_kitchen"
          />
        </div>

        <div class="form-group">
          <label for="btn_label_input">Label / Nama Tombol:</label>
          <input
            id="btn_label_input"
            type="text"
            bind:value={formLabel}
            required
            placeholder="misal: Lampu Dapur Utama"
          />
        </div>

        <div class="form-group">
          <label for="btn_cmd_select">Jenis Perintah (Command):</label>
          <select id="btn_cmd_select" bind:value={formCommand}>
            {#each KNOWN_COMMANDS as kc}
              <option value={kc.cmd}>{kc.label}</option>
            {/each}
          </select>
        </div>

        {#if formCommand === 'set_lamp'}
          <div class="form-group">
            <label for="btn_room_input">Parameter Room / Zona Lampu:</label>
            <input
              id="btn_room_input"
              type="text"
              bind:value={formRoom}
              placeholder="misal: living_room, kitchen, yard_front, all"
            />
          </div>
        {:else if formCommand === 'set_relay'}
          <div class="form-group">
            <label for="btn_ch_input">Nomor Saluran Relay (Channel):</label>
            <input
              id="btn_ch_input"
              type="number"
              bind:value={formChannel}
              placeholder="1 atau 2"
              min="1"
              max="16"
            />
          </div>
        {:else if formCommand === 'set_valve'}
          <div class="form-group">
            <label for="btn_valve_input">Valve Identifier (ID Katup / Solenoid Valve):</label>
            <input
              id="btn_valve_input"
              type="text"
              bind:value={formValveId}
              placeholder="misal: main_gas, irrigation_divert, emergency_drain"
              required
            />
            <div class="form-hint">
              ID katup untuk gas shutoff atau pengalihan jalur kran air (irrigation divert).
            </div>
          </div>
        {:else if formCommand === 'trigger_emergency'}
          <div class="form-group">
            <span class="checkbox-label" style="font-weight: 700; color: #f87171;">Emergency Subsystem Parameters:</span>
            <div class="form-hint">
              Menjalankan sirene alarm peringatan dan pemutus darurat saat terjadi gas leak atau over-limit sensor.
            </div>
          </div>
        {:else if formCommand === 'custom'}
          <div class="form-group">
            <label for="btn_custom_room">Target / Parameter Tambahan (Opsional):</label>
            <input
              id="btn_custom_room"
              type="text"
              bind:value={formRoom}
              placeholder="Target key atau payload string"
            />
          </div>
        {/if}

        <div class="form-row-2">
          <div class="form-group">
            <label for="btn_type_select">Tipe Kontrol:</label>
            <select id="btn_type_select" bind:value={formType}>
              <option value="toggle">Toggle (Nyala / Mati)</option>
              <option value="trigger">Trigger (Satu Kali Eksekusi)</option>
            </select>
          </div>

          <div class="form-group">
            <label for="btn_danger_check">Proteksi Kritis:</label>
            <div class="checkbox-row">
              <input
                id="btn_danger_check"
                type="checkbox"
                bind:checked={formDanger}
              />
              <span class="checkbox-label">Tandai sebagai Tombol Kritis (Breaker/Reboot)</span>
            </div>
          </div>
        </div>

        <div class="modal-actions">
          <button type="button" onclick={() => (isModalOpen = false)}>
            [BATAL]
          </button>
          <button type="submit" class="btn-success">
            {editMode === 'add' ? '[+ TAMBAHKAN TOMBOL]' : '[SIMPAN TOMBOL]'}
          </button>
        </div>
      </form>
    </div>
  </div>
{/if}

<style>
  .header-info {
    display: flex;
    flex-direction: column;
    gap: 2px;
  }

  .device-sub {
    font-family: var(--font-mono);
    font-size: 10px;
    color: var(--text-secondary);
  }

  .lock-notice {
    background-color: var(--status-warning-bg);
    color: var(--status-warning-text);
    border-bottom: 1px solid var(--status-warning-border);
    padding: 6px 12px;
    font-family: var(--font-mono);
    font-size: 11px;
    font-weight: 700;
  }

  .alert-red {
    background-color: var(--status-alert-bg);
    color: var(--status-alert-text);
    border-bottom-color: var(--status-alert-border);
  }

  .test-feedback {
    padding: 8px 12px;
    font-family: var(--font-mono);
    font-size: 11px;
    border-bottom: 1px solid var(--border-medium);
  }

  .feedback-success {
    background-color: var(--status-active-bg);
    color: var(--status-active-text);
    border-color: var(--status-active-border);
  }

  .feedback-error {
    background-color: var(--status-alert-bg);
    color: var(--status-alert-text);
    border-color: var(--status-alert-border);
  }

  .feedback-meta {
    display: flex;
    justify-content: space-between;
    align-items: center;
    font-weight: 700;
    margin-bottom: 4px;
  }

  .feedback-msg {
    font-size: 10px;
    word-break: break-all;
  }

  .buttons-table-wrapper {
    overflow-x: auto;
  }

  .buttons-table {
    width: 100%;
    border-collapse: collapse;
  }

  .row-disabled td {
    opacity: 0.5;
  }

  .btn-name {
    color: var(--text-primary);
    font-size: 12px;
  }

  .btn-sub-id {
    font-size: 10px;
    color: var(--text-muted);
  }

  .mono-code {
    background-color: #000;
    border: 1px solid var(--border-medium);
    padding: 1px 5px;
    color: #4ade80;
    font-family: var(--font-mono);
  }

  .sub-target {
    margin-left: 6px;
    font-size: 10px;
    color: var(--text-secondary);
  }

  .btn-actions-inline {
    display: flex;
    justify-content: flex-end;
    gap: 4px;
    flex-wrap: wrap;
  }

  .btn-sm {
    padding: 2px 6px;
    font-size: 10px;
  }

  .empty-state {
    padding: 30px;
    text-align: center;
    background-color: var(--bg-card);
    border: 1px dashed var(--border-medium);
  }

  .empty-title {
    font-family: var(--font-mono);
    font-size: 12px;
    font-weight: 700;
    color: var(--text-secondary);
  }

  .empty-desc {
    font-family: var(--font-mono);
    font-size: 11px;
    color: var(--text-muted);
    margin-top: 6px;
  }

  .form-row-2 {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 12px;
  }

  .checkbox-row {
    display: flex;
    align-items: center;
    gap: 8px;
    margin-top: 6px;
  }

  .checkbox-label {
    font-family: var(--font-mono);
    font-size: 10px;
    color: var(--text-secondary);
  }

  .modal-actions {
    display: flex;
    justify-content: flex-end;
    gap: 8px;
    margin-top: 16px;
    padding-top: 12px;
    border-top: 1px solid var(--border-light);
  }

  .test-controls-bar {
    display: flex;
    align-items: center;
    gap: 12px;
    padding: 8px 12px;
    background-color: #141414;
    border-bottom: 1px solid var(--border-medium);
    font-family: var(--font-mono);
    font-size: 11px;
    flex-wrap: wrap;
    margin-bottom: 8px;
  }

  .test-ctrl-label {
    font-weight: 700;
    color: var(--text-secondary);
  }

  .test-select-group {
    display: flex;
    align-items: center;
    gap: 6px;
  }

  .test-source-label {
    color: var(--text-muted);
    font-size: 10px;
  }

  .test-select {
    padding: 3px 6px;
    background-color: #0d0d0d;
    border: 1px solid var(--border-medium);
    color: #4ade80;
    font-family: var(--font-mono);
    font-size: 11px;
  }

  .test-hint {
    margin-left: auto;
  }

  @media (max-width: 600px) {
    .form-row-2 {
      grid-template-columns: 1fr;
    }
    .test-controls-bar {
      flex-direction: column;
      align-items: flex-start;
      gap: 6px;
    }
    .test-hint {
      margin-left: 0;
    }
  }
</style>
