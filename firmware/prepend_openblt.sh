#!/bin/bash
cd $(dirname "$0")
OPENBLT_BIN=bootloader/openblt_stm32f4_discovery.bin
OPENBLT_HEX=bootloader/openblt_stm32f4_discovery.hex
FIRMWARE_HEX=build/rusefi.hex
OUTPUT=build/rusefi_combined.bin

# Convert OpenBLT bin to hex
arm-none-eabi-objcopy -I binary -O ihex --change-addresses=0x08000000 $OPENBLT_BIN $OPENBLT_HEX

# Get firmware base address and calculate checksum address
HEX_BASE_ADDRESS=$(arm-none-eabi-objdump -h -j .vectors build/rusefi.elf | awk '/.vectors/ {print $5}')
CHECKSUM_ADDRESS=0x$(python3 -c "print(hex(int('$HEX_BASE_ADDRESS', 16) + 0x1C)[2:].upper())")

echo "Base: $HEX_BASE_ADDRESS, Checksum: $CHECKSUM_ADDRESS"

# Combine with proper checksum patching
../misc/encedo_hex2dfu/hex2dfu.bin -i $OPENBLT_HEX -i $FIRMWARE_HEX -c $CHECKSUM_ADDRESS -b $OUTPUT

echo "Combined binary: $OUTPUT"
ls -lh $OUTPUT
