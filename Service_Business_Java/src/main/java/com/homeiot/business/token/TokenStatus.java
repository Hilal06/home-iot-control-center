package com.homeiot.business.token;

import com.fasterxml.jackson.annotation.JsonProperty;

/**
 * Immutable snapshot of prepaid PLN electricity token credit and burn-rate forecast.
 */
public record TokenStatus(
    @JsonProperty("remaining_kwh") float remainingKwh,
    @JsonProperty("remaining_idr") float remainingIdr,
    @JsonProperty("tariff_per_kwh") float tariffPerKwh,
    @JsonProperty("daily_burn_rate_kwh") float dailyBurnRateKwh,
    @JsonProperty("estimated_days_remaining") float estimatedDaysRemaining,
    @JsonProperty("is_low_balance") boolean isLowBalance,
    @JsonProperty("low_balance_threshold_kwh") float lowBalanceThresholdKwh
) {}

