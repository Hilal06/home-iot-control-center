package com.homeiot.business.notification;

import java.time.Instant;

/**
 * Lightweight immutable representation of a system alert event.
 */
public record AlertEvent(
    String alertType,
    String title,
    String message,
    AlertLevel level,
    Instant timestamp
) {
    public enum AlertLevel {
        INFO,
        WARNING,
        CRITICAL
    }

    public static AlertEvent of(String alertType, String title, String message, AlertLevel level) {
        return new AlertEvent(alertType, title, message, level, Instant.now());
    }
}

