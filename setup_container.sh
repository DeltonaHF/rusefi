#!/bin/bash
# Run this once after starting a fresh container session
set -e

echo "=== Setting up rusEFI build environment ==="

# Fix git safe directory
git config --global --add safe.directory /rusefi/rusefi
echo "✓ Git safe directory configured"

# Install bc for checksum address calculation in bundle.mk
apt-get install -y bc > /dev/null 2>&1
echo "✓ bc installed"

apt install 7zip


# Download and set up GCC 11.3 (required - GCC 14 produces broken firmware)
#bash firmware/ChibiOS/tools/provide_gcc.sh
echo "✓ GCC 11.3 toolchain ready"

# Add GCC 11.3 to PATH for this session
#export PATH=/rusefi/rusefi/gcc-arm-none-eabi/bin:$PATH
 GCC 11.3 added to PATH"echo "

echo ""
echo "=== Setup complete ==="
echo "GCC version: $(arm-none-eabi-gcc --version | head -1)"
echo ""
echo "To build f407-discovery:"
echo "  cd /rusefi/rusefi/firmware && bash bin/compile.sh config/boards/f407-discovery/meta-info.env"
echo ""
echo "To build RP-f405-board:"
echo "  cd /rusefi/rusefi/firmware && bash bin/compile.sh config/boards/RP-f405-board/meta-info.env"
echo ""
echo "NOTE: PATH is set for this session only. Re-run this script in each new terminal."
