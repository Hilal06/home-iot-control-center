package com.homeiot.business.rules;

import com.homeiot.business.domain.TelemetryPayload;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Service;

@Service
public class SafetyRuleEngine {

    private static final Logger log = LoggerFactory.getLogger(SafetyRuleEngine.class);

    @Value("${iot.rules.pln-limit-watts:900.0}")
    private float plnLimitWatts;

    @Value("${iot.rules.pln-warning-threshold:860.0}")
    private float plnWarningThreshold;

    @Value("${iot.rules.gas-ppm-threshold:300}")
    private int gasPpmThreshold;

    private volatile RuleEvaluationResult latestResult = RuleEvaluationResult.normal();

    public RuleEvaluationResult evaluate(TelemetryPayload data) {
        boolean gasHazard = data.gasPpmVal() > gasPpmThreshold;
        boolean waterLeak = data.waterLeakVal();
        boolean plnOverload = data.housePowerWVal() > plnWarningThreshold;
        boolean solarSurplus = data.pvPowerWattsVal() > (data.housePowerWVal() + 300.0f) && data.upsBatteryPctVal() > 95.0f;

        StringBuilder alertBuilder = new StringBuilder();

        if (gasHazard) {
            alertBuilder.append(String.format("[BAHAYA GAS] Konsentrasi %d ppm melebihi batas aman %d ppm! ",
                    data.gasPpmVal(), gasPpmThreshold));
            log.error("SAFETY INTERLOCK: Kebocoran Gas Terdeteksi: {} ppm", data.gasPpmVal());
        }

        if (waterLeak) {
            alertBuilder.append("[BAHAYA AIR] Sensor mendeteksi kebocoran air aktif! ");
            log.error("SAFETY INTERLOCK: Kebocoran Air Terdeteksi di fasilitas!");
        }

        if (plnOverload) {
            alertBuilder.append(String.format("[PERINGATAN OVERLOAD] Beban rumah %.1fW mendekati batas PLN %.0fW! ",
                    data.housePowerWVal(), plnLimitWatts));
            log.warn("OVERLOAD GUARD: Beban rumah {}W melebihi ambang batas {}W (Limit PLN {}W)",
                    data.housePowerWVal(), plnWarningThreshold, plnLimitWatts);
        }

        if (solarSurplus) {
            log.info("SOLAR SURPLUS: Pembangkitan surya %.1fW > Beban %.1fW & Baterai %.1f%%",
                    data.pvPowerWattsVal(), data.housePowerWVal(), data.upsBatteryPctVal());
        }

        String alertMessage = alertBuilder.length() > 0 ? alertBuilder.toString().trim() : "NORMAL";
        RuleEvaluationResult result = new RuleEvaluationResult(plnOverload, gasHazard, waterLeak, solarSurplus, alertMessage);
        this.latestResult = result;
        return result;
    }

    public RuleEvaluationResult getLatestResult() { return latestResult; }
    public float getPlnLimitWatts() { return plnLimitWatts; }
    public float getPlnWarningThreshold() { return plnWarningThreshold; }
    public int getGasPpmThreshold() { return gasPpmThreshold; }
}
