<script>
  import { deviceStore } from '../deviceStore.js';
  import Plus from '@lucide/svelte/icons/plus';
  import Play from '@lucide/svelte/icons/play';
  import Edit3 from '@lucide/svelte/icons/edit-3';
  import Trash2 from '@lucide/svelte/icons/trash-2';
  import FlaskConical from '@lucide/svelte/icons/flask-conical';
  import Lock from '@lucide/svelte/icons/lock';
  import Check from '@lucide/svelte/icons/check';
  import AlertTriangle from '@lucide/svelte/icons/alert-triangle';
  import X from '@lucide/svelte/icons/x';

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
        time: new Date().toLocaleTimeString('id-ID'),
        message: `Command '${btn.command}' [Source: ${testSource}] berhasil diproses gateway: ${JSON.stringify(res || payload)}`,
      };
    } catch (err) {
      testResult = {
        btnId: btn.id,
        success: false,
        time: new Date().toLocaleTimeString('id-ID'),
        message: `Response dari backend untuk '${btn.command}' [Source: ${testSource}]: ${err.message}`,
      };
    } finally {
      testLoadingId = null;
    }
  }
</script>

<div class="mgr-container">
  <!-- Top Action Bar -->
  <div class="mgr-top-bar">
    <div class="mgr-meta">
      <span class="mgr-count-badge">
        <strong>{device.buttons?.length || 0}</strong> TOMBOL TERDAFTAR
      </span>
      <span class="mgr-meta-id mono">NODE: {device.id}</span>
      {#if device.is_locked}
        <span class="badge-locked-pill">
          <Lock size={12} />
          <span>KONFIGURASI TERKUNCI</span>
        </span>
      {/if}
    </div>

    <button
      type="button"
      class="btn-add-button"
      onclick={openAddModal}
      disabled={device.is_locked}
      title={device.is_locked ? 'Buka kunci perangkat untuk menambah tombol' : 'Tambah konfigurasi tombol fungsi baru'}
    >
      <Plus size={15} />
      <span>TAMBAH TOMBOL FUNGSI</span>
    </button>
  </div>

  {#if device.is_locked}
    <div class="lock-notice">
      <Lock size={14} />
      <span>STATUS PROTEKSI SMART LOCK: Seluruh tombol pada perangkat ini terkunci. Eksekusi manual dari UI/REST diblokir, namun automasi terjadwal & sensor darurat tetap dapat dieksekusi melalui bypass.</span>
    </div>
  {/if}

  {#if testResult}
    <div class="test-feedback {testResult.success ? 'feedback-success' : 'feedback-error'}">
      <div class="feedback-meta">
        <span class="feedback-title">
          {#if testResult.success}
            <Check size={14} />
          {:else}
            <AlertTriangle size={14} />
          {/if}
          <span>{testResult.success ? 'HASIL DISPATCH: SUKSES' : 'HASIL DISPATCH: GAGAL'}</span>
          <span class="feedback-time mono">({testResult.time})</span>
        </span>
        <button type="button" onclick={() => (testResult = null)} class="btn-feedback-close">
          <X size={13} />
          <span>TUTUP</span>
        </button>
      </div>
      <div class="feedback-msg mono">{testResult.message}</div>
    </div>
  {/if}

  <!-- Test Bench & Smart Lock Simulation Controls -->
  <div class="test-bench-bar">
    <div class="test-bench-label">
      <FlaskConical size={14} class="test-bench-icon" />
      <span>SIMULASI DISPATCH:</span>
    </div>
    <div class="test-bench-select-wrap">
      <select id="test_source_select" bind:value={testSource} class="test-bench-select">
        <option value="MANUAL">MANUAL (Perintah Pengguna - Ditolak jika Locked)</option>
        <option value="SCHEDULE">SCHEDULE (Timer Terjadwal - Bypass Lock)</option>
        <option value="AUTOMATION">AUTOMATION (Event Sensor / Mesin - Bypass Lock)</option>
        <option value="DEVICE_TRIGGER">DEVICE_TRIGGER (Sensor Gas / Kebocoran - Bypass Lock)</option>
        <option value="EMERGENCY">EMERGENCY (Prosedur Darurat Bencana - Bypass Lock)</option>
      </select>
    </div>
    <div class="test-bench-hint">
      {#if device.is_locked}
        <span class="test-badge {testSource === 'MANUAL' ? 'badge-rejected' : 'badge-bypass'}">
          {testSource === 'MANUAL' ? 'LOCKED: MANUAL REJECTED (423)' : 'LOCKED: BYPASS ALLOWED'}
        </span>
      {:else}
        <span class="test-badge badge-unlocked">
          <Check size={12} />
          <span>UNLOCKED: ALL PASS</span>
        </span>
      {/if}
    </div>
  </div>

  {#if !device.buttons || device.buttons.length === 0}
    <div class="empty-state">
      <div class="empty-icon-wrap">
        <Plus size={24} />
      </div>
      <span class="empty-title">BELUM ADA TOMBOL FUNGSI TERDAFTAR</span>
      <p class="empty-desc">Perangkat ini belum memiliki konfigurasi tombol kontrol. Klik tombol di atas untuk menambahkan fungsi tombol pertama.</p>
    </div>
  {:else}
    <div class="table-container">
      <table class="buttons-table">
        <thead>
          <tr>
            <th class="th-status">STATUS</th>
            <th class="th-name">NAMA & IDENTIFIER TOMBOL</th>
            <th class="th-cmd">PERINTAH (COMMAND & TARGET)</th>
            <th class="th-type">TIPE</th>
            <th class="th-actions">AKSI KONTROL</th>
          </tr>
        </thead>
        <tbody>
          {#each device.buttons as btn}
            <tr class={btn.enabled ? '' : 'row-disabled'}>
              <!-- Status Toggle -->
              <td class="td-status">
                <div class="toggle-cell">
                  <label class="toggle-switch-industrial" title={btn.enabled ? 'Aktif - Klik untuk menonaktifkan' : 'Nonaktif - Klik untuk mengaktifkan'}>
                    <input
                      type="checkbox"
                      checked={btn.enabled}
                      disabled={device.is_locked}
                      onchange={() => handleToggleEnabled(btn)}
                    />
                    <span class="toggle-slider"></span>
                  </label>
                  <span class="toggle-state-text {btn.enabled ? 'state-on' : 'state-off'}">
                    {btn.enabled ? 'AKTIF' : 'OFF'}
                  </span>
                </div>
              </td>

              <!-- Name & ID -->
              <td class="td-name">
                <div class="btn-name-wrap">
                  <span class="btn-label-text">{btn.label}</span>
                  <span class="btn-id-badge mono">ID: {btn.id}</span>
                </div>
              </td>

              <!-- Command & Target -->
              <td class="td-cmd">
                <div class="cmd-target-wrap">
                  <span class="cmd-pill mono">{btn.command}</span>
                  {#if btn.room}
                    <span class="target-chip">room: <strong>{btn.room}</strong></span>
                  {/if}
                  {#if btn.channel !== undefined}
                    <span class="target-chip">ch: <strong>{btn.channel}</strong></span>
                  {/if}
                  {#if btn.valve_id}
                    <span class="target-chip">valve: <strong>{btn.valve_id}</strong></span>
                  {/if}
                </div>
              </td>

              <!-- Type -->
              <td class="td-type">
                <span class="type-badge mono">{btn.type?.toUpperCase() || 'TOGGLE'}</span>
              </td>

              <!-- Action Controls -->
              <td class="td-actions">
                <div class="action-btn-group">
                  <!-- UJI / TEST -->
                  <button
                    type="button"
                    class="btn-act btn-act-test"
                    disabled={!btn.enabled || testLoadingId === btn.id}
                    onclick={() => handleTestDispatch(btn)}
                    title="Uji coba dispatch perintah ke node perangkat"
                  >
                    <Play size={12} fill="currentColor" />
                    <span>{testLoadingId === btn.id ? 'MENGUJI...' : 'UJI'}</span>
                  </button>

                  <!-- EDIT -->
                  <button
                    type="button"
                    class="btn-act btn-act-edit"
                    disabled={device.is_locked}
                    onclick={() => openEditModal(btn)}
                    title="Edit konfigurasi tombol"
                  >
                    <Edit3 size={12} />
                    <span>EDIT</span>
                  </button>

                  <!-- HAPUS -->
                  <button
                    type="button"
                    class="btn-act btn-act-danger"
                    disabled={device.is_locked}
                    onclick={() => handleDeleteButton(btn)}
                    title="Hapus tombol dari node"
                  >
                    <Trash2 size={12} />
                    <span>HAPUS</span>
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

<!-- Modal Tambah/Edit Tombol (Sub-Modal) -->
{#if isModalOpen}
  <!-- svelte-ignore a11y_click_events_have_key_events a11y_no_noninteractive_element_interactions -->
  <div class="sub-modal-backdrop" onclick={() => (isModalOpen = false)} onkeydown={(e) => e.key === 'Escape' && (isModalOpen = false)} role="dialog" aria-modal="true" tabindex="-1">
    <!-- svelte-ignore a11y_no_noninteractive_element_interactions -->
    <div class="sub-modal-window" onclick={(e) => e.stopPropagation()} role="document">
      <div class="sub-modal-header">
        <div class="sub-modal-title">
          {#if editMode === 'add'}
            <Plus size={16} class="accent-amber" />
            <span>TAMBAH TOMBOL FUNGSI BARU</span>
          {:else}
            <Edit3 size={16} class="accent-amber" />
            <span>EDIT KONFIGURASI TOMBOL</span>
          {/if}
        </div>
        <button type="button" onclick={() => (isModalOpen = false)} class="sub-modal-close" aria-label="Tutup Dialog">
          <X size={15} />
        </button>
      </div>

      {#if formError}
        <div class="form-error-banner">
          <AlertTriangle size={14} />
          <span>PERINGATAN: {formError}</span>
        </div>
      {/if}

      <form onsubmit={handleSaveButton} class="sub-modal-body">
        <div class="form-group">
          <label for="btn_id_input">Button Identifier (ID Unik):</label>
          <input
            id="btn_id_input"
            type="text"
            class="input-scada"
            bind:value={formId}
            disabled={editMode === 'edit'}
            required
            placeholder="misal: lamp_kitchen"
          />
          <span class="form-hint">Format huruf kecil, angka, garis bawah (misal: lamp_living, relay_ch1)</span>
        </div>

        <div class="form-group">
          <label for="btn_label_input">Label / Nama Tombol Tampilan:</label>
          <input
            id="btn_label_input"
            type="text"
            class="input-scada"
            bind:value={formLabel}
            required
            placeholder="misal: Lampu Dapur Utama"
          />
        </div>

        <div class="form-group">
          <label for="btn_cmd_select">Jenis Perintah (Command Action):</label>
          <select id="btn_cmd_select" class="input-scada" bind:value={formCommand}>
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
              class="input-scada"
              bind:value={formRoom}
              placeholder="misal: living_room, kitchen, yard_front, all"
            />
          </div>
        {:else if formCommand === 'set_relay'}
          <div class="form-group">
            <label for="btn_ch_input">Nomor Saluran Pin Relay (Channel 1-16):</label>
            <input
              id="btn_ch_input"
              type="number"
              class="input-scada"
              bind:value={formChannel}
              min="1"
              max="16"
              placeholder="1"
            />
          </div>
        {:else if formCommand === 'set_valve'}
          <div class="form-group">
            <label for="btn_valve_input">Valve Identifier (ID Keran Solenoid):</label>
            <input
              id="btn_valve_input"
              type="text"
              class="input-scada"
              bind:value={formValveId}
              placeholder="misal: main_inlet, tank_drain, gas_valve_1"
            />
          </div>
        {:else if formCommand === 'custom'}
          <div class="form-group">
            <label for="btn_custom_input">Custom Command Action:</label>
            <input
              id="btn_custom_input"
              type="text"
              class="input-scada"
              bind:value={formCommand}
              placeholder="misal: raw_gpio_write"
            />
          </div>
        {/if}

        <div class="form-row-2">
          <div class="form-group">
            <label for="btn_type_select">Mode Perilaku Tombol:</label>
            <select id="btn_type_select" class="input-scada" bind:value={formType}>
              <option value="toggle">Toggle (Saklar ON / OFF Dua Arah)</option>
              <option value="momentary">Momentary (Tekan Pulsa Sesaat)</option>
              <option value="trigger">Trigger (Satu Kali Eksekusi Perintah)</option>
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

        <div class="sub-modal-actions">
          <button type="button" class="btn-sub-cancel" onclick={() => (isModalOpen = false)}>
            BATAL
          </button>
          <button type="submit" class="btn-sub-submit">
            {editMode === 'add' ? '+ DAFTARKAN TOMBOL' : 'SIMPAN PERUBAHAN'}
          </button>
        </div>
      </form>
    </div>
  </div>
{/if}

<style>
  .mgr-container {
    display: flex;
    flex-direction: column;
    gap: 12px;
  }

  /* Top Action Bar */
  .mgr-top-bar {
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding: 10px 14px;
    background-color: var(--bg-card);
    border: 1px solid var(--border-medium);
    border-radius: 0;
  }

  .mgr-meta {
    display: flex;
    align-items: center;
    gap: 10px;
    flex-wrap: wrap;
  }

  .mgr-count-badge {
    font-family: var(--font-mono);
    font-size: 11px;
    font-weight: 700;
    color: var(--text-primary);
    background-color: var(--bg-hover);
    padding: 3px 8px;
    border: 1px solid var(--border-light);
    border-radius: 0;
    letter-spacing: 0.04em;
  }

  .mgr-meta-id {
    font-size: 11px;
    color: var(--text-secondary);
  }

  .badge-locked-pill {
    display: inline-flex;
    align-items: center;
    gap: 5px;
    padding: 3px 8px;
    background-color: #fef3c7;
    border: 1px solid #fde68a;
    color: #b45309;
    font-family: var(--font-mono);
    font-size: 10px;
    font-weight: 700;
    border-radius: 0;
    letter-spacing: 0.03em;
  }

  .btn-add-button {
    display: inline-flex;
    align-items: center;
    justify-content: center;
    gap: 6px;
    height: 34px;
    padding: 0 16px;
    background-color: #15803d;
    border: 1px solid #166534;
    border-radius: 0;
    color: #ffffff;
    font-family: var(--font-mono);
    font-size: 11px;
    font-weight: 700;
    letter-spacing: 0.04em;
    cursor: pointer;
    transition: background-color 0.15s ease, border-color 0.15s ease;
  }

  .btn-add-button:hover:not(:disabled) {
    background-color: #166534;
    border-color: #14532d;
  }

  .btn-add-button:disabled {
    opacity: 0.5;
    cursor: not-allowed;
  }

  /* Lock Notice */
  .lock-notice {
    display: flex;
    align-items: flex-start;
    gap: 8px;
    background-color: #fffbeb;
    border: 1px solid #fde68a;
    border-left: 3px solid #d97706;
    color: #92400e;
    padding: 8px 12px;
    font-family: var(--font-mono);
    font-size: 11px;
    line-height: 1.4;
    border-radius: 0;
  }

  /* Test Feedback */
  .test-feedback {
    padding: 10px 14px;
    font-family: var(--font-mono);
    font-size: 11px;
    border: 1px solid var(--border-medium);
    border-radius: 0;
  }

  .feedback-success {
    background-color: #f0fdf4;
    border-color: #86efac;
    color: #166534;
  }

  .feedback-error {
    background-color: #fef2f2;
    border-color: #fca5a5;
    color: #991b1b;
  }

  .feedback-meta {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 4px;
  }

  .feedback-title {
    display: flex;
    align-items: center;
    gap: 6px;
    font-weight: 700;
  }

  .feedback-time {
    font-weight: 400;
    color: var(--text-secondary);
  }

  .btn-feedback-close {
    display: inline-flex;
    align-items: center;
    gap: 4px;
    padding: 2px 6px;
    background: transparent;
    border: 1px solid currentColor;
    border-radius: 0;
    font-family: var(--font-mono);
    font-size: 10px;
    font-weight: 700;
    cursor: pointer;
    color: inherit;
  }

  .feedback-msg {
    font-size: 11px;
    word-break: break-all;
    line-height: 1.4;
  }

  /* Test Bench Bar */
  .test-bench-bar {
    display: flex;
    align-items: center;
    gap: 12px;
    padding: 8px 14px;
    background-color: var(--bg-panel-header);
    border: 1px solid var(--border-medium);
    border-radius: 0;
    flex-wrap: wrap;
  }

  .test-bench-label {
    display: flex;
    align-items: center;
    gap: 6px;
    font-family: var(--font-mono);
    font-size: 11px;
    font-weight: 700;
    color: var(--text-secondary);
    letter-spacing: 0.03em;
  }

  :global(.test-bench-icon) {
    color: var(--accent-amber);
  }

  .test-bench-select-wrap {
    flex: 1;
    min-width: 260px;
  }

  .test-bench-select {
    width: 100%;
    padding: 6px 10px;
    background-color: #ffffff;
    border: 1px solid var(--border-heavy);
    border-radius: 0;
    color: var(--text-primary);
    font-family: var(--font-mono);
    font-size: 11px;
    font-weight: 600;
  }

  .test-bench-hint {
    margin-left: auto;
  }

  .test-badge {
    display: inline-flex;
    align-items: center;
    gap: 4px;
    padding: 4px 8px;
    font-family: var(--font-mono);
    font-size: 10px;
    font-weight: 700;
    border-radius: 0;
    letter-spacing: 0.03em;
  }

  .badge-rejected {
    background-color: #fee2e2;
    border: 1px solid #fca5a5;
    color: #b91c1c;
  }

  .badge-bypass {
    background-color: #fef9c3;
    border: 1px solid #fde047;
    color: #854d0e;
  }

  .badge-unlocked {
    background-color: #dcfce7;
    border: 1px solid #86efac;
    color: #15803d;
  }

  /* Table Container */
  .table-container {
    overflow-x: auto;
    border: 1px solid var(--border-medium);
    border-radius: 0;
    background-color: var(--bg-card);
  }

  .buttons-table {
    width: 100%;
    border-collapse: collapse;
    text-align: left;
  }

  .buttons-table th {
    padding: 10px 14px;
    background-color: var(--bg-panel-header);
    border-bottom: 2px solid var(--border-heavy);
    font-family: var(--font-mono);
    font-size: 11px;
    font-weight: 700;
    color: var(--text-secondary);
    letter-spacing: 0.05em;
    white-space: nowrap;
  }

  .th-status {
    width: 120px;
  }

  .th-type {
    width: 90px;
  }

  .th-actions {
    text-align: right;
    width: 250px;
  }

  .buttons-table td {
    padding: 10px 14px;
    border-bottom: 1px solid var(--border-light);
    vertical-align: middle;
  }

  .buttons-table tr:last-child td {
    border-bottom: none;
  }

  .buttons-table tr:hover {
    background-color: var(--bg-hover);
  }

  .row-disabled td {
    opacity: 0.55;
    background-color: #f8fafc;
  }

  /* Status Toggle Switch - Industrial SCADA Rectilinear */
  .toggle-cell {
    display: flex;
    align-items: center;
    gap: 10px;
  }

  .toggle-switch-industrial {
    position: relative;
    display: inline-block;
    width: 42px;
    height: 22px;
    cursor: pointer;
    flex-shrink: 0;
  }

  .toggle-switch-industrial input {
    opacity: 0;
    width: 0;
    height: 0;
  }

  .toggle-slider {
    position: absolute;
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    background-color: #cbd5e1;
    border: 1px solid #94a3b8;
    border-radius: 0;
    transition: 0.15s ease;
  }

  .toggle-slider:before {
    position: absolute;
    content: '';
    height: 14px;
    width: 14px;
    left: 3px;
    bottom: 3px;
    background-color: #ffffff;
    box-shadow: 0 1px 2px rgba(0, 0, 0, 0.25);
    border-radius: 0;
    transition: 0.15s ease;
  }

  .toggle-switch-industrial input:checked + .toggle-slider {
    background-color: #16a34a;
    border-color: #15803d;
  }

  .toggle-switch-industrial input:checked + .toggle-slider:before {
    transform: translateX(20px);
  }

  .toggle-switch-industrial input:disabled + .toggle-slider {
    opacity: 0.5;
    cursor: not-allowed;
  }

  .toggle-state-text {
    font-family: var(--font-mono);
    font-size: 11px;
    font-weight: 700;
    letter-spacing: 0.04em;
  }

  .toggle-state-text.state-on {
    color: #15803d;
  }

  .toggle-state-text.state-off {
    color: var(--text-muted);
  }

  /* Name & ID column */
  .btn-name-wrap {
    display: flex;
    flex-direction: column;
    gap: 3px;
  }

  .btn-label-text {
    font-size: 13px;
    font-weight: 700;
    color: var(--text-primary);
  }

  .btn-id-badge {
    font-size: 10px;
    color: var(--text-muted);
  }

  /* Command & Target column */
  .cmd-target-wrap {
    display: flex;
    align-items: center;
    gap: 8px;
    flex-wrap: wrap;
  }

  .cmd-pill {
    padding: 2px 7px;
    background-color: var(--bg-hover);
    border: 1px solid var(--border-medium);
    border-radius: 0;
    color: #15803d;
    font-size: 11px;
    font-weight: 700;
  }

  .target-chip {
    font-size: 11px;
    color: var(--text-secondary);
  }

  .target-chip strong {
    color: var(--text-primary);
    font-family: var(--font-mono);
  }

  /* Type column */
  .type-badge {
    display: inline-block;
    padding: 3px 8px;
    background-color: #f1f5f9;
    border: 1px solid var(--border-medium);
    border-radius: 0;
    font-size: 10px;
    font-weight: 700;
    color: var(--text-secondary);
    letter-spacing: 0.04em;
  }

  /* Action Buttons Group */
  .action-btn-group {
    display: flex;
    align-items: center;
    justify-content: flex-end;
    gap: 6px;
  }

  .btn-act {
    display: inline-flex;
    align-items: center;
    justify-content: center;
    gap: 5px;
    height: 32px;
    padding: 0 12px;
    font-family: var(--font-mono);
    font-size: 11px;
    font-weight: 700;
    border-radius: 0;
    letter-spacing: 0.03em;
    cursor: pointer;
    transition: all 0.15s ease;
    text-transform: uppercase;
    white-space: nowrap;
  }

  .btn-act-test {
    background-color: #ffffff;
    border: 1px solid var(--border-heavy);
    color: var(--text-primary);
  }

  .btn-act-test:hover:not(:disabled) {
    background-color: #fffbeb;
    border-color: var(--accent-amber);
    color: var(--accent-amber);
  }

  .btn-act-edit {
    background-color: #f8fafc;
    border: 1px solid var(--border-medium);
    color: var(--text-secondary);
  }

  .btn-act-edit:hover:not(:disabled) {
    background-color: #f1f5f9;
    border-color: var(--border-heavy);
    color: var(--text-primary);
  }

  .btn-act-danger {
    background-color: #fef2f2;
    border: 1px solid #fecaca;
    color: #b91c1c;
  }

  .btn-act-danger:hover:not(:disabled) {
    background-color: #fee2e2;
    border-color: #f87171;
    color: #991b1b;
  }

  .btn-act:disabled {
    opacity: 0.45;
    cursor: not-allowed;
  }

  /* Empty state */
  .empty-state {
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    padding: 40px 20px;
    background-color: var(--bg-card);
    border: 1px dashed var(--border-medium);
    border-radius: 0;
    text-align: center;
  }

  .empty-icon-wrap {
    width: 44px;
    height: 44px;
    display: flex;
    align-items: center;
    justify-content: center;
    background-color: var(--bg-hover);
    border: 1px solid var(--border-medium);
    color: var(--text-muted);
    margin-bottom: 12px;
  }

  .empty-title {
    font-family: var(--font-mono);
    font-size: 12px;
    font-weight: 700;
    color: var(--text-secondary);
    letter-spacing: 0.04em;
  }

  .empty-desc {
    font-family: var(--font-mono);
    font-size: 11px;
    color: var(--text-muted);
    margin-top: 6px;
    max-width: 450px;
    line-height: 1.5;
  }

  /* Sub-Modal Tambah/Edit Tombol */
  .sub-modal-backdrop {
    position: fixed;
    inset: 0;
    background-color: rgba(15, 23, 42, 0.65);
    backdrop-filter: blur(2px);
    display: flex;
    align-items: center;
    justify-content: center;
    z-index: 1000;
  }

  .sub-modal-window {
    background-color: #ffffff;
    border: 1px solid var(--border-heavy);
    box-shadow: 0 20px 25px -5px rgba(0, 0, 0, 0.25);
    width: 92%;
    max-width: 580px;
    max-height: 90vh;
    overflow-y: auto;
    border-radius: 0;
    animation: fadeIn 0.15s ease-out;
  }

  @keyframes fadeIn {
    from {
      opacity: 0;
      transform: scale(0.98);
    }
    to {
      opacity: 1;
      transform: scale(1);
    }
  }

  .sub-modal-header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding: 12px 16px;
    background-color: var(--bg-panel-header);
    border-bottom: 1px solid var(--border-medium);
    border-radius: 0;
  }

  .sub-modal-title {
    display: flex;
    align-items: center;
    gap: 8px;
    font-family: var(--font-mono);
    font-size: 12px;
    font-weight: 700;
    color: var(--text-primary);
    letter-spacing: 0.04em;
  }

  .sub-modal-close {
    display: flex;
    align-items: center;
    justify-content: center;
    width: 28px;
    height: 28px;
    background: transparent;
    border: 1px solid var(--border-medium);
    border-radius: 0;
    cursor: pointer;
    color: var(--text-secondary);
    transition: all 0.15s ease;
  }

  .sub-modal-close:hover {
    background-color: var(--bg-hover);
    color: var(--text-primary);
    border-color: var(--border-heavy);
  }

  .form-error-banner {
    display: flex;
    align-items: center;
    gap: 8px;
    padding: 8px 14px;
    background-color: #fee2e2;
    border-bottom: 1px solid #fca5a5;
    color: #991b1b;
    font-family: var(--font-mono);
    font-size: 11px;
    font-weight: 700;
  }

  .sub-modal-body {
    padding: 16px;
    display: flex;
    flex-direction: column;
    gap: 14px;
  }

  .form-group {
    display: flex;
    flex-direction: column;
    gap: 6px;
  }

  .form-group label {
    font-family: var(--font-mono);
    font-size: 11px;
    font-weight: 700;
    color: var(--text-primary);
    letter-spacing: 0.03em;
  }

  .input-scada {
    width: 100%;
    padding: 7px 10px;
    background-color: #ffffff;
    border: 1px solid var(--border-medium);
    border-radius: 0;
    color: var(--text-primary);
    font-family: var(--font-mono);
    font-size: 12px;
    transition: border-color 0.15s ease;
  }

  .input-scada:focus {
    outline: none;
    border-color: var(--accent-amber);
    box-shadow: 0 0 0 1px var(--accent-amber);
  }

  .form-hint {
    font-size: 10px;
    color: var(--text-muted);
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
    height: 34px;
  }

  .checkbox-label {
    font-family: var(--font-mono);
    font-size: 11px;
    color: var(--text-secondary);
  }

  .sub-modal-actions {
    display: flex;
    justify-content: flex-end;
    gap: 10px;
    margin-top: 8px;
    padding-top: 14px;
    border-top: 1px solid var(--border-light);
  }

  .btn-sub-cancel {
    height: 36px;
    padding: 0 16px;
    background-color: #f1f5f9;
    border: 1px solid var(--border-medium);
    border-radius: 0;
    color: var(--text-secondary);
    font-family: var(--font-mono);
    font-size: 11px;
    font-weight: 700;
    cursor: pointer;
  }

  .btn-sub-cancel:hover {
    background-color: #e2e8f0;
    color: var(--text-primary);
  }

  .btn-sub-submit {
    height: 36px;
    padding: 0 18px;
    background-color: #15803d;
    border: 1px solid #166534;
    border-radius: 0;
    color: #ffffff;
    font-family: var(--font-mono);
    font-size: 11px;
    font-weight: 700;
    letter-spacing: 0.04em;
    cursor: pointer;
  }

  .btn-sub-submit:hover {
    background-color: #166534;
  }

  @media (max-width: 680px) {
    .form-row-2 {
      grid-template-columns: 1fr;
    }
    .test-bench-bar {
      flex-direction: column;
      align-items: flex-start;
    }
    .test-bench-hint {
      margin-left: 0;
    }
    .action-btn-group {
      flex-wrap: wrap;
    }
  }
</style>
