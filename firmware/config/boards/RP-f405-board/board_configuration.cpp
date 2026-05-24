/**
 * @file board_configuration.cpp
 * Generic STM32F405RGT6 (LQFP64) board - RP-f405-board
 * WeAct Studio STM32F405 Core Board V1.1
 *
 * Available GPIO: PA0-PA15, PB0-PB15, PC0-PC15, PD2 only
 * PH0/PH1 exist as OSC_IN/OSC_OUT but are not GPIO-capable.
 *
 * NOTE ON PB8/PB9: These are normal GPIO with CAN1 capability (AF9).
 *   The "VCAP" label seen on some STM32F405 documents refers to dedicated
 *   internal power-supply pins in the package — NOT PB8 or PB9.
 *   PB8 = CAN1_RX, PB9 = CAN1_TX. Both are fully usable.
 *
 * NOTE ON PA15: JTDI by default; remapped at startup by rusefi SWJ init
 *   so TIM2_CH1 (AF1) is available. SWD (PA13/PA14) is unaffected.
 *
 * NOTE ON PB2: BOOT1, sampled at reset only. Safe as GPIO after boot.
 *   The AC-request pull-down on the conditioning board keeps it LOW
 *   during reset, preventing spurious BOOT1 entry.
 *
 * NOTE ON PC13: 3 mA source/sink limit. Always drive via transistor/MOSFET
 *   stage; never connect a relay coil directly.
 *
 * ── Pin allocation ────────────────────────────────────────────────────────
 *
 * ANALOG INPUTS (3.3 V max, all op-amp buffered via MCP6004):
 *   PA0 / ADC1_IN0  - MAP sensor
 *   PA1 / ADC1_IN1  - TPS (throttle position)
 *   PA2 / ADC1_IN2  - Knock integrator output
 *   PA3 / ADC1_IN3  - CO-pot (mixture trim)
 *   PA4 / ADC1_IN4  - IAT / ATS intake air temperature NTC
 *   PA5 / ADC1_IN5  - CLT / CTS coolant temperature NTC
 *   PA6 / ADC1_IN6  - Narrowband O2 (5 V signal scaled to 3.3 V)
 *   PA7 / ADC1_IN7  - Auxiliary analog input
 *   PC0 / ADC1_IN10 - Battery voltage sense (resistor divider)
 *
 * DIGITAL INPUTS (5 V tolerant via 74LVC14AD Schmitt or conditioning):
 *   PB2             - A/C compressor request   (OEM ECU pin 21) [also BOOT1]
 *   PB4             - Auxiliary switch 1        (OEM ECU pin 8)
 *   PB13            - Vehicle speed sensor VSS  (3.3 V input)
 *   PB14            - ABS signal input          (OEM ECU pin 14)
 *   PC5             - Auxiliary switch 2        (OEM ECU pin 13)
 *
 * TRIGGER / EVENT INPUTS:
 *   PC6             - Crank sensor (CRS)        (OEM ECU pin 4, 5 V)
 *   PC7             - Cam sensor (CAS)          (OEM ECU pin 23, 5 V)
 *
 * IGNITION OUTPUTS (3.3 V logic → TC4427 / smart coil driver):
 *   PC2             - Ignition 2  (OEM ECU pin 26)
 *   PC3             - Ignition 3
 *   PC1             - Ignition 1  (OEM ECU pin 25)
 *   PC4             - Ignition 4
 *
 * INJECTOR OUTPUTS (low-side drivers):
 *   PB10            - Injector 1  (OEM ECU pin 35)
 *   PB11            - Injector 2  (OEM ECU pin 32)
 *   PB15            - Injector 3  (OEM ECU pin 33)
 *   PB12            - Injector 4  (OEM ECU pin 18)
 *
 * PWM / LOW-SIDE OUTPUTS:
 *   PA15 / TIM2_CH1 - Overboost valve PWM (OBV)  (OEM ECU pin 16)
 *   PB0  / TIM3_CH3 - Idle air control PWM (IAC)  (OEM ECU pin 34)
 *   PB1  / TIM3_CH4 - Auxiliary PWM output        (OEM ECU pin 27)
 *   PB3  / TIM2_CH2 - EVAP purge solenoid PWM     (OEM ECU pin 7)
 *
 * RELAY / LAMP OUTPUTS:
 *   PB5             - MIL warning lamp (OEM ECU pin 12)
 *   PC13            - Fuel pump relay  (OEM ECU pin 28) [3 mA limit — use driver]
 *
 * RESERVED (do not reassign):
 *   PA8             - TFCARD_PRESENT  (SD card detect, board pull-up hardwired)
 *   PA9  / USART1TX - TunerStudio serial TX (shared with SW debug header)
 *   PA10 / USART1RX - TunerStudio serial RX (shared with SW debug header)
 *   PA11            - USB OTG FS D-
 *   PA12            - USB OTG FS D+
 *   PA13            - SWDIO
 *   PA14            - SWCLK
 *   PB6  / I2C1_SCL - I2C1 clock (future use: display / knock IC)
 *   PB7  / I2C1_SDA - I2C1 data  (future use)
 *   PB8  / CAN1_RX  - CAN bus receive
 *   PB9  / CAN1_TX  - CAN bus transmit
 *   PC8  / SDIO_D0  - SD card data 0
 *   PC9  / SDIO_D1  - SD card data 1
 *   PC10 / SDIO_D2  - SD card data 2
 *   PC11 / SDIO_D3  - SD card data 3
 *   PC12 / SDIO_CK  - SD card clock
 *   PC14            - OSC32_IN  (RTC 32.768 kHz crystal)
 *   PC15            - OSC32_OUT (RTC 32.768 kHz crystal)
 *   PD2  / SDIO_CMD - SD card command
 */
#include "pch.h"
#include "board_overrides.h"

Gpio getWarningLedPin() {
    return Gpio::Unassigned;
}

Gpio getCommsLedPin() {
    return Gpio::Unassigned;
}

Gpio getRunningLedPin() {
    return Gpio::Unassigned;
}

static void rp_f405_boardDefaultConfiguration() {

    // === CAN ===
    // PB8 = CAN1_RX (AF9), PB9 = CAN1_TX (AF9).
    // PA15 has no CAN alternate function and must NOT be used for CAN.
    engineConfiguration->canTxPin = Gpio::B9;
    engineConfiguration->canRxPin = Gpio::B8;

    // === TunerStudio UART (USART1) ===
    // PA9/PA10 match the netlist and the physical debug header.
    // board.mk must define TS_SECONDARY_UxART_PORT=SD1 to activate this port.
    engineConfiguration->binarySerialTxPin = Gpio::A9;
    engineConfiguration->binarySerialRxPin = Gpio::A10;

    // === Trigger inputs ===
    // PC6 = CRS_5V_D_IN (crank, OEM ECU pin 4)
    // PC7 = CAS_5V_D_IN (cam,   OEM ECU pin 23)
    engineConfiguration->triggerInputPins[0] = Gpio::C6;   // crank
    engineConfiguration->triggerInputPins[1] = Gpio::Unassigned; // cam — enable when fitted

    // === Vehicle speed sensor ===
    engineConfiguration->vehicleSpeedSensorInputPin = Gpio::B13;

    // === Ignition outputs ===
    // 3.3 V logic outputs; coil drivers 
    // default IAW wasted spark, alternative TC4427 and/or smart coils.
    engineConfiguration->ignitionPinMode = OM_INVERTED;
    engineConfiguration->ignitionMode = IM_WASTED_SPARK;
    engineConfiguration->ignitionPins[0] = Gpio::C2;   // IGN1 (OEM pin 26)
    engineConfiguration->ignitionPins[1] = Gpio::Unassigned; //Gpio::C3;   // IGN3
    engineConfiguration->ignitionPins[2] = Gpio::C1;   // IGN2 (OEM pin 25)
    engineConfiguration->ignitionPins[3] = Gpio::Unassigned; //Gpio::C4;   // IGN4

    // === Injection outputs ===
    engineConfiguration->injectionPins[0] = Gpio::B10;  // INJ1 (OEM pin 35)
    engineConfiguration->injectionPins[1] = Gpio::B15;  // INJ2 (OEM pin 32)
    engineConfiguration->injectionPins[2] = Gpio::B11;  // INJ3 (OEM pin 33)
    engineConfiguration->injectionPins[3] = Gpio::B12;  // INJ4 (OEM pin 18)

    // === Analog sensors ===
    // ADC channel numbers follow directly from GPIO port/pin (see header comment).
    engineConfiguration->map.sensor.hwChannel = EFI_ADC_0;   // PA0 - MAP
    engineConfiguration->tps1_1AdcChannel     = EFI_ADC_1;   // PA1 - TPS
    // PA2 (EFI_ADC_2) = knock integrator — assigned via knock config, not here
    // PA3 (EFI_ADC_3) = CO-pot — assign to a GPPWM/aux input channel as needed
    engineConfiguration->iat.adcChannel       = EFI_ADC_4;   // PA4 - IAT
    engineConfiguration->iat.useVoltageTable  = true;

    engineConfiguration->clt.adcChannel       = EFI_ADC_5;   // PA5 - CLT
    engineConfiguration->clt.useVoltageTable  = true;

    // PA6 (EFI_ADC_6) = narrowband O2 — assign to o2Sensor1 channel as needed
    // PA7 (EFI_ADC_7) = auxiliary analog — assign as needed
    engineConfiguration->vbattAdcChannel      = EFI_ADC_10;  // PC0 - battery voltage
    engineConfiguration->vbattDividerCoeff = 6.36; // todo: comment computation, e.g., (33 + 6.8) / 6.8; // 5.835

    engineConfiguration->analogInputDividerCoefficient = 1.76f;
 
    // === Idle control ===
    // PB0 = IAC_PWM_OUT (OEM ECU pin 34); TIM3_CH3 available via AF2.
    engineConfiguration->idle.solenoidPin       = Gpio::B0;
    engineConfiguration->idle.solenoidFrequency = 200;
    engineConfiguration->idle.solenoidPinMode = OM_INVERTED; // active-low driver is used

    // === PWM outputs ===
    // PA15 = OBV overboost valve (OEM ECU pin 16), TIM2_CH1 via AF1.
    // PA15 starts as JTDI; rusefi remaps SWJ at startup — SWD is unaffected.
    // Assign via GPPWM channel in TunerStudio; no static C assignment needed.
    //
    // PB1 = AUX_PWM_OUT (OEM ECU pin 27), TIM3_CH4 via AF2.
    // Assign via GPPWM channel in TunerStudio.
    //
    // PB3 = EVAP_PWM_OUT (OEM ECU pin 7), TIM2_CH2 via AF1.
    engineConfiguration->etbIo[0].controlPin = Gpio::Unassigned; // not used
    // EVAP and AUX PWM are best configured via TunerStudio GPPWM.

    // === Fuel pump relay ===
    // PC13 has a 3 mA drive limit; ensure a transistor/MOSFET driver is in circuit.
    engineConfiguration->fuelPumpPin = Gpio::C13;
    engineConfiguration->fuelPumpPinMode = OM_INVERTED; // active-low driver is used

    // === MIL (check engine light) ===
    // PB5 = MIL_D_OUT (OEM ECU pin 12).
    // Active level depends on external driver polarity — adjust PinMode if needed.
    engineConfiguration->malfunctionIndicatorPin     = Gpio::B5;
    engineConfiguration->malfunctionIndicatorPinMode = OM_DEFAULT;

    // === Digital inputs ===
    // PB2 = AC_5V_D_IN (OEM ECU pin 21). Note: PB2 is also BOOT1 — the
    // conditioning board pull-down holds it LOW at reset, preventing boot mode.
    engineConfiguration->acSwitch = Gpio::B2;

    // PB4 = AuxSwt_5V_D_IN (OEM ECU pin 8) — available for launch/antilag/etc.
    // PC5 = AuxSwt_5V_D_IN (OEM ECU pin 13) — available for flex/etc.
    // PB14 = ABS_5V_D_IN   (OEM ECU pin 14) — available for traction control.
    // Assign these via TunerStudio digital input channels as needed.

    // === SD card ===
    // SDIO 4-bit mode via PC8-PC12 + PD2. No SPI CS pin.
    // PA8 is hardwired on the WeAct board as TFCARD_PRESENT with pull-up.
    engineConfiguration->sdCardCsPin = Gpio::Unassigned;

    // === ADC reference ===
    engineConfiguration->adcVcc = 3.3f;

    // === Trigger type default ===
    // 60-2 single-channel as a safe starting point.
    // Set triggerInputPins[1] = Gpio::C7 in TunerStudio when cam sensor is wired.
    engineConfiguration->trigger.type = trigger_type_e::TT_TOOTHED_WHEEL_60_2;

    // === NTC sensors via voltage-table lookup ===
    // Declared in ntc_table_sensors.cpp; replaces the standard Steinhart-Hart chain
    // because the OEM pull-up network distorts the thermistor curve beyond what
    // ResistanceFunc can fit accurately.
//    void initRpBoardNtcSensors();
//    initRpBoardNtcSensors();
}

void setup_custom_board_overrides() {
    custom_board_DefaultConfiguration = rp_f405_boardDefaultConfiguration;
}
