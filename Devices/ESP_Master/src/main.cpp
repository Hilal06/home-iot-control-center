#include "web_ui.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WebServer.h>
#include <WiFi.h>
#include <stdarg.h>

// ==========================================
// CONFIGURATION & CREDENTIALS
// Auto-loaded from root '.env' via PlatformIO extra_scripts,
// or from an optional local 'include/secrets.h'.
// ==========================================
#if __has_include("secrets.h")
  #include "secrets.h"
#endif

#ifndef CONFIG_DEVICE_ID
  #define CONFIG_DEVICE_ID "esp32_master"
#endif
#ifndef CONFIG_WIFI_SSID
  #define CONFIG_WIFI_SSID "Your_WiFi_SSID"
#endif
#ifndef CONFIG_WIFI_PASSWORD
  #define CONFIG_WIFI_PASSWORD "Your_WiFi_Password"
#endif
#ifndef CONFIG_MQTT_SERVER
  #define CONFIG_MQTT_SERVER "192.168.1.100"
#endif
#ifndef CONFIG_MQTT_PORT
  #define CONFIG_MQTT_PORT 1883
#endif

const char *DEVICE_ID = CONFIG_DEVICE_ID;
const char *WIFI_SSID = CONFIG_WIFI_SSID;
const char *WIFI_PASSWORD = CONFIG_WIFI_PASSWORD;
const char *MQTT_SERVER = CONFIG_MQTT_SERVER;
const int MQTT_PORT = CONFIG_MQTT_PORT;

// Telemetry Topics
const char *TELEMETRY_TOPIC = "home/sensors/telemetry";
const char *DEVICE_TELEMETRY_TOPIC = "home/sensors/esp32_master/telemetry";

// Actuator Command Topics (Subscribed to both legacy master topic and
// device-specific route)
const char *COMMAND_TOPIC = "home/actuators/control";
const char *DEVICE_COMMAND_TOPIC = "home/actuators/esp32_master/control";

// Reconnect intervals
const unsigned long MQTT_RETRY_INTERVAL_MS = 5000;
const unsigned long WIFI_RETRY_INTERVAL_MS = 10000;
const unsigned long WIFI_SETUP_TIMEOUT_MS = 15000;

// ==========================================
// SYSTEM & HARDWARE STATES
// ==========================================

// 11 Lamp Zones Structure
struct LampZone {
  const char *id;
  const char *name;
  bool state;
};

LampZone lampZones[] = {{"living_room", "Living Room", false},
                        {"main_room", "Main Room", false},
                        {"garage", "Garage Lamp", false},
                        {"kitchen", "Kitchen", false},
                        {"stair_garage", "Stair Garage", false},
                        {"stair_fl2", "Stair Lt 2", false},
                        {"yard_front", "Halaman Depan", false},
                        {"yard_side", "Halaman Samping", false},
                        {"yard_back", "Halaman Belakang", false},
                        {"server_room", "Server Room", false},
                        {"electrical_room", "Electrical Room", false}};
const int NUM_LAMPS = sizeof(lampZones) / sizeof(lampZones[0]);

int findLampIndex(const char *key) {
  if (!key)
    return -1;
  for (int i = 0; i < NUM_LAMPS; i++) {
    if (strcasecmp(lampZones[i].id, key) == 0)
      return i;
  }
  // Aliases support
  if (strcasecmp(key, "stair_lt2") == 0 || strcasecmp(key, "stair2") == 0)
    return 5;
  if (strcasecmp(key, "front_yard") == 0)
    return 6;
  if (strcasecmp(key, "side_yard") == 0)
    return 7;
  if (strcasecmp(key, "back_yard") == 0 || strcasecmp(key, "backyard") == 0)
    return 8;
  // Dynamic zone numbers (zone_1 ... zone_11)
  if (strncasecmp(key, "zone_", 5) == 0) {
    int z = atoi(key + 5);
    if (z >= 1 && z <= NUM_LAMPS) {
      return z - 1;
    }
  }
  return -1;
}

// Actuators & Switches
bool garageDoorOpen = false;
bool kitchenBreakerState = true;
int hvacTargetTemp = 22;
bool acPower = false;
int telemetryInterval = 1000;

bool solarChargeEnabled = true;
// Off-grid Solar Inverter rated for ~12.6 m2 array (3m x 4.2m ≈ 2400 Wp)
int solarInverterMaxWatts = 2400;

// Household Grid Capacity (PLN R-1 900VA / 900W limit)
const float PLN_METER_MAX_WATTS = 900.0f;

// 8 Relays Channel
bool relayStates[8] = {false, false, false, false, false, false, false, false};

// Valves Structure (Water, Gas, Tank bypass)
struct ValveZone {
  const char *id;
  const char *name;
  bool state;
};

ValveZone valveZones[] = {{"main_water", "Main Water Supply", true},
                          {"garden_valve", "Garden Irrigation", false},
                          {"kitchen_gas", "Kitchen Gas Supply", true},
                          {"tank_bypass", "Water Tank Bypass", false}};
const int NUM_VALVES = sizeof(valveZones) / sizeof(valveZones[0]);

// Safety & Emergency State
bool emergencyAlarmActive = false;
bool emergencyCutoffActive = false;
char lastEmergencyReason[64] = "None";

// Network & Diagnostic tracking
unsigned long lastTelemetryMillis = 0;
unsigned long lastMqttRetryMillis = 0;
unsigned long lastWifiRetryMillis = 0;
int mqttRetryAttempts = 0;
bool wifiApFallbackActive = false;

// Snapshot of latest telemetry for Web UI (Calibrated for PLN 900W & 2400Wp
// Off-grid Solar)
struct TelemetrySnapshot {
  float pv_array_voltage = 68.5f;
  float pv_array_current = 12.4f;
  float pv_power_watts = 850.0f;
  float solar_yield_kwh = 7.45f;
  float panel_temp_c = 38.5f;
  float mains_voltage = 223.5f;
  float house_power_w = 340.0f; // Sesuai profil daya rumah tangga 900W
  float ups_battery_pct = 98.0f;
  float ups_load_pct = 14.2f; // Beban terhadap inverter off-grid 2400W
  float rack_temp_c = 24.2f;
  float temp_outdoor_c = 29.0f;
  float humidity_pct = 56.0f;
  unsigned long timestamp = 0;
} latestTelemetry;

// ==========================================
// IN-MEMORY CONSOLE LOG BUFFER
// ==========================================
const int LOG_BUFFER_SIZE = 40;
struct LogEntry {
  unsigned long timestamp_ms;
  char level[8]; // INFO, WARN, ERROR, ACTION
  char message[96];
};

LogEntry logBuffer[LOG_BUFFER_SIZE];
int logHead = 0;
int logCount = 0;

void formatTimeStr(unsigned long ms, char *buf, size_t maxLen) {
  unsigned long sec = ms / 1000;
  unsigned int h = (sec / 3600) % 24;
  unsigned int m = (sec / 60) % 60;
  unsigned int s = sec % 60;
  snprintf(buf, maxLen, "%02u:%02u:%02u", h, m, s);
}

void appLog(const char *level, const char *format, ...) {
  char msg[96];
  va_list args;
  va_start(args, format);
  vsnprintf(msg, sizeof(msg), format, args);
  va_end(args);

  // Print to Serial
  char timeStr[16];
  formatTimeStr(millis(), timeStr, sizeof(timeStr));
  Serial.printf("[%s] [%s] %s\n", timeStr, level, msg);

  // Push to circular buffer
  LogEntry &entry = logBuffer[logHead];
  entry.timestamp_ms = millis();
  strncpy(entry.level, level, sizeof(entry.level) - 1);
  entry.level[sizeof(entry.level) - 1] = '\0';
  strncpy(entry.message, msg, sizeof(entry.message) - 1);
  entry.message[sizeof(entry.message) - 1] = '\0';

  logHead = (logHead + 1) % LOG_BUFFER_SIZE;
  if (logCount < LOG_BUFFER_SIZE) {
    logCount++;
  }
}

// Helper function for random values
float getRandomValue(float minVal, float maxVal) {
  return minVal + (float)random(0, 1000) / 1000.0f * (maxVal - minVal);
}

// Human-readable MQTT state
const char *getMqttStateDesc(int state) {
  switch (state) {
  case -4:
    return "CONNECTION_TIMEOUT";
  case -3:
    return "CONNECTION_LOST";
  case -2:
    return "CONNECT_FAILED (Broker Unreachable)";
  case -1:
    return "DISCONNECTED";
  case 0:
    return "CONNECTED";
  case 1:
    return "CONNECT_BAD_PROTOCOL";
  case 2:
    return "CONNECT_BAD_CLIENT_ID";
  case 3:
    return "CONNECT_UNAVAILABLE";
  case 4:
    return "CONNECT_BAD_CREDENTIALS";
  case 5:
    return "CONNECT_UNAUTHORIZED";
  default:
    return "UNKNOWN_ERROR";
  }
}

// ==========================================
// CLIENT INSTANCES
// ==========================================
WiFiClient espClient;
PubSubClient client(espClient);
WebServer server(80);

// ==========================================
// CENTRAL DISPATCHER & ACTIONS
// ==========================================

bool handleSolarControl(JsonObject payload, const char *source, String &err) {
  bool updated = false;
  if (payload.containsKey("charge_enabled")) {
    if (payload["charge_enabled"].is<bool>()) {
      solarChargeEnabled = payload["charge_enabled"].as<bool>();
      appLog("ACTION", "[%s] Solar Charger -> %s", source,
             solarChargeEnabled ? "ENABLED" : "DISABLED");
      updated = true;
    } else {
      err = "'charge_enabled' must be a boolean";
      return false;
    }
  }
  if (payload.containsKey("max_watts")) {
    if (payload["max_watts"].is<int>()) {
      int watts = payload["max_watts"].as<int>();
      if (watts >= 100 && watts <= 15000) {
        solarInverterMaxWatts = watts;
        appLog("ACTION", "[%s] Solar Inverter Max Output -> %d W", source,
               solarInverterMaxWatts);
        updated = true;
      } else {
        err = "'max_watts' out of range (100 - 15000 W)";
        return false;
      }
    } else {
      err = "'max_watts' must be an integer";
      return false;
    }
  }
  if (!updated) {
    err = "No recognized solar properties found";
    return false;
  }
  return true;
}

bool handleLampControl(JsonObject payload, const char *source, String &err) {
  if (!payload.containsKey("state") || !payload["state"].is<bool>()) {
    err = "Missing or invalid boolean 'state' in lamp control";
    return false;
  }
  bool targetState = payload["state"].as<bool>();

  // Check if target room/zone is specified
  const char *roomKey = nullptr;
  if (payload.containsKey("room") && payload["room"].is<const char *>()) {
    roomKey = payload["room"].as<const char *>();
  } else if (payload.containsKey("zone") &&
             payload["zone"].is<const char *>()) {
    roomKey = payload["zone"].as<const char *>();
  }

  // Master switch: ALL lamps
  if (roomKey && strcasecmp(roomKey, "all") == 0) {
    for (int i = 0; i < NUM_LAMPS; i++) {
      lampZones[i].state = targetState;
    }
    appLog("ACTION", "[%s] All 11 Lamps -> %s", source,
           targetState ? "ON" : "OFF");
    return true;
  }

  // Specific room
  if (roomKey) {
    int idx = findLampIndex(roomKey);
    if (idx >= 0) {
      lampZones[idx].state = targetState;
      appLog("ACTION", "[%s] Lamp [%s] -> %s", source, lampZones[idx].name,
             targetState ? "ON" : "OFF");
      return true;
    }
    err = String("Unknown lamp zone: ") + roomKey;
    return false;
  }

  // Default backwards-compatible fallback (Living Room)
  lampZones[0].state = targetState;
  appLog("ACTION", "[%s] Lamp [%s] -> %s", source, lampZones[0].name,
         targetState ? "ON" : "OFF");
  return true;
}

bool handleGarageControl(JsonObject payload, const char *source, String &err) {
  if (!payload.containsKey("state")) {
    err = "Missing 'state' in garage control";
    return false;
  }
  if (payload["state"].is<bool>()) {
    garageDoorOpen = payload["state"].as<bool>();
    appLog("ACTION", "[%s] Garage Door -> %s", source,
           garageDoorOpen ? "OPEN" : "CLOSED");
    return true;
  }
  if (payload["state"].is<const char *>()) {
    const char *action = payload["state"].as<const char *>();
    if (action != nullptr) {
      garageDoorOpen = (strcasecmp(action, "open") == 0);
      appLog("ACTION", "[%s] Garage Door -> %s", source,
             garageDoorOpen ? "OPEN" : "CLOSED");
      return true;
    }
  }
  err = "'state' must be boolean or string ('open'/'closed')";
  return false;
}

bool handleBreakerControl(JsonObject payload, const char *source, String &err) {
  if (!payload.containsKey("state") || !payload["state"].is<bool>()) {
    err = "'state' must be a boolean";
    return false;
  }
  kitchenBreakerState = payload["state"].as<bool>();
  appLog("ACTION", "[%s] Kitchen Breaker -> %s", source,
         kitchenBreakerState ? "ON" : "OFF");
  return true;
}

bool handleHVACControl(JsonObject payload, const char *source, String &err) {
  bool updated = false;
  if (payload.containsKey("power")) {
    if (payload["power"].is<bool>()) {
      acPower = payload["power"].as<bool>();
      appLog("ACTION", "[%s] HVAC Power -> %s", source, acPower ? "ON" : "OFF");
      updated = true;
    } else {
      err = "'power' must be a boolean";
      return false;
    }
  }
  if (payload.containsKey("target_temp")) {
    if (payload["target_temp"].is<int>()) {
      int temp = payload["target_temp"].as<int>();
      if (temp >= 16 && temp <= 32) {
        hvacTargetTemp = temp;
        appLog("ACTION", "[%s] HVAC Target Temp -> %d C", source,
               hvacTargetTemp);
        updated = true;
      } else {
        err = "'target_temp' out of range (16 - 32 C)";
        return false;
      }
    } else {
      err = "'target_temp' must be an integer";
      return false;
    }
  }
  if (!updated) {
    err = "No valid HVAC properties found ('power', 'target_temp')";
    return false;
  }
  return true;
}

bool handleConfigChange(JsonObject payload, const char *source, String &err) {
  if (payload.containsKey("telemetry_interval_ms")) {
    if (payload["telemetry_interval_ms"].is<int>()) {
      int interval = payload["telemetry_interval_ms"].as<int>();
      if (interval >= 500 && interval <= 60000) {
        telemetryInterval = interval;
        appLog("ACTION", "[%s] Telemetry interval set to %d ms", source,
               telemetryInterval);
        return true;
      }
      err = "'telemetry_interval_ms' out of bounds (500 - 60000 ms)";
      return false;
    }
    err = "'telemetry_interval_ms' must be an integer";
    return false;
  }
  err = "Missing 'telemetry_interval_ms'";
  return false;
}

bool handleRelayControl(JsonObject payload, const char *source, String &err) {
  if (!payload.containsKey("channel") || !payload["channel"].is<int>()) {
    err = "Missing or invalid integer 'channel'";
    return false;
  }
  if (!payload.containsKey("state") || !payload["state"].is<bool>()) {
    err = "Missing or invalid boolean 'state'";
    return false;
  }
  int channel = payload["channel"].as<int>();
  if (channel < 1 || channel > 32) {
    err = "'channel' out of range (1 - 32)";
    return false;
  }
  bool state = payload["state"].as<bool>();
  if (channel <= 8) {
    relayStates[channel - 1] = state;
  }
  appLog("ACTION", "[%s] Relay CH%d -> %s", source, channel,
         state ? "ON" : "OFF");
  return true;
}

bool handleValveControl(JsonObject payload, const char *source, String &err) {
  const char *valveId = nullptr;
  if (payload.containsKey("valve_id") &&
      payload["valve_id"].is<const char *>()) {
    valveId = payload["valve_id"].as<const char *>();
  } else if (payload.containsKey("target") &&
             payload["target"].is<const char *>()) {
    valveId = payload["target"].as<const char *>();
  }

  if (!valveId || strlen(valveId) == 0) {
    err = "Missing or empty 'valve_id'";
    return false;
  }
  if (!payload.containsKey("state") || !payload["state"].is<bool>()) {
    err = "Missing or invalid boolean 'state'";
    return false;
  }
  bool state = payload["state"].as<bool>();

  bool found = false;
  for (int i = 0; i < NUM_VALVES; i++) {
    if (strcasecmp(valveZones[i].id, valveId) == 0) {
      valveZones[i].state = state;
      appLog("ACTION", "[%s] Valve [%s] -> %s", source, valveZones[i].name,
             state ? "OPEN" : "CLOSED");
      found = true;
      break;
    }
  }

  if (!found) {
    appLog("ACTION", "[%s] Generic Valve [%s] -> %s", source, valveId,
           state ? "OPEN" : "CLOSED");
  }
  return true;
}

bool handleEmergencyTrigger(JsonObject payload, const char *source,
                            String &err) {
  bool alarm = false;
  if (payload.containsKey("alarm") && payload["alarm"].is<bool>()) {
    alarm = payload["alarm"].as<bool>();
  }

  bool cutoff = false;
  if (payload.containsKey("cutoff") && payload["cutoff"].is<bool>()) {
    cutoff = payload["cutoff"].as<bool>();
  }

  const char *reason = "Emergency Triggered";
  if (payload.containsKey("reason") && payload["reason"].is<const char *>()) {
    reason = payload["reason"].as<const char *>();
  }
  strncpy(lastEmergencyReason, reason, sizeof(lastEmergencyReason) - 1);
  lastEmergencyReason[sizeof(lastEmergencyReason) - 1] = '\0';

  emergencyAlarmActive = alarm;
  emergencyCutoffActive = cutoff;

  if (cutoff) {
    // Cut off high-power breaker and close safety valves
    kitchenBreakerState = false;
    for (int i = 0; i < NUM_VALVES; i++) {
      if (strcasecmp(valveZones[i].id, "kitchen_gas") == 0 ||
          strcasecmp(valveZones[i].id, "main_water") == 0) {
        valveZones[i].state = false;
      }
    }
    for (int i = 0; i < 8; i++) {
      relayStates[i] = false;
    }
  }

  appLog("WARN", "[%s] EMERGENCY: Alarm=%s Cutoff=%s Reason='%s'", source,
         alarm ? "ACTIVE" : "INACTIVE", cutoff ? "TRIPPED" : "NORMAL", reason);
  return true;
}

bool handleCustomCommand(JsonObject payload, const char *source, String &err) {
  if (!payload.containsKey("action") || !payload["action"].is<const char *>()) {
    err = "Missing or invalid 'action'";
    return false;
  }
  const char *action = payload["action"].as<const char *>();
  const char *target =
      payload.containsKey("target") && payload["target"].is<const char *>()
          ? payload["target"].as<const char *>()
          : "system";

  appLog("ACTION", "[%s] Custom Action '%s' executed on target '%s'", source,
         action, target);
  return true;
}

bool executeCommand(JsonObject root, String &err) {
  // 1. Device ID filtering: Ensure command is for esp32_master or broadcast
  if (root.containsKey("device_id") && root["device_id"].is<const char *>()) {
    const char *targetDev = root["device_id"].as<const char *>();
    if (targetDev && strlen(targetDev) > 0 &&
        strcasecmp(targetDev, DEVICE_ID) != 0 &&
        strcasecmp(targetDev, "all") != 0 &&
        strcasecmp(targetDev, "broadcast") != 0) {
      // Command targeted to a peripheral device (e.g. smart_lock, gas_sensor).
      // Safely ignore without error on ESP Master.
      return true;
    }
  }

  if (!root.containsKey("command") || !root["command"].is<const char *>()) {
    err = "Missing or invalid 'command' field";
    return false;
  }

  const char *command = root["command"];
  if (command == nullptr) {
    err = "'command' is null";
    return false;
  }

  // Extract source ("MANUAL", "SCHEDULE", "AUTOMATION", "DEVICE_TRIGGER",
  // "EMERGENCY")
  const char *source = "MANUAL";
  if (root.containsKey("source") && root["source"].is<const char *>()) {
    source = root["source"].as<const char *>();
  }

  if (strcmp(command, "set_solar") == 0)
    return handleSolarControl(root, source, err);
  if (strcmp(command, "set_lamp") == 0)
    return handleLampControl(root, source, err);
  if (strcmp(command, "set_garage") == 0)
    return handleGarageControl(root, source, err);
  if (strcmp(command, "set_breaker") == 0)
    return handleBreakerControl(root, source, err);
  if (strcmp(command, "set_hvac") == 0)
    return handleHVACControl(root, source, err);
  if (strcmp(command, "set_config") == 0)
    return handleConfigChange(root, source, err);
  if (strcmp(command, "set_relay") == 0)
    return handleRelayControl(root, source, err);
  if (strcmp(command, "set_valve") == 0)
    return handleValveControl(root, source, err);
  if (strcmp(command, "trigger_emergency") == 0)
    return handleEmergencyTrigger(root, source, err);
  if (strcmp(command, "custom") == 0)
    return handleCustomCommand(root, source, err);
  if (strcmp(command, "ping") == 0) {
    appLog("INFO", "[%s] Ping probe received. Node '%s' online.", source,
           DEVICE_ID);
    return true;
  }
  if (strcmp(command, "reboot_esp") == 0) {
    appLog("WARN", "[%s] System Reboot requested via command.", source);
    return true;
  }

  err = String("Unknown command: ") + command;
  return false;
}

// MQTT callback with thorough error handling
void callback(char *topic, byte *payload, unsigned int length) {
  if (length == 0 || length > 1536) {
    appLog("ERROR", "Invalid payload length received: %u bytes", length);
    return;
  }

  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, payload, length);

  if (error) {
    appLog("ERROR", "MQTT JSON Deserialization failed: %s", error.c_str());
    return;
  }

  if (!doc.is<JsonObject>()) {
    appLog("ERROR", "MQTT JSON payload is not an object");
    return;
  }

  JsonObject root = doc.as<JsonObject>();
  String errReason;
  if (!executeCommand(root, errReason)) {
    appLog("ERROR", "Command execution failed: %s", errReason.c_str());
  } else if (root.containsKey("command") &&
             strcmp(root["command"], "reboot_esp") == 0) {
    delay(1000);
    ESP.restart();
  }
}

// ==========================================
// WEB SERVER API HANDLERS
// ==========================================

void handleApiStatus() {
  StaticJsonDocument<2048> doc;

  doc["device_id"] = DEVICE_ID;
  bool isWifiOk = (WiFi.status() == WL_CONNECTED);
  doc["wifi_connected"] = isWifiOk;
  doc["wifi_ssid"] =
      isWifiOk ? WiFi.SSID()
               : (wifiApFallbackActive ? "AP: ESP32_Master_Node" : WIFI_SSID);
  doc["ip_address"] =
      isWifiOk
          ? WiFi.localIP().toString()
          : (wifiApFallbackActive ? WiFi.softAPIP().toString() : "0.0.0.0");
  doc["wifi_rssi"] = isWifiOk ? WiFi.RSSI() : 0;

  doc["mqtt_connected"] = client.connected();
  doc["mqtt_server"] = MQTT_SERVER;
  doc["mqtt_port"] = MQTT_PORT;
  doc["mqtt_state_code"] = client.state();
  doc["mqtt_state_desc"] = getMqttStateDesc(client.state());
  doc["mqtt_retries"] = mqttRetryAttempts;

  doc["uptime_sec"] = millis() / 1000;
  doc["free_heap_kb"] = ESP.getFreeHeap() / 1024;

  // 11 Lamp states
  JsonObject lampsObj = doc.createNestedObject("lamps");
  for (int i = 0; i < NUM_LAMPS; i++) {
    lampsObj[lampZones[i].id] = lampZones[i].state;
  }
  doc["lamp_status"] = lampZones[0].state; // backwards compat for living room

  // Actuator states
  doc["garage_status"] = garageDoorOpen;
  doc["kitchen_breaker"] = kitchenBreakerState;
  doc["hvac_power"] = acPower;
  doc["hvac_target_temp"] = hvacTargetTemp;
  doc["solar_charge_enabled"] = solarChargeEnabled;
  doc["solar_max_watts"] = solarInverterMaxWatts;
  doc["telemetry_interval_ms"] = telemetryInterval;

  // Relays and Valves
  JsonArray relaysArr = doc.createNestedArray("relays");
  for (int i = 0; i < 8; i++) {
    relaysArr.add(relayStates[i]);
  }

  JsonObject valvesObj = doc.createNestedObject("valves");
  for (int i = 0; i < NUM_VALVES; i++) {
    valvesObj[valveZones[i].id] = valveZones[i].state;
  }

  // Safety & Emergency
  doc["emergency_alarm"] = emergencyAlarmActive;
  doc["emergency_cutoff"] = emergencyCutoffActive;
  doc["last_emergency_reason"] = lastEmergencyReason;

  // Latest telemetry snapshot
  JsonObject t = doc.createNestedObject("last_telemetry");
  t["pv_array_voltage"] = latestTelemetry.pv_array_voltage;
  t["pv_array_current"] = latestTelemetry.pv_array_current;
  t["pv_power_watts"] = latestTelemetry.pv_power_watts;
  t["solar_yield_kwh"] = latestTelemetry.solar_yield_kwh;
  t["panel_temp_c"] = latestTelemetry.panel_temp_c;
  t["mains_voltage"] = latestTelemetry.mains_voltage;
  t["house_power_w"] = latestTelemetry.house_power_w;
  t["ups_battery_pct"] = latestTelemetry.ups_battery_pct;
  t["ups_load_pct"] = latestTelemetry.ups_load_pct;
  t["rack_temp_c"] = latestTelemetry.rack_temp_c;
  t["temp_outdoor_c"] = latestTelemetry.temp_outdoor_c;
  t["humidity_pct"] = latestTelemetry.humidity_pct;

  if (latestTelemetry.timestamp > 0) {
    doc["last_telemetry_age_s"] = (millis() - latestTelemetry.timestamp) / 1000;
  } else {
    doc["last_telemetry_age_s"] = -1;
  }

  String res;
  serializeJson(doc, res);
  server.send(200, "application/json", res);
}

void handleApiLogs() {
  DynamicJsonDocument doc(5120);
  JsonArray arr = doc.to<JsonArray>();

  int startIdx = (logCount < LOG_BUFFER_SIZE) ? 0 : logHead;
  for (int i = 0; i < logCount; i++) {
    int idx = (startIdx + i) % LOG_BUFFER_SIZE;
    JsonObject item = arr.createNestedObject();

    char timeStr[16];
    formatTimeStr(logBuffer[idx].timestamp_ms, timeStr, sizeof(timeStr));
    item["time"] = timeStr;
    item["level"] = logBuffer[idx].level;
    item["msg"] = logBuffer[idx].message;
  }

  String res;
  serializeJson(doc, res);
  server.send(200, "application/json", res);
}

void handleApiControl() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json",
                "{\"error\":\"Missing request body\"}");
    return;
  }

  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, server.arg("plain"));
  if (error) {
    server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }

  JsonObject root = doc.as<JsonObject>();
  String errReason;
  if (executeCommand(root, errReason)) {
    server.send(200, "application/json", "{\"success\":true}");
  } else {
    StaticJsonDocument<256> errDoc;
    errDoc["success"] = false;
    errDoc["error"] = errReason;
    String res;
    serializeJson(errDoc, res);
    server.send(400, "application/json", res);
  }
}

void handleApiRestart() {
  server.send(200, "application/json",
              "{\"success\":true,\"message\":\"Restarting node...\"}");
  appLog("WARN", "Node restart triggered via Web UI.");
  delay(1000);
  ESP.restart();
}

// ==========================================
// NETWORK CONNECTION MANAGEMENT
// ==========================================

void setup_wifi() {
  appLog("INFO", "Initializing WiFi connection to '%s'...", WIFI_SSID);
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - startAttemptTime < WIFI_SETUP_TIMEOUT_MS) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    appLog("INFO", "WiFi connected! IP: %s (RSSI: %d dBm)",
           WiFi.localIP().toString().c_str(), WiFi.RSSI());
  } else {
    wifiApFallbackActive = true;
    WiFi.softAP("ESP32_Master_Node", "admin1234");
    appLog("WARN",
           "WiFi STA timeout! Fallback AP 'ESP32_Master_Node' active at %s",
           WiFi.softAPIP().toString().c_str());
  }
}

void manageWifi() {
  if (WiFi.status() != WL_CONNECTED) {
    unsigned long now = millis();
    if (now - lastWifiRetryMillis > WIFI_RETRY_INTERVAL_MS) {
      lastWifiRetryMillis = now;
      appLog("WARN", "WiFi STA disconnected. Attempting reconnect to '%s'...",
             WIFI_SSID);
      WiFi.reconnect();
    }
  }
}

void manageMqtt() {
  if (WiFi.status() != WL_CONNECTED) {
    return; // Cannot connect without active network
  }

  if (!client.connected()) {
    unsigned long now = millis();
    if (now - lastMqttRetryMillis > MQTT_RETRY_INTERVAL_MS) {
      lastMqttRetryMillis = now;
      mqttRetryAttempts++;

      String clientId = "ESP32_Infra_Node_" + WiFi.macAddress().substring(12);
      const char *willTopic = "home/esp32_master/status";
      const char *willMessage =
          "{\"status\":\"OFFLINE\",\"device_id\":\"esp32_master\"}";
      uint8_t willQos = 1;
      boolean willRetain = true;

      appLog("WARN", "MQTT reconnect #%d to %s:%d (ClientID: %s)...",
             mqttRetryAttempts, MQTT_SERVER, MQTT_PORT, clientId.c_str());

      if (client.connect(clientId.c_str(), willTopic, willQos, willRetain,
                         willMessage)) {
        appLog("INFO", "MQTT connected! Subscribing to %s & %s", COMMAND_TOPIC,
               DEVICE_COMMAND_TOPIC);

        // Publish ONLINE status immediately with retain flag
        const char *onlineMessage =
            "{\"status\":\"ONLINE\",\"device_id\":\"esp32_master\"}";
        client.publish(willTopic, onlineMessage, true);

        if (!client.subscribe(COMMAND_TOPIC)) {
          appLog("ERROR", "Failed to subscribe to topic: %s", COMMAND_TOPIC);
        }
        if (!client.subscribe(DEVICE_COMMAND_TOPIC)) {
          appLog("ERROR", "Failed to subscribe to topic: %s",
                 DEVICE_COMMAND_TOPIC);
        }
        mqttRetryAttempts = 0;
      } else {
        int state = client.state();
        appLog("ERROR", "MQTT connect failed: %s (code %d)",
               getMqttStateDesc(state), state);
      }
    }
  } else {
    client.loop();
  }
}

// ==========================================
// STATUS LED MANAGEMENT (GPIO 2 / D2)
// ==========================================
// Pola Indikator LED D2:
// 1. WiFi Gagal / Putus: Lampu MATI (LOW)
// 2. WiFi OK, Server Putus: Nyala 2 kali cepat, delay 2 detik, berulang (Siklus
// 2300ms)
// 3. WiFi OK, Server OK: Nyala tiap 3 detik (Heartbeat pulse 200ms, Siklus
// 3000ms)
const int STATUS_LED_PIN = 2; // Pin D2 pada ESP32 DevKit V1 (Onboard Blue LED)

void updateStatusLed() {
  unsigned long now = millis();
  static bool ledState = false;

  // 1. Ketika WiFi gagal / belum terkoneksi -> Lampu MATI
  if (WiFi.status() != WL_CONNECTED) {
    if (ledState) {
      ledState = false;
      digitalWrite(STATUS_LED_PIN, LOW);
    }
    return;
  }

  // 2. Ketika WiFi berhasil terkoneksi DAN Server berhasil terkoneksi -> Nyala
  // tiap 3 detik
  if (client.connected()) {
    unsigned long cycle = now % 3000; // Siklus 3 detik (3000ms)
    bool shouldBeOn = (cycle < 200);  // Nyala 200ms tiap 3 detik
    if (ledState != shouldBeOn) {
      ledState = shouldBeOn;
      digitalWrite(STATUS_LED_PIN, ledState ? HIGH : LOW);
    }
    return;
  }

  // 3. Ketika WiFi berhasil terkoneksi NAMUN tidak terkoneksi ke Server:
  //    Menyala 2 kali cepat lalu delay 2 detik, lalu 2 kali cepat, dan terus
  //    berulang Siklus 2300ms:
  //    - 0 s/d 100ms   : ON (Kedip 1)
  //    - 100 s/d 200ms : OFF
  //    - 200 s/d 300ms : ON (Kedip 2)
  //    - 300 s/d 2300ms: OFF (Delay 2000ms / 2 detik)
  unsigned long cycle = now % 2300;
  bool shouldBeOn = (cycle < 100) || (cycle >= 200 && cycle < 300);
  if (ledState != shouldBeOn) {
    ledState = shouldBeOn;
    digitalWrite(STATUS_LED_PIN, ledState ? HIGH : LOW);
  }
}

// ==========================================
// MAIN SETUP & LOOP
// ==========================================

void setup() {
  Serial.begin(115200);
  delay(200);

  // Initialize status LED D2 (GPIO 2)
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);

  appLog("INFO", "ESP32 Master Node (%s) booting up...", DEVICE_ID);

  setup_wifi();

  // Configure MQTT client buffer and server
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setBufferSize(1536);
  client.setKeepAlive(15);
  client.setSocketTimeout(5);
  client.setCallback(callback);

  // Setup Web Server routes
  server.on("/", HTTP_GET,
            []() { server.send_P(200, "text/html", INDEX_HTML); });
  server.on("/api/status", HTTP_GET, handleApiStatus);
  server.on("/api/logs", HTTP_GET, handleApiLogs);
  server.on("/api/control", HTTP_POST, handleApiControl);
  server.on("/api/command", HTTP_POST,
            handleApiControl); // alias for versatility
  server.on("/api/restart", HTTP_POST, handleApiRestart);
  server.on("/api/telemetry", HTTP_GET, []() {
    StaticJsonDocument<1024> doc;
    doc["device_id"] = DEVICE_ID;
    doc["pv_power_watts"] = latestTelemetry.pv_power_watts;
    doc["mains_voltage"] = latestTelemetry.mains_voltage;
    doc["house_power_w"] = latestTelemetry.house_power_w;
    doc["ups_battery_pct"] = latestTelemetry.ups_battery_pct;
    doc["rack_temp_c"] = latestTelemetry.rack_temp_c;
    doc["rssi_dbm"] = WiFi.RSSI();
    String res;
    serializeJson(doc, res);
    server.send(200, "application/json", res);
  });
  server.onNotFound([]() {
    server.send(404, "application/json", "{\"error\":\"Endpoint not found\"}");
  });

  server.begin();
  appLog("INFO", "HTTP Web Server started on port 80.");
  if (WiFi.status() == WL_CONNECTED) {
    appLog("INFO", "Web UI Dashboard: http://%s",
           WiFi.localIP().toString().c_str());
  } else {
    appLog("INFO", "Web UI AP Dashboard: http://%s",
           WiFi.softAPIP().toString().c_str());
  }
}

void loop() {
  server.handleClient();
  manageWifi();
  manageMqtt();
  updateStatusLed();

  unsigned long now = millis();
  if (now - lastTelemetryMillis > (unsigned long)telemetryInterval) {
    lastTelemetryMillis = now;

    float dtSec = (float)telemetryInterval / 1000.0f;

    // 1. Off-Grid Solar Array & Battery Telemetry Simulation
    // Array Size: ~3.0m x 4.2m ≈ 12.6 m2 (~2400 Wp peak, ~6 panels @ 400Wp)
    float simulatedSolarWatts = 0.0f;
    if (solarChargeEnabled && kitchenBreakerState && !emergencyCutoffActive) {
      // Natural diurnal solar generation curve with subtle atmospheric drift
      float solarCycle =
          (sin((float)now / 16000.0f) + 1.0f) * 0.5f; // 0.0 to 1.0
      float targetWatts =
          300.0f + solarCycle * ((float)solarInverterMaxWatts - 450.0f);
      float jitter = getRandomValue(-15.0f, 15.0f);
      simulatedSolarWatts =
          constrain(targetWatts + jitter, 50.0f, (float)solarInverterMaxWatts);
    }

    latestTelemetry.pv_power_watts = simulatedSolarWatts;
    // Off-grid MPPT string voltage: 6x panel array (nominal string ~64V - 74V)
    latestTelemetry.pv_array_voltage =
        (simulatedSolarWatts > 0.0f)
            ? constrain(
                  72.0f -
                      (simulatedSolarWatts / (float)solarInverterMaxWatts) *
                          6.5f +
                      getRandomValue(-0.25f, 0.25f),
                  58.0f, 75.0f)
            : 64.0f;
    latestTelemetry.pv_array_current =
        (simulatedSolarWatts > 0.0f && latestTelemetry.pv_array_voltage > 1.0f)
            ? (simulatedSolarWatts / latestTelemetry.pv_array_voltage)
            : 0.0f;
    // Monotonically accumulating daily energy yield (typical ~8 - 11 kWh/day)
    latestTelemetry.solar_yield_kwh +=
        (simulatedSolarWatts * dtSec) / 3600000.0f;
    latestTelemetry.panel_temp_c = constrain(
        32.0f + (simulatedSolarWatts / (float)solarInverterMaxWatts) * 22.0f +
            getRandomValue(-0.3f, 0.3f),
        28.0f, 65.0f);

    // 2. Household Power Consumption Simulation (PLN 900W Grid Sizing)
    // Base standby load for 900W household (refrigerator inverter, wifi router,
    // chargers): ~120W - 190W
    float baseLoad = 150.0f + (sin((float)now / 8000.0f) * 35.0f);

    // Add HVAC consumption: In a 900W home, a low-watt 0.5 PK AC draws ~320W -
    // 360W
    if (acPower) {
      float hvacWatts =
          330.0f + (float)(26 - constrain(hvacTargetTemp, 16, 26)) * 10.0f;
      baseLoad += hvacWatts + getRandomValue(-10.0f, 10.0f);
    }

    // Add LED lighting load: ~9W per active zone for household LED bulbs (~100W
    // if all 11 zones active)
    int activeLamps = 0;
    for (int i = 0; i < NUM_LAMPS; i++) {
      if (lampZones[i].state)
        activeLamps++;
    }
    baseLoad += (float)activeLamps * 9.0f;

    // Safety trip handling: if breaker or emergency cutoff trips, drop to
    // minimal emergency standby (~45W)
    if (!kitchenBreakerState || emergencyCutoffActive) {
      baseLoad = 45.0f + getRandomValue(0.0f, 15.0f);
    }

    // Ensure total household load stays realistically within 900W PLN meter
    // capacity
    latestTelemetry.mains_voltage = 223.5f +
                                    (sin((float)now / 11000.0f) * 1.8f) +
                                    getRandomValue(-0.3f, 0.3f);
    latestTelemetry.house_power_w =
        constrain(baseLoad + getRandomValue(-6.0f, 6.0f), 40.0f, 880.0f);

    // 3. Off-Grid Battery Storage System (~4.8 kWh LiFePO4 for 900W household)
    // In daytime, solar power (up to 2400W) easily covers house load (150-700W)
    // and keeps battery topped up (>96%)
    if (emergencyCutoffActive) {
      latestTelemetry.ups_battery_pct = 92.0f;
    } else if (simulatedSolarWatts > latestTelemetry.house_power_w) {
      latestTelemetry.ups_battery_pct =
          constrain(97.0f + (sin((float)now / 30000.0f) * 2.5f) +
                        getRandomValue(-0.2f, 0.2f),
                    95.0f, 100.0f);
    } else {
      latestTelemetry.ups_battery_pct =
          constrain(88.0f + (sin((float)now / 25000.0f) * 4.0f) +
                        getRandomValue(-0.3f, 0.3f),
                    70.0f, 96.0f);
    }
    // Inverter load percentage relative to off-grid inverter capacity (2400W)
    latestTelemetry.ups_load_pct = constrain(
        (latestTelemetry.house_power_w / (float)solarInverterMaxWatts) * 100.0f,
        3.0f, 50.0f);

    // 3. Environmental & Diagnostics Telemetry Simulation
    latestTelemetry.rack_temp_c = 24.2f + (sin((float)now / 20000.0f) * 1.3f) +
                                  getRandomValue(-0.15f, 0.15f);
    latestTelemetry.temp_outdoor_c = 28.5f +
                                     (cos((float)now / 26000.0f) * 2.8f) +
                                     getRandomValue(-0.2f, 0.2f);
    latestTelemetry.humidity_pct =
        constrain(56.0f - (sin((float)now / 22000.0f) * 6.0f) +
                      getRandomValue(-0.8f, 0.8f),
                  30.0f, 90.0f);
    latestTelemetry.timestamp = now;

    // Publish to MQTT if connected
    if (client.connected()) {
      StaticJsonDocument<1280> doc;
      doc["device_id"] = DEVICE_ID;
      doc["pv_array_voltage"] = latestTelemetry.pv_array_voltage;
      doc["pv_array_current"] = latestTelemetry.pv_array_current;
      doc["pv_power_watts"] = latestTelemetry.pv_power_watts;
      doc["solar_yield_kwh"] = latestTelemetry.solar_yield_kwh;
      doc["panel_temp_c"] = latestTelemetry.panel_temp_c;
      doc["solar_charger_active"] = solarChargeEnabled;

      doc["mains_voltage"] = latestTelemetry.mains_voltage;
      doc["house_power_w"] = latestTelemetry.house_power_w;
      doc["ups_battery_pct"] = latestTelemetry.ups_battery_pct;
      doc["ups_load_pct"] = latestTelemetry.ups_load_pct;

      doc["rack_temp_c"] = latestTelemetry.rack_temp_c;
      doc["temp_outdoor_c"] = latestTelemetry.temp_outdoor_c;
      doc["humidity_pct"] = latestTelemetry.humidity_pct;
      doc["water_leak"] = false;

      doc["ping_gateway_ms"] = getRandomValue(1.8f, 4.5f);
      doc["rssi_dbm"] = (WiFi.status() == WL_CONNECTED) ? WiFi.RSSI() : -54;
      doc["esp_free_heap_kb"] = ESP.getFreeHeap() / 1024;

      // 11 Lamps in Telemetry
      JsonObject lampsObj = doc.createNestedObject("lamps");
      for (int i = 0; i < NUM_LAMPS; i++) {
        lampsObj[lampZones[i].id] = lampZones[i].state;
      }
      doc["lamp_status"] = lampZones[0].state; // backwards compat

      doc["garage_status"] = garageDoorOpen;
      doc["kitchen_breaker"] = kitchenBreakerState;
      doc["hvac_power"] = acPower;
      doc["hvac_target_temp"] = hvacTargetTemp;
      doc["emergency_alarm"] = emergencyAlarmActive;
      doc["emergency_cutoff"] = emergencyCutoffActive;

      char buffer[1280];
      size_t written = serializeJson(doc, buffer, sizeof(buffer));
      if (written == 0) {
        appLog("ERROR", "Telemetry serialization failed (buffer overflow)");
      } else {
        bool ok = client.publish(TELEMETRY_TOPIC, buffer);
        if (!ok) {
          appLog("ERROR", "MQTT publish failed to %s (%u bytes)",
                 TELEMETRY_TOPIC, written);
        } else {
          // Periodic heartbeat log every 5 seconds so transmission traffic is
          // visible in console
          static unsigned long lastTrafficLog = 0;
          if (now - lastTrafficLog >= 5000) {
            lastTrafficLog = now;
            appLog("INFO",
                   "TX Telemetry OK -> PV: %.1fW | House: %.1fW | V: %.1fV | "
                   "Temp: %.1fC",
                   latestTelemetry.pv_power_watts,
                   latestTelemetry.house_power_w, latestTelemetry.mains_voltage,
                   latestTelemetry.temp_outdoor_c);
          }
        }
      }
    }
  }
}
