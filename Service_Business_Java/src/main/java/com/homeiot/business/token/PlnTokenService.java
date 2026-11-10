package com.homeiot.business.token;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.homeiot.business.notification.AlertEvent;
import com.homeiot.business.notification.NotificationService;
import jakarta.annotation.PostConstruct;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.data.redis.core.StringRedisTemplate;
import org.springframework.stereotype.Service;

import java.util.Map;

/**
 * Lean prepaid PLN token manager calculating burn-rate, credit exhaustion forecast,
 * and low-balance warnings before physical meter trip.
 */
@Service
public class PlnTokenService {

    private static final Logger log = LoggerFactory.getLogger(PlnTokenService.class);
    private static final String REDIS_TOKEN_KEY = "iot:pln:token";

    private final StringRedisTemplate redisTemplate;
    private final NotificationService notificationService;
    private final ObjectMapper objectMapper;

    @Value("${iot.rules.pln-tariff-per-kwh:1352.0}")
    private float tariffRateIdr;

    @Value("${iot.token.low-balance-threshold-kwh:15.0}")
    private float lowBalanceThresholdKwh;

    private volatile float remainingKwh = 50.0f;
    private volatile float dailyBurnRateKwh = 6.5f; // Initial moving daily estimation

    public PlnTokenService(StringRedisTemplate redisTemplate,
                           NotificationService notificationService,
                           ObjectMapper objectMapper) {
        this.redisTemplate = redisTemplate;
        this.notificationService = notificationService;
        this.objectMapper = objectMapper;
    }

    @PostConstruct
    public void init() {
        if (redisTemplate != null) {
            try {
                String saved = redisTemplate.opsForValue().get(REDIS_TOKEN_KEY);
                if (saved != null) {
                    Map<?, ?> map = objectMapper.readValue(saved, Map.class);
                    if (map.containsKey("remaining_kwh")) {
                        this.remainingKwh = ((Number) map.get("remaining_kwh")).floatValue();
                    }
                    if (map.containsKey("daily_burn_rate_kwh")) {
                        this.dailyBurnRateKwh = ((Number) map.get("daily_burn_rate_kwh")).floatValue();
                    }
                } else {
                    persistToRedis();
                }
            } catch (Exception e) {
                log.warn("Could not load PLN token state from Redis: {}", e.getMessage());
            }
        }
        log.info("PlnTokenService initialized. Remaining: {} kWh, Burn rate: {} kWh/day",
                remainingKwh, dailyBurnRateKwh);
    }

    public synchronized void deduct(float netPlnWatts, float dtHours) {
        if (dtHours <= 0.0f || netPlnWatts < 0.0f) {
            return;
        }

        float kwhBurned = (netPlnWatts * dtHours) / 1000.0f;
        this.remainingKwh = Math.max(0.0f, this.remainingKwh - kwhBurned);

        // Update moving burn rate (95% previous weight, 5% current hourly projection)
        float currentDailyProjection = (netPlnWatts * 24.0f) / 1000.0f;
        this.dailyBurnRateKwh = (this.dailyBurnRateKwh * 0.95f) + (currentDailyProjection * 0.05f);

        if (this.remainingKwh < lowBalanceThresholdKwh) {
            notificationService.dispatch(AlertEvent.of(
                    "PLN_LOW_TOKEN",
                    "Peringatan Pulsa Listrik PLN Kritis!",
                    String.format("Sisa pulsa listrik PLN %.1f kWh (Rp %.0f). Segera lakukan isi ulang!",
                            this.remainingKwh, this.remainingKwh * tariffRateIdr),
                    AlertEvent.AlertLevel.WARNING
            ));
        }

        persistToRedis();
    }

    public synchronized TokenStatus recharge(RechargeRequest request) {
        float kwhToAdd = 0.0f;

        if (request.addedKwh() != null && request.addedKwh() > 0.0f) {
            kwhToAdd = request.addedKwh();
        } else if (request.amountIdr() != null && request.amountIdr() > 0.0f) {
            kwhToAdd = request.amountIdr() / tariffRateIdr;
        } else {
            throw new IllegalArgumentException("Harus menyertakan 'amount_idr' atau 'added_kwh' yang bernilai positif");
        }

        this.remainingKwh += kwhToAdd;
        persistToRedis();

        log.info("PLN TOKEN RECHARGED: Added +{:.2f} kWh. Total balance now: {:.2f} kWh",
                kwhToAdd, this.remainingKwh);

        notificationService.dispatch(AlertEvent.of(
                "PLN_RECHARGE_SUCCESS",
                "Isi Ulang Token PLN Berhasil",
                String.format("Token bertambah +%.2f kWh. Saldo total saat ini: %.2f kWh (Rp %.0f).",
                        kwhToAdd, this.remainingKwh, this.remainingKwh * tariffRateIdr),
                AlertEvent.AlertLevel.INFO
        ));

        return getStatus();
    }

    public TokenStatus getStatus() {
        float remainingIdr = this.remainingKwh * tariffRateIdr;
        float daysRemaining = this.dailyBurnRateKwh > 0.05f
                ? (this.remainingKwh / this.dailyBurnRateKwh)
                : 99.0f;
        boolean isLow = this.remainingKwh < lowBalanceThresholdKwh;

        return new TokenStatus(
                this.remainingKwh,
                remainingIdr,
                tariffRateIdr,
                this.dailyBurnRateKwh,
                daysRemaining,
                isLow,
                lowBalanceThresholdKwh
        );
    }

    private void persistToRedis() {
        if (redisTemplate == null) return;
        try {
            String json = objectMapper.writeValueAsString(Map.of(
                    "remaining_kwh", this.remainingKwh,
                    "daily_burn_rate_kwh", this.dailyBurnRateKwh
            ));
            redisTemplate.opsForValue().set(REDIS_TOKEN_KEY, json);
        } catch (Exception e) {
            log.warn("Failed to persist token state to Redis: {}", e.getMessage());
        }
    }

    // Setters & helpers for testing
    public void setRemainingKwhForTest(float kwh) { this.remainingKwh = kwh; }
    public void setDailyBurnRateForTest(float burnRate) { this.dailyBurnRateKwh = burnRate; }
}

