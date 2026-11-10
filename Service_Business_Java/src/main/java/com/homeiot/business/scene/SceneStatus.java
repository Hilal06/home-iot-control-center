package com.homeiot.business.scene;

import com.fasterxml.jackson.annotation.JsonProperty;
import java.time.Instant;
import java.util.List;

/**
 * Immutable status of the current active home scene.
 */
public record SceneStatus(
    @JsonProperty("current_scene") String currentScene,
    @JsonProperty("available_scenes") List<String> availableScenes,
    @JsonProperty("activated_at") Instant activatedAt,
    @JsonProperty("description") String description
) {}

