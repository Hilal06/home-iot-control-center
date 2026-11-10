package com.homeiot.business.controller;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.homeiot.business.domain.EnergySummary;
import com.homeiot.business.rules.RuleEvaluationResult;
import com.homeiot.business.rules.SafetyRuleEngine;
import com.homeiot.business.service.EnergyAnalyticsService;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.util.Map;

@RestController
@RequestMapping("/api/v1/business")
public class AnalyticsController {

    private final EnergyAnalyticsService energyAnalyticsService;
    private final SafetyRuleEngine safetyRuleEngine;

    public AnalyticsController(EnergyAnalyticsService energyAnalyticsService, SafetyRuleEngine safetyRuleEngine) {
        this.energyAnalyticsService = energyAnalyticsService;
        this.safetyRuleEngine = safetyRuleEngine;
    }

    @GetMapping("/analytics/energy")
    public ResponseEntity<EnergySummary> getEnergySummary() {
        return ResponseEntity.ok(energyAnalyticsService.getSummary());
    }

    @GetMapping("/rules/status")
    public ResponseEntity<RuleStatusResponse> getRuleStatus() {
        RuleEvaluationResult res = safetyRuleEngine.getLatestResult();
        return ResponseEntity.ok(new RuleStatusResponse(
                safetyRuleEngine.getPlnLimitWatts(),
                safetyRuleEngine.getPlnWarningThreshold(),
                safetyRuleEngine.getGasPpmThreshold(),
                res.isPlnOverloadTriggered(),
                res.isGasHazardTriggered(),
                res.isWaterLeakTriggered(),
                res.isSolarSurplusTriggered(),
                res.alertMessage()
        ));
    }

    @PostMapping("/analytics/reset")
    public ResponseEntity<Map<String, String>> resetStats() {
        energyAnalyticsService.resetDailyStats();
        return ResponseEntity.ok(Map.of("message", "Statistik energi berhasil direset"));
    }

    public record RuleStatusResponse(
        @JsonProperty("pln_limit_watts") float plnLimitWatts,
        @JsonProperty("pln_warning_threshold_watts") float plnWarningThresholdWatts,
        @JsonProperty("gas_ppm_threshold") int gasPpmThreshold,
        @JsonProperty("pln_overload_triggered") boolean isPlnOverloadTriggered,
        @JsonProperty("gas_hazard_triggered") boolean isGasHazardTriggered,
        @JsonProperty("water_leak_triggered") boolean isWaterLeakTriggered,
        @JsonProperty("solar_surplus_triggered") boolean isSolarSurplusTriggered,
        @JsonProperty("alert_message") String alertMessage
    ) {}
}
