package com.homeiot.business.token;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.homeiot.business.notification.NotificationService;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.springframework.test.util.ReflectionTestUtils;
import org.springframework.web.client.RestClient;

import static org.junit.jupiter.api.Assertions.*;

class PlnTokenServiceTest {

    private PlnTokenService tokenService;
    private NotificationService notificationService;

    @BeforeEach
    void setUp() {
        notificationService = new NotificationService(RestClient.builder());
        tokenService = new PlnTokenService(null, notificationService, new ObjectMapper());
        ReflectionTestUtils.setField(tokenService, "tariffRateIdr", 1352.0f);
        ReflectionTestUtils.setField(tokenService, "lowBalanceThresholdKwh", 15.0f);
        tokenService.init();
    }

    @Test
    @DisplayName("Should initialize with default 50 kWh credit and calculate IDR equivalent")
    void testInitialStatus() {
        TokenStatus status = tokenService.getStatus();
        assertEquals(50.0f, status.remainingKwh(), 0.01f);
        assertEquals(50.0f * 1352.0f, status.remainingIdr(), 1.0f);
        assertFalse(status.isLowBalance());
    }

    @Test
    @DisplayName("Should top up token balance via Rupiah amount")
    void testRechargeViaIdr() {
        RechargeRequest request = new RechargeRequest(135200.0f, null); // should add exactly 100.0 kWh
        TokenStatus updated = tokenService.recharge(request);

        assertEquals(150.0f, updated.remainingKwh(), 0.01f);
    }

    @Test
    @DisplayName("Should top up token balance via direct kWh")
    void testRechargeViaKwh() {
        RechargeRequest request = new RechargeRequest(null, 25.5f);
        TokenStatus updated = tokenService.recharge(request);

        assertEquals(75.5f, updated.remainingKwh(), 0.01f);
    }

    @Test
    @DisplayName("Should deduct kWh based on power and time, and trigger low balance warning")
    void testDeductAndLowBalanceAlert() {
        tokenService.setRemainingKwhForTest(15.2f);
        tokenService.setDailyBurnRateForTest(5.0f);

        // Deduct 0.5 kWh (500W load for 1 hour)
        tokenService.deduct(500.0f, 1.0f);

        TokenStatus status = tokenService.getStatus();
        assertEquals(14.7f, status.remainingKwh(), 0.01f);
        assertTrue(status.isLowBalance(), "Balance below 15 kWh must be flagged as low balance");
        assertTrue(status.estimatedDaysRemaining() < 3.0f);
    }

    @Test
    @DisplayName("Should reject invalid recharge requests")
    void testInvalidRecharge() {
        assertThrows(IllegalArgumentException.class, () -> tokenService.recharge(new RechargeRequest(null, null)));
        assertThrows(IllegalArgumentException.class, () -> tokenService.recharge(new RechargeRequest(-50000.0f, null)));
    }
}

