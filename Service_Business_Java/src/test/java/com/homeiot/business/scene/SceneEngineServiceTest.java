package com.homeiot.business.scene;

import com.homeiot.business.notification.NotificationService;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.springframework.web.client.RestClient;

import static org.junit.jupiter.api.Assertions.*;

class SceneEngineServiceTest {

    private SceneEngineService sceneEngineService;
    private NotificationService notificationService;

    @BeforeEach
    void setUp() {
        notificationService = new NotificationService(RestClient.builder());
        sceneEngineService = new SceneEngineService(null, notificationService);
        sceneEngineService.init();
    }

    @Test
    @DisplayName("Should initialize with default NORMAL scene")
    void testInitialScene() {
        SceneStatus status = sceneEngineService.getCurrentSceneStatus();
        assertEquals("NORMAL", status.currentScene());
        assertEquals(3, status.availableScenes().size());
        assertTrue(status.availableScenes().contains("AWAY"));
        assertTrue(status.availableScenes().contains("SLEEP"));
    }

    @Test
    @DisplayName("Should successfully activate AWAY and SLEEP scenes")
    void testActivateScenes() {
        SceneStatus away = sceneEngineService.activateScene("AWAY");
        assertEquals("AWAY", away.currentScene());

        SceneStatus sleep = sceneEngineService.activateScene("sleep"); // case insensitive
        assertEquals("SLEEP", sleep.currentScene());

        SceneStatus normal = sceneEngineService.activateScene("NORMAL");
        assertEquals("NORMAL", normal.currentScene());
    }

    @Test
    @DisplayName("Should reject invalid scene names")
    void testInvalidScene() {
        assertThrows(IllegalArgumentException.class, () -> sceneEngineService.activateScene("PARTY_MODE"));
        assertThrows(IllegalArgumentException.class, () -> sceneEngineService.activateScene(null));
    }
}

