package com.homeiot.business.consumer;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.homeiot.business.device.DeviceRegistryService;
import com.homeiot.business.domain.TelemetryPayload;
import com.homeiot.business.notification.AlertEvent;
import com.homeiot.business.notification.NotificationService;
import com.homeiot.business.rules.SafetyRuleEngine;
import com.homeiot.business.service.EnergyAnalyticsService;
import com.homeiot.business.token.PlnTokenService;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.mockito.ArgumentCaptor;

import static org.junit.jupiter.api.Assertions.*;
import static org.mockito.Mockito.*;

class TelemetryRedisConsumerTest {

    private NotificationService notificationService;
    private TelemetryRedisConsumer consumer;

    @BeforeEach
    void setUp() {
        notificationService = mock(NotificationService.class);
        consumer = new TelemetryRedisConsumer(
                mock(ObjectMapper.class),
                mock(SafetyRuleEngine.class),
                mock(EnergyAnalyticsService.class),
                notificationService,
                mock(DeviceRegistryService.class),
                mock(PlnTokenService.class)
        );
    }

    @Test
    @DisplayName("Initial telemetry packet should establish baseline without triggering actuator notifications")
    void shouldNotAlertOnInitialBaseline() {
        TelemetryPayload initial = new TelemetryPayload(
                "esp32_master", 1000L, 850f, 7.5f, true, 340f, 98f, 120, false, false, false, true, false
        );

        consumer.dispatchActuatorChangeAlerts(initial);
        verify(notificationService, never()).dispatch(any());
    }

    @Test
    @DisplayName("Toggling solar charger should dispatch notification")
    void shouldAlertOnSolarChargerToggle() {
        // 1. Baseline: Solar ON
        TelemetryPayload p1 = new TelemetryPayload(
                "esp32_master", 1000L, 850f, 7.5f, true, 340f, 98f, 120, false, false, false, true, false
        );
        consumer.dispatchActuatorChangeAlerts(p1);

        // 2. Solar turned OFF
        TelemetryPayload p2 = new TelemetryPayload(
                "esp32_master", 2000L, 0f, 7.5f, false, 340f, 98f, 120, false, false, false, true, false
        );
        consumer.dispatchActuatorChangeAlerts(p2);

        ArgumentCaptor<AlertEvent> captor = ArgumentCaptor.forClass(AlertEvent.class);
        verify(notificationService, times(1)).dispatch(captor.capture());

        AlertEvent event = captor.getValue();
        assertEquals("ACTUATOR_SOLAR_OFF", event.alertType());
        assertEquals(AlertEvent.AlertLevel.WARNING, event.level());
        assertTrue(event.message().contains("DIMATIKAN"));

        // 3. Solar turned back ON
        TelemetryPayload p3 = new TelemetryPayload(
                "esp32_master", 3000L, 850f, 7.5f, true, 340f, 98f, 120, false, false, false, true, false
        );
        consumer.dispatchActuatorChangeAlerts(p3);

        verify(notificationService, times(2)).dispatch(captor.capture());
        AlertEvent eventOn = captor.getValue();
        assertEquals("ACTUATOR_SOLAR_ON", eventOn.alertType());
        assertEquals(AlertEvent.AlertLevel.INFO, eventOn.level());
        assertTrue(eventOn.message().contains("DIAKTIFKAN"));
    }

    @Test
    @DisplayName("Activating emergency alarm should dispatch critical alert")
    void shouldAlertOnEmergencyAlarm() {
        // Baseline: Alarm OFF
        consumer.dispatchActuatorChangeAlerts(new TelemetryPayload(
                "esp32_master", 1000L, 850f, 7.5f, true, 340f, 98f, 120, false, false, false, true, false
        ));

        // Alarm turned ON
        consumer.dispatchActuatorChangeAlerts(new TelemetryPayload(
                "esp32_master", 2000L, 850f, 7.5f, true, 340f, 98f, 120, false, true, true, true, false
        ));

        ArgumentCaptor<AlertEvent> captor = ArgumentCaptor.forClass(AlertEvent.class);
        verify(notificationService, times(1)).dispatch(captor.capture());

        AlertEvent event = captor.getValue();
        assertEquals("ACTUATOR_ALARM_ON", event.alertType());
        assertEquals(AlertEvent.AlertLevel.CRITICAL, event.level());
        assertTrue(event.title().contains("Sirene Alarm"));
    }

    @Test
    @DisplayName("Kitchen breaker tripping or turning off should dispatch warning alert")
    void shouldAlertOnKitchenBreakerTrip() {
        // Baseline: Breaker ON
        consumer.dispatchActuatorChangeAlerts(new TelemetryPayload(
                "esp32_master", 1000L, 850f, 7.5f, true, 340f, 98f, 120, false, false, false, true, false
        ));

        // Breaker turned OFF
        consumer.dispatchActuatorChangeAlerts(new TelemetryPayload(
                "esp32_master", 2000L, 850f, 7.5f, true, 340f, 98f, 120, false, false, false, false, false
        ));

        ArgumentCaptor<AlertEvent> captor = ArgumentCaptor.forClass(AlertEvent.class);
        verify(notificationService, times(1)).dispatch(captor.capture());

        AlertEvent event = captor.getValue();
        assertEquals("ACTUATOR_BREAKER_OFF", event.alertType());
        assertEquals(AlertEvent.AlertLevel.WARNING, event.level());
    }

    @Test
    @DisplayName("Opening garage door should dispatch info alert")
    void shouldAlertOnGarageDoorOpen() {
        // Baseline: Garage CLOSED
        consumer.dispatchActuatorChangeAlerts(new TelemetryPayload(
                "esp32_master", 1000L, 850f, 7.5f, true, 340f, 98f, 120, false, false, false, true, false
        ));

        // Garage OPENED
        consumer.dispatchActuatorChangeAlerts(new TelemetryPayload(
                "esp32_master", 2000L, 850f, 7.5f, true, 340f, 98f, 120, false, false, false, true, true
        ));

        ArgumentCaptor<AlertEvent> captor = ArgumentCaptor.forClass(AlertEvent.class);
        verify(notificationService, times(1)).dispatch(captor.capture());

        AlertEvent event = captor.getValue();
        assertEquals("ACTUATOR_GARAGE_OPEN", event.alertType());
        assertEquals(AlertEvent.AlertLevel.INFO, event.level());
    }
}
