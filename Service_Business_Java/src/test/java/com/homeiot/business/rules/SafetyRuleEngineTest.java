package com.homeiot.business.rules;

import com.homeiot.business.domain.TelemetryPayload;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.springframework.test.util.ReflectionTestUtils;

import static org.junit.jupiter.api.Assertions.*;

public class SafetyRuleEngineTest {

    private SafetyRuleEngine ruleEngine;

    @BeforeEach
    public void setup() {
        ruleEngine = new SafetyRuleEngine();
        ReflectionTestUtils.setField(ruleEngine, "plnLimitWatts", 900.0f);
        ReflectionTestUtils.setField(ruleEngine, "plnWarningThreshold", 860.0f);
        ReflectionTestUtils.setField(ruleEngine, "gasPpmThreshold", 300);
    }

    @Test
    public void testNormalOperation() {
        TelemetryPayload payload = TelemetryPayload.ofTest(350.0f, 120, false);
        RuleEvaluationResult result = ruleEngine.evaluate(payload);

        assertFalse(result.isPlnOverloadTriggered());
        assertFalse(result.isGasHazardTriggered());
        assertFalse(result.isWaterLeakTriggered());
        assertEquals("NORMAL", result.alertMessage());
    }

    @Test
    public void testPlnOverloadTriggered() {
        TelemetryPayload payload = TelemetryPayload.ofTest(875.0f, 100, false); // > 860.0W threshold
        RuleEvaluationResult result = ruleEngine.evaluate(payload);

        assertTrue(result.isPlnOverloadTriggered());
        assertTrue(result.alertMessage().contains("[PERINGATAN OVERLOAD]"));
    }

    @Test
    public void testGasLeakHazardTriggered() {
        TelemetryPayload payload = TelemetryPayload.ofTest(200.0f, 450, false); // > 300 ppm threshold
        RuleEvaluationResult result = ruleEngine.evaluate(payload);

        assertTrue(result.isGasHazardTriggered());
        assertTrue(result.alertMessage().contains("[BAHAYA GAS]"));
    }
}
