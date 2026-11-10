package com.homeiot.business.controller;

import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RestController;

import java.util.Map;

@RestController
public class HealthController {

    @GetMapping("/health")
    public ResponseEntity<Map<String, Object>> health() {
        return ResponseEntity.ok(Map.of(
                "status", "UP",
                "service", "home-iot-business-engine",
                "version", "0.1.0",
                "runtime", "Java 17 OpenJDK / Spring Boot 3.3.5"
        ));
    }
}

