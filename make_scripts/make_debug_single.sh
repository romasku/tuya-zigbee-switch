#!/usr/bin/env bash

# Builds debugging firmware for a single device using the build system.
# Updates indexes, converters, quirks, and supported devices list.

# Estimated runtime: 5 seconds

# Requires dependencies, sdk and toolchain.
#   Get them by runnnig make_install.sh.

# Debug flag enables prints. To view them:
#   connect a serial monitor to the UART TX pin.

# Suggestion: Copy this script to a gitignored folder
#   and update it for your device.

set -e                                           # Exit on error.
cd "$(dirname "$(dirname "$(realpath "$0")")")"  # Go to project root.

DEVICE=AVATTO_TS0004  # Change this to your device

# Check if device exists in database
if ! yq -e ".${DEVICE}" device_db.yaml >/dev/null 2>&1; then
    echo "Error: Device '$DEVICE' not found in device_db.yaml"
    echo "Available devices:"
    yq 'keys[]' device_db.yaml | head -10
    echo "... (run 'yq keys device_db.yaml' to see all)"
    exit 1
fi

# Get device info from database
MCU=$(yq -r ".${DEVICE}.mcu" device_db.yaml)
PLATFORM=$(if [[ "$MCU" == "TLSR8258" ]]; then echo "telink"; else echo "silabs"; fi)

echo "Building debug firmware for device: $DEVICE (MCU: $MCU, Platform: $PLATFORM)"

# Build router version with debug enabled
echo "=== Building router version with debug ==="
TYPE=router
BOARD=$DEVICE DEVICE_TYPE=$TYPE DEBUG=1 make -f board.mk build-firmware
echo "Checking if files were created for board: $DEVICE ($TYPE)"
ls -l bin/$TYPE/$DEVICE/ 2>/dev/null || echo "No router files found"

# Build end_device version with debug enabled  
echo "=== Building end_device version with debug ==="
TYPE=end_device
BOARD=$DEVICE DEVICE_TYPE=$TYPE DEBUG=1 make -f board.mk build-firmware
echo "Checking if files were created for board: $DEVICE ($TYPE)"
ls -l bin/$TYPE/${DEVICE}_END_DEVICE/ 2>/dev/null || echo "No end_device files found"

echo "=== Updating integration files ==="
make tools/update_converters
make tools/update_zha_quirk
make tools/update_supported_devices

echo "=== Build complete ==="
echo "Debug firmware built for $DEVICE"
echo "Connect UART to view debug output on TX pin"