#!/usr/bin/env bash

# Builds the firmware for all devices found in device_db.yaml
#   with up to 16 parallel jobs (-j16) for faster compilation.
# Updates indexes, converters, quirks, and readme.

# Estimated runtime: 5 mins

# Requires dependencies, sdk and toolchain.
#   Get them by runnnig make_install.sh.

# Alternatively, build online:
#   Run the Build Firmware workflow (build.yml) on GitHub Actions.

set -e                                           # Exit on error.
cd "$(dirname "$(dirname "$(realpath "$0")")")"  # Go to project root.

echo [] > zigbee2mqtt/ota/index_router.json 
echo [] > zigbee2mqtt/ota/index_end_device.json 
echo [] > zigbee2mqtt/ota/index_router-FORCE.json 
echo [] > zigbee2mqtt/ota/index_end_device-FORCE.json 

yq -r '(keys)[]' device_db.yaml | while read ITER; do
    BOARD=$ITER make clean && BOARD=$ITER make -j16
done

make update_converters
make update_zha_quirk
make update_readme