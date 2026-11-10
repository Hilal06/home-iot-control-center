package com.homeiot.business.device;

import java.time.Instant;

/**
 * Immutable representation of a registered smart home device.
 */
public record Device(
    String id,
    String name,
    String room,
    String type,        // "RELAY", "SENSOR", "METER"
    String status,      // "ONLINE", "OFFLINE"
    Instant lastSeen
) {
    public Device withStatus(String newStatus, Instant newLastSeen) {
        return new Device(this.id, this.name, this.room, this.type, newStatus, newLastSeen);
    }

    public Device withMetadata(String newName, String newRoom) {
        return new Device(this.id, newName != null ? newName : this.name,
                newRoom != null ? newRoom : this.room, this.type, this.status, this.lastSeen);
    }
}

