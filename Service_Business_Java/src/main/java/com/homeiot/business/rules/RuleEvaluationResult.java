package com.homeiot.business.rules;

import com.fasterxml.jackson.annotation.JsonProperty;

/**
 * Immutable evaluation result produced by the SafetyRuleEngine.
 */
public record RuleEvaluationResult(
    @JsonProperty("pln_overload_triggered") boolean isPlnOverloadTriggered,
    @JsonProperty("gas_hazard_triggered") boolean isGasHazardTriggered,
    @JsonProperty("water_leak_triggered") boolean isWaterLeakTriggered,
    @JsonProperty("solar_surplus_triggered") boolean isSolarSurplusTriggered,
    @JsonProperty("alert_message") String alertMessage
) {
    public static RuleEvaluationResult normal() {
        return new RuleEvaluationResult(false, false, false, false, "NORMAL");
    }
}
