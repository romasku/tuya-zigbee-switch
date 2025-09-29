#!/usr/bin/env bash

# Builds the firmware for all devices found in device_db.yaml
#   with up to 16 parallel jobs (-j16) for faster compilation.
# Updates indexes, converters, quirks, and supported devices list.

# Estimated runtime: 1-5 mins

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

yq -r 'to_entries | sort_by(.key)[] | "\(.key) \(.value.device_type) \(.value.build)"' device_db.yaml | while read ITER TYPE BUILD; do

  if [ "$BUILD" = "no" ]; then
    echo "Skipping $ITER as building this model is disabled in the db."
    continue
  fi

  echo "Building for board: $ITER (router)"
  BOARD=$ITER DEVICE_TYPE=router make clean && BOARD=$ITER DEVICE_TYPE=router make -j16
  echo "Checking if files were created for board: $ITER (router)"
  ls -l bin/router/$ITER/
  
  if [ "$TYPE" = "end_device" ]; then
    echo "Building for board: $ITER (end_device)"
    BOARD=${ITER} DEVICE_TYPE=end_device make clean && BOARD=${ITER} DEVICE_TYPE=end_device make -j16
    echo "Checking if files were created for board: $ITER (end_device)"
    ls -l bin/end_device/${ITER}_END_DEVICE/
  fi
done

make update_converters
make update_zha_quirk
make update_supported_devices