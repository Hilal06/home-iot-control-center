package com.homeiot.business.domain;

import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import com.fasterxml.jackson.annotation.JsonProperty;

/**
 * Lean immutable telemetry payload containing only fields required for business logic.
 * Unknown JSON attributes from hardware devices are ignored automatically.
 */
@JsonIgnoreProperties(ignoreUnknown = true)
public record TelemetryPayload(
    @JsonProperty("device_id") String deviceId,
    @JsonProperty("timestamp") Long timestamp,
    @JsonProperty("pv_power_watts") Float pvPowerWatts,
    @JsonProperty("solar_yield_kwh") Float solarYieldKwh,
    @JsonProperty("house_power_w") Float housePowerW,
    @JsonProperty("ups_battery_pct") Float upsBatteryPct,
    @JsonProperty("gas_ppm") Integer gasPpm,
    @JsonProperty("water_leak") Boolean waterLeak
) {
    public float pvPowerWattsVal() { return pvPowerWatts != null ? pvPowerWatts : 0.0f; }
    public float solarYieldKwhVal() { return solarYieldKwh != null ? solarYieldKwh : 0.0f; }
    public float housePowerWVal() { return housePowerW != null ? housePowerW : 0.0f; }
    public float upsBatteryPctVal() { return upsBatteryPct != null ? upsBatteryPct : 98.0f; }
    public int gasPpmVal() { return gasPpm != null ? gasPpm : 0; }
    public boolean waterLeakVal() { return Boolean.TRUE.equals(waterLeak); }

    public static TelemetryPayload ofTest(float housePowerW, int gasPpm, boolean waterLeak) {
        return new TelemetryPayload("esp32_master", System.currentTimeMillis(), 0f, 0f, housePowerW, 100f, gasPpm, waterLeak);
    }
}
