<script>
  import { onMount } from 'svelte';
  import { Tabs, Dialog } from 'bits-ui';
  import { deviceStore } from '../deviceStore.js';
  import { createDeviceApi, updateDeviceApi, deleteDeviceApi } from '../api.js';
  import DeviceModal from './DeviceModal.svelte';
  import DeviceButtonManager from './DeviceButtonManager.svelte';
  import DeviceIndicatorViewer from './DeviceIndicatorViewer.svelte';
  import Cpu from '@lucide/svelte/icons/cpu';
  import Lightbulb from '@lucide/svelte/icons/lightbulb';
  import Droplets from '@lucide/svelte/icons/droplets';
  import Flame from '@lucide/svelte/icons/flame';
  import Zap from '@lucide/svelte/icons/zap';
  import Fan from '@lucide/svelte/icons/fan';
  import Activity from '@lucide/svelte/icons/activity';
  import Lock from '@lucide/svelte/icons/lock';
  import Unlock from '@lucide/svelte/icons/unlock';
  import Sliders from '@lucide/svelte/icons/sliders';
  import Edit3 from '@lucide/svelte/icons/edit-3';
  import Trash2 from '@lucide/svelte/icons/trash-2';
  import Plus from '@lucide/svelte/icons/plus';
  import LayoutGrid from '@lucide/svelte/icons/layout-grid';
  import TableIcon from '@lucide/svelte/icons/table';
  import X from '@lucide/svelte/icons/x';

  let { telemetry = {}, onCommand = () => {} } = $props();

  let storeState = $state(deviceStore.getState());
  let selectedDeviceId = $state(deviceStore.getState().activeDeviceId || 'esp32_master');
  let searchQuery = $state('');
  let viewMode = $state('cards'); // 'cards' | 'table'
  let activeTab = $state('buttons'); // 'buttons' | 'indicators'
  let isConfigModalOpen = $state(false);

  // Modal states
  let isModalOpen = $state(false);
  let modalMode = $state('add'); // 'add' | 'edit'
  let editingDevice = $state(null);

  // In-app Delete Confirmation Modal
  let isDeleteModalOpen = $state(false);
  let deviceToDelete = $state(null);
  let deleteConfirmInput = $state('');
  let deleteError = $state('');

  // Toast / Status notification
  let notification = $state(null);
  let notificationTimer = null;

  function showNotification(msg, type = 'success') {
    clearTimeout(notificationTimer);
    notification = { msg, type };
    notificationTimer = setTimeout(() => {
      notification = null;
    }, 4000);
  }

  onMount(() => {
    const unsub = deviceStore.subscribe((s) => {
      storeState = s;
      if (!storeState.devices.some((d) => d.id === selectedDeviceId)) {
        selectedDeviceId = storeState.activeDeviceId || 'esp32_master';
      }
    });
    return () => {
      unsub();
      clearTimeout(notificationTimer);
    };
  });

  let selectedDevice = $derived(
    storeState.devices.find((d) => d.id === selectedDeviceId) ||
    storeState.devices[0] ||
    {}
  );

  let filteredDevices = $derived(
    storeState.devices.filter((d) => {
      if (!searchQuery) return true;
      const q = searchQuery.toLowerCase();
      return (
        d.id.toLowerCase().includes(q) ||
        (d.name && d.name.toLowerCase().includes(q)) ||
        (d.ip_address && d.ip_address.toLowerCase().includes(q)) ||
        (d.role && d.role.toLowerCase().includes(q))
      );
    })
  );

  let onlineCount = $derived(storeState.devices.filter((d) => d.status === 'ONLINE').length);
  let lockedCount = $derived(storeState.devices.filter((d) => d.is_locked).length);

  function getDeviceIcon(dev) {
    const text = `${dev?.role || ''} ${dev?.name || ''} ${dev?.id || ''}`.toLowerCase();
    if (text.includes('light') || text.includes('lampu') || text.includes('penerangan')) return Lightbulb;
    if (text.includes('water') || text.includes('air') || text.includes('pump') || text.includes('tank') || text.includes('tandon')) return Droplets;
    if (text.includes('gas') || text.includes('hazard') || text.includes('api') || text.includes('fire')) return Flame;
    if (text.includes('hvac') || text.includes('fan') || text.includes('kipas') || text.includes('climate') || text.includes('ac')) return Fan;
    if (text.includes('power') || text.includes('solar') || text.includes('relay') || text.includes('inverter') || text.includes('listrik')) return Zap;
    if (text.includes('sensor') || text.includes('telemetry') || text.includes('monitor')) return Activity;
    return Cpu;
  }

  function getDeviceColor(dev) {
    const text = `${dev?.role || ''} ${dev?.name || ''} ${dev?.id || ''}`.toLowerCase();
    if (text.includes('light') || text.includes('lampu') || text.includes('penerangan')) return '#d97706';
    if (text.includes('water') || text.includes('air') || text.includes('pump') || text.includes('tank') || text.includes('tandon')) return '#0284c7';
    if (text.includes('gas') || text.includes('hazard') || text.includes('api') || text.includes('fire')) return '#dc2626';
    if (text.includes('hvac') || text.includes('fan') || text.includes('kipas') || text.includes('climate') || text.includes('ac')) return '#0d9488';
    if (text.includes('power') || text.includes('solar') || text.includes('relay') || text.includes('inverter') || text.includes('listrik')) return '#ca8a04';
    if (text.includes('sensor') || text.includes('telemetry') || text.includes('monitor')) return '#16a34a';
    return '#4f46e5';
  }

  function handleOpenAdd() {
    modalMode = 'add';
    editingDevice = null;
    isModalOpen = true;
  }

  function handleOpenEdit(dev) {
    if (dev.is_locked) {
      showNotification(`Perangkat '${dev.name}' sedang TERKUNCI. Buka kunci untuk mengedit.`, 'warning');
      return;
    }
    modalMode = 'edit';
    editingDevice = dev;
    isModalOpen = true;
  }

  function handleOpenConfig(dev) {
    selectedDeviceId = dev.id;
    deviceStore.setActiveDevice(dev.id);
    isConfigModalOpen = true;
  }

  async function handleSaveDevice(data) {
    if (modalMode === 'add') {
      const created = deviceStore.addDevice(data);
      selectedDeviceId = created.id;
      await createDeviceApi(created);
      showNotification(`Perangkat baru [${created.name}] berhasil ditambahkan.`);
    } else if (modalMode === 'edit' && editingDevice) {
      deviceStore.updateDevice(editingDevice.id, data);
      await updateDeviceApi(editingDevice.id, data);
      showNotification(`Informasi [${editingDevice.name}] berhasil diperbarui.`);
    }
  }

  function handleToggleLock(dev) {
    const newLockState = deviceStore.toggleLock(dev.id);
    if (newLockState) {
      showNotification(`Perangkat [${dev.name}] TERKUNCI. Konfigurasi diproteksi.`, 'warning');
    } else {
      showNotification(`Perangkat [${dev.name}] DIBUKA. Konfigurasi dapat diubah.`);
    }
  }

  function handlePromptDelete(dev) {
    if (dev.is_locked) {
      showNotification(`Perangkat [${dev.name}] TERKUNCI dan tidak dapat dihapus. Buka kunci terlebih dahulu.`, 'warning');
      return;
    }
    if (dev.id === 'esp32_master') {
      showNotification(`Master Gateway adalah pengontrol utama dan tidak dapat dihapus.`, 'alert');
      return;
    }

    deviceToDelete = dev;
    deleteConfirmInput = '';
    deleteError = '';
    isDeleteModalOpen = true;
  }

  async function handleConfirmDelete() {
    if (!deviceToDelete) return;
    if (deleteConfirmInput.trim() !== deviceToDelete.id) {
      deleteError = `Ketik persis '${deviceToDelete.id}' untuk konfirmasi.`;
      return;
    }

    try {
      const id = deviceToDelete.id;
      deviceStore.deleteDevice(id);
      await deleteDeviceApi(id);
      isDeleteModalOpen = false;
      showNotification(`Perangkat [${deviceToDelete.name}] berhasil dihapus.`);
    } catch (err) {
      deleteError = err.message;
    }
  }
</script>

<div class="settings-container">
  <!-- Status Banner / Notification -->
  {#if notification}
    <div class="notification-banner {notification.type === 'warning' ? 'banner-warning' : notification.type === 'alert' ? 'banner-alert' : 'banner-success'}">
      <span>{notification.msg}</span>
      <button onclick={() => (notification = null)} class="btn-sm">[TUTUP]</button>
    </div>
  {/if}

  <!-- 1. SUMMARY STATS & ACTION BAR -->
  <div class="panel">
    <div class="panel-header">
      <div class="title-with-bar">
        <span class="amber-bar"></span>
        <h2>PERANGKAT PINTAR RUMAH</h2>
        <span class="badge amber active-counter">{onlineCount} / {storeState.devices.length} Terhubung</span>
      </div>

      <div class="header-actions-group">
        <!-- View Mode Switcher: Cards vs Table -->
        <div class="view-switch-btns">
          <button
            type="button"
            class="btn-view {viewMode === 'cards' ? 'active' : ''}"
            onclick={() => (viewMode = 'cards')}
            title="Tampilan Kartu"
          >
            <LayoutGrid size={13} />
          </button>
          <button
            type="button"
            class="btn-view {viewMode === 'table' ? 'active' : ''}"
            onclick={() => (viewMode = 'table')}
            title="Tampilan Tabel"
          >
            <TableIcon size={13} />
          </button>
        </div>

        <button class="btn-amber flex-center" onclick={handleOpenAdd}>
          <Plus size={13} />
          <span>TAMBAH PERANGKAT</span>
        </button>
      </div>
    </div>

    <!-- Search and Filter Bar -->
    <div class="filter-bar">
      <div class="search-input-wrapper">
        <input
          type="text"
          bind:value={searchQuery}
          placeholder="Cari perangkat berdasarkan Nama, Lokasi, atau IP..."
        />
      </div>
      {#if searchQuery}
        <button onclick={() => (searchQuery = '')} class="btn-clear">BERSIHKAN FILTER</button>
      {/if}
    </div>

    <!-- 2. MAIN VIEW: PRELINE DEVICE CARDS GRID (DEFAULT) -->
    {#if viewMode === 'cards'}
      <div class="device-cards-grid">
        {#each filteredDevices as dev}
          {@const isOnline = dev.status === 'ONLINE'}
          {@const DevIcon = getDeviceIcon(dev)}
          <div class="device-card {isOnline ? 'card-online' : 'card-offline'}">
            <div class="dev-card-top">
              <div class="dev-card-header-left">
                <div class="dev-icon-wrap" style="color: {getDeviceColor(dev)};">
                  <DevIcon size={20} />
                </div>
                <div>
                  <h4 class="dev-card-name">{dev.name}</h4>
                  <span class="dev-card-role">{dev.role || 'Kontroler Ruangan'} &bull; ID: {dev.id}</span>
                </div>
              </div>
              <span class="badge {isOnline ? 'active' : 'inactive'}">
                {isOnline ? 'TERHUBUNG' : 'MATI'}
              </span>
            </div>

            <div class="dev-card-meta">
              <div class="meta-row">
                <span class="meta-lbl">Alamat IP:</span>
                <span class="meta-val mono">{dev.ip_address || '192.168.100.x'}</span>
              </div>
              <div class="meta-row">
                <span class="meta-lbl">Jumlah Saklar:</span>
                <span class="meta-val">{dev.buttons?.length || 0} Tombol Kontrol</span>
              </div>
              <div class="meta-row">
                <span class="meta-lbl">Status Proteksi:</span>
                <span class="badge {dev.is_locked ? 'warning' : 'inactive'}">
                  {dev.is_locked ? '🔒 TERKUNCI' : '🔓 BEBAS'}
                </span>
              </div>
            </div>

            <div class="dev-card-actions">
              <button
                type="button"
                class="btn-dev-action btn-config"
                onclick={() => handleOpenConfig(dev)}
                title="Kelola fungsi tombol dan sensor node ini"
              >
                <Sliders size={14} />
                <span>Pengaturan</span>
              </button>

              <button
                type="button"
                class="btn-dev-action {dev.is_locked ? 'btn-unlock' : 'btn-lock'}"
                onclick={() => handleToggleLock(dev)}
                title={dev.is_locked ? 'Buka kunci proteksi' : 'Kunci konfigurasi'}
              >
                {#if dev.is_locked}
                  <Unlock size={14} />
                  <span>Buka</span>
                {:else}
                  <Lock size={14} />
                  <span>Kunci</span>
                {/if}
              </button>

              <button
                type="button"
                class="btn-dev-action btn-icon-only"
                disabled={dev.is_locked}
                onclick={() => handleOpenEdit(dev)}
                title="Edit data perangkat"
              >
                <Edit3 size={14} />
              </button>

              {#if dev.id !== 'esp32_master'}
                <button
                  type="button"
                  class="btn-dev-action btn-icon-only btn-danger-action"
                  disabled={dev.is_locked}
                  onclick={() => handlePromptDelete(dev)}
                  title="Hapus perangkat"
                >
                  <Trash2 size={14} />
                </button>
              {/if}
            </div>
          </div>
        {/each}
      </div>
    {:else}
      <!-- ALTERNATIVE VIEW: CLEAN TABLE -->
      <div class="table-wrapper">
        <table>
          <thead>
            <tr>
              <th style="width: 90px;">STATUS</th>
              <th style="width: 100px;">PROTEKSI</th>
              <th>NAMA & ID PERANGKAT</th>
              <th>IP ADDRESS</th>
              <th>LOKASI / ROLE</th>
              <th>TOMBOL</th>
              <th style="text-align: right;">AKSI</th>
            </tr>
          </thead>
          <tbody>
            {#each filteredDevices as dev}
              {@const TableDevIcon = getDeviceIcon(dev)}
              <tr>
                <td>
                  <span class="badge {dev.status === 'ONLINE' ? 'active' : 'inactive'}">
                    {dev.status === 'ONLINE' ? 'TERHUBUNG' : 'MATI'}
                  </span>
                </td>
                <td>
                  <span class="badge {dev.is_locked ? 'warning' : 'inactive'}">
                    {dev.is_locked ? 'TERKUNCI' : 'BEBAS'}
                  </span>
                </td>
                <td>
                  <div class="table-dev-cell">
                    <div class="table-dev-icon" style="color: {getDeviceColor(dev)};">
                      <TableDevIcon size={16} />
                    </div>
                    <div>
                      <strong class="dev-name">{dev.name}</strong>
                      <div class="dev-id">ID: {dev.id}</div>
                    </div>
                  </div>
                </td>
                <td><span class="mono">{dev.ip_address || '192.168.100.x'}</span></td>
                <td><span class="dev-role">{dev.role || 'Edge Node'}</span></td>
                <td><span class="badge">{dev.buttons?.length || 0} TOMBOL</span></td>
                <td style="text-align: right;">
                  <div class="action-buttons-inline">
                    <button class="btn-action-md btn-config-sm" onclick={() => handleOpenConfig(dev)}>
                      PENGATURAN
                    </button>
                    <button
                      class="btn-action-md {dev.is_locked ? 'btn-warning' : ''}"
                      onclick={() => handleToggleLock(dev)}
                    >
                      {dev.is_locked ? 'BUKA' : 'KUNCI'}
                    </button>
                    <button
                      class="btn-action-md"
                      disabled={dev.is_locked}
                      onclick={() => handleOpenEdit(dev)}
                    >
                      EDIT
                    </button>
                    {#if dev.id !== 'esp32_master'}
                      <button
                        class="btn-action-md btn-danger"
                        disabled={dev.is_locked}
                        onclick={() => handlePromptDelete(dev)}
                      >
                        HAPUS
                      </button>
                    {/if}
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

<!-- Modal Form Add/Edit -->
<DeviceModal
  bind:isOpen={isModalOpen}
  mode={modalMode}
  initialData={editingDevice}
  onSave={handleSaveDevice}
  onClose={() => (isModalOpen = false)}
/>

<!-- Modal Developer Config (TOMBOL & INDIKATOR REGISTRY) -->
<Dialog.Root bind:open={isConfigModalOpen}>
  <Dialog.Portal>
    <Dialog.Overlay class="modal-backdrop" />
    <Dialog.Content class="modal-window modal-wide">
      <div class="panel-header modal-cfg-header">
        <div class="modal-cfg-title-wrap">
          <div class="modal-cfg-icon">
            <Sliders size={18} />
          </div>
          <div>
            <Dialog.Title class="modal-title">
              PENGATURAN TEKNIS: {selectedDevice.name}
            </Dialog.Title>
            <div class="modal-sub-meta mono">
              <span>NODE_ID: {selectedDevice.id}</span>
              <span class="meta-dot">&bull;</span>
              <span>HOST: {selectedDevice.ip_address || '192.168.100.x'}</span>
              <span class="meta-dot">&bull;</span>
              <span class="badge-cfg-status {selectedDevice.is_locked ? 'locked' : 'unlocked'}">
                {selectedDevice.is_locked ? 'TERKUNCI' : 'BEBAS'}
              </span>
            </div>
          </div>
        </div>
        <Dialog.Close class="modal-cfg-close-btn" aria-label="Tutup Dialog">
          <X size={15} />
          <span>TUTUP / ESC</span>
        </Dialog.Close>
      </div>

      <div class="modal-config-tabs">
        <button
          type="button"
          class="modal-tab-btn {activeTab === 'buttons' ? 'tab-active' : ''}"
          onclick={() => (activeTab = 'buttons')}
        >
          <span class="tab-idx">01.</span>
          <span class="tab-label">KELOLA TOMBOL & KONTROL</span>
          <span class="tab-count-pill">{selectedDevice.buttons?.length || 0}</span>
        </button>
        <button
          type="button"
          class="modal-tab-btn {activeTab === 'indicators' ? 'tab-active' : ''}"
          onclick={() => (activeTab = 'indicators')}
        >
          <span class="tab-idx">02.</span>
          <span class="tab-label">INDIKATOR & SENSOR</span>
        </button>
        <button
          type="button"
          class="modal-tab-btn {activeTab === 'info' ? 'tab-active' : ''}"
          onclick={() => (activeTab = 'info')}
        >
          <span class="tab-idx">03.</span>
          <span class="tab-label">INFORMASI & PROTEKSI</span>
        </button>
      </div>

      <div class="modal-config-body">
        {#if activeTab === 'buttons'}
          <DeviceButtonManager device={selectedDevice} {onCommand} />
        {:else if activeTab === 'indicators'}
          <DeviceIndicatorViewer device={selectedDevice} {telemetry} />
        {:else}
          <!-- Tab 3: Device Info & Smart Lock Protection -->
          <div class="panel-body">
            <div class="info-section">
              <h3 class="section-title">DETAIL PERANGKAT KERAS (HARDWARE PROFILE)</h3>
              <div class="data-grid-2col">
                <div class="info-item">
                  <span class="info-lbl">Identifier Unik (ID):</span>
                  <span class="info-val mono">{selectedDevice.id}</span>
                </div>
                <div class="info-item">
                  <span class="info-lbl">Nama Perangkat:</span>
                  <span class="info-val">{selectedDevice.name}</span>
                </div>
                <div class="info-item">
                  <span class="info-lbl">Alamat IP / Host:</span>
                  <span class="info-val mono">{selectedDevice.ip_address || '192.168.100.x'}</span>
                </div>
                <div class="info-item">
                  <span class="info-lbl">Peran / Kategori:</span>
                  <span class="info-val">{selectedDevice.role || 'Edge Node'}</span>
                </div>
                <div class="info-item">
                  <span class="info-lbl">Status Koneksi:</span>
                  <span class="badge {selectedDevice.status === 'ONLINE' ? 'active' : 'inactive'}">
                    {selectedDevice.status === 'ONLINE' ? 'TERHUBUNG' : 'MATI'}
                  </span>
                </div>
                <div class="info-item">
                  <span class="info-lbl">Terdaftar Sejak:</span>
                  <span class="info-val">{selectedDevice.created_at ? new Date(selectedDevice.created_at).toLocaleString('id-ID') : '--'}</span>
                </div>
              </div>

              <div class="info-item full-width" style="margin-top: 10px;">
                <span class="info-lbl">Catatan Lokasi Fisik:</span>
                <p class="desc-text">{selectedDevice.description || 'Tidak ada catatan lokasi fisik terdaftar.'}</p>
              </div>
            </div>

            <!-- Smart Lock & Modification Section -->
            <div class="protection-section">
              <h3 class="section-title">STATUS PROTEKSI SMART LOCK</h3>
              <div class="protection-box {selectedDevice.is_locked ? 'box-locked' : 'box-unlocked'}">
                <div class="prot-desc">
                  <span class="badge {selectedDevice.is_locked ? 'warning' : 'active'}">
                    {selectedDevice.is_locked ? '🔒 PERANGKAT TERKUNCI' : '🔓 PERANGKAT BEBAS'}
                  </span>
                  <p class="prot-text">
                    {#if selectedDevice.is_locked}
                      Konfigurasi dan pemetaan tombol perangkat ini diproteksi. Tombol manual tidak dapat diubah atau dihapus, namun jadwal otomatis dan sensor darurat tetap beroperasi normal.
                    {:else}
                      Konfigurasi dalam mode bebas (Unprotected). Anda dapat mengedit saluran pin relay, nama, dan mapping perintah MQTT kapan saja.
                    {/if}
                  </p>
                </div>

                <div class="prot-actions">
                  <button
                    type="button"
                    class="btn-dev-action {selectedDevice.is_locked ? 'btn-unlock' : 'btn-lock'}"
                    onclick={() => handleToggleLock(selectedDevice)}
                  >
                    {#if selectedDevice.is_locked}
                      <Unlock size={14} />
                      <span>BUKA KUNCI PENGATURAN</span>
                    {:else}
                      <Lock size={14} />
                      <span>KUNCI KONFIGURASI</span>
                    {/if}
                  </button>

                  <button
                    type="button"
                    class="btn-dev-action"
                    disabled={selectedDevice.is_locked}
                    onclick={() => {
                      isConfigModalOpen = false;
                      handleOpenEdit(selectedDevice);
                    }}
                  >
                    <Edit3 size={14} />
                    <span>EDIT INFORMASI NODE</span>
                  </button>
                </div>
              </div>
            </div>
          </div>
        {/if}
      </div>
    </Dialog.Content>
  </Dialog.Portal>
</Dialog.Root>

<!-- In-app Delete Confirmation Modal (BITS-UI DIALOG) -->
<Dialog.Root bind:open={isDeleteModalOpen}>
  <Dialog.Portal>
    <Dialog.Overlay class="modal-backdrop" />
    <Dialog.Content class="modal-window">
      <div class="panel-header">
        <Dialog.Title class="modal-title-danger">
          KONFIRMASI HAPUS PERANGKAT
        </Dialog.Title>
        <Dialog.Close class="btn-sm">TUTUP / ESC</Dialog.Close>
      </div>

      {#if deleteError}
        <div class="notification-banner banner-alert">
          PERINGATAN: {deleteError}
        </div>
      {/if}

      {#if deviceToDelete}
        <div class="panel-body">
          <p style="margin-bottom: 12px; color: var(--text-secondary); line-height: 1.5; font-size: 11px;">
            Anda akan menghapus perangkat <strong>{deviceToDelete.name}</strong> (ID: <strong style="color: var(--accent-amber);">{deviceToDelete.id}</strong>).<br />
            Semua konfigurasi tombol dan perintah terkait akan dihapus secara permanen.
          </p>

          <div class="form-group">
            <label for="delete_confirm_input">Ketik ID perangkat untuk konfirmasi:</label>
            <input
              id="delete_confirm_input"
              type="text"
              bind:value={deleteConfirmInput}
              placeholder={deviceToDelete.id}
            />
          </div>

          <div class="modal-actions">
            <Dialog.Close class="btn-cancel">
              BATAL
            </Dialog.Close>
            <button
              type="button"
              class="btn-danger"
              disabled={deleteConfirmInput.trim() !== deviceToDelete.id}
              onclick={handleConfirmDelete}
            >
              HAPUS PERANGKAT
            </button>
          </div>
        </div>
      {/if}
    </Dialog.Content>
  </Dialog.Portal>
</Dialog.Root>

<style>
  .settings-container {
    display: flex;
    flex-direction: column;
    gap: 16px;
    font-family: var(--font-mono);
  }

  .notification-banner {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 8px 12px;
    font-size: 11px;
    font-weight: 700;
    border-radius: var(--radius-sm);
  }

  .banner-success {
    background-color: var(--status-active-bg);
    color: var(--status-active-text);
    border: 1px solid var(--status-active-border);
  }

  .banner-warning {
    background-color: var(--status-warning-bg);
    color: var(--status-warning-text);
    border: 1px solid var(--status-warning-border);
  }

  .banner-alert {
    background-color: var(--status-alert-bg);
    color: var(--status-alert-text);
    border: 1px solid var(--status-alert-border);
  }

  .title-with-bar {
    display: flex;
    align-items: center;
    gap: 8px;
  }

  .amber-bar {
    width: 3px;
    height: 14px;
    background-color: var(--accent-amber);
    border-radius: var(--radius-sm);
  }

  .active-counter {
    font-size: 10px;
  }

  .header-actions-group {
    display: flex;
    align-items: center;
    gap: 10px;
  }

  .view-switch-btns {
    display: inline-flex;
    border: 1px solid var(--border-medium);
    border-radius: var(--radius-sm);
    overflow: hidden;
  }

  .btn-view {
    padding: 4px 8px;
    background-color: var(--bg-card);
    border: none;
    color: var(--text-muted);
    cursor: pointer;
    display: inline-flex;
    align-items: center;
  }

  .btn-view.active {
    background-color: var(--bg-hover);
    color: var(--accent-amber);
  }

  .flex-center {
    display: inline-flex;
    align-items: center;
    gap: 6px;
  }

  .filter-bar {
    padding: 8px 12px;
    background-color: var(--bg-panel-header);
    border-bottom: 1px solid var(--border-medium);
    display: flex;
    gap: 8px;
    align-items: center;
  }

  .search-input-wrapper {
    flex: 1;
  }

  .btn-clear {
    padding: 5px 8px;
    font-size: 10px;
    border-radius: var(--radius-sm);
  }

  /* Preline Device Cards Grid */
  .device-cards-grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(280px, 1fr));
    gap: 12px;
    padding: 14px;
  }

  .device-card {
    display: flex;
    flex-direction: column;
    justify-content: space-between;
    gap: 12px;
    background-color: var(--bg-card);
    border: 1px solid var(--border-medium);
    border-radius: var(--radius-sm);
    padding: 12px;
    transition: all 0.15s ease;
  }

  .device-card:hover {
    border-color: var(--border-heavy);
  }

  .dev-card-top {
    display: flex;
    justify-content: space-between;
    align-items: flex-start;
    gap: 8px;
  }

  .dev-card-header-left {
    display: flex;
    align-items: center;
    gap: 10px;
  }

  .dev-icon-wrap {
    width: 38px;
    height: 38px;
    display: inline-flex;
    align-items: center;
    justify-content: center;
    border: 1px solid var(--border-medium);
    border-radius: var(--radius-sm);
    background-color: var(--bg-hover);
    flex-shrink: 0;
  }

  .dev-card-name {
    margin: 0;
    font-size: 12px;
    font-weight: 700;
    color: var(--text-primary);
  }

  .dev-card-role {
    font-size: 10px;
    color: var(--text-muted);
  }

  .dev-card-meta {
    display: flex;
    flex-direction: column;
    gap: 4px;
    background-color: var(--bg-panel-header);
    padding: 8px 10px;
    border: 1px solid var(--border-medium);
    border-radius: var(--radius-sm);
    font-size: 10px;
  }

  .meta-row {
    display: flex;
    justify-content: space-between;
    align-items: center;
  }

  .meta-lbl {
    color: var(--text-muted);
  }

  .meta-val {
    font-weight: 600;
    color: var(--text-primary);
  }

  .mono {
    font-family: var(--font-mono);
  }

  .dev-card-actions {
    display: flex;
    gap: 6px;
    align-items: center;
  }

  .btn-dev-action {
    display: inline-flex;
    align-items: center;
    justify-content: center;
    gap: 6px;
    padding: 7px 12px;
    font-family: var(--font-mono);
    font-size: 11px;
    font-weight: 700;
    background-color: var(--bg-panel-header);
    border: 1px solid var(--border-medium);
    border-radius: var(--radius-sm);
    color: var(--text-secondary);
    cursor: pointer;
  }

  .btn-dev-action:hover:not(:disabled) {
    background-color: var(--bg-hover);
    color: var(--text-primary);
    border-color: var(--border-heavy);
  }

  .btn-dev-action:disabled {
    opacity: 0.5;
    cursor: not-allowed;
  }

  .btn-config {
    flex: 1;
    background-color: #fffbeb;
    color: #d97706;
    border-color: var(--accent-amber);
  }

  .btn-icon-only {
    padding: 7px 9px;
  }

  .btn-danger-action:hover:not(:disabled) {
    color: #dc2626;
    border-color: #fca5a5;
    background-color: #fee2e2;
  }

  /* Table View */
  .table-wrapper {
    overflow-x: auto;
  }

  .table-dev-cell {
    display: flex;
    align-items: center;
    gap: 8px;
  }

  .table-dev-icon {
    width: 28px;
    height: 28px;
    display: inline-flex;
    align-items: center;
    justify-content: center;
    background-color: var(--bg-hover);
    border: 1px solid var(--border-medium);
    border-radius: var(--radius-sm);
    flex-shrink: 0;
  }

  .dev-name {
    color: var(--text-primary);
    font-size: 12px;
  }

  .dev-id {
    font-size: 10px;
    color: var(--text-muted);
  }

  .dev-role {
    font-size: 10px;
    color: var(--text-secondary);
  }

  .action-buttons-inline {
    display: flex;
    justify-content: flex-end;
    gap: 6px;
    flex-wrap: wrap;
  }

  .btn-action-md {
    display: inline-flex;
    align-items: center;
    justify-content: center;
    padding: 5px 10px;
    font-family: var(--font-mono);
    font-size: 11px;
    font-weight: 700;
    letter-spacing: 0.03em;
    background-color: #ffffff;
    border: 1px solid var(--border-medium);
    border-radius: var(--radius-sm);
    color: var(--text-primary);
    cursor: pointer;
    transition: background-color 150ms ease, border-color 150ms ease;
  }
  .btn-action-md:hover:not(:disabled) {
    background-color: var(--bg-hover);
    border-color: var(--border-heavy);
  }
  .btn-action-md:disabled {
    opacity: 0.5;
    cursor: not-allowed;
  }

  .btn-config-sm {
    background-color: #fffbeb;
    color: #d97706;
    border-color: var(--accent-amber);
  }

  .btn-sm {
    padding: 3px 6px;
    font-size: 10px;
    border-radius: var(--radius-sm);
  }

  /* Modal Technical Configuration */
  :global(.modal-wide) {
    max-width: 960px !important;
  }

  .modal-cfg-header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding: 12px 16px;
    background-color: var(--bg-panel-header);
    border-bottom: 1px solid var(--border-medium);
  }

  .modal-cfg-title-wrap {
    display: flex;
    align-items: center;
    gap: 12px;
  }

  .modal-cfg-icon {
    width: 36px;
    height: 36px;
    display: flex;
    align-items: center;
    justify-content: center;
    background-color: #fffbeb;
    border: 1px solid var(--accent-amber);
    color: var(--accent-amber);
    border-radius: 0;
    flex-shrink: 0;
  }

  .modal-sub-meta {
    display: flex;
    align-items: center;
    gap: 8px;
    font-size: 11px;
    color: var(--text-secondary);
    margin-top: 2px;
  }

  .meta-dot {
    color: var(--border-heavy);
  }

  .badge-cfg-status {
    padding: 1px 6px;
    font-size: 10px;
    font-weight: 700;
    border-radius: 0;
  }
  .badge-cfg-status.locked {
    background-color: #fef3c7;
    color: #b45309;
    border: 1px solid #fde68a;
  }
  .badge-cfg-status.unlocked {
    background-color: #dcfce7;
    color: #15803d;
    border: 1px solid #86efac;
  }

  :global(.modal-cfg-close-btn) {
    display: inline-flex;
    align-items: center;
    gap: 6px;
    height: 32px;
    padding: 0 12px;
    background-color: #ffffff;
    border: 1px solid var(--border-medium);
    border-radius: 0;
    font-family: var(--font-mono);
    font-size: 11px;
    font-weight: 700;
    color: var(--text-secondary);
    cursor: pointer;
    transition: all 0.15s ease;
  }
  :global(.modal-cfg-close-btn:hover) {
    background-color: var(--bg-hover);
    color: var(--text-primary);
    border-color: var(--border-heavy);
  }

  .modal-config-tabs {
    display: flex;
    gap: 4px;
    padding: 8px 16px 0 16px;
    background-color: var(--bg-panel-header);
    border-bottom: 1px solid var(--border-medium);
  }

  .modal-tab-btn {
    display: inline-flex;
    align-items: center;
    gap: 8px;
    padding: 9px 16px;
    background: transparent;
    border: 1px solid transparent;
    border-bottom: none;
    border-radius: 0;
    font-family: var(--font-mono);
    font-size: 11px;
    font-weight: 700;
    color: var(--text-secondary);
    cursor: pointer;
    letter-spacing: 0.03em;
    transition: all 0.15s ease;
  }

  .tab-idx {
    color: var(--accent-amber);
  }

  .tab-count-pill {
    padding: 1px 6px;
    background-color: var(--bg-hover);
    border: 1px solid var(--border-medium);
    border-radius: 0;
    font-size: 10px;
    color: var(--text-primary);
  }

  .modal-tab-btn.tab-active {
    background-color: #ffffff;
    border-color: var(--border-medium);
    border-top: 2px solid var(--accent-amber);
    color: var(--text-primary);
  }
  .modal-tab-btn.tab-active .tab-count-pill {
    background-color: #fffbeb;
    border-color: var(--accent-amber);
    color: var(--accent-amber);
  }

  .modal-config-body {
    padding: 16px;
    max-height: 72vh;
    overflow-y: auto;
  }

  .panel-body {
    padding: 14px;
  }

  .modal-actions {
    display: flex;
    justify-content: flex-end;
    gap: 8px;
    margin-top: 14px;
  }

  /* Tab 3 Info & Protection Styles */
  .info-section, .protection-section {
    margin-bottom: 16px;
  }

  .section-title {
    font-size: 11px;
    font-weight: 700;
    color: var(--text-secondary);
    letter-spacing: 0.05em;
    margin-bottom: 8px;
    padding-bottom: 4px;
    border-bottom: 1px solid var(--border-light);
  }

  .data-grid-2col {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 10px;
  }

  .info-item {
    display: flex;
    flex-direction: column;
    gap: 3px;
    background: var(--bg-hover);
    border: 1px solid var(--border-medium);
    border-radius: var(--radius-sm);
    padding: 8px 10px;
  }

  .info-item.full-width {
    grid-column: 1 / -1;
  }

  .info-lbl {
    font-size: 10px;
    color: var(--text-muted);
  }

  .info-val {
    font-size: 12px;
    color: var(--text-primary);
    font-weight: 600;
  }

  .desc-text {
    font-size: 11px;
    color: var(--text-secondary);
    line-height: 1.4;
    margin: 0;
  }

  .protection-box {
    border: 1px solid var(--border-medium);
    border-radius: var(--radius-sm);
    padding: 12px;
    display: flex;
    flex-direction: column;
    gap: 10px;
  }

  .box-locked {
    background-color: var(--status-warning-bg);
    border-color: var(--status-warning-border);
  }

  .box-unlocked {
    background-color: var(--status-active-bg);
    border-color: var(--status-active-border);
  }

  .prot-text {
    font-size: 11px;
    color: var(--text-secondary);
    line-height: 1.4;
    margin: 6px 0 0 0;
  }

  .prot-actions {
    display: flex;
    gap: 8px;
    margin-top: 4px;
  }

  @media (max-width: 600px) {
    .data-grid-2col {
      grid-template-columns: 1fr;
    }
  }
</style>
