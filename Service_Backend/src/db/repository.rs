use anyhow::Result;
use sqlx::{PgPool, postgres::PgPoolOptions};
use std::time::Duration;
use tracing::info;

use crate::models::{
    device::{
        ActuatorEvent, CreateDeviceRequest, Device, DeviceState, TelemetryRecord,
        UpdateDeviceRequest,
    },
    telemetry::TelemetryPayload,
};

pub async fn create_db_pool(database_url: &str) -> Result<PgPool> {
    info!("Connecting to PostgreSQL/TimescaleDB at {}", database_url);
    let pool = PgPoolOptions::new()
        .max_connections(10)
        .acquire_timeout(Duration::from_secs(5))
        .connect(database_url)
        .await?;

    Ok(pool)
}

pub async fn run_migrations(pool: &PgPool) -> Result<()> {
    info!("Verifying database schema...");
    let schema_sql = include_str!("../../init-db/01-schema.sql");
    sqlx::raw_sql(schema_sql).execute(pool).await?;

    // Ensure all multi-device columns exist even on legacy DB instances
    let migration_sql = r#"
        ALTER TABLE devices ADD COLUMN IF NOT EXISTS role VARCHAR(64) NOT NULL DEFAULT 'Edge Gateway';
        ALTER TABLE devices ADD COLUMN IF NOT EXISTS is_locked BOOLEAN NOT NULL DEFAULT FALSE;
        ALTER TABLE devices ADD COLUMN IF NOT EXISTS allow_automation_when_locked BOOLEAN NOT NULL DEFAULT TRUE;
        ALTER TABLE devices ADD COLUMN IF NOT EXISTS description TEXT;
        ALTER TABLE devices ADD COLUMN IF NOT EXISTS buttons JSONB NOT NULL DEFAULT '[]'::jsonb;
        ALTER TABLE devices ADD COLUMN IF NOT EXISTS automation_rules JSONB NOT NULL DEFAULT '[]'::jsonb;
    "#;
    sqlx::raw_sql(migration_sql).execute(pool).await?;

    info!("Database schema verified successfully.");
    Ok(())
}

pub async fn save_telemetry(
    pool: &PgPool,
    device_id: &str,
    telem: &TelemetryPayload,
) -> Result<()> {
    sqlx::query(
        r#"
        INSERT INTO telemetry_readings (
            recorded_at, device_id,
            pv_array_voltage, pv_array_current, pv_power_watts, solar_yield_kwh, panel_temp_c,
            mains_voltage, house_power_w, ups_battery_pct, ups_load_pct,
            rack_temp_c, temp_outdoor_c, humidity_pct, water_leak,
            ping_gateway_ms, rssi_dbm, esp_free_heap_kb
        ) VALUES (
            NOW(), $1,
            $2, $3, $4, $5, $6,
            $7, $8, $9, $10,
            $11, $12, $13, $14,
            $15, $16, $17
        )
        "#
    )
    .bind(device_id)
    .bind(telem.pv_array_voltage)
    .bind(telem.pv_array_current)
    .bind(telem.pv_power_watts)
    .bind(telem.solar_yield_kwh)
    .bind(telem.panel_temp_c)
    .bind(telem.mains_voltage)
    .bind(telem.house_power_w)
    .bind(telem.ups_battery_pct)
    .bind(telem.ups_load_pct)
    .bind(telem.rack_temp_c)
    .bind(telem.temp_outdoor_c)
    .bind(telem.humidity_pct)
    .bind(telem.water_leak.unwrap_or(false))
    .bind(telem.ping_gateway_ms)
    .bind(telem.rssi_dbm)
    .bind(telem.esp_free_heap_kb)
    .execute(pool)
    .await?;

    // Update Device status & last_seen
    let _ = sqlx::query(
        r#"
        UPDATE devices 
        SET status = 'ONLINE', last_seen = NOW()
        WHERE id = $1
        "#
    )
    .bind(device_id)
    .execute(pool)
    .await;

    // Update DeviceState if lamps or actuators present
    if telem.lamps.is_some() || telem.garage_status.is_some() || telem.hvac_power.is_some() {
        let lamp_json = serde_json::to_value(&telem.lamps).unwrap_or(serde_json::json!({}));
        let garage = telem.garage_status.unwrap_or(false);
        let hvac_pwr = telem.hvac_power.unwrap_or(false);
        let hvac_temp = telem.hvac_target_temp.unwrap_or(22);

        let _ = sqlx::query(
            r#"
            INSERT INTO device_states (
                device_id, lamp_states, garage_status, hvac_power, hvac_target_temp, updated_at
            ) VALUES ($1, $2, $3, $4, $5, NOW())
            ON CONFLICT (device_id) DO UPDATE SET
                lamp_states = CASE WHEN $2 = '{}'::jsonb THEN device_states.lamp_states ELSE $2 END,
                garage_status = $3,
                hvac_power = $4,
                hvac_target_temp = $5,
                updated_at = NOW()
            "#
        )
        .bind(device_id)
        .bind(lamp_json)
        .bind(garage)
        .bind(hvac_pwr)
        .bind(hvac_temp)
        .execute(pool)
        .await;
    }

    Ok(())
}

pub async fn save_actuator_event(
    pool: &PgPool,
    device_id: &str,
    command: &str,
    target: Option<&str>,
    payload: &serde_json::Value,
    source: &str,
    success: bool,
) -> Result<()> {
    sqlx::query(
        r#"
        INSERT INTO actuator_events (
            device_id, command, target, payload, source, success, executed_at
        ) VALUES ($1, $2, $3, $4, $5, $6, NOW())
        "#
    )
    .bind(device_id)
    .bind(command)
    .bind(target)
    .bind(payload)
    .bind(source)
    .bind(success)
    .execute(pool)
    .await?;

    Ok(())
}

pub async fn get_device_state(pool: &PgPool, device_id: &str) -> Result<Option<DeviceState>> {
    let state = sqlx::query_as::<_, DeviceState>(
        r#"
        SELECT device_id, lamp_states, garage_status, kitchen_breaker,
               hvac_power, hvac_target_temp, solar_charge_enabled,
               solar_max_watts, telemetry_interval_ms, updated_at
        FROM device_states
        WHERE device_id = $1
        "#
    )
    .bind(device_id)
    .fetch_optional(pool)
    .await?;

    Ok(state)
}

pub async fn list_devices(pool: &PgPool) -> Result<Vec<Device>> {
    let devices = sqlx::query_as::<_, Device>(
        r#"
        SELECT id, name, ip_address,
               CASE
                 WHEN last_seen IS NULL THEN 'OFFLINE'
                 WHEN last_seen < NOW() - INTERVAL '5 seconds' THEN 'OFFLINE'
                 ELSE status
               END as status,
               role, is_locked,
               allow_automation_when_locked, description, buttons, automation_rules,
               last_seen, created_at
        FROM devices
        ORDER BY id ASC
        "#
    )
    .fetch_all(pool)
    .await?;

    Ok(devices)
}

pub async fn get_device_by_id(pool: &PgPool, id: &str) -> Result<Option<Device>> {
    let device = sqlx::query_as::<_, Device>(
        r#"
        SELECT id, name, ip_address,
               CASE
                 WHEN last_seen IS NULL THEN 'OFFLINE'
                 WHEN last_seen < NOW() - INTERVAL '5 seconds' THEN 'OFFLINE'
                 ELSE status
               END as status,
               role, is_locked,
               allow_automation_when_locked, description, buttons, automation_rules,
               last_seen, created_at
        FROM devices
        WHERE id = $1
        "#
    )
    .bind(id)
    .fetch_optional(pool)
    .await?;

    Ok(device)
}

pub async fn create_device(pool: &PgPool, req: &CreateDeviceRequest) -> Result<Device> {
    let device = sqlx::query_as::<_, Device>(
        r#"
        INSERT INTO devices (
            id, name, ip_address, status, role, is_locked,
            allow_automation_when_locked, description, buttons, automation_rules,
            last_seen, created_at
        ) VALUES (
            $1, $2, $3, 'STANDBY', $4, $5,
            $6, $7, $8, $9,
            NOW(), NOW()
        )
        RETURNING id, name, ip_address, status, role, is_locked,
                  allow_automation_when_locked, description, buttons, automation_rules,
                  last_seen, created_at
        "#
    )
    .bind(&req.id)
    .bind(&req.name)
    .bind(&req.ip_address)
    .bind(&req.role)
    .bind(req.is_locked)
    .bind(req.allow_automation_when_locked)
    .bind(&req.description)
    .bind(&req.buttons)
    .bind(&req.automation_rules)
    .fetch_one(pool)
    .await?;

    // Initialize device_states snapshot row for this device
    let _ = sqlx::query(
        r#"
        INSERT INTO device_states (device_id)
        VALUES ($1)
        ON CONFLICT (device_id) DO NOTHING
        "#
    )
    .bind(&req.id)
    .execute(pool)
    .await;

    Ok(device)
}

pub async fn update_device(pool: &PgPool, id: &str, req: &UpdateDeviceRequest) -> Result<Device> {
    let device = sqlx::query_as::<_, Device>(
        r#"
        UPDATE devices
        SET
            name = COALESCE($2, name),
            ip_address = COALESCE($3, ip_address),
            role = COALESCE($4, role),
            status = COALESCE($5, status),
            description = COALESCE($6, description),
            allow_automation_when_locked = COALESCE($7, allow_automation_when_locked)
        WHERE id = $1
        RETURNING id, name, ip_address, status, role, is_locked,
                  allow_automation_when_locked, description, buttons, automation_rules,
                  last_seen, created_at
        "#
    )
    .bind(id)
    .bind(&req.name)
    .bind(&req.ip_address)
    .bind(&req.role)
    .bind(&req.status)
    .bind(&req.description)
    .bind(req.allow_automation_when_locked)
    .fetch_one(pool)
    .await?;

    Ok(device)
}

pub async fn delete_device(pool: &PgPool, id: &str) -> Result<()> {
    sqlx::query(r#"DELETE FROM devices WHERE id = $1"#)
        .bind(id)
        .execute(pool)
        .await?;
    Ok(())
}

pub async fn set_device_lock(pool: &PgPool, id: &str, is_locked: bool) -> Result<bool> {
    sqlx::query(r#"UPDATE devices SET is_locked = $2 WHERE id = $1"#)
        .bind(id)
        .bind(is_locked)
        .execute(pool)
        .await?;
    Ok(is_locked)
}

pub async fn update_device_buttons(
    pool: &PgPool,
    id: &str,
    buttons: &serde_json::Value,
) -> Result<()> {
    sqlx::query(r#"UPDATE devices SET buttons = $2 WHERE id = $1"#)
        .bind(id)
        .bind(buttons)
        .execute(pool)
        .await?;
    Ok(())
}

pub async fn update_device_rules(
    pool: &PgPool,
    id: &str,
    rules: &serde_json::Value,
) -> Result<()> {
    sqlx::query(r#"UPDATE devices SET automation_rules = $2 WHERE id = $1"#)
        .bind(id)
        .bind(rules)
        .execute(pool)
        .await?;
    Ok(())
}

pub async fn get_telemetry_history(
    pool: &PgPool,
    device_id: &str,
    limit: i64,
) -> Result<Vec<TelemetryRecord>> {
    let records = sqlx::query_as::<_, TelemetryRecord>(
        r#"
        SELECT recorded_at, device_id,
               pv_array_voltage, pv_array_current, pv_power_watts, solar_yield_kwh, panel_temp_c,
               mains_voltage, house_power_w, ups_battery_pct, ups_load_pct,
               rack_temp_c, temp_outdoor_c, humidity_pct, water_leak,
               ping_gateway_ms, rssi_dbm, esp_free_heap_kb
        FROM telemetry_readings
        WHERE device_id = $1
        ORDER BY recorded_at DESC
        LIMIT $2
        "#
    )
    .bind(device_id)
    .bind(limit)
    .fetch_all(pool)
    .await?;

    Ok(records)
}

pub async fn get_actuator_events(
    pool: &PgPool,
    device_id: &str,
    limit: i64,
) -> Result<Vec<ActuatorEvent>> {
    let events = sqlx::query_as::<_, ActuatorEvent>(
        r#"
        SELECT id, device_id, command, target, payload, source, success, executed_at
        FROM actuator_events
        WHERE device_id = $1
        ORDER BY executed_at DESC
        LIMIT $2
        "#
    )
    .bind(device_id)
    .bind(limit)
    .fetch_all(pool)
    .await?;

    Ok(events)
}
