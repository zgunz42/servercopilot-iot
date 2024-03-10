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
        if n.startswith("CUSTOM_ENV"):
            envValue = os.environ.get(n, "")
            print(f'Add env {n} with value "{envValue}"')
            CPPDEFINES.append((n, f'\\\"{envValue}\\\"'))
    CPPDEFINES.append(("CUSTOM_ENV_BUILD_TIME", f'\\\"{datetime.now().strftime("%Y-%m-%d %H:%M:%S")}\\\"'))
    return CPPDEFINES


# Append cpp defined, same result by using build_flag = -Dxxx=... in the platform.ini file
env.Append(CPPDEFINES=getCppDefines())