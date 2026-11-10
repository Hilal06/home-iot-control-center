package com.homeiot.business.controller;

import com.homeiot.business.scene.SceneEngineService;
import com.homeiot.business.scene.SceneStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.util.Map;

/**
 * REST controller for viewing and triggering dynamic smart home scenes.
 */
@RestController
@RequestMapping("/api/v1/business/scenes")
public class SceneController {

    private final SceneEngineService sceneEngineService;

    public SceneController(SceneEngineService sceneEngineService) {
        this.sceneEngineService = sceneEngineService;
    }

    @GetMapping
    public ResponseEntity<SceneStatus> getCurrentScene() {
        return ResponseEntity.ok(sceneEngineService.getCurrentSceneStatus());
    }

    @PostMapping("/{sceneName}/activate")
    public ResponseEntity<?> activateScene(@PathVariable String sceneName) {
        try {
            SceneStatus status = sceneEngineService.activateScene(sceneName);
            return ResponseEntity.ok(status);
        } catch (IllegalArgumentException e) {
            return ResponseEntity.badRequest().body(Map.of("error", e.getMessage()));
        }
    }
}

