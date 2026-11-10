<script>
  import { onMount } from 'svelte';
  import {
    fetchDevices,
    fetchDeviceStatus,
    fetchTelemetryLatest,
    sendControlCommand,
    initWebSocket,
  } from './lib/api.js';
  import { deviceStore } from './lib/deviceStore.js';

  import Sidebar from './lib/components/Sidebar.svelte';
  import TopGlanceBar from './lib/components/TopGlanceBar.svelte';
  import PowerSection from './lib/components/PowerSection.svelte';
  import LampsMatrix from './lib/components/LampsMatrix.svelte';
  import ActuatorsSection from './lib/components/ActuatorsSection.svelte';
  import EnvironmentSection from './lib/components/EnvironmentSection.svelte';
  import ConsolesView from './lib/components/ConsolesView.svelte';
  import DeviceSettings from './lib/components/DeviceSettings.svelte';
  import TelemetryLineChart from './lib/components/TelemetryLineChart.svelte';
  import OtherSensorsSection from './lib/components/OtherSensorsSection.svelte';
  import SceneControlBar from './lib/components/SceneControlBar.svelte';

  function getViewFromHash() {
    const hash = window.location.hash;
    if (hash.startsWith('#settings') || hash.startsWith('#devices')) return 'settings';
    if (hash.startsWith('#consoles') || hash.startsWith('#logs')) return 'consoles';
    return 'monitor';
  }

  let currentView = $state(getViewFromHash()); // 'monitor' | 'settings' | 'consoles'

  function changeView(newView) {
    currentView = newView;
    if (newView === 'settings' || newView === 'devices') {
      window.location.hash = '#devices';
    } else if (newView === 'consoles') {
      window.location.hash = '#consoles';
    } else {
      window.location.hash = '#monitor';
    }
  }
  let wsStatus = $state('CONNECTING');
  let storeState = $state(deviceStore.getState());

  let activeDevice = $derived(storeState.activeDevice);
  let isDeviceOnline = $state(false);
  let lastTelemetryTimestamp = $state(0);

  let telemetry = $state({
    pv_array_voltage: 42.5,
    pv_array_current: 4.1,
    pv_power_watts: 174.25,
    solar_yield_kwh: 12.4,
    panel_temp_c: 38.6,
    solar_charger_active: true,
    mains_voltage: 224.8,
    house_power_w: 1420.5,
    ups_battery_pct: 98.0,
    ups_load_pct: 28.0,
    rack_temp_c: 24.3,
    temp_outdoor_c: 29.1,
    humidity_pct: 58.0,
    water_leak: false,
    ping_gateway_ms: 3.4,
    rssi_dbm: -52,
    esp_free_heap_kb: 226,
    garage_status: false,
    kitchen_breaker: true,
    hvac_power: true,
    hvac_target_temp: 22,
    gas_ppm: 145,
    water_level_pct: 82,
    ultrasonic_distance_cm: 18.4,
    lamps: {
      living_room: false,
      main_room: false,
      garage: false,
      kitchen: false,
      stair_garage: false,
      stair_fl2: false,
      yard_front: false,
      yard_side: false,
      yard_back: false,
      server_room: false,
      electrical_room: false,
    },
  });

  let telemetryLogs = $state([]);
  let controlLogs = $state([]);

  function addTelemetryLog(data) {
    const time = new Date().toLocaleTimeString();
    const row = {
      time,
      solar_w: data.pv_power_watts ?? telemetry.pv_power_watts ?? 0,
      house_w: data.house_power_w ?? telemetry.house_power_w ?? 0,
      mains_v: data.mains_voltage ?? telemetry.mains_voltage ?? 220,
      battery_pct: data.ups_battery_pct ?? telemetry.ups_battery_pct ?? 98,
      temp_c: data.temp_outdoor_c ?? telemetry.temp_outdoor_c ?? 28,
      gas_ppm: data.gas_ppm ?? telemetry.gas_ppm ?? 145,
      water_pct: data.water_level_pct ?? telemetry.water_level_pct ?? 82,
      rssi: data.rssi_dbm ?? telemetry.rssi_dbm ?? -55,
    };
    telemetryLogs = [row, ...telemetryLogs.slice(0, 99)];
  }

  function addControlLog(category, action, details, status) {
    const time = new Date().toLocaleTimeString();
    const row = { time, category, action, details, status };
    controlLogs = [row, ...controlLogs.slice(0, 99)];
  }

  async function handleCommand(cmd) {
    const targetDev = activeDevice;
    const actionDesc = `${cmd.command}${cmd.room ? ' (' + cmd.room + ')' : ''}`;

    const isBypass = cmd.force || ['SCHEDULE', 'AUTOMATION', 'DEVICE_TRIGGER', 'EMERGENCY'].includes((cmd.source || '').toUpperCase());

    // Smart lock validation: automated routines, schedule timers, and safety triggers bypass lock automatically.
    // For manual user clicks on a locked device, require explicit confirmation to force bypass.
    if (targetDev && targetDev.is_locked && !isBypass) {
      const confirmProceed = confirm(
        `[DEVICE LOCKED]: Perangkat '${targetDev.name}' sedang dalam status TERKUNCI.\n\n` +
        `Perintah manual '${actionDesc}' akan ditolak oleh keamanan hardware (HTTP 423).\n\n` +
        `Apakah Anda ingin mengeksekusi dengan override paksa (force: true)?`
      );
      if (!confirmProceed) {
        addControlLog('SECURITY', actionDesc, `Dibatalkan: Device '${targetDev.name}' terkunci`, 'WARNING');
        return { status: 'cancelled_by_user_lock' };
      }
      cmd.force = true;
    }

    // Periksa status aktif perangkat keras (Error Handling Perangkat Mati/Offline)
    if (!isDeviceOnline && !cmd.force) {
      const proceedOffline = confirm(
        `[PERANGKAT MATI/OFFLINE]: Perangkat '${targetDev?.name || 'ESP32 Master'}' saat ini tidak aktif / offline.\n\n` +
        `Perintah '${actionDesc}' tidak akan dapat direspons langsung oleh perangkat keras fisik.\n\n` +
        `Apakah Anda tetap ingin mengirimkan perintah ini ke antrian (queue) server backend?`
      );
      if (!proceedOffline) {
        addControlLog('KONTROL', actionDesc, `Dibatalkan: Perangkat '${targetDev?.name || 'ESP32 Master'}' sedang offline`, 'WARNING');
        return { status: 'cancelled_device_offline' };
      }
      cmd.force = true;
    }

    try {
      const targetId = targetDev?.id || 'esp32_master';
      const res = await sendControlCommand(targetId, cmd);
      const category = cmd.command === 'set_lamp' ? 'LAMPU' : ['set_garage', 'set_hvac', 'set_breaker', 'set_solar'].includes(cmd.command) ? 'AKTUATOR' : 'KONTROL';
      addControlLog(category, actionDesc, JSON.stringify(cmd), 'SUCCESS');

      // Optimistically update local states for immediate responsiveness
      if (cmd.command === 'set_lamp') {
        if (cmd.room === 'all') {
          const updated = { ...telemetry.lamps };
          for (const k in updated) updated[k] = cmd.state;
          telemetry.lamps = updated;
        } else if (cmd.room && telemetry.lamps) {
          telemetry.lamps = { ...telemetry.lamps, [cmd.room]: cmd.state };
        }
      } else if (cmd.command === 'set_garage') {
        telemetry.garage_status = typeof cmd.state === 'string' ? cmd.state === 'open' : !!cmd.state;
      } else if (cmd.command === 'set_hvac') {
        telemetry.hvac_power = cmd.power;
        telemetry.hvac_target_temp = cmd.target_temp;
      } else if (cmd.command === 'set_breaker') {
        telemetry.kitchen_breaker = !!cmd.state;
      } else if (cmd.command === 'set_solar') {
        telemetry.solar_charger_active = cmd.charge_enabled;
      }

      return res;
    } catch (err) {
      addControlLog('KONTROL', actionDesc, err.message, 'ERROR');
      console.error('Failed to send command:', err);
      alert(`Error sending command: ${err.message}`);
      throw err;
    }
  }

  function handleSelectDevice(newDeviceId) {
    deviceStore.setActiveDevice(newDeviceId);
    loadDeviceData(newDeviceId);
    addControlLog('SYSTEM', 'Ganti Node Aktif', `Node aktif beralih ke [${newDeviceId}]`, 'SUCCESS');
  }

  function handleSceneChange(scene) {
    addControlLog('SCENE', 'Mode Otomasi Rumah', `Profil diubah ke Mode [${scene}]`, 'SUCCESS');
    if (scene === 'AWAY') {
      const updated = { ...telemetry.lamps };
      ['living_room', 'main_room', 'kitchen', 'stair_fl2', 'stair_garage'].forEach((k) => {
        if (updated[k] !== undefined) updated[k] = false;
      });
      telemetry.lamps = updated;
    }
  }

  function handleToggleLock(deviceId) {
    try {
      const isLocked = deviceStore.toggleLock(deviceId);
      addControlLog(
        'SECURITY',
        `Kunci Node [${deviceId}]`,
        `Status penguncian: ${isLocked ? 'TERKUNCI' : 'TERBUKA'}`,
        isLocked ? 'WARNING' : 'SUCCESS'
      );
    } catch (err) {
      addControlLog('SECURITY', `Kunci Node [${deviceId}]`, err.message, 'ERROR');
    }
  }

  function loadDeviceData(deviceId) {
    fetchDeviceStatus(deviceId)
      .then((data) => {
        if (data.state?.lamp_states) {
          telemetry.lamps = data.state.lamp_states;
        }
        if (data.state?.garage_status !== undefined) telemetry.garage_status = data.state.garage_status;
        if (data.state?.kitchen_breaker !== undefined) telemetry.kitchen_breaker = data.state.kitchen_breaker;
        if (data.state?.hvac_power !== undefined) telemetry.hvac_power = data.state.hvac_power;
        if (data.state?.hvac_target_temp !== undefined) telemetry.hvac_target_temp = data.state.hvac_target_temp;
        if (data.state?.solar_charge_enabled !== undefined) telemetry.solar_charger_active = data.state.solar_charge_enabled;
      })
      .catch((e) => console.warn(`Status fetch error for ${deviceId}:`, e));

    fetchTelemetryLatest(deviceId)
      .then((data) => {
        if (data && data.telemetry) {
          telemetry = { ...telemetry, ...data.telemetry };
        } else if (data) {
          telemetry = { ...telemetry, ...data };
        }
      })
      .catch((e) => console.warn(`Telemetry latest fetch error for ${deviceId}:`, e));
  }

  onMount(() => {
    // Subscribe to store updates
    const unsubStore = deviceStore.subscribe((s) => {
      storeState = s;
    });

    // Sync devices list from backend
    fetchDevices()
      .then((data) => {
        if (data && data.devices) {
          deviceStore.syncWithBackend(data.devices);
        }
      })
      .catch((e) => console.warn('Fetch backend devices error:', e));

    // Initial snapshot fetch
    const initialId = storeState.activeDeviceId || 'esp32_master';
    loadDeviceData(initialId);

    // Setup real-time WebSocket connection
    const cleanupWs = initWebSocket(
      (data) => {
        const wasOffline = !isDeviceOnline;
        lastTelemetryTimestamp = Date.now();
        isDeviceOnline = true;

        telemetry = { ...telemetry, ...data };
        if (data.lamps) {
          telemetry.lamps = { ...telemetry.lamps, ...data.lamps };
        }
        addTelemetryLog(data);

        if (wasOffline) {
          addControlLog(
            'SISTEM',
            'Status Perangkat',
            `Perangkat [${activeDevice?.id || 'esp32_master'}] TERHUBUNG / ONLINE`,
            'SUCCESS'
          );
        }
      },
      (status) => {
        wsStatus = status;
        addControlLog('SYSTEM', 'Koneksi Server', `Status WebSocket: ${status}`, status === 'CONNECTED' ? 'SUCCESS' : 'WARNING');
      }
    );

    // Watchdog pengecekan keaktifan hardware secara real-time
    // Jika data telemetri terhenti > 3.5 detik (ESP32 dimatikan / putus koneksi), tandai OFFLINE
    const watchdogInterval = setInterval(() => {
      const now = Date.now();
      if (isDeviceOnline && lastTelemetryTimestamp > 0 && now - lastTelemetryTimestamp > 3500) {
        isDeviceOnline = false;
        addControlLog(
          'SISTEM',
          'Status Perangkat',
          `Perangkat [${activeDevice?.id || 'esp32_master'}] TERPUTUS / MATI (Tidak ada sinyal > 3.5 detik)`,
          'WARNING'
        );
      }
    }, 1000);

    // Polling sinkronisasi daftar perangkat dari backend (termasuk status timeout DB)
    const pollInterval = setInterval(() => {
      fetchDevices()
        .then((data) => {
          if (data && data.devices) {
            deviceStore.syncWithBackend(data.devices);
            const current = data.devices.find((d) => d.id === (activeDevice?.id || 'esp32_master'));
            if (current && current.status === 'OFFLINE' && isDeviceOnline && Date.now() - lastTelemetryTimestamp > 3000) {
              isDeviceOnline = false;
            }
          }
        })
        .catch(() => {});
    }, 4000);

    const onHashChange = () => {
      currentView = getViewFromHash();
    };
    window.addEventListener('hashchange', onHashChange);

    return () => {
      window.removeEventListener('hashchange', onHashChange);
      clearInterval(watchdogInterval);
      clearInterval(pollInterval);
      unsubStore();
      cleanupWs();
    };
  });
</script>

<div class="app-layout">
  <!-- Left Side: Collapsible SCADA Navigation Sidebar -->
  <Sidebar
    {currentView}
    onViewChange={changeView}
    devices={storeState.devices}
    activeDeviceId={activeDevice?.id || 'esp32_master'}
    onDeviceSelect={handleSelectDevice}
    {telemetry}
    {wsStatus}
    device={activeDevice}
    onToggleLock={handleToggleLock}
    {isDeviceOnline}
  />

  <!-- Right Side: Main Operational Viewport -->
  <div class="main-viewport">
    <!-- Top Glance Overview Bar (PLN 900W, Solar 2400Wp, Battery 4.8kWh, Clock) -->
    <TopGlanceBar {telemetry} device={activeDevice} {isDeviceOnline} />

    <!-- Scrollable Content Area -->
    <main class="content-scroll">
      {#if currentView === 'monitor'}
        <!-- VIEW 01: DASHBOARD MONITOR (SIMPEL, INFORMATIF, BEBAS BUZZWORD) -->

        <!-- Operational Scene Bar (NORMAL / AWAY / SLEEP) -->
        <SceneControlBar onSceneChange={handleSceneChange} />

        <!-- Section 1: Real-time Split Telemetry Charts (Daya Listrik & Tangki Air/Suhu) -->
        <TelemetryLineChart
          {telemetry}
          activeDeviceId={activeDevice?.id || 'esp32_master'}
          {isDeviceOnline}
        />

        <!-- Section 2: Kontrol Lampu Rumah (11 Ruangan) -->
        <LampsMatrix lamps={telemetry.lamps || {}} onCommand={handleCommand} />

        <!-- Section 3: Daya & Kelistrikan Rumah (PLN 900W, Solar 2400W, Baterai 4.8kWh) -->
        <PowerSection {telemetry} onCommand={handleCommand} />

        <!-- Section 4: Peralatan Rumah & Tangki/Gas (Row 2 Kolom) -->
        <div class="row-2col">
          <ActuatorsSection {telemetry} onCommand={handleCommand} />
          <OtherSensorsSection
            {telemetry}
            devices={storeState.devices}
            onCommand={handleCommand}
          />
        </div>

        <!-- Section 5: Lingkungan & Jaringan -->
        <EnvironmentSection {telemetry} {isDeviceOnline} device={activeDevice} />

      {:else if currentView === 'settings' || currentView === 'devices'}
        <!-- VIEW 02: PENGATURAN & MANAJEMEN DEVICES -->
        <DeviceSettings {telemetry} onCommand={handleCommand} />

      {:else if currentView === 'consoles'}
        <!-- VIEW 03: HALAMAN KONSOL (TERBAGI DUA: TELEMETRI & FEEDBACK KONTROL) -->
        <ConsolesView
          {telemetryLogs}
          {controlLogs}
          onClearTelemetry={() => (telemetryLogs = [])}
          onClearControl={() => (controlLogs = [])}
          {wsStatus}
        />
      {/if}
    </main>
  </div>
</div>

<style>
  :global(html),
  :global(body) {
    margin: 0;
    padding: 0;
    height: 100%;
    overflow: hidden;
    background-color: var(--bg-app);
  }

  .app-layout {
    display: flex;
    width: 100vw;
    height: 100vh;
    overflow: hidden;
    background-color: var(--bg-app);
  }

  .main-viewport {
    flex: 1;
    display: flex;
    flex-direction: column;
    min-width: 0;
    height: 100vh;
    overflow: hidden;
  }

  .content-scroll {
    flex: 1;
    overflow-y: auto;
    overflow-x: hidden;
    padding: 16px;
    display: flex;
    flex-direction: column;
    gap: 16px;
  }

  .row-2col {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(480px, 1fr));
    gap: 16px;
  }

  @media (max-width: 768px) {
    .app-layout {
      flex-direction: column;
    }
    .content-scroll {
      padding: 8px;
      gap: 10px;
    }
    .row-2col {
      grid-template-columns: 1fr;
    }
  }
</style>
