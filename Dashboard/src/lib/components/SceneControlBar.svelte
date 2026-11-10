<script>
  import { onMount } from 'svelte';
  import { fetchScenes, activateSceneApi } from '../api.js';

  let { onSceneChange = () => {} } = $props();

  let currentScene = $state('NORMAL');
  let availableScenes = $state(['NORMAL', 'AWAY', 'SLEEP']);
  let sceneDescription = $state('Operasi standar rumah harian.');
  let activatedTime = $state('--:--:--');
  let isActivating = $state(false);

  async function loadSceneStatus() {
    try {
      const data = await fetchScenes();
      if (data) {
        currentScene = data.current_scene || 'NORMAL';
        availableScenes = data.available_scenes || ['NORMAL', 'AWAY', 'SLEEP'];
        sceneDescription = data.description || '';
        if (data.activated_at) {
          activatedTime = new Date(data.activated_at).toLocaleTimeString();
        }
      }
    } catch (e) {
      console.warn('Could not fetch scene status from Business Engine:', e);
    }
  }

  async function handleActivate(scene) {
    if (scene === currentScene || isActivating) return;
    isActivating = true;
    try {
      const res = await activateSceneApi(scene);
      currentScene = res.current_scene;
      sceneDescription = res.description;
      activatedTime = new Date(res.activated_at).toLocaleTimeString();
      onSceneChange(res.current_scene);
    } catch (err) {
      console.error('Failed to activate scene:', err);
      alert(`Gagal mengaktifkan mode ${scene}: ${err.message}`);
    } finally {
      isActivating = false;
    }
  }

  onMount(() => {
    loadSceneStatus();
  });
</script>

<div class="scene-bar">
  <div class="scene-header">
    <div class="title-wrap">
      <span class="amber-sq"></span>
      <span class="title">PROFIL SCENE OTOMASI RUMAH</span>
    </div>
    <div class="scene-meta">
      <span class="meta-label">AKTIF SEJAK:</span>
      <span class="meta-val">{activatedTime}</span>
    </div>
  </div>

  <div class="scene-controls">
    <div class="btn-group">
      {#each availableScenes as scene}
        <button
          type="button"
          class="scene-btn {scene.toLowerCase()} {currentScene === scene ? 'active' : ''}"
          disabled={isActivating}
          onclick={() => handleActivate(scene)}
        >
          <span class="indicator-dot"></span>
          <span class="btn-text">MODE {scene}</span>
        </button>
      {/each}
    </div>

    <div class="scene-desc">
      <span class="desc-badge">KONDISI:</span>
      <span class="desc-text">{sceneDescription}</span>
    </div>
  </div>
</div>

<style>
  .scene-bar {
    background-color: var(--bg-panel);
    border: 1px solid var(--border-medium);
    border-radius: 0;
    padding: 10px 14px;
    display: flex;
    flex-direction: column;
    gap: 8px;
    font-family: var(--font-mono);
  }

  .scene-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    border-bottom: 1px solid var(--border-subtle);
    padding-bottom: 6px;
  }

  .title-wrap {
    display: flex;
    align-items: center;
    gap: 8px;
  }

  .amber-sq {
    width: 6px;
    height: 12px;
    background-color: var(--accent-amber);
    border-radius: 0;
  }

  .title {
    font-size: 11px;
    font-weight: 700;
    color: var(--text-primary);
    letter-spacing: 0.08em;
  }

  .scene-meta {
    font-size: 10px;
    display: flex;
    gap: 6px;
  }

  .meta-label {
    color: var(--text-muted);
  }

  .meta-val {
    color: var(--text-secondary);
    font-weight: 600;
  }

  .scene-controls {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 16px;
    flex-wrap: wrap;
  }

  .btn-group {
    display: flex;
    gap: 8px;
  }

  .scene-btn {
    border-radius: 0;
    border: 1px solid var(--border-medium);
    background-color: var(--bg-card);
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 11px;
    font-weight: 700;
    padding: 6px 14px;
    cursor: pointer;
    display: flex;
    align-items: center;
    gap: 6px;
    letter-spacing: 0.05em;
    transition: all 0.15s ease;
  }

  .indicator-dot {
    width: 6px;
    height: 6px;
    border-radius: 0;
    background-color: var(--border-medium);
  }

  .scene-btn:hover:not(:disabled) {
    border-color: var(--text-secondary);
    color: var(--text-primary);
  }

  .scene-btn.normal.active {
    border-color: var(--accent-amber);
    background-color: rgba(245, 158, 11, 0.15);
    color: var(--accent-amber);
  }
  .scene-btn.normal.active .indicator-dot {
    background-color: var(--accent-amber);
    box-shadow: 0 0 6px var(--accent-amber);
  }

  .scene-btn.away.active {
    border-color: var(--accent-cyan);
    background-color: rgba(6, 182, 212, 0.15);
    color: var(--accent-cyan);
  }
  .scene-btn.away.active .indicator-dot {
    background-color: var(--accent-cyan);
    box-shadow: 0 0 6px var(--accent-cyan);
  }

  .scene-btn.sleep.active {
    border-color: var(--accent-green);
    background-color: rgba(16, 185, 129, 0.15);
    color: var(--accent-green);
  }
  .scene-btn.sleep.active .indicator-dot {
    background-color: var(--accent-green);
    box-shadow: 0 0 6px var(--accent-green);
  }

  .scene-btn:disabled {
    opacity: 0.5;
    cursor: not-allowed;
  }

  .scene-desc {
    display: flex;
    align-items: center;
    gap: 8px;
    font-size: 11px;
    background: rgba(0, 0, 0, 0.2);
    padding: 4px 10px;
    border: 1px solid var(--border-subtle);
  }

  .desc-badge {
    color: var(--accent-amber);
    font-weight: 700;
    font-size: 10px;
  }

  .desc-text {
    color: var(--text-secondary);
  }

  @media (max-width: 640px) {
    .scene-controls {
      flex-direction: column;
      align-items: stretch;
    }
    .btn-group {
      width: 100%;
    }
    .scene-btn {
      flex: 1;
      justify-content: center;
    }
  }
</style>

