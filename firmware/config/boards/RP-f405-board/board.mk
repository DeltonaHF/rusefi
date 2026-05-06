# Tell the build system that board.h (and any halconf.h / mcuconf.h overrides)
# live in this directory, not the shared f407-discovery tree.
HALCONFDIR = $(BOARD_DIR)

BOARDCPPSRC = $(BOARD_DIR)/board_configuration.cpp 
#              $(BOARD_DIR)/ntc_table_sensors.cpp 

#BOARDCPPSRC = $(BOARD_DIR)/board_configuration.cpp \
#              $(BOARD_DIR)/ntc_table_sensors.cpp 

# board.c supplies __early_init() and the SDC card-detect / write-protect
# callbacks (sdc_lld_is_card_inserted / sdc_lld_is_write_protected).
# It must be compiled as C, not C++, so it goes in BOARDSRC not BOARDCPPSRC.
# BOARDSRC = $(BOARD_DIR)/board.c

# F405, not F407 (overrides the default STM32F407xx set in hw_ports.mk)
DDEFS += -DSTM32F405xx
DDEFS += -USTM32F407xx

DDEFS += -DFIRMWARE_ID=\"f407-discovery\"
DDEFS += -DSTATIC_BOARD_ID=STATIC_BOARD_ID_F407_DISCOVERY

# ADC1 is sufficient for all analog inputs (PA0-PA7 = IN0-IN7, PC0 = IN10).
# ADC3 is not wired to any analog input on this board — do not enable it.

# TunerStudio serial: USART1 on PA9 (TX) / PA10 (RX).
# These match the netlist and the physical SW-debug header.
# Do NOT use USART3 (SD3) — its default pins PB10/PB11 are injector outputs.
DDEFS += -DSTM32_SERIAL_USE_USART1=TRUE -DSTM32_SERIAL_USART1_PRIORITY=6
DDEFS += -DTS_SECONDARY_UxART_PORT=SD1 -DEFI_TS_SECONDARY_IS_SERIAL=TRUE

DDEFS += -DEFI_ONBOARD_MEMS=FALSE

# ── SD card via SDIO ────────────────────────────────────────────────────────
#
# The WeAct board routes the SD slot over SDIO (4-bit, PC8-PC12 + PD2),
# NOT SPI.  PA8 is hardwired as TFCARD_PRESENT (active-low card detect).
#
# STM32_SDC_USE_SDIO and HAL_USE_SDC are now set in the local mcuconf.h
# and halconf.h respectively — no DDEFS overrides needed here.
# DMA2 Stream3 conflict check is documented in mcuconf.h.

# Use MINIMAL_PINS engine type as default to avoid Frankenso pin conflicts
ifeq ($(VAR_DEF_ENGINE_TYPE),)
  VAR_DEF_ENGINE_TYPE = -DDEFAULT_ENGINE_TYPE=engine_type_e::MINIMAL_PINS
endif
DDEFS += $(VAR_DEF_ENGINE_TYPE)
