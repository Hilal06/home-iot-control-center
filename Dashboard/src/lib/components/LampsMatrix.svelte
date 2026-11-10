<script>
  import { SvelteSet } from 'svelte/reactivity';

  let { lamps = {}, onCommand = () => {} } = $props();

  const LAMP_DEFINITIONS = [
    { id: 'living_room', name: 'Ruang Tamu' },
    { id: 'main_room', name: 'Kamar Utama' },
    { id: 'garage', name: 'Garasi Mobil' },
    { id: 'kitchen', name: 'Dapur' },
    { id: 'stair_garage', name: 'Tangga Garasi' },
    { id: 'stair_fl2', name: 'Tangga Lantai 2' },
    { id: 'yard_front', name: 'Halaman Depan' },
    { id: 'yard_side', name: 'Halaman Samping' },
    { id: 'yard_back', name: 'Halaman Belakang' },
    { id: 'server_room', name: 'Ruang Server' },
    { id: 'electrical_room', name: 'Ruang Panel Listrik' },
  ];

  let pendingRooms = new SvelteSet();

  let activeCount = $derived(
    LAMP_DEFINITIONS.filter((l) => !!lamps[l.id]).length
  );
  let totalWattsEstimated = $derived(activeCount * 9);

  async function toggleLamp(roomId, currentState) {
    pendingRooms.add(roomId);
    try {
      await onCommand({
        command: 'set_lamp',
        room: roomId,
        state: !currentState,
      });
    } finally {
      pendingRooms.delete(roomId);
    }
  }

  async function setAllLamps(state) {
    pendingRooms.add('all');
    try {
      await onCommand({
        command: 'set_lamp',
        room: 'all',
        state: state,
      });
    } finally {
      pendingRooms.delete('all');
    }
  }
</script>

<div class="panel">
  <div class="panel-header">
    <div class="title-with-bar">
      <span class="amber-bar"></span>
      <h2>KONTROL LAMPU RUMAH</h2>
      <span class="badge amber active-counter">{activeCount} / {LAMP_DEFINITIONS.length} Menyala (~{totalWattsEstimated} W)</span>
    </div>

    <div class="master-actions">
      <button
        type="button"
        class="btn-success"
        disabled={pendingRooms.has('all')}
        onclick={() => setAllLamps(true)}
      >
        NYALAKAN SEMUA
      </button>
      <button
        type="button"
        class="btn-danger"
        disabled={pendingRooms.has('all')}
        onclick={() => setAllLamps(false)}
      >
        MATIKAN SEMUA
      </button>
    </div>
  </div>

  <div class="lamps-grid">
    {#each LAMP_DEFINITIONS as lamp}
      {@const isOn = !!lamps[lamp.id]}
      {@const isBusy = pendingRooms.has(lamp.id) || pendingRooms.has('all')}

      <div class="lamp-card {isOn ? 'is-on' : 'is-off'}">
        <div class="lamp-meta">
          <span class="lamp-name">{lamp.name}</span>
          <span class="lamp-status-text {isOn ? 'text-green' : 'text-muted'}">
            {isOn ? '● MENYALA (~9W)' : '○ MATI (0W)'}
          </span>
        </div>

        <div class="lamp-controls">
          <button
            type="button"
            class="lamp-toggle-btn {isOn ? 'btn-danger' : 'btn-success'}"
            disabled={isBusy}
            onclick={() => toggleLamp(lamp.id, isOn)}
          >
            {isBusy ? '...' : isOn ? 'MATIKAN' : 'NYALAKAN'}
          </button>
        </div>
      </div>
    {/each}
  </div>
</div>

<style>
  .title-with-bar {
    display: flex;
    align-items: center;
    gap: 8px;
    flex-wrap: wrap;
  }

  .amber-bar {
    width: 3px;
    height: 14px;
    background-color: var(--accent-amber);
  }

  .active-counter {
    font-size: 10px;
    padding: 2px 6px;
  }

  .master-actions {
    display: flex;
    gap: 8px;
  }

  .lamps-grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(250px, 1fr));
    gap: 8px;
    padding: 12px;
  }

  .lamp-card {
    display: flex;
    justify-content: space-between;
    align-items: center;
    background-color: var(--bg-card);
    border: 1px solid var(--border-medium);
    padding: 10px 14px;
    font-family: var(--font-mono);
  }

  .lamp-card.is-on {
    border-color: rgba(16, 185, 129, 0.45);
    background-color: rgba(16, 185, 129, 0.08);
  }

  .lamp-card.is-off {
    border-color: var(--border-medium);
  }

  .lamp-meta {
    display: flex;
    flex-direction: column;
    gap: 3px;
  }

  .lamp-name {
    font-size: 13px;
    font-weight: 700;
    color: var(--text-primary);
  }

  .lamp-status-text {
    font-size: 11px;
    font-weight: 600;
  }

  .text-green {
    color: var(--solar-green-text);
  }

  .text-muted {
    color: var(--text-muted);
  }

  .lamp-controls {
    display: flex;
    align-items: center;
    gap: 8px;
  }

  .lamp-toggle-btn {
    min-width: 85px;
    padding: 6px 12px;
    font-size: 11px;
    font-weight: 700;
    letter-spacing: 0.04em;
  }
</style>
