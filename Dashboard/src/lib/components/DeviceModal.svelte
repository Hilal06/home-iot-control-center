<script>
  import { BUTTON_TEMPLATES } from '../deviceStore.js';

  let {
    isOpen = false,
    mode = 'add', // 'add' | 'edit'
    initialData = null,
    onSave = () => {},
    onClose = () => {},
  } = $props();

  let id = $state('');
  let name = $state('');
  let ip_address = $state('');
  let role = $state('Edge Gateway');
  let templateId = $state('LIGHTING_4ZONE');
  let description = $state('');
  let allow_automation_when_locked = $state(true);
  let errorMsg = $state('');

  $effect(() => {
    if (isOpen) {
      errorMsg = '';
      if (mode === 'edit' && initialData) {
        id = initialData.id || '';
        name = initialData.name || '';
        ip_address = initialData.ip_address || '';
        role = initialData.role || 'Edge Gateway';
        description = initialData.description || '';
        allow_automation_when_locked = initialData.allow_automation_when_locked ?? true;
      } else {
        id = '';
        name = '';
        ip_address = '192.168.100.';
        role = 'Lighting Controller';
        templateId = 'LIGHTING_4ZONE';
        description = '';
        allow_automation_when_locked = true;
      }
    }
  });

  function handleSubmit(e) {
    e.preventDefault();
    errorMsg = '';

    if (mode === 'add') {
      const cleanId = id.trim().toLowerCase().replace(/[^a-z0-9_-]/g, '_');
      if (!cleanId) {
        errorMsg = 'ID Perangkat wajib diisi (contoh: esp32_relay_sub1).';
        return;
      }
      try {
        onSave({
          id: cleanId,
          name: name.trim() || `Perangkat ${cleanId}`,
          ip_address: ip_address.trim() || '192.168.100.x',
          role: role.trim(),
          templateId,
          description: description.trim(),
          allow_automation_when_locked,
        });
        onClose();
      } catch (err) {
        errorMsg = err.message;
      }
    } else {
      try {
        onSave({
          name: name.trim(),
          ip_address: ip_address.trim(),
          role: role.trim(),
          description: description.trim(),
          allow_automation_when_locked,
        });
        onClose();
      } catch (err) {
        errorMsg = err.message;
      }
    }
  }
</script>

{#if isOpen}
  <!-- svelte-ignore a11y_click_events_have_key_events a11y_no_noninteractive_element_interactions -->
  <div class="modal-backdrop" onclick={onClose} onkeydown={(e) => e.key === 'Escape' && onClose()} role="dialog" aria-modal="true" tabindex="-1">
    <!-- svelte-ignore a11y_no_noninteractive_element_interactions -->
    <div class="modal-window" onclick={(e) => e.stopPropagation()} role="document">
      <div class="panel-header">
        <h2>{mode === 'add' ? '[+ TAMBAH DEVICE BARU]' : '[EDIT INFORMASI DEVICE]'}</h2>
        <button onclick={onClose} class="btn-close">[ESC / X]</button>
      </div>

      {#if errorMsg}
        <div class="error-banner">
          [ERROR]: {errorMsg}
        </div>
      {/if}

      <form onsubmit={handleSubmit} class="panel-body">
        <div class="form-group">
          <label for="dev_id">Device Identifier (ID Unique):</label>
          <input
            id="dev_id"
            type="text"
            bind:value={id}
            placeholder="misal: esp32_garden_hub"
            disabled={mode === 'edit'}
            required={mode === 'add'}
          />
          <div class="form-hint">
            {mode === 'edit'
              ? 'ID perangkat bersifat permanen dan tidak dapat diubah.'
              : 'Gunakan format huruf kecil, angka, atau garis bawah (_).'}
          </div>
        </div>

        <div class="form-group">
          <label for="dev_name">Nama Tampilan Perangkat:</label>
          <input
            id="dev_name"
            type="text"
            bind:value={name}
            placeholder="misal: ESP32 Taman & Penerangan Luar"
            required
          />
        </div>

        <div class="form-row-2">
          <div class="form-group">
            <label for="dev_ip">Alamat IP / Hostname:</label>
            <input
              id="dev_ip"
              type="text"
              bind:value={ip_address}
              placeholder="192.168.100.x"
            />
          </div>

          <div class="form-group">
            <label for="dev_role">Peran / Klasifikasi Perangkat:</label>
            <select id="dev_role" bind:value={role}>
              <option value="Edge Gateway">Edge Gateway & Hub</option>
              <option value="Gas & Hazard Safety Node">Gas & Hazard Safety Controller</option>
              <option value="Water Tank & Pump Controller">Water Tank & Pump Controller</option>
              <option value="Lighting Controller">Lighting Substation</option>
              <option value="HVAC / Climate Controller">HVAC & Climate Node</option>
              <option value="Power & Solar Inverter">Power & Inverter Node</option>
              <option value="Actuator Relay Module">Smart Relay / Actuator</option>
              <option value="Sensor Telemetry Node">Sensor Monitor Node</option>
            </select>
          </div>
        </div>

        {#if mode === 'add'}
          <div class="form-group">
            <label for="dev_template">Profil Preset Tombol Awal:</label>
            <select id="dev_template" bind:value={templateId}>
              {#each Object.values(BUTTON_TEMPLATES) as tpl}
                <option value={tpl.id}>{tpl.name}</option>
              {/each}
            </select>
            <div class="form-hint">
              Fungsi tombol dapat dikustomisasi, ditambah, diedit, atau dinonaktifkan nanti pada menu Kelola Tombol.
            </div>
          </div>
        {/if}

        <div class="form-group">
          <label for="dev_desc">Catatan Lokasi & Deskripsi Tambahan:</label>
          <textarea
            id="dev_desc"
            bind:value={description}
            rows="2"
            placeholder="misal: Terpasang di panel listrik lantai 2 sisi barat..."
          ></textarea>
        </div>

        <div class="form-group smart-lock-box">
          <div class="checkbox-row">
            <input
              id="dev_allow_auto"
              type="checkbox"
              bind:checked={allow_automation_when_locked}
            />
            <label for="dev_allow_auto" class="smart-lock-label">
              [SMART LOCK]: Izinkan Jadwal Timer, Automasi, & Sensor Darurat saat Terkunci
            </label>
          </div>
          <div class="form-hint" style="margin-left: 24px;">
            Ketika node di-Lock, tombol manual diblokir untuk mencegah ketidaksengajaan. Namun timer jadwal, event kebocoran gas, atau pengalihan kran air otomatis tetap diizinkan berjalan demi keselamatan & keandalan sistem.
          </div>
        </div>

        <div class="modal-actions">
          <button type="button" onclick={onClose}>
            [BATAL]
          </button>
          <button type="submit" class="btn-success">
            {mode === 'add' ? '[+ DAFTARKAN DEVICE]' : '[SIMPAN PERUBAHAN]'}
          </button>
        </div>
      </form>
    </div>
  </div>
{/if}

<style>
  .btn-close {
    padding: 2px 6px;
    font-size: 10px;
    background: transparent;
    border: 1px solid var(--border-medium);
    color: var(--text-secondary);
  }
  .btn-close:hover {
    color: var(--text-primary);
    border-color: var(--border-heavy);
  }

  .error-banner {
    background-color: var(--status-alert-bg);
    color: var(--status-alert-text);
    border-bottom: 1px solid var(--status-alert-border);
    padding: 6px 12px;
    font-family: var(--font-mono);
    font-size: 11px;
    font-weight: 700;
  }

  .form-row-2 {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 12px;
  }

  .modal-actions {
    display: flex;
    justify-content: flex-end;
    gap: 8px;
    margin-top: 16px;
    padding-top: 12px;
    border-top: 1px solid var(--border-light);
  }

  .smart-lock-box {
    background-color: #151515;
    border: 1px solid var(--border-medium);
    padding: 10px;
    margin-top: 4px;
  }

  .checkbox-row {
    display: flex;
    align-items: center;
    gap: 8px;
  }

  .smart-lock-label {
    font-family: var(--font-mono);
    font-size: 11px;
    font-weight: 700;
    color: #4ade80;
    cursor: pointer;
  }

  @media (max-width: 600px) {
    .form-row-2 {
      grid-template-columns: 1fr;
    }
  }
</style>
