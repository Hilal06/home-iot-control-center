<script>
  import { onMount } from 'svelte';
  import { fetchScenes, activateSceneApi } from '../api.js';
  import Home from '@lucide/svelte/icons/home';
  import LogOut from '@lucide/svelte/icons/log-out';
  import Moon from '@lucide/svelte/icons/moon';
  import SlidersHorizontal from '@lucide/svelte/icons/sliders-horizontal';

  let { onSceneChange = () => {} } = $props();

  let currentScene = $state('NORMAL');
  let availableScenes = $state(['NORMAL', 'AWAY', 'SLEEP']);
  let isActivating = $state(false);

  const SCENE_INFO = {
    NORMAL: { label: 'Di Rumah', icon: Home },
    AWAY: { label: 'Sedang Pergi', icon: LogOut },
    SLEEP: { label: 'Waktu Tidur', icon: Moon },
  };

  async function loadSceneStatus() {
    try {
      const data = await fetchScenes();
      if (data) {
        currentScene = data.current_scene || 'NORMAL';
        availableScenes = data.available_scenes || ['NORMAL', 'AWAY', 'SLEEP'];
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
  <div class="scene-label-group">
    <span class="amber-bar"></span>
    <span class="scene-title">MODE OTOMASI RUMAH</span>
  </div>

  <div class="scene-pills">
    {#each availableScenes as scene}
      {@const info = SCENE_INFO[scene] || { label: scene, icon: SlidersHorizontal }}
      {@const Icon = info.icon}
      <button
        type="button"
        class="scene-pill {scene.toLowerCase()} {currentScene === scene ? 'active' : ''}"
        disabled={isActivating}
        onclick={() => handleActivate(scene)}
      >
        <span class="pill-icon"><Icon size={13} /></span>
        <span class="pill-text">{info.label}</span>
      </button>
    {/each}
  </div>
</div>

<style>
  .scene-bar {
    background-color: var(--bg-panel);
    border: 1px solid var(--border-medium);
    border-radius: var(--radius-sm);
    padding: 8px 14px;
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 12px;
    font-family: var(--font-mono);
    flex-wrap: wrap;
  }

  .scene-label-group {
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

  .scene-title {
    font-size: 11px;
    font-weight: 700;
    letter-spacing: 0.05em;
    color: var(--text-primary);
  }

  .scene-pills {
    display: flex;
    gap: 8px;
    flex-wrap: wrap;
  }

  .scene-pill {
    display: inline-flex;
    align-items: center;
    gap: 6px;
    padding: 6px 14px;
    font-family: var(--font-mono);
    font-size: 11px;
    font-weight: 700;
    background-color: var(--bg-card);
    border: 1px solid var(--border-medium);
    border-radius: var(--radius-sm);
    color: var(--text-secondary);
    cursor: pointer;
    transition: all 0.15s ease;
  }

  .pill-icon {
    display: inline-flex;
    align-items: center;
  }

  .scene-pill:hover:not(:disabled) {
    background-color: var(--bg-hover);
    color: var(--text-primary);
    border-color: var(--border-heavy);
  }

  .scene-pill.normal.active {
    background-color: #fffbeb;
    border-color: var(--accent-amber);
    color: #d97706;
  }

  .scene-pill.away.active {
    background-color: #f0f9ff;
    border-color: var(--accent-cyan);
    color: #0284c7;
  }

  .scene-pill.sleep.active {
    background-color: #f0fdf4;
    border-color: var(--accent-green);
    color: #16a34a;
  }

  .scene-pill:disabled {
    opacity: 0.5;
    cursor: not-allowed;
  }

  @media (max-width: 640px) {
    .scene-bar {
      flex-direction: column;
      align-items: stretch;
    }
    .scene-pills {
      width: 100%;
    }
    .scene-pill {
      flex: 1;
      justify-content: center;
    }
  }
</style>
