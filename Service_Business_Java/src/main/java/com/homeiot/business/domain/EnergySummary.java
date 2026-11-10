package com.homeiot.business.domain;

import com.fasterxml.jackson.annotation.JsonProperty;

/**
 * Immutable daily energy summary and PLN cost estimation record.
 */
public record EnergySummary(
    @JsonProperty("device_id") String deviceId,
    @JsonProperty("pln_contract_limit_watts") float plnContractLimitWatts,
    @JsonProperty("tariff_rate_idr_per_kwh") float tariffRateIdrPerKwh,
    @JsonProperty("daily_pln_kwh") float dailyPlnKwh,
    @JsonProperty("daily_pln_cost_idr") float dailyPlnCostIdr,
    @JsonProperty("daily_solar_yield_kwh") float dailySolarYieldKwh,
    @JsonProperty("daily_solar_savings_idr") float dailySolarSavingsIdr,
    @JsonProperty("solar_self_sufficiency_pct") float solarSelfSufficiencyPct,
    @JsonProperty("current_load_watts") float currentLoadWatts,
    @JsonProperty("current_solar_watts") float currentSolarWatts,
    @JsonProperty("system_status") String systemStatus
) {
    public static EnergySummary initial(float limitWatts, float tariffRate) {
        return new EnergySummary(
                "esp32_master", limitWatts, tariffRate, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, "INITIALIZING"
        );
    }
}
