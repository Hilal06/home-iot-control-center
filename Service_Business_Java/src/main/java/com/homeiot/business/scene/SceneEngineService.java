package com.homeiot.business.scene;

import com.homeiot.business.notification.AlertEvent;
import com.homeiot.business.notification.NotificationService;
import jakarta.annotation.PostConstruct;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.data.redis.core.StringRedisTemplate;
import org.springframework.stereotype.Service;

import java.time.Instant;
import java.util.List;
import java.util.Locale;

/**
 * Lean Dynamic Scene Engine managing whole-home profiles (NORMAL, AWAY, SLEEP).
 * State is stored in Redis with resilient in-memory fallback.
 */
@Service
public class SceneEngineService {

    private static final Logger log = LoggerFactory.getLogger(SceneEngineService.class);
    private static final String REDIS_SCENE_KEY = "iot:scene:current";
    public static final List<String> AVAILABLE_SCENES = List.of("NORMAL", "AWAY", "SLEEP");

    private final StringRedisTemplate redisTemplate;
    private final NotificationService notificationService;

    private volatile String currentScene = "NORMAL";
    private volatile Instant activatedAt = Instant.now();

    public SceneEngineService(StringRedisTemplate redisTemplate, NotificationService notificationService) {
        this.redisTemplate = redisTemplate;
        this.notificationService = notificationService;
    }

    @PostConstruct
    public void init() {
        if (redisTemplate != null) {
            try {
                String saved = redisTemplate.opsForValue().get(REDIS_SCENE_KEY);
                if (saved != null && AVAILABLE_SCENES.contains(saved.toUpperCase(Locale.ROOT))) {
                    this.currentScene = saved.toUpperCase(Locale.ROOT);
                }
            } catch (Exception e) {
                log.warn("Could not read initial scene from Redis: {}", e.getMessage());
            }
        }
        log.info("SceneEngineService initialized. Current active scene: {}", currentScene);
    }

    public SceneStatus getCurrentSceneStatus() {
        String description = switch (currentScene) {
            case "AWAY" -> "Lampu interior non-aktif & proteksi keamanan diperketat.";
            case "SLEEP" -> "Lampu utama redup/padam & penerangan luar aktif.";
            default -> "Operasi standar rumah harian.";
        };
        return new SceneStatus(currentScene, AVAILABLE_SCENES, activatedAt, description);
    }

    public SceneStatus activateScene(String sceneName) {
        if (sceneName == null) {
            throw new IllegalArgumentException("Scene name cannot be null");
        }

        String normalized = sceneName.trim().toUpperCase(Locale.ROOT);
        if (!AVAILABLE_SCENES.contains(normalized)) {
            throw new IllegalArgumentException(
                    String.format("Invalid scene '%s'. Available scenes: %s", sceneName, AVAILABLE_SCENES));
        }

        this.currentScene = normalized;
        this.activatedAt = Instant.now();

        if (redisTemplate != null) {
            try {
                redisTemplate.opsForValue().set(REDIS_SCENE_KEY, normalized);
                // Publish scene transition event for edge/actuator listeners
                redisTemplate.convertAndSend("iot:scene:events", normalized);
            } catch (Exception e) {
                log.warn("Could not persist scene change to Redis: {}", e.getMessage());
            }
        }

        String alertMsg = switch (normalized) {
            case "AWAY" -> "🏠 [MODE AWAY] Seluruh lampu dalam ruangan dipadamkan dan sistem disiagakan.";
            case "SLEEP" -> "🌙 [MODE SLEEP] Lampu utama dipadamkan dan penerangan luar dipertahankan.";
            default -> "☀️ [MODE NORMAL] Rumah dikembalikan ke profil operasi harian standar.";
        };

        log.info("SCENE ACTIVATED: [{}] - {}", normalized, alertMsg);
        notificationService.dispatch(AlertEvent.of(
                "SCENE_CHANGE_" + normalized,
                "Mode Rumah Diubah",
                alertMsg,
                AlertEvent.AlertLevel.INFO
        ));

        return getCurrentSceneStatus();
    }

    // Setter for testability
    public void setCurrentSceneForTest(String scene) {
        this.currentScene = scene;
        this.activatedAt = Instant.now();
    }
}

