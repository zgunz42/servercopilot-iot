import subprocess
import sys

def install(package):
    # The function installs the given module in the PlatformIO env
    subprocess.check_call([sys.executable, "-m", "pip", "install", package])

# Install the needed modules. They will be used in the extra_script.py
install("python-dotenv")