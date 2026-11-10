package com.homeiot.business.device;

import com.fasterxml.jackson.databind.ObjectMapper;
import jakarta.annotation.PostConstruct;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.data.redis.core.StringRedisTemplate;
import org.springframework.stereotype.Service;

import java.time.Instant;
import java.util.*;
import java.util.concurrent.ConcurrentHashMap;

/**
 * Lean Device Registry managing IoT hardware metadata and live presence.
 * Backed by Redis Hash for persistence with an in-memory fallback cache.
 */
@Service
public class DeviceRegistryService {

    private static final Logger log = LoggerFactory.getLogger(DeviceRegistryService.class);
    private static final String REDIS_HASH_KEY = "iot:registry:devices";

    private final StringRedisTemplate redisTemplate;
    private final ObjectMapper objectMapper;
    private final Map<String, Device> localCache = new ConcurrentHashMap<>();

    public DeviceRegistryService(StringRedisTemplate redisTemplate, ObjectMapper objectMapper) {
        this.redisTemplate = redisTemplate;
        this.objectMapper = objectMapper;
    }

    @PostConstruct
    public void init() {
        loadFromRedis();
        if (localCache.isEmpty()) {
            seedDefaultDevices();
        }
        log.info("DeviceRegistry initialized with {} registered devices.", localCache.size());
    }

    public List<Device> getAllDevices() {
        return new ArrayList<>(localCache.values());
    }

    public Optional<Device> getDevice(String id) {
        return Optional.ofNullable(localCache.get(id));
    }

    public Device saveDevice(Device device) {
        localCache.put(device.id(), device);
        persistToRedis(device);
        return device;
    }

    public Device updateStatus(String id, String status) {
        Device existing = localCache.get(id);
        Instant now = Instant.now();
        Device updated = (existing != null)
                ? existing.withStatus(status, now)
                : new Device(id, id, "Unknown", "UNKNOWN", status, now);

        return saveDevice(updated);
    }

    public Device recordHeartbeat(String id) {
        return updateStatus(id, "ONLINE");
    }

    public Optional<Device> updateMetadata(String id, String newName, String newRoom) {
        Device existing = localCache.get(id);
        if (existing == null) {
            return Optional.empty();
        }
        Device updated = existing.withMetadata(newName, newRoom);
        return Optional.of(saveDevice(updated));
    }

    public boolean deleteDevice(String id) {
        boolean removed = localCache.remove(id) != null;
        if (removed && redisTemplate != null) {
            try {
                redisTemplate.opsForHash().delete(REDIS_HASH_KEY, id);
            } catch (Exception e) {
                log.warn("Failed to delete device {} from Redis: {}", id, e.getMessage());
            }
        }
        return removed;
    }

    private void loadFromRedis() {
        if (redisTemplate == null) return;
        try {
            Map<Object, Object> entries = redisTemplate.opsForHash().entries(REDIS_HASH_KEY);
            for (Map.Entry<Object, Object> entry : entries.entrySet()) {
                String json = (String) entry.getValue();
                Device device = objectMapper.readValue(json, Device.class);
                localCache.put(device.id(), device);
            }
        } catch (Exception e) {
            log.warn("Could not load devices from Redis (falling back to memory): {}", e.getMessage());
        }
    }

    private void persistToRedis(Device device) {
        if (redisTemplate == null) return;
        try {
            String json = objectMapper.writeValueAsString(device);
            redisTemplate.opsForHash().put(REDIS_HASH_KEY, device.id(), json);
        } catch (Exception e) {
            log.warn("Failed to persist device {} to Redis: {}", device.id(), e.getMessage());
        }
    }

    private void seedDefaultDevices() {
        Instant now = Instant.now();
        List<Device> seeds = List.of(
                new Device("lamp_front_porch", "Lampu Teras Depan", "Teras", "RELAY", "ONLINE", now),
                new Device("lamp_living_room", "Lampu Ruang Tamu", "Ruang Tamu", "RELAY", "ONLINE", now),
                new Device("lamp_kitchen", "Lampu Dapur", "Dapur", "RELAY", "ONLINE", now),
                new Device("lamp_bedroom_1", "Lampu Kamar Utama", "Kamar Tidur 1", "RELAY", "ONLINE", now),
                new Device("lamp_bedroom_2", "Lampu Kamar Anak", "Kamar Tidur 2", "RELAY", "ONLINE", now),
                new Device("lamp_bathroom", "Lampu Kamar Mandi", "Kamar Mandi", "RELAY", "ONLINE", now),
                new Device("lamp_backyard", "Lampu Halaman Belakang", "Halaman", "RELAY", "ONLINE", now),
                new Device("lamp_garage", "Lampu Garasi", "Garasi", "RELAY", "ONLINE", now),
                new Device("sensor_environment", "Sensor Gas & Kebocoran Air", "Dapur / Service", "SENSOR", "ONLINE", now),
                new Device("meter_power_grid", "Meteran Daya PLN & Solar", "Panel Utama", "METER", "ONLINE", now)
        );

        for (Device seed : seeds) {
            saveDevice(seed);
        }
    }
}

