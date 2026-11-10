package com.homeiot.business.controller;

import com.homeiot.business.token.PlnTokenService;
import com.homeiot.business.token.RechargeRequest;
import com.homeiot.business.token.TokenStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.util.Map;

/**
 * REST controller for monitoring and topping up PLN prepaid electricity tokens.
 */
@RestController
@RequestMapping("/api/v1/business/token")
public class PlnTokenController {

    private final PlnTokenService tokenService;

    public PlnTokenController(PlnTokenService tokenService) {
        this.tokenService = tokenService;
    }

    @GetMapping("/status")
    public ResponseEntity<TokenStatus> getTokenStatus() {
        return ResponseEntity.ok(tokenService.getStatus());
    }

    @PostMapping("/recharge")
    public ResponseEntity<?> rechargeToken(@RequestBody RechargeRequest request) {
        try {
            TokenStatus updated = tokenService.recharge(request);
            return ResponseEntity.ok(updated);
        } catch (IllegalArgumentException e) {
            return ResponseEntity.badRequest().body(Map.of("error", e.getMessage()));
        }
    }
}

