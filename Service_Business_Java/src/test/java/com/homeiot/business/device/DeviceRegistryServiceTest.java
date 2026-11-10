package com.homeiot.business.device;

import com.fasterxml.jackson.databind.ObjectMapper;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;

import java.util.List;
import java.util.Optional;

import static org.junit.jupiter.api.Assertions.*;

class DeviceRegistryServiceTest {

    private DeviceRegistryService registryService;

    @BeforeEach
    void setUp() {
        // StringRedisTemplate is passed as null to test in-memory fallback resilience
        registryService = new DeviceRegistryService(null, new ObjectMapper());
        registryService.init();
    }

    @Test
    @DisplayName("Should seed default smart home devices on clean startup")
    void shouldSeedDefaultDevices() {
        List<Device> devices = registryService.getAllDevices();
        assertEquals(10, devices.size(), "Should initialize 8 lamps + 2 sensors/meters");

        Optional<Device> lamp = registryService.getDevice("lamp_front_porch");
        assertTrue(lamp.isPresent());
        assertEquals("Lampu Teras Depan", lamp.get().name());
        assertEquals("RELAY", lamp.get().type());
        assertEquals("ONLINE", lamp.get().status());
    }

    @Test
    @DisplayName("Should update status and record heartbeat correctly")
    void shouldUpdateStatusAndHeartbeat() {
        Device offline = registryService.updateStatus("lamp_front_porch", "OFFLINE");
        assertEquals("OFFLINE", offline.status());

        Device backOnline = registryService.recordHeartbeat("lamp_front_porch");
        assertEquals("ONLINE", backOnline.status());
    }

    @Test
    @DisplayName("Should update device metadata (name & room)")
    void shouldUpdateMetadata() {
        Optional<Device> updated = registryService.updateMetadata("lamp_kitchen", "Lampu Dapur Utama", "Area Memasak");
        assertTrue(updated.isPresent());
        assertEquals("Lampu Dapur Utama", updated.get().name());
        assertEquals("Area Memasak", updated.get().room());
    }

    @Test
    @DisplayName("Should delete device cleanly")
    void shouldDeleteDevice() {
        assertTrue(registryService.deleteDevice("lamp_garage"));
        assertFalse(registryService.getDevice("lamp_garage").isPresent());
    }
}

