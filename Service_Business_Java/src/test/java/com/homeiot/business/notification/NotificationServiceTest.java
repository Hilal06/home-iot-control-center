package com.homeiot.business.notification;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.springframework.web.client.RestClient;

import static org.junit.jupiter.api.Assertions.*;

class NotificationServiceTest {

    private NotificationService notificationService;

    @BeforeEach
    void setUp() {
        notificationService = new NotificationService(RestClient.builder());
        notificationService.setCooldownSeconds(60);
        notificationService.clearCooldowns();
    }

    @Test
    @DisplayName("Should throttle identical alert types within cooldown window")
    void shouldThrottleDuplicateAlerts() {
        String alertType = "GAS_HAZARD";

        // First event should not be throttled
        assertFalse(notificationService.shouldThrottle(alertType));

        // Immediate second event of same type should be throttled
        assertTrue(notificationService.shouldThrottle(alertType));

        // Different alert type should not be throttled
        assertFalse(notificationService.shouldThrottle("WATER_LEAK"));
    }

    @Test
    @DisplayName("Should run in safe No-Op mode when credentials are missing")
    void shouldHandleNoOpGracefully() {
        assertFalse(notificationService.isTelegramConfigured());
        assertFalse(notificationService.isDiscordConfigured());

        AlertEvent event = AlertEvent.of(
                "TEST_ALERT",
                "Uji Coba Sistem",
                "Pesan simulasi tanpa kredensial",
                AlertEvent.AlertLevel.INFO
        );

        // Dispatch should succeed smoothly without throwing exceptions
        boolean dispatched = notificationService.dispatch(event);
        assertTrue(dispatched);
    }
}

