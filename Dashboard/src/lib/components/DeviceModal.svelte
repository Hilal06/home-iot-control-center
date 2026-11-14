<script>
  import { Dialog } from 'bits-ui';
  import { BUTTON_TEMPLATES } from '../deviceStore.js';
  import Lightbulb from '@lucide/svelte/icons/lightbulb';
  import Droplets from '@lucide/svelte/icons/droplets';
  import Flame from '@lucide/svelte/icons/flame';
  import Zap from '@lucide/svelte/icons/zap';
  import Fan from '@lucide/svelte/icons/fan';
  import Activity from '@lucide/svelte/icons/activity';
  import Cpu from '@lucide/svelte/icons/cpu';

  let {
    isOpen = $bindable(false),
    mode = 'add', // 'add' | 'edit'
    initialData = null,
    onSave = () => {},
    onClose = () => {},
  } = $props();

  let id = $state('');
  let name = $state('');
  let ip_address = $state('');
  let role = $state('Lighting Controller');
  let templateId = $state('LIGHTING_4ZONE');
  let description = $state('');
  let allow_automation_when_locked = $state(true);
  let errorMsg = $state('');

  function getRoleIcon(r = '') {
    const roleLower = (r || '').toLowerCase();
    if (roleLower.includes('light')) return Lightbulb;
    if (roleLower.includes('water') || roleLower.includes('pump')) return Droplets;
    if (roleLower.includes('gas') || roleLower.includes('hazard') || roleLower.includes('fire')) return Flame;
    if (roleLower.includes('hvac') || roleLower.includes('climate') || roleLower.includes('fan')) return Fan;
    if (roleLower.includes('power') || roleLower.includes('solar') || roleLower.includes('relay') || roleLower.includes('inverter')) return Zap;
    if (roleLower.includes('sensor') || roleLower.includes('telemetry')) return Activity;
    return Cpu;
  }

  let RoleIcon = $derived(getRoleIcon(role));

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

<Dialog.Root bind:open={isOpen} onOpenChange={(open) => { if (!open) onClose(); }}>
  <Dialog.Portal>
    <Dialog.Overlay class="modal-backdrop" />
    <Dialog.Content class="modal-window">
      <div class="panel-header modal-header-bar">
        <div class="modal-title-wrap">
          <div class="modal-icon-badge">
            <RoleIcon size={20} />
          </div>
          <div>
            <Dialog.Title class="modal-title">
              {mode === 'add' ? 'TAMBAH PERANGKAT BARU' : 'EDIT INFORMASI PERANGKAT'}
            </Dialog.Title>
            <div class="modal-subtitle">
              {mode === 'add' ? 'Daftarkan node ESP32 controller baru ke sistem' : `Modifikasi parameter ${id || 'node'}`}
            </div>
          </div>
        </div>
        <Dialog.Close class="btn-close">TUTUP / ESC</Dialog.Close>
      </div>
      <Dialog.Description class="sr-only">
        {mode === 'add' ? 'Formulir pendaftaran node ESP32 baru' : 'Formulir pengaturan informasi node ESP32'}
      </Dialog.Description>

      {#if errorMsg}
        <div class="error-banner">
          PERINGATAN: {errorMsg}
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
              SMART LOCK: Izinkan Jadwal Timer, Automasi, & Sensor Darurat saat Terkunci
            </label>
          </div>
          <div class="form-hint" style="margin-left: 24px;">
            Ketika node di-Lock, tombol manual diblokir untuk mencegah ketidaksengajaan. Namun timer jadwal, event kebocoran gas, atau pengalihan kran air otomatis tetap diizinkan berjalan demi keselamatan & keandalan sistem.
          </div>
        </div>

        <div class="modal-actions">
          <Dialog.Close class="btn-cancel">
            BATAL
          </Dialog.Close>
          <button type="submit" class="btn-success btn-submit">
            {mode === 'add' ? '+ DAFTARKAN PERANGKAT' : 'SIMPAN PERUBAHAN'}
          </button>
        </div>
      </form>
    </Dialog.Content>
  </Dialog.Portal>
</Dialog.Root>

<style>
  .modal-header-bar {
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding: 10px 14px;
    border-bottom: 1px solid var(--border-medium);
  }

  .modal-title-wrap {
    display: flex;
    align-items: center;
    gap: 12px;
  }

  .modal-icon-badge {
    width: 38px;
    height: 38px;
    display: flex;
    align-items: center;
    justify-content: center;
    background-color: var(--bg-hover);
    border: 1px solid var(--border-medium);
    border-radius: var(--radius-sm);
    color: var(--accent-amber);
    flex-shrink: 0;
  }

  :global(.modal-title) {
    font-size: 13px;
    font-weight: 700;
    color: var(--accent-amber);
    margin: 0;
    font-family: var(--font-mono);
    letter-spacing: 0.04em;
  }

  .modal-subtitle {
    font-size: 10px;
    color: var(--text-muted);
    font-family: var(--font-mono);
    margin-top: 2px;
  }

  :global(.btn-cancel) {
    padding: 8px 16px;
    font-size: 11px;
    font-weight: 700;
    background: transparent;
    border: 1px solid var(--border-medium);
    border-radius: var(--radius-sm);
    color: var(--text-secondary);
    cursor: pointer;
    font-family: var(--font-mono);
  }
  :global(.btn-cancel:hover) {
    color: var(--text-primary);
    border-color: var(--border-heavy);
  }

  :global(.btn-submit) {
    padding: 8px 20px;
    font-size: 11px;
    font-weight: 700;
  }

  :global(.sr-only) {
    position: absolute;
    width: 1px;
    height: 1px;
    padding: 0;
    margin: -1px;
    overflow: hidden;
    clip: rect(0, 0, 0, 0);
    white-space: nowrap;
    border-width: 0;
  }

  :global(.btn-close) {
    padding: 4px 10px;
    font-size: 10px;
    font-weight: 700;
    background: transparent;
    border: 1px solid var(--border-medium);
    border-radius: var(--radius-sm);
    color: var(--text-secondary);
  }
  :global(.btn-close:hover) {
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

  .panel-body {
    padding: 14px;
    overflow-y: auto;
    max-height: calc(90vh - 80px);
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
    background-color: var(--bg-hover);
    border: 1px solid var(--border-medium);
    border-radius: var(--radius-sm);
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
    color: #15803d;
    cursor: pointer;
  }

  @media (max-width: 600px) {
    .panel-body {
      padding: 14px;
      overflow-y: auto;
      max-height: calc(90vh - 80px);
    }

    .form-row-2 {
      grid-template-columns: 1fr;
    }
  }
</style>
