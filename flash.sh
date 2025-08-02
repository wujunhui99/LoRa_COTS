#!/bin/bash

# Check if required parameters are provided

# Clean build directory
sudo rm -rf ./*

SHIELD="${2:-"SX1272MB2DAS"}"

# 根据第二个参数的值设定SHIELD
case "$2" in
        2)
                SHIELD="SX1272MB2DAS"
                ;;
        6)
                SHIELD="SX1276MB1MAS"
                ;;
esac

echo "Building with SHIELD: $SHIELD"

# Configure with correct toolchain
cmake -DAPPLICATION=${1:-"fhsst"} -DMBED_RADIO_SHIELD=$SHIELD -DTOOLCHAIN_PREFIX=/opt/homebrew ..

# Build the project
make -j8

# Check if build was successful
if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

# Find the application binary file
APP_NAME="${1:-"fhsst"}"
BIN_FILE=""

# Look for the binary file in the build directory
if [ -f "src/apps/${APP_NAME}/${APP_NAME}.bin" ]; then
    BIN_FILE="src/apps/${APP_NAME}/${APP_NAME}.bin"
elif [ -f "${APP_NAME}.bin" ]; then
    BIN_FILE="${APP_NAME}.bin"
else
    # Search for any .bin file
    BIN_FILE=$(find . -name "*.bin" -type f | head -1)
fi

if [ -z "$BIN_FILE" ]; then
    echo "Error: Could not find binary file for application ${APP_NAME}"
    echo "Available files:"
    find . -name "*.bin" -o -name "*.hex" | head -10
    exit 1
fi

echo "Found binary file: $BIN_FILE"

# Flash the binary
echo "Flashing $BIN_FILE to device..."
st-flash write "$BIN_FILE" 0x8000000

