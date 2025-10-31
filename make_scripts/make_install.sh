#!/usr/bin/env bash

# Installs dependencies and (re)downloads required sdk and toolchain.
# Run this script only once after cloning the repository.

set -e                                           # Exit on error.
cd "$(dirname "$(dirname "$(realpath "$0")")")"  # Go to project root.

sudo apt-get update
sudo apt-get install -y make python3 yq unzip wget gcc-arm-none-eabi

make setup