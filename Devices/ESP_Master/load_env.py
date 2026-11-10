import os
from SCons.Script import Import

Import("env")

# Locate root .env (two levels up from Devices/ESP_Master)
project_dir = env.get("PROJECT_DIR")
root_env_path = os.path.abspath(os.path.join(project_dir, "..", "..", ".env"))

if os.path.exists(root_env_path):
    print(f"[PlatformIO] Ingesting credentials from root .env: {root_env_path}")
    cpp_defines = []
    with open(root_env_path, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#") or "=" not in line:
                continue
            key, val = line.split("=", 1)
            key = key.strip()
            val = val.strip().strip('"').strip("'")

            # Map .env keys to C++ preprocessor definitions
            if key == "WIFI_SSID" and val:
                cpp_defines.append(("CONFIG_WIFI_SSID", f'\\"{val}\\"'))
            elif key == "WIFI_PASSWORD" and val:
                cpp_defines.append(("CONFIG_WIFI_PASSWORD", f'\\"{val}\\"'))
            elif key == "MQTT_BROKER_IP" and val:
                cpp_defines.append(("CONFIG_MQTT_SERVER", f'\\"{val}\\"'))
            elif key == "DEVICE_ID" and val:
                cpp_defines.append(("CONFIG_DEVICE_ID", f'\\"{val}\\"'))

    if cpp_defines:
        env.Append(CPPDEFINES=cpp_defines)
else:
    print(f"[PlatformIO] Note: Root .env not found at {root_env_path}. Using header/fallback defaults.")

