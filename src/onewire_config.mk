# Peripheral tokens start at field 3. A token beginning with W is a 1-Wire pin.
# Manufacturer and model are the first two fields and are not pins.
HAS_ONEWIRE := $(shell printf '%s' '$(CONFIG_STR)' | awk -F';' '{for(i=3;i<=NF;i++) if ($$i ~ /^W/) found=1} END{print found+0}')
