package com.homeiot.business.token;

import com.fasterxml.jackson.annotation.JsonProperty;

/**
 * Top-up request supporting either Rupiah amount (e.g. 100000 IDR) or direct kWh credit.
 */
public record RechargeRequest(
    @JsonProperty("amount_idr") Float amountIdr,
    @JsonProperty("added_kwh") Float addedKwh
) {}

