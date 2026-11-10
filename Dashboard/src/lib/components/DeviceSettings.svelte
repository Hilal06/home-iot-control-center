<script>
  import { onMount } from 'svelte';
  import { deviceStore } from '../deviceStore.js';
  import { createDeviceApi, updateDeviceApi, deleteDeviceApi } from '../api.js';
  import DeviceModal from './DeviceModal.svelte';
  import DeviceButtonManager from './DeviceButtonManager.svelte';
  import DeviceIndicatorViewer from './DeviceIndicatorViewer.svelte';

  let { telemetry = {}, onCommand = () => {} } = $props();

  let storeState = $state(deviceStore.getState());
  let selectedDeviceId = $state(deviceStore.getState().activeDeviceId || 'esp32_master');
  let searchQuery = $state('');
  let activeTab = $state(window.location.hash.includes('indicators') ? 'indicators' : 'buttons'); // 'buttons' | 'indicators'

  function setTab(tab) {
    activeTab = tab;
    window.location.hash = `#settings/${tab}`;
  }

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

  function handleOpenAdd() {
    modalMode = 'add';
    editingDevice = null;
    isModalOpen = true;
  }

  function handleOpenEdit(dev) {
    if (dev.is_locked) {
      showNotification(`Device '${dev.name}' sedang TERKUNCI. Buka lock untuk mengedit.`, 'warning');
      return;
    }
    modalMode = 'edit';
    editingDevice = dev;
    isModalOpen = true;
  }

  async function handleSaveDevice(data) {
    if (modalMode === 'add') {
      const created = deviceStore.addDevice(data);
      selectedDeviceId = created.id;
      await createDeviceApi(created);
      showNotification(`Device baru [${created.id}] berhasil ditambahkan ke sistem.`);
    } else if (modalMode === 'edit' && editingDevice) {
      deviceStore.updateDevice(editingDevice.id, data);
      await updateDeviceApi(editingDevice.id, data);
      showNotification(`Informasi device [${editingDevice.id}] berhasil diperbarui.`);
    }
  }

  function handleToggleLock(dev) {
    const newLockState = deviceStore.toggleLock(dev.id);
    if (newLockState) {
      showNotification(`Device [${dev.id}] TERKUNCI (LOCKED). Konfigurasi diproteksi.`, 'warning');
    } else {
      showNotification(`Device [${dev.id}] DIBUKA (UNLOCKED). Konfigurasi dapat diubah.`);
    }
  }

  function handlePromptDelete(dev) {
    if (dev.is_locked) {
      showNotification(`Device [${dev.id}] TERKUNCI dan tidak dapat dihapus. Buka lock terlebih dahulu.`, 'warning');
      return;
    }
    if (dev.id === 'esp32_master') {
      showNotification(`ESP32 Master Node adalah gateway utama dan tidak dapat dihapus.`, 'alert');
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
      showNotification(`Device [${id}] berhasil dihapus dari sistem.`);
    } catch (err) {
      deleteError = err.message;
    }
  }

  function handleSelectDevice(id) {
    selectedDeviceId = id;
    deviceStore.setActiveDevice(id);
  }
</script>

<div class="settings-container">
  <!-- Status Banner / Notification -->
  {#if notification}
    <div class="notification-banner {notification.type === 'warning' ? 'banner-warning' : notification.type === 'alert' ? 'banner-alert' : 'banner-success'}">
      <span>[SYSTEM NOTIFICATION]: {notification.msg}</span>
      <button onclick={() => (notification = null)} class="btn-sm">[TUTUP]</button>
    </div>
  {/if}

  <!-- 1. SUMMARY STATS & ACTION BAR -->
  <div class="panel">
    <div class="panel-header">
      <div class="title-with-bar">
        <span class="amber-bar"></span>
        <h2>MANAJEMEN PERANGKAT (DEVICES)</h2>
      </div>
      <div class="top-stats">
        <span class="stat-badge">TOTAL: <strong>{storeState.devices.length}</strong></span>
        <span class="stat-badge">ONLINE: <strong class="color-active">{onlineCount}</strong></span>
        <span class="stat-badge">LOCKED: <strong class="color-warning">{lockedCount}</strong></span>
        <button class="btn-amber" onclick={handleOpenAdd}>
          + TAMBAH PERANGKAT
        </button>
      </div>
    </div>

    <!-- Search and Filter Bar -->
    <div class="filter-bar">
      <div class="search-input-wrapper">
        <input
          type="text"
          bind:value={searchQuery}
          placeholder="Filter berdasarkan Nama, ID, IP, atau Role..."
        />
      </div>
      {#if searchQuery}
        <button onclick={() => (searchQuery = '')} class="btn-clear">[BERSIHKAN FILTER]</button>
      {/if}
    </div>

    <!-- 2. DEVICES TABLE -->
    <div class="table-wrapper">
      <table>
        <thead>
          <tr>
            <th style="width: 90px;">STATUS</th>
            <th style="width: 100px;">LOCK</th>
            <th>DEVICE ID & NAMA</th>
            <th>IP ADDRESS</th>
            <th>ROLE</th>
            <th>TOMBOL</th>
            <th style="text-align: right;">AKSI MANAJEMEN</th>
          </tr>
        </thead>
        <tbody>
          {#each filteredDevices as dev}
            {@const isSelected = dev.id === selectedDeviceId}
            <tr class={isSelected ? 'row-selected' : ''}>
              <td>
                <span class="badge {dev.status === 'ONLINE' ? 'active' : 'inactive'}">
                  [{dev.status || 'OFFLINE'}]
                </span>
              </td>

              <td>
                <span class="badge {dev.is_locked ? 'warning' : 'inactive'}">
                  {dev.is_locked ? '[LOCKED]' : '[UNLOCKED]'}
                </span>
                {#if dev.is_locked && dev.allow_automation_when_locked !== false}
                  <div class="lock-sub-hint" title="Jadwal timer & sensor trigger tetap diizinkan">[SMART AUTO]</div>
                {/if}
              </td>

              <td>
                <strong class="dev-name">{dev.name}</strong>
                <div class="dev-id">ID: {dev.id}</div>
              </td>

              <td>
                <span class="mono">{dev.ip_address || '192.168.100.x'}</span>
              </td>

              <td>
                <span class="dev-role">{dev.role || 'Edge Node'}</span>
              </td>

              <td>
                <span class="badge">{dev.buttons?.length || 0} TOMBOL</span>
              </td>

              <td style="text-align: right;">
                <div class="action-buttons-inline">
                  <!-- Select / View Buttons -->
                  <button
                    class={isSelected && activeTab === 'buttons' ? 'btn-success' : ''}
                    onclick={() => { handleSelectDevice(dev.id); setTab('buttons'); }}
                    title="Kelola fungsi tombol untuk perangkat ini"
                  >
                    [TOMBOL]
                  </button>

                  <!-- View Indicators -->
                  <button
                    class={isSelected && activeTab === 'indicators' ? 'btn-success' : ''}
                    onclick={() => { handleSelectDevice(dev.id); setTab('indicators'); }}
                    title="Lihat indikator telemetri untuk perangkat ini"
                  >
                    [INDIKATOR]
                  </button>

                  <!-- Lock / Unlock Toggle -->
                  <button
                    class={dev.is_locked ? 'btn-warning' : ''}
                    onclick={() => handleToggleLock(dev)}
                    title={dev.is_locked ? 'Buka kunci perangkat' : 'Kunci konfigurasi perangkat'}
                  >
                    {dev.is_locked ? '[UNLOCK]' : '[LOCK]'}
                  </button>

                  <!-- Edit -->
                  <button
                    disabled={dev.is_locked}
                    onclick={() => handleOpenEdit(dev)}
                    title={dev.is_locked ? 'Perangkat terkunci tidak dapat diedit' : 'Edit nama/IP perangkat'}
                  >
                    [EDIT]
                  </button>

                  <!-- Delete -->
                  <button
                    class="btn-danger"
                    disabled={dev.is_locked || dev.id === 'esp32_master'}
                    onclick={() => handlePromptDelete(dev)}
                    title={
                      dev.is_locked
                        ? 'Perangkat terkunci tidak dapat dihapus'
                        : dev.id === 'esp32_master'
                        ? 'Master node tidak dapat dihapus'
                        : 'Hapus perangkat'
                    }
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
  </div>

  <!-- 3. SUB-PANEL: DETAIL SELECTED DEVICE TABS -->
  {#if selectedDevice && selectedDevice.id}
    <div class="detail-section">
      <!-- Tabs Switcher -->
      <div class="tabs-nav">
        <button
          class="tab-btn {activeTab === 'buttons' ? 'tab-active' : ''}"
          onclick={() => setTab('buttons')}
        >
          [01. KELOLA PERFUNGSI TOMBOL ({selectedDevice.buttons?.length || 0})]
        </button>

        <button
          class="tab-btn {activeTab === 'indicators' ? 'tab-active' : ''}"
          onclick={() => setTab('indicators')}
        >
          [02. LIHAT INDIKATOR & SENSOR]
        </button>
      </div>

      <!-- Tab Content 1: Buttons Manager -->
      {#if activeTab === 'buttons'}
        <DeviceButtonManager device={selectedDevice} {onCommand} />
      {/if}

      <!-- Tab Content 2: Indicators Viewer -->
      {#if activeTab === 'indicators'}
        <DeviceIndicatorViewer device={selectedDevice} {telemetry} />
      {/if}
    </div>
  {/if}
</div>

<!-- Modal Form Add/Edit -->
<DeviceModal
  isOpen={isModalOpen}
  mode={modalMode}
  initialData={editingDevice}
  onSave={handleSaveDevice}
  onClose={() => (isModalOpen = false)}
/>

<!-- In-app Delete Confirmation Modal -->
{#if isDeleteModalOpen && deviceToDelete}
  <!-- svelte-ignore a11y_click_events_have_key_events a11y_no_noninteractive_element_interactions -->
  <div class="modal-backdrop" onclick={() => (isDeleteModalOpen = false)} onkeydown={(e) => e.key === 'Escape' && (isDeleteModalOpen = false)} role="dialog" aria-modal="true" tabindex="-1">
    <!-- svelte-ignore a11y_no_noninteractive_element_interactions -->
    <div class="modal-window" onclick={(e) => e.stopPropagation()} role="document">
      <div class="panel-header">
        <h2 style="color: #f87171;">[PERINGATAN: KONFIRMASI HAPUS PERANGKAT]</h2>
        <button onclick={() => (isDeleteModalOpen = false)} class="btn-sm">[X]</button>
      </div>

      {#if deleteError}
        <div class="notification-banner banner-alert">
          [ERROR]: {deleteError}
        </div>
      {/if}

      <div class="panel-body">
        <p style="margin-bottom: 12px; color: var(--text-secondary); line-height: 1.5;">
          Anda akan menghapus perangkat <strong style="color: #fff;">{deviceToDelete.name}</strong> dengan ID <strong style="color: #fde047;">{deviceToDelete.id}</strong>.<br />
          Tindakan ini permanen dan akan menghapus semua konfigurasi tombol terkait dari daftar.
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
          <button type="button" onclick={() => (isDeleteModalOpen = false)}>
            [BATAL]
          </button>
          <button
            type="button"
            class="btn-danger"
            disabled={deleteConfirmInput.trim() !== deviceToDelete.id}
            onclick={handleConfirmDelete}
          >
            [HAPUS PERANGKAT INI]
          </button>
        </div>
      </div>
    </div>
  </div>
{/if}

<style>
  .settings-container {
    display: flex;
    flex-direction: column;
    gap: 16px;
  }

  .notification-banner {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 8px 12px;
    font-family: var(--font-mono);
    font-size: 11px;
    font-weight: 700;
    border: 1px solid transparent;
  }

  .banner-success {
    background-color: var(--status-active-bg);
    color: var(--status-active-text);
    border-color: var(--status-active-border);
  }

  .banner-warning {
    background-color: var(--status-warning-bg);
    color: var(--status-warning-text);
    border-color: var(--status-warning-border);
  }

  .banner-alert {
    background-color: var(--status-alert-bg);
    color: var(--status-alert-text);
    border-color: var(--status-alert-border);
  }

  .top-stats {
    display: flex;
    align-items: center;
    gap: 10px;
    font-family: var(--font-mono);
    font-size: 11px;
    flex-wrap: wrap;
  }

  .stat-badge {
    background-color: var(--bg-card);
    border: 1px solid var(--border-light);
    padding: 3px 8px;
    color: var(--text-secondary);
  }

  .stat-badge strong {
    color: var(--text-primary);
  }

  .color-active {
    color: #4ade80 !important;
  }

  .color-warning {
    color: #fde047 !important;
  }

  .filter-bar {
    padding: 8px 12px;
    background-color: #111;
    border-bottom: 1px solid var(--border-light);
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
  }

  .table-wrapper {
    overflow-x: auto;
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
  }

  .row-selected td {
    background-color: var(--bg-hover);
    border-bottom-color: var(--accent-amber);
  }

  .dev-name {
    color: var(--text-primary);
    font-size: 12px;
  }

  .dev-id {
    font-family: var(--font-mono);
    font-size: 10px;
    color: var(--text-muted);
  }

  .dev-role {
    font-family: var(--font-mono);
    font-size: 10px;
    color: var(--text-secondary);
  }

  .action-buttons-inline {
    display: flex;
    justify-content: flex-end;
    gap: 4px;
    flex-wrap: wrap;
  }

  .detail-section {
    display: flex;
    flex-direction: column;
    gap: 12px;
  }

  .tabs-nav {
    display: flex;
    gap: 4px;
    border-bottom: 1px solid var(--border-medium);
    padding-bottom: 0;
  }

  .tab-btn {
    background-color: var(--bg-panel);
    border: 1px solid var(--border-medium);
    border-bottom: none;
    color: var(--text-secondary);
    padding: 8px 14px;
    font-family: var(--font-mono);
    font-size: 11px;
    font-weight: 700;
  }

  .tab-btn:hover {
    color: var(--text-primary);
    background-color: var(--bg-hover);
  }

  .tab-btn.tab-active {
    background-color: var(--bg-panel-header);
    color: var(--text-amber);
    border-color: var(--border-medium);
    border-top: 2px solid var(--accent-amber);
  }

  .modal-actions {
    display: flex;
    justify-content: flex-end;
    gap: 8px;
    margin-top: 16px;
    padding-top: 12px;
    border-top: 1px solid var(--border-light);
  }

  .btn-sm {
    padding: 2px 6px;
    font-size: 10px;
    background: transparent;
    border: 1px solid currentColor;
    color: inherit;
  }

  .lock-sub-hint {
    font-family: var(--font-mono);
    font-size: 9px;
    color: #4ade80;
    margin-top: 2px;
  }
</style>
