package com.homeiot.business.controller;

import com.homeiot.business.device.Device;
import com.homeiot.business.device.DeviceRegistryService;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.util.List;

/**
 * REST controller for device registry queries and metadata updates.
 */
@RestController
@RequestMapping("/api/v1/business/devices")
public class DeviceController {

    private final DeviceRegistryService registryService;

    public DeviceController(DeviceRegistryService registryService) {
        this.registryService = registryService;
    }

    @GetMapping
    public ResponseEntity<List<Device>> listAll() {
        return ResponseEntity.ok(registryService.getAllDevices());
    }

    @GetMapping("/{id}")
    public ResponseEntity<Device> getById(@PathVariable String id) {
        return registryService.getDevice(id)
                .map(ResponseEntity::ok)
                .orElse(ResponseEntity.notFound().build());
    }

    @PutMapping("/{id}")
    public ResponseEntity<Device> updateMetadata(
            @PathVariable String id,
            @RequestBody UpdateDeviceRequest request) {
        return registryService.updateMetadata(id, request.name(), request.room())
                .map(ResponseEntity::ok)
                .orElse(ResponseEntity.notFound().build());
    }

    @DeleteMapping("/{id}")
    public ResponseEntity<Void> deleteDevice(@PathVariable String id) {
        if (registryService.deleteDevice(id)) {
            return ResponseEntity.noContent().build();
        }
        return ResponseEntity.notFound().build();
    }

    public record UpdateDeviceRequest(String name, String room) {}
}

