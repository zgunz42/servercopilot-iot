from dotenv import load_dotenv
import os
from datetime import datetime
Import("env")

# Injects all env vars defined in the .env file as os environment variables
load_dotenv(override=True)


def getCppDefines():
    # This function get all environment variables which name is starting with CUSTOM_ENV
    # The env var name and its value (os.environ.get(..)) are added to a CPPDEFINES array
    # the array is returned by the function
    CPPDEFINES = []
    for n in os.environ.keys():
        if n.startswith("SC_ENV_"):
            envValue = os.environ.get(n, "")
            # remove SV_ENV_
            n = n[6:]
            print(f'Add env {n} with value "{envValue}"')
            CPPDEFINES.append((n, f'\\\"{envValue}\\\"'))
    wifiSsid = os.environ.get("WIFI_SSID_NAME", "")
    wifiPassword = os.environ.get("WIFI_SSID_PASSWORD", "")
    mqttHost = os.environ.get("MQTT_HOST", "")
    mqttPort = os.environ.get("MQTT_PORT", "")
    mqttUser = os.environ.get("MQTT_USER", "")
    mqttPassword = os.environ.get("MQTT_PASSWORD", "")

    CPPDEFINES.append(("WIFI_SSID_NAME", f'\\\"{wifiSsid}\\\"'))
    CPPDEFINES.append(("WIFI_SSID_PASSWORD", f'\\\"{wifiPassword}\\\"'))
    CPPDEFINES.append(("MQTT_HOST", f'\\\"{mqttHost}\\\"'))
    CPPDEFINES.append(("MQTT_PORT", f'\\\"{mqttPort}\\\"'))
    CPPDEFINES.append(("MQTT_USER", f'\\\"{mqttUser}\\\"'))
    CPPDEFINES.append(("MQTT_PASSWORD", f'\\\"{mqttPassword}\\\"'))
    CPPDEFINES.append(("CUSTOM_ENV_BUILD_TIME", f'\\\"{datetime.now().strftime("%Y-%m-%d %H:%M:%S")}\\\"'))
    
    
    print(f'Add env WIFI_SSID_NAME with value "{wifiSsid}"')
    print(f'Add env WIFI_SSID_PASSWORD with value "{wifiPassword}"')
    print(f'Add env CUSTOM_ENV_BUILD_TIME with value "{datetime.now().strftime("%Y-%m-%d %H:%M:%S")}"')
    return CPPDEFINES


# Append cpp defined, same result by using build_flag = -Dxxx=... in the platform.ini file
env.Append(CPPDEFINES=getCppDefines())