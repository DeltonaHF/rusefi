/**
 * Direct voltage→temperature table sensors for RP-f405-board.
 *
 * The CLT/IAT circuits have a non-standard topology (original ECU pull-up
 * network + tap divider) that makes the standard ResistanceFunc→ThermistorFunc
 * chain inaccurate. We bypass it entirely with a TableFunc that maps the actual
 * ADC voltage directly to temperature using a piecewise linear interpolation
 * over the full NTC table.
 *
 * Table computed for: Vcc=5V, Rpu=2k7, Rs=358R, Rd1=6k8, Rd2=10k, NTC=3kΩ@25°C
 * Accuracy: exact at every NTC table point, <0.1°C between points.
 */

#include "pch.h"
#include "adc_subscription.h"
#include "functional_sensor.h"
#include "table_func.h"

// Voltage bins must be strictly ascending (low voltage = high temp for NTC).
// Each value is the ADC voltage produced by the circuit at the corresponding temperature.
static float cltVoltageBins[] = {
    0.0937f, 0.1380f, 0.1807f, 0.2459f, 0.3225f, 0.4281f,
    0.5659f, 0.7415f, 0.9621f, 1.2169f, 1.3520f, 1.4900f,
    1.7566f, 1.9899f, 2.1784f, 2.3124f, 2.4009f, 2.4550f
};

static float cltTemperatureValues[] = {
    125.0f, 110.0f, 100.0f, 90.0f, 80.0f, 70.0f,
     60.0f,  50.0f,  40.0f, 30.0f, 25.0f, 20.0f,
     10.0f,   0.0f, -10.0f,-20.0f,-30.0f,-40.0f
};

static_assert(std::size(cltVoltageBins) == std::size(cltTemperatureValues),
              "Bin/value array size mismatch");

// Single TableFunc instance — shared for CLT and IAT since same circuit
static TableFunc<float, float, std::size(cltVoltageBins)>
    ntcTableFunc(cltVoltageBins, cltTemperatureValues);

// Wrapper SensorConverter that FuncChain/FunctionalSensor can hold by ref
struct NtcTableConverter : public SensorConverter {
    SensorResult convert(float input) const override {
        // Clamp: below 0.09V or above 2.46V means sensor is disconnected/shorted
        if (input < 0.08f) return UnexpectedCode::High;   // very hot or short
        if (input > 2.46f) return UnexpectedCode::Low;    // very cold or open
        return ntcTableFunc.convert(input);
    }
};

static NtcTableConverter ntcConverter;

static FunctionalSensor cltTableSensor(SensorType::Clt, MS2NT(10));
static FunctionalSensor iatTableSensor(SensorType::Iat, MS2NT(10));

void initRpBoardNtcSensors() {
    auto cltChannel = engineConfiguration->clt.adcChannel;
    auto iatChannel = engineConfiguration->iat.adcChannel;

    if (isAdcChannelValid(cltChannel)) {
        cltTableSensor.setFunction(ntcConverter);
        AdcSubscription::SubscribeSensor(cltTableSensor, cltChannel, /*filterOrder=*/2);
        cltTableSensor.Register();
    }

    if (isAdcChannelValid(iatChannel)) {
        iatTableSensor.setFunction(ntcConverter);
        AdcSubscription::SubscribeSensor(iatTableSensor, iatChannel, /*filterOrder=*/2);
        iatTableSensor.Register();
    }
}