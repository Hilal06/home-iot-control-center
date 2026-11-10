package com.homeiot.business.consumer;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.homeiot.business.device.DeviceRegistryService;
import com.homeiot.business.domain.DeviceStatusEvent;
import com.homeiot.business.domain.TelemetryPayload;
import com.homeiot.business.notification.AlertEvent;
import com.homeiot.business.notification.NotificationService;
import com.homeiot.business.rules.RuleEvaluationResult;
import com.homeiot.business.rules.SafetyRuleEngine;
import com.homeiot.business.service.EnergyAnalyticsService;
import com.homeiot.business.token.PlnTokenService;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.data.redis.connection.Message;
import org.springframework.data.redis.connection.MessageListener;
import org.springframework.stereotype.Component;

import java.nio.charset.StandardCharsets;

@Component
public class TelemetryRedisConsumer implements MessageListener {

    private static final Logger log = LoggerFactory.getLogger(TelemetryRedisConsumer.class);

    private final ObjectMapper objectMapper;
    private final SafetyRuleEngine safetyRuleEngine;
    private final EnergyAnalyticsService energyAnalyticsService;
    private final NotificationService notificationService;
    private final DeviceRegistryService deviceRegistryService;
    private final PlnTokenService plnTokenService;

    private long lastTelemetryMillis = System.currentTimeMillis();

    public TelemetryRedisConsumer(ObjectMapper objectMapper,
                                  SafetyRuleEngine safetyRuleEngine,
                                  EnergyAnalyticsService energyAnalyticsService,
                                  NotificationService notificationService,
                                  DeviceRegistryService deviceRegistryService,
                                  PlnTokenService plnTokenService) {
        this.objectMapper = objectMapper;
        this.safetyRuleEngine = safetyRuleEngine;
        this.energyAnalyticsService = energyAnalyticsService;
        this.notificationService = notificationService;
        this.deviceRegistryService = deviceRegistryService;
        this.plnTokenService = plnTokenService;
    }

    @Override
    public void onMessage(Message message, byte[] pattern) {
        String channel = new String(message.getChannel(), StandardCharsets.UTF_8);
        String body = new String(message.getBody(), StandardCharsets.UTF_8);

        try {
            if ("iot:telemetry:stream".equals(channel)) {
                TelemetryPayload payload = objectMapper.readValue(body, TelemetryPayload.class);

                // 1. Evaluate safety rules and dispatch alerts
                RuleEvaluationResult eval = safetyRuleEngine.evaluate(payload);
                dispatchSafetyAlerts(payload, eval);

                // 2. Accumulate energy analytics
                energyAnalyticsService.accumulate(payload);

                // 2b. Deduct prepaid PLN token based on real-time net grid draw
                long now = System.currentTimeMillis();
                long dtMillis = now - lastTelemetryMillis;
                lastTelemetryMillis = now;
                if (dtMillis > 0 && dtMillis < 60000) {
                    float dtHours = dtMillis / 3600000.0f;
                    float netPlnWatts = Math.max(0.0f, payload.housePowerWVal() - payload.pvPowerWattsVal());
                    plnTokenService.deduct(netPlnWatts, dtHours);
                }

                // 3. Update heartbeat
                deviceRegistryService.recordHeartbeat("sensor_environment");
                deviceRegistryService.recordHeartbeat("meter_power_grid");

            } else if ("iot:device:status".equals(channel)) {
                DeviceStatusEvent statusEvent = objectMapper.readValue(body, DeviceStatusEvent.class);
                log.warn("REDIS EVENT: Device [{}] status: {} (Reason: {})",
                        statusEvent.deviceId(), statusEvent.status(), statusEvent.reason());

                deviceRegistryService.updateStatus(statusEvent.deviceId(), statusEvent.status());

                if ("OFFLINE".equalsIgnoreCase(statusEvent.status())) {
                    notificationService.dispatch(AlertEvent.of(
                            "DEVICE_OFFLINE_" + statusEvent.deviceId(),
                            "Perangkat Terputus",
                            String.format("Perangkat [%s] berstatus OFFLINE (Alasan: %s)",
                                    statusEvent.deviceId(), statusEvent.reason()),
                            AlertEvent.AlertLevel.WARNING
                    ));
                }
            }
        } catch (Exception e) {
            log.error("Failed to process Redis message on channel [{}]: {}", channel, e.getMessage());
        }
    }

    private void dispatchSafetyAlerts(TelemetryPayload payload, RuleEvaluationResult eval) {
        if (eval.isGasHazardTriggered()) {
            notificationService.dispatch(AlertEvent.of(
                    "GAS_HAZARD",
                    "Bahaya Kebocoran Gas!",
                    String.format("Konsentrasi gas %d ppm melebihi ambang batas aman %d ppm!",
                            payload.gasPpmVal(), safetyRuleEngine.getGasPpmThreshold()),
                    AlertEvent.AlertLevel.CRITICAL
            ));
        }

        if (eval.isWaterLeakTriggered()) {
            notificationService.dispatch(AlertEvent.of(
                    "WATER_LEAK",
                    "Bahaya Kebocoran Air!",
                    "Sensor mendeteksi adanya genangan/kebocoran air aktif pada fasilitas!",
                    AlertEvent.AlertLevel.CRITICAL
            ));
        }

        if (eval.isPlnOverloadTriggered()) {
            notificationService.dispatch(AlertEvent.of(
                    "PLN_OVERLOAD",
                    "Peringatan Beban Listrik PLN",
                    String.format("Beban rumah %.1fW mendekati limit daya PLN %.0fW!",
                            payload.housePowerWVal(), safetyRuleEngine.getPlnLimitWatts()),
                    AlertEvent.AlertLevel.WARNING
            ));
        }
    }
}
