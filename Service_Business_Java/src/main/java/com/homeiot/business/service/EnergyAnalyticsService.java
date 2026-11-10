package com.homeiot.business.service;

import com.homeiot.business.domain.EnergySummary;
import com.homeiot.business.domain.TelemetryPayload;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Service;

import java.util.concurrent.atomic.AtomicReference;

@Service
public class EnergyAnalyticsService {

    private static final Logger log = LoggerFactory.getLogger(EnergyAnalyticsService.class);

    @Value("${iot.rules.pln-limit-watts:900.0}")
    private float plnLimitWatts;

    @Value("${iot.rules.pln-tariff-per-kwh:1352.0}")
    private float tariffRateIdr;

    private float accumulatedPlnKwh = 3.85f; // Initial daily base estimation
    private float accumulatedSolarYieldKwh = 7.45f;
    private long lastCalculationMillis = System.currentTimeMillis();

    private final AtomicReference<EnergySummary> currentSummary = new AtomicReference<>(null);

    public synchronized void accumulate(TelemetryPayload payload) {
        long now = System.currentTimeMillis();
        long dtMillis = now - lastCalculationMillis;
        lastCalculationMillis = now;

        if (dtMillis > 0 && dtMillis < 60000) {
            float dtHours = dtMillis / 3600000.0f;
            float houseWatts = payload.housePowerWVal();
            float solarWatts = payload.pvPowerWattsVal();

            float netPlnWatts = Math.max(0.0f, houseWatts - solarWatts);
            accumulatedPlnKwh += (netPlnWatts * dtHours) / 1000.0f;

            if (payload.solarYieldKwhVal() > 0.0f) {
                accumulatedSolarYieldKwh = payload.solarYieldKwhVal();
            } else {
                accumulatedSolarYieldKwh += (solarWatts * dtHours) / 1000.0f;
            }
        }

        float totalEnergy = accumulatedPlnKwh + accumulatedSolarYieldKwh;
        float selfSufficiency = totalEnergy > 0 ? (accumulatedSolarYieldKwh / totalEnergy) * 100.0f : 0.0f;
        String systemStatus = payload.housePowerWVal() > plnLimitWatts ? "OVERLOAD_TRIP_RISK" : "OPTIMAL";
        String deviceId = payload.deviceId() != null ? payload.deviceId() : "esp32_master";

        currentSummary.set(new EnergySummary(
                deviceId,
                plnLimitWatts,
                tariffRateIdr,
                accumulatedPlnKwh,
                accumulatedPlnKwh * tariffRateIdr,
                accumulatedSolarYieldKwh,
                accumulatedSolarYieldKwh * tariffRateIdr,
                selfSufficiency,
                payload.housePowerWVal(),
                payload.pvPowerWattsVal(),
                systemStatus
        ));
    }

    public EnergySummary getSummary() {
        EnergySummary summary = currentSummary.get();
        if (summary == null) {
            summary = EnergySummary.initial(plnLimitWatts, tariffRateIdr);
            currentSummary.compareAndSet(null, summary);
        }
        return summary;
    }

    public void resetDailyStats() {
        accumulatedPlnKwh = 0.0f;
        accumulatedSolarYieldKwh = 0.0f;
        log.info("ENERGY ACCOUNTING: Statistik energi harian telah di-reset untuk siklus hari baru.");
    }
}
