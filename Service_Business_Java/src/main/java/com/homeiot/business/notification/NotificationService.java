package com.homeiot.business.notification;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.http.MediaType;
import org.springframework.stereotype.Service;
import org.springframework.web.client.RestClient;

import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

/**
 * Lean notification service dispatching alerts to Telegram & Discord
 * with built-in anti-spam throttling and zero heavy SDK dependencies.
 */
@Service
public class NotificationService {

    private static final Logger log = LoggerFactory.getLogger(NotificationService.class);

    private final RestClient restClient;
    private final Map<String, Long> lastSentTimestamps = new ConcurrentHashMap<>();

    @Value("${iot.notification.cooldown-seconds:60}")
    private long cooldownSeconds;

    @Value("${iot.notification.telegram.bot-token:}")
    private String telegramBotToken;

    @Value("${iot.notification.telegram.chat-id:}")
    private String telegramChatId;

    @Value("${iot.notification.discord.webhook-url:}")
    private String discordWebhookUrl;

    public NotificationService(RestClient.Builder restClientBuilder) {
        this.restClient = restClientBuilder.build();
    }

    /**
     * Dispatch an alert event to configured channels if not throttled.
     * @return true if dispatched, false if throttled or no channels active.
     */
    public boolean dispatch(AlertEvent event) {
        if (shouldThrottle(event.alertType())) {
            log.debug("Notification throttled for type: {}", event.alertType());
            return false;
        }

        boolean telegramConfigured = isTelegramConfigured();
        boolean discordConfigured = isDiscordConfigured();

        if (!telegramConfigured && !discordConfigured) {
            log.info("[NO-OP NOTIFICATION] {} - {} : {}", event.level(), event.title(), event.message());
            return true;
        }

        if (telegramConfigured) {
            sendTelegram(event);
        }

        if (discordConfigured) {
            sendDiscord(event);
        }

        return true;
    }

    public boolean shouldThrottle(String alertType) {
        long now = System.currentTimeMillis();
        long cooldownMillis = cooldownSeconds * 1000L;
        Long lastSent = lastSentTimestamps.get(alertType);

        if (lastSent != null && (now - lastSent) < cooldownMillis) {
            return true;
        }

        lastSentTimestamps.put(alertType, now);
        return false;
    }

    private void sendTelegram(AlertEvent event) {
        try {
            String url = "https://api.telegram.org/bot" + telegramBotToken.trim() + "/sendMessage";
            String text = String.format("🚨 *[%s]* *%s*\n\n%s\n\n⏰ _%s_",
                    event.level(), event.title(), event.message(), event.timestamp());

            Map<String, Object> body = Map.of(
                    "chat_id", telegramChatId.trim(),
                    "text", text,
                    "parse_mode", "Markdown"
            );

            restClient.post()
                    .uri(url)
                    .contentType(MediaType.APPLICATION_JSON)
                    .body(body)
                    .retrieve()
                    .toBodilessEntity();

            log.info("Telegram alert sent successfully for: {}", event.alertType());
        } catch (Exception e) {
            log.error("Failed to send Telegram alert [{}]: {}", event.alertType(), e.getMessage());
        }
    }

    private void sendDiscord(AlertEvent event) {
        try {
            String emoji = switch (event.level()) {
                case CRITICAL -> "🔥";
                case WARNING -> "⚠️";
                case INFO -> "ℹ️";
            };

            String content = String.format("%s **[%s] %s**\n%s\n*Timestamp: %s*",
                    emoji, event.level(), event.title(), event.message(), event.timestamp());

            Map<String, String> body = Map.of("content", content);

            restClient.post()
                    .uri(discordWebhookUrl.trim())
                    .contentType(MediaType.APPLICATION_JSON)
                    .body(body)
                    .retrieve()
                    .toBodilessEntity();

            log.info("Discord alert sent successfully for: {}", event.alertType());
        } catch (Exception e) {
            log.error("Failed to send Discord alert [{}]: {}", event.alertType(), e.getMessage());
        }
    }

    public boolean isTelegramConfigured() {
        return telegramBotToken != null && !telegramBotToken.isBlank()
                && telegramChatId != null && !telegramChatId.isBlank();
    }

    public boolean isDiscordConfigured() {
        return discordWebhookUrl != null && !discordWebhookUrl.isBlank();
    }

    // Setters for testability
    public void setCooldownSeconds(long cooldownSeconds) { this.cooldownSeconds = cooldownSeconds; }
    public void setTelegramBotToken(String telegramBotToken) { this.telegramBotToken = telegramBotToken; }
    public void setTelegramChatId(String telegramChatId) { this.telegramChatId = telegramChatId; }
    public void setDiscordWebhookUrl(String discordWebhookUrl) { this.discordWebhookUrl = discordWebhookUrl; }
    public void clearCooldowns() { this.lastSentTimestamps.clear(); }
}

