#!/usr/bin/env bash

# Builds debugging firmware for a single device
#   with up to 16 parallel jobs (-j16) for faster compilation.
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

TYPE=router
BOARD=$DEVICE DEVICE_TYPE=$TYPE make clean && BOARD=$DEVICE DEVICE_TYPE=$TYPE DEBUG=1 make -j16
echo "Checking if files were created for board: $DEVICE ($TYPE)"
ls -l bin/$TYPE/$DEVICE/

TYPE=end_device
BOARD=$DEVICE DEVICE_TYPE=$TYPE make clean && BOARD=$DEVICE DEVICE_TYPE=$TYPE DEBUG=1 make -j16
echo "Checking if files were created for board: $DEVICE ($TYPE)"
ls -l bin/$TYPE/${DEVICE}_END_DEVICE/

make update_converters
make update_zha_quirk
make supported