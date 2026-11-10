-- Initialize TimescaleDB Extension (if available)
CREATE EXTENSION IF NOT EXISTS timescaledb CASCADE;

-- 1. Devices Master Table
CREATE TABLE IF NOT EXISTS devices (
    id VARCHAR(64) PRIMARY KEY,
    name VARCHAR(128) NOT NULL,
    ip_address VARCHAR(64),
    status VARCHAR(32) NOT NULL DEFAULT 'OFFLINE',
    role VARCHAR(64) NOT NULL DEFAULT 'Edge Gateway',
    is_locked BOOLEAN NOT NULL DEFAULT FALSE,
    allow_automation_when_locked BOOLEAN NOT NULL DEFAULT TRUE,
    description TEXT,
    buttons JSONB NOT NULL DEFAULT '[]'::jsonb,
    automation_rules JSONB NOT NULL DEFAULT '[]'::jsonb,
    last_seen TIMESTAMPTZ,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

-- Idempotent column migrations for existing databases
ALTER TABLE devices ADD COLUMN IF NOT EXISTS role VARCHAR(64) NOT NULL DEFAULT 'Edge Gateway';
ALTER TABLE devices ADD COLUMN IF NOT EXISTS is_locked BOOLEAN NOT NULL DEFAULT FALSE;
ALTER TABLE devices ADD COLUMN IF NOT EXISTS allow_automation_when_locked BOOLEAN NOT NULL DEFAULT TRUE;
ALTER TABLE devices ADD COLUMN IF NOT EXISTS description TEXT;
ALTER TABLE devices ADD COLUMN IF NOT EXISTS buttons JSONB NOT NULL DEFAULT '[]'::jsonb;
ALTER TABLE devices ADD COLUMN IF NOT EXISTS automation_rules JSONB NOT NULL DEFAULT '[]'::jsonb;

-- Seed default ESP32 Master device
INSERT INTO devices (id, name, ip_address, status, role, is_locked, allow_automation_when_locked, description, last_seen)
VALUES (
    'esp32_master',
    'ESP32 Master Node (Edge Gateway)',
    '192.168.100.76',
    'ONLINE',
    'Edge Gateway',
    FALSE,
    TRUE,
    'Primary Edge Controller and telemetry aggregator for power, lights, and environment.',
    NOW()
)
ON CONFLICT (id) DO UPDATE SET
    role = EXCLUDED.role,
    description = COALESCE(devices.description, EXCLUDED.description);


-- 2. Device States Table (Latest Snapshot)
CREATE TABLE IF NOT EXISTS device_states (
    device_id VARCHAR(64) PRIMARY KEY REFERENCES devices(id) ON DELETE CASCADE,
    lamp_states JSONB NOT NULL DEFAULT '{
        "living_room": false,
        "main_room": false,
        "garage": false,
        "kitchen": false,
        "stair_garage": false,
        "stair_fl2": false,
        "yard_front": false,
        "yard_side": false,
        "yard_back": false,
        "server_room": false,
        "electrical_room": false
    }',
    garage_status BOOLEAN NOT NULL DEFAULT FALSE,
    kitchen_breaker BOOLEAN NOT NULL DEFAULT TRUE,
    hvac_power BOOLEAN NOT NULL DEFAULT FALSE,
    hvac_target_temp INT NOT NULL DEFAULT 22,
    solar_charge_enabled BOOLEAN NOT NULL DEFAULT TRUE,
    solar_max_watts INT NOT NULL DEFAULT 3000,
    telemetry_interval_ms INT NOT NULL DEFAULT 2000,
    updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

INSERT INTO device_states (device_id)
VALUES ('esp32_master')
ON CONFLICT (device_id) DO NOTHING;

-- 3. Telemetry Readings Table (Time-Series)
CREATE TABLE IF NOT EXISTS telemetry_readings (
    recorded_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    device_id VARCHAR(64) NOT NULL REFERENCES devices(id) ON DELETE CASCADE,
    pv_array_voltage REAL,
    pv_array_current REAL,
    pv_power_watts REAL,
    solar_yield_kwh REAL,
    panel_temp_c REAL,
    mains_voltage REAL,
    house_power_w REAL,
    ups_battery_pct REAL,
    ups_load_pct REAL,
    rack_temp_c REAL,
    temp_outdoor_c REAL,
    humidity_pct REAL,
    water_leak BOOLEAN DEFAULT FALSE,
    ping_gateway_ms REAL,
    rssi_dbm INT,
    esp_free_heap_kb INT
);

-- Create hypertable if timescaledb is loaded
DO $$
BEGIN
    IF EXISTS (SELECT 1 FROM pg_extension WHERE extname = 'timescaledb') THEN
        PERFORM create_hypertable('telemetry_readings', 'recorded_at', if_not_exists => TRUE);
    END IF;
EXCEPTION WHEN OTHERS THEN
    -- Fallback gracefully to standard index if hypertable cannot be created
    RAISE NOTICE 'Skipping hypertable creation: %', SQLERRM;
END $$;

-- Indexes for fast query on device and time range
CREATE INDEX IF NOT EXISTS idx_telemetry_device_time 
ON telemetry_readings (device_id, recorded_at DESC);

-- 4. Actuator Events Audit Log
CREATE TABLE IF NOT EXISTS actuator_events (
    id BIGSERIAL PRIMARY KEY,
    device_id VARCHAR(64) NOT NULL REFERENCES devices(id) ON DELETE CASCADE,
    command VARCHAR(64) NOT NULL,
    target VARCHAR(64),
    payload JSONB NOT NULL,
    source VARCHAR(32) NOT NULL DEFAULT 'REST_API',
    success BOOLEAN NOT NULL DEFAULT TRUE,
    executed_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_actuator_events_device_time 
ON actuator_events (device_id, executed_at DESC);
