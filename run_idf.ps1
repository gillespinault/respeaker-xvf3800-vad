# ESP-IDF Build Script for PowerShell

# Clear MSYSTEM to avoid MSys/Mingw detection
Remove-Item Env:MSYSTEM -ErrorAction SilentlyContinue

# Set IDF_PATH
$env:IDF_PATH = "C:\Users\gille\esp\esp-idf-v5.3.2"

# Set tool paths
$env:OPENOCD_SCRIPTS = "C:\Users\gille\.espressif\tools\openocd-esp32\v0.12.0-esp32-20241016\openocd-esp32\share\openocd\scripts"
$env:IDF_CCACHE_ENABLE = "1"
$env:ESP_ROM_ELF_DIR = "C:\Users\gille\.espressif\tools\esp-rom-elfs\20240305\"
$env:IDF_PYTHON_ENV_PATH = "C:\Users\gille\.espressif\python_env\idf5.3_py3.11_env"
$env:ESP_IDF_VERSION = "5.3"
$env:IDF_TARGET = "esp32s3"

# Add tools to PATH
$toolPaths = @(
    "C:\Users\gille\.espressif\tools\xtensa-esp-elf-gdb\14.2_20240403\xtensa-esp-elf-gdb\bin",
    "C:\Users\gille\.espressif\tools\xtensa-esp-elf\esp-13.2.0_20240530\xtensa-esp-elf\bin",
    "C:\Users\gille\.espressif\tools\riscv32-esp-elf\esp-13.2.0_20240530\riscv32-esp-elf\bin",
    "C:\Users\gille\.espressif\tools\esp32ulp-elf\2.38_20240113\esp32ulp-elf\bin",
    "C:\Users\gille\.espressif\tools\cmake\3.30.2\bin",
    "C:\Users\gille\.espressif\tools\openocd-esp32\v0.12.0-esp32-20241016\openocd-esp32\bin",
    "C:\Users\gille\.espressif\tools\ninja\1.12.1\",
    "C:\Users\gille\.espressif\tools\idf-exe\1.0.3\",
    "C:\Users\gille\.espressif\tools\ccache\4.10.2\ccache-4.10.2-windows-x86_64",
    "C:\Users\gille\.espressif\tools\dfu-util\0.11\dfu-util-0.11-win64",
    "C:\Users\gille\.espressif\python_env\idf5.3_py3.11_env\Scripts",
    "C:\Users\gille\esp\esp-idf-v5.3.2\tools"
)
$env:PATH = ($toolPaths -join ";") + ";" + $env:PATH

# Run idf.py with arguments
python "$env:IDF_PATH\tools\idf.py" $args
