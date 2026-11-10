// Device State & Management Store for Home IoT Service
import { toggleDeviceLockApi, updateDeviceButtonsApi } from './api.js';

const STORAGE_KEY = 'home_iot_devices_v1';
const ACTIVE_DEVICE_KEY = 'home_iot_active_device_id';

export const BUTTON_TEMPLATES = {
  FULL_GATEWAY: {
    id: 'FULL_GATEWAY',
    name: 'Full Gateway Profile (11 Lamps + Actuators + Solar)',
    buttons: [
      { id: 'lamp_living', label: 'Living Room Lamp', command: 'set_lamp', room: 'living_room', type: 'toggle', enabled: true },
      { id: 'lamp_main', label: 'Main Room Lamp', command: 'set_lamp', room: 'main_room', type: 'toggle', enabled: true },
      { id: 'lamp_garage', label: 'Garage Lamp', command: 'set_lamp', room: 'garage', type: 'toggle', enabled: true },
      { id: 'lamp_kitchen', label: 'Kitchen Lamp', command: 'set_lamp', room: 'kitchen', type: 'toggle', enabled: true },
      { id: 'lamp_stair_garage', label: 'Stair Garage Lamp', command: 'set_lamp', room: 'stair_garage', type: 'toggle', enabled: true },
      { id: 'lamp_stair_fl2', label: 'Stair Lt 2 Lamp', command: 'set_lamp', room: 'stair_fl2', type: 'toggle', enabled: true },
      { id: 'lamp_yard_front', label: 'Halaman Depan Lamp', command: 'set_lamp', room: 'yard_front', type: 'toggle', enabled: true },
      { id: 'lamp_yard_side', label: 'Halaman Samping Lamp', command: 'set_lamp', room: 'yard_side', type: 'toggle', enabled: true },
      { id: 'lamp_yard_back', label: 'Halaman Belakang Lamp', command: 'set_lamp', room: 'yard_back', type: 'toggle', enabled: true },
      { id: 'lamp_server', label: 'Server Room Lamp', command: 'set_lamp', room: 'server_room', type: 'toggle', enabled: true },
      { id: 'lamp_electrical', label: 'Electrical Room Lamp', command: 'set_lamp', room: 'electrical_room', type: 'toggle', enabled: true },
      { id: 'act_garage', label: 'Garage Door Motor', command: 'set_garage', type: 'toggle', state_key: 'garage_status', enabled: true },
      { id: 'act_hvac', label: 'HVAC AC Unit Power', command: 'set_hvac', type: 'toggle', state_key: 'hvac_power', enabled: true },
      { id: 'act_breaker', label: 'Kitchen Main Breaker', command: 'set_breaker', type: 'toggle', state_key: 'kitchen_breaker', danger: true, enabled: true },
      { id: 'act_solar', label: 'Solar Charger Relay', command: 'set_solar', type: 'toggle', state_key: 'solar_charger_active', enabled: true },
      { id: 'cmd_reboot', label: 'Restart Node MCU', command: 'reboot_esp', type: 'trigger', danger: true, enabled: true },
    ]
  },
  LIGHTING_4ZONE: {
    id: 'LIGHTING_4ZONE',
    name: 'Lighting Substation (4-Zone Relays)',
    buttons: [
      { id: 'sub_lamp_1', label: 'Zone 1: Interior Light', command: 'set_lamp', room: 'zone_1', type: 'toggle', enabled: true },
      { id: 'sub_lamp_2', label: 'Zone 2: Exterior Light', command: 'set_lamp', room: 'zone_2', type: 'toggle', enabled: true },
      { id: 'sub_lamp_3', label: 'Zone 3: Pathway Light', command: 'set_lamp', room: 'zone_3', type: 'toggle', enabled: true },
      { id: 'sub_lamp_4', label: 'Zone 4: Security Floodlight', command: 'set_lamp', room: 'zone_4', type: 'toggle', enabled: true },
    ]
  },
  SMART_RELAY: {
    id: 'SMART_RELAY',
    name: 'Dual Smart Relay Switch',
    buttons: [
      { id: 'relay_ch1', label: 'Channel 1 Relay Power', command: 'set_relay', channel: 1, type: 'toggle', enabled: true },
      { id: 'relay_ch2', label: 'Channel 2 Relay Power', command: 'set_relay', channel: 2, type: 'toggle', enabled: true },
    ]
  },
  GAS_SAFETY: {
    id: 'GAS_SAFETY',
    name: 'Gas & Hazard Safety Node (MQ-Sensor + Cutoff Valve + Siren)',
    buttons: [
      { id: 'gas_cutoff_valve', label: 'Main Gas Supply Cutoff Valve', command: 'set_valve', valve_id: 'main_gas', type: 'toggle', danger: true, enabled: true },
      { id: 'gas_siren_alarm', label: 'Emergency Siren Alarm', command: 'trigger_emergency', alarm: true, cutoff: true, type: 'trigger', danger: true, enabled: true },
      { id: 'gas_sensor_ping', label: 'Trigger MQ Sensor Health Check', command: 'ping', type: 'trigger', enabled: true },
    ]
  },
  WATER_TANK: {
    id: 'WATER_TANK',
    name: 'Smart Water Storage & Pump (Pump Relay + Overflow Divert Valve)',
    buttons: [
      { id: 'water_pump_main', label: 'Main Water Pump Power', command: 'set_relay', channel: 1, type: 'toggle', enabled: true },
      { id: 'water_divert_valve', label: 'Secondary Overflow / Irrigation Divert Valve', command: 'set_valve', valve_id: 'irrigation_divert', type: 'toggle', enabled: true },
      { id: 'water_drain_valve', label: 'Emergency Reservoir Drain Valve', command: 'set_valve', valve_id: 'emergency_drain', type: 'toggle', danger: true, enabled: true },
      { id: 'water_level_ping', label: 'Measure Ultrasonic Level Now', command: 'ping', type: 'trigger', enabled: true },
    ]
  },
  SENSOR_ONLY: {
    id: 'SENSOR_ONLY',
    name: 'Sensor Monitor Node (Telemetry Only)',
    buttons: [
      { id: 'ping_check', label: 'Trigger Telemetry Ping', command: 'ping', type: 'trigger', enabled: true },
    ]
  },
  CUSTOM_EMPTY: {
    id: 'CUSTOM_EMPTY',
    name: 'Custom Node (No Initial Buttons)',
    buttons: []
  }
};

const DEFAULT_MASTER_DEVICE = {
  id: 'esp32_master',
  name: 'ESP32 Master Node (Edge Gateway)',
  ip_address: '192.168.100.76',
  status: 'ONLINE',
  role: 'Edge Gateway',
  is_locked: false,
  created_at: new Date().toISOString(),
  buttons: BUTTON_TEMPLATES.FULL_GATEWAY.buttons,
  description: 'Primary Edge Controller and telemetry aggregator for power, lights, and environment.',
};

class DeviceStore {
  constructor() {
    this.devices = this.loadInitialDevices();
    this.activeDeviceId = localStorage.getItem(ACTIVE_DEVICE_KEY) || 'esp32_master';
    this.subscribers = new Set();
  }

  loadInitialDevices() {
    try {
      const saved = localStorage.getItem(STORAGE_KEY);
      if (saved) {
        const parsed = JSON.parse(saved);
        if (Array.isArray(parsed) && parsed.length > 0) {
          // Ensure master device exists
          const hasMaster = parsed.some((d) => d.id === 'esp32_master');
          if (!hasMaster) {
            parsed.unshift(DEFAULT_MASTER_DEVICE);
          }
          return parsed;
        }
      }
    } catch (e) {
      console.warn('Failed to parse saved devices, fallback to default:', e);
    }
    return [DEFAULT_MASTER_DEVICE];
  }

  save() {
    try {
      localStorage.setItem(STORAGE_KEY, JSON.stringify(this.devices));
      localStorage.setItem(ACTIVE_DEVICE_KEY, this.activeDeviceId);
    } catch (e) {
      console.error('Failed to save devices to storage:', e);
    }
    this.notify();
  }

  subscribe(callback) {
    this.subscribers.add(callback);
    callback(this.getState());
    return () => this.subscribers.delete(callback);
  }

  notify() {
    const state = this.getState();
    this.subscribers.forEach((cb) => cb(state));
  }

  getState() {
    return {
      devices: [...this.devices],
      activeDeviceId: this.activeDeviceId,
      activeDevice: this.getActiveDevice(),
    };
  }

  getActiveDevice() {
    return (
      this.devices.find((d) => d.id === this.activeDeviceId) ||
      this.devices[0] ||
      DEFAULT_MASTER_DEVICE
    );
  }

  setActiveDevice(id) {
    const found = this.devices.find((d) => d.id === id);
    if (found) {
      this.activeDeviceId = id;
      this.save();
    }
  }

  // Merge devices loaded from backend REST /api/devices
  syncWithBackend(backendDevices = []) {
    let changed = false;
    for (const bDev of backendDevices) {
      const idx = this.devices.findIndex((d) => d.id === bDev.id);
      if (idx >= 0) {
        // Sync attributes from backend
        this.devices[idx].status = bDev.status || this.devices[idx].status;
        this.devices[idx].ip_address = bDev.ip_address || this.devices[idx].ip_address;
        this.devices[idx].last_seen = bDev.last_seen || this.devices[idx].last_seen;
        if (bDev.role) this.devices[idx].role = bDev.role;
        if (bDev.is_locked !== undefined) this.devices[idx].is_locked = bDev.is_locked;
        if (bDev.allow_automation_when_locked !== undefined) {
          this.devices[idx].allow_automation_when_locked = bDev.allow_automation_when_locked;
        }
        if (bDev.description !== undefined) this.devices[idx].description = bDev.description;
        if (bDev.buttons && Array.isArray(bDev.buttons) && bDev.buttons.length > 0) {
          this.devices[idx].buttons = bDev.buttons;
        }
        if (bDev.automation_rules && Array.isArray(bDev.automation_rules)) {
          this.devices[idx].automation_rules = bDev.automation_rules;
        }
        changed = true;
      } else {
        // New device from backend not yet in local state
        const initialTpl =
          bDev.id.includes('gas') ? BUTTON_TEMPLATES.GAS_SAFETY :
          bDev.id.includes('water') ? BUTTON_TEMPLATES.WATER_TANK :
          bDev.id.includes('relay') ? BUTTON_TEMPLATES.SMART_RELAY :
          BUTTON_TEMPLATES.LIGHTING_4ZONE;

        this.devices.push({
          id: bDev.id,
          name: bDev.name || `Device ${bDev.id}`,
          ip_address: bDev.ip_address || '',
          status: bDev.status || 'ONLINE',
          role: bDev.role || 'Peripheral Node',
          is_locked: bDev.is_locked ?? false,
          allow_automation_when_locked: bDev.allow_automation_when_locked ?? true,
          created_at: bDev.created_at || new Date().toISOString(),
          buttons: bDev.buttons && Array.isArray(bDev.buttons) && bDev.buttons.length > 0
            ? bDev.buttons
            : JSON.parse(JSON.stringify(initialTpl.buttons)),
          automation_rules: bDev.automation_rules || [],
          description: bDev.description || 'Device synchronized from Home-IOT backend.',
        });
        changed = true;
      }
    }
    if (changed) this.save();
  }

  // CRUD Operations
  addDevice({
    id,
    name,
    ip_address,
    role = 'Node Controller',
    templateId = 'LIGHTING_4ZONE',
    description = '',
    allow_automation_when_locked = true,
  }) {
    const cleanId = id.trim().toLowerCase().replace(/[^a-z0-9_-]/g, '_');
    if (!cleanId) throw new Error('ID Perangkat tidak boleh kosong.');
    if (this.devices.some((d) => d.id === cleanId)) {
      throw new Error(`Device dengan ID '${cleanId}' sudah terdaftar.`);
    }

    const template = BUTTON_TEMPLATES[templateId] || BUTTON_TEMPLATES.LIGHTING_4ZONE;
    const newDevice = {
      id: cleanId,
      name: name.trim() || `Perangkat ${cleanId}`,
      ip_address: ip_address.trim() || '192.168.100.x',
      status: 'STANDBY',
      role,
      is_locked: false,
      allow_automation_when_locked: !!allow_automation_when_locked,
      created_at: new Date().toISOString(),
      buttons: JSON.parse(JSON.stringify(template.buttons)),
      automation_rules: [],
      description: description.trim(),
    };

    this.devices.push(newDevice);
    this.save();
    return newDevice;
  }

  updateDevice(id, data) {
    const dev = this.devices.find((d) => d.id === id);
    if (!dev) throw new Error(`Device '${id}' tidak ditemukan.`);
    if (dev.is_locked) {
      throw new Error(`Device '${dev.name}' sedang TERKUNCI (LOCKED). Buka kunci terlebih dahulu untuk mengedit.`);
    }

    if (data.name !== undefined) dev.name = data.name.trim();
    if (data.ip_address !== undefined) dev.ip_address = data.ip_address.trim();
    if (data.role !== undefined) dev.role = data.role.trim();
    if (data.description !== undefined) dev.description = data.description.trim();
    if (data.allow_automation_when_locked !== undefined) {
      dev.allow_automation_when_locked = !!data.allow_automation_when_locked;
    }
    if (data.status !== undefined) dev.status = data.status;

    this.save();
    return dev;
  }

  deleteDevice(id) {
    const dev = this.devices.find((d) => d.id === id);
    if (!dev) throw new Error(`Device '${id}' tidak ditemukan.`);
    if (dev.is_locked) {
      throw new Error(`Device '${dev.name}' sedang TERKUNCI (LOCKED). Perangkat terkunci tidak dapat dihapus.`);
    }
    if (id === 'esp32_master') {
      throw new Error(`ESP32 Master Node adalah gateway utama dan tidak dapat dihapus dari sistem.`);
    }

    this.devices = this.devices.filter((d) => d.id !== id);
    if (this.activeDeviceId === id) {
      this.activeDeviceId = 'esp32_master';
    }
    this.save();
  }

  toggleLock(id) {
    const dev = this.devices.find((d) => d.id === id);
    if (!dev) throw new Error(`Device '${id}' tidak ditemukan.`);
    dev.is_locked = !dev.is_locked;
    this.save();
    toggleDeviceLockApi(id, dev.is_locked).catch((e) =>
      console.warn(`Failed to sync lock state to backend for ${id}:`, e)
    );
    return dev.is_locked;
  }

  // Button Management
  toggleButton(deviceId, buttonId) {
    const dev = this.devices.find((d) => d.id === deviceId);
    if (!dev) return;
    if (dev.is_locked) {
      throw new Error(`Device '${dev.name}' dalam keadaan TERKUNCI. Konfigurasi tombol tidak dapat diubah.`);
    }

    const btn = dev.buttons.find((b) => b.id === buttonId);
    if (btn) {
      btn.enabled = !btn.enabled;
      this.save();
      updateDeviceButtonsApi(deviceId, dev.buttons).catch((e) =>
        console.warn(`Failed to sync buttons to backend for ${deviceId}:`, e)
      );
    }
  }

  addButton(deviceId, btnConfig) {
    const dev = this.devices.find((d) => d.id === deviceId);
    if (!dev) throw new Error(`Device '${deviceId}' tidak ditemukan.`);
    if (dev.is_locked) {
      throw new Error(`Device '${dev.name}' dalam keadaan TERKUNCI. Tidak dapat menambah tombol.`);
    }

    const cleanId = (btnConfig.id || `btn_${Date.now()}`).trim().toLowerCase().replace(/[^a-z0-9_-]/g, '_');
    if (dev.buttons.some((b) => b.id === cleanId)) {
      throw new Error(`Tombol dengan ID '${cleanId}' sudah ada pada perangkat ini.`);
    }

    const newBtn = {
      id: cleanId,
      label: btnConfig.label.trim() || 'Tombol Kustom',
      command: btnConfig.command.trim() || 'set_lamp',
      room: btnConfig.room ? btnConfig.room.trim() : undefined,
      channel: btnConfig.channel ? Number(btnConfig.channel) : undefined,
      valve_id: btnConfig.valve_id ? btnConfig.valve_id.trim() : undefined,
      type: btnConfig.type || 'toggle',
      state_key: btnConfig.state_key ? btnConfig.state_key.trim() : undefined,
      danger: !!btnConfig.danger,
      enabled: true,
    };

    dev.buttons.push(newBtn);
    this.save();
    updateDeviceButtonsApi(deviceId, dev.buttons).catch((e) =>
      console.warn(`Failed to sync buttons to backend for ${deviceId}:`, e)
    );
    return newBtn;
  }

  updateButton(deviceId, buttonId, btnConfig) {
    const dev = this.devices.find((d) => d.id === deviceId);
    if (!dev) throw new Error(`Device '${deviceId}' tidak ditemukan.`);
    if (dev.is_locked) {
      throw new Error(`Device '${dev.name}' dalam keadaan TERKUNCI. Tidak dapat mengedit tombol.`);
    }

    const btn = dev.buttons.find((b) => b.id === buttonId);
    if (!btn) throw new Error(`Tombol '${buttonId}' tidak ditemukan.`);

    if (btnConfig.label !== undefined) btn.label = btnConfig.label.trim();
    if (btnConfig.command !== undefined) btn.command = btnConfig.command.trim();
    if (btnConfig.room !== undefined) btn.room = btnConfig.room ? btnConfig.room.trim() : undefined;
    if (btnConfig.channel !== undefined) btn.channel = btnConfig.channel ? Number(btnConfig.channel) : undefined;
    if (btnConfig.valve_id !== undefined) btn.valve_id = btnConfig.valve_id ? btnConfig.valve_id.trim() : undefined;
    if (btnConfig.type !== undefined) btn.type = btnConfig.type;
    if (btnConfig.state_key !== undefined) btn.state_key = btnConfig.state_key ? btnConfig.state_key.trim() : undefined;
    if (btnConfig.danger !== undefined) btn.danger = !!btnConfig.danger;
    if (btnConfig.enabled !== undefined) btn.enabled = !!btnConfig.enabled;

    this.save();
    updateDeviceButtonsApi(deviceId, dev.buttons).catch((e) =>
      console.warn(`Failed to sync buttons to backend for ${deviceId}:`, e)
    );
    return btn;
  }

  deleteButton(deviceId, buttonId) {
    const dev = this.devices.find((d) => d.id === deviceId);
    if (!dev) throw new Error(`Device '${deviceId}' tidak ditemukan.`);
    if (dev.is_locked) {
      throw new Error(`Device '${dev.name}' dalam keadaan TERKUNCI. Tidak dapat menghapus tombol.`);
    }

    dev.buttons = dev.buttons.filter((b) => b.id !== buttonId);
    this.save();
    updateDeviceButtonsApi(deviceId, dev.buttons).catch((e) =>
      console.warn(`Failed to sync buttons to backend for ${deviceId}:`, e)
    );
  }
}

export const deviceStore = new DeviceStore();
