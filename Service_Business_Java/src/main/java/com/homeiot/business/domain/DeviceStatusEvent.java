package com.homeiot.business.domain;

import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import com.fasterxml.jackson.annotation.JsonProperty;

/**
 * Immutable device status event received from Redis Pub/Sub.
 */
@JsonIgnoreProperties(ignoreUnknown = true)
public record DeviceStatusEvent(
    @JsonProperty("device_id") String deviceId,
    @JsonProperty("status") String status,
    @JsonProperty("reason") String reason,
    @JsonProperty("timestamp") Long timestamp
) {}
