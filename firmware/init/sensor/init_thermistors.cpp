#include "pch.h"

#include "adc_subscription.h"
#include "functional_sensor.h"
#include "linear_func.h"
#include "voltage_table_func.h"
#include "thermistor_func.h"

// Each sensor can be linear, thermistor (Steinhart-Hart), or voltage-table
struct FuncPair {
LinearFunc linear;
thermistor_t thermistor;
VoltageTableFunc voltageTable;
};

static CCM_OPTIONAL FunctionalSensor clt(SensorType::Clt, MS2NT(10));
static CCM_OPTIONAL FunctionalSensor iat(SensorType::Iat, MS2NT(10));
static CCM_OPTIONAL FunctionalSensor aux1(SensorType::AuxTemp1, MS2NT(10));
static CCM_OPTIONAL FunctionalSensor aux2(SensorType::AuxTemp2, MS2NT(10));

static CCM_OPTIONAL FunctionalSensor oilTempSensor(SensorType::OilTemperature, MS2NT(10));
static CCM_OPTIONAL FunctionalSensor fuelTempSensor(SensorType::FuelTemperature, MS2NT(10));
static CCM_OPTIONAL FunctionalSensor ambientTempSensor(SensorType::AmbientTemperature, MS2NT(10));
static CCM_OPTIONAL FunctionalSensor compressorDischargeTemp(SensorType::CompressorDischargeTemperature, MS2NT(10));

static FuncPair fclt, fiat, faux1, faux2, foil, ffuel, fambient, fcdt;

static void validateThermistorConfig(const char* msg, thermistor_conf_s& cfg) {
if (cfg.tempC_1 >= cfg.tempC_2 || cfg.tempC_2 >= cfg.tempC_3) {
firmwareError(
ObdCode::OBD_ThermistorConfig,
"Invalid thermistor %s configuration: please check that temperatures are in the ascending order %f %f "
"%f",
msg,
(float)cfg.tempC_1,
(float)cfg.tempC_2,
(float)cfg.tempC_3);
}
}

static SensorConverter& configureTempSensorFunction(const char* msg, thermistor_conf_s& cfg, FuncPair& p, bool isLinear, 
bool isPulldown, bool isVoltageTable = false, const voltage_table_conf_s* voltTableCfg = nullptr) 
{
if (isVoltageTable && voltTableCfg != nullptr) {
p.voltageTable.configure(*voltTableCfg);
if (p.voltageTable.m_count < 2) {
         firmwareError(ObdCode::OBD_ThermistorConfig, "Voltage table for %s has fewer than 2 valid points", msg);
}
         return p.voltageTable;
         } else if (isLinear) {
p.linear.configure(cfg.resistance_1, cfg.tempC_1, cfg.resistance_2, cfg.tempC_2, -50, 250);

return p.linear;
} else /* sensor is thermistor (Steinhart-Hart) */ {
validateThermistorConfig(msg, cfg);

p.thermistor.get<resist>().configure(5.0f, cfg.bias_resistor, isPulldown);
p.thermistor.get<therm>().configure(cfg);

return p.thermistor;
}
}

static void configTherm(
const char* msg,
FunctionalSensor& sensor,
FuncPair& p,
ThermistorConf& p_config,
bool isLinear,
bool isPulldown,
bool isVoltageTable = false,
        const voltage_table_conf_s* voltTableCfg = nullptr) {
// nothing to do if no channel
if (!isAdcChannelValid(p_config.adcChannel)) {
return;
}

// Configure the conversion function for this sensor
sensor.setFunction(configureTempSensorFunction(msg, p_config.config, p, isLinear, isPulldown, isVoltageTable, voltTableCfg));
}

static void configureTempSensor(
const char* msg,
FunctionalSensor& sensor,
FuncPair& p,
ThermistorConf& p_config,
bool isLinear,
bool isPulldown = false,
bool isVoltageTable = false,
const voltage_table_conf_s* voltTableCfg = nullptr) {
auto channel = p_config.adcChannel;

// Only register if we have a sensor
if (!isAdcChannelValid(channel)) {
return;
}

configTherm(msg, sensor, p, p_config, isLinear, isPulldown, isVoltageTable, voltTableCfg);

// Register & subscribe
AdcSubscription::SubscribeSensor(sensor, channel, 2);
sensor.Register();
}

void initThermistors() {
if (!engineConfiguration->consumeObdSensors) {
configureTempSensor(
"clt",
clt,
fclt,
engineConfiguration->clt,
engineConfiguration->useLinearCltSensor,
engineConfiguration->cltSensorPulldown,
         engineConfiguration->clt.useVoltageTable,
         &engineConfiguration->cltVoltageTable);

configureTempSensor(
"iat",
iat,
fiat,
engineConfiguration->iat,
engineConfiguration->useLinearIatSensor,
engineConfiguration->iatSensorPulldown, 
         engineConfiguration->iat.useVoltageTable,
         &engineConfiguration->iatVoltageTable);
}

configureTempSensor("oil temp", oilTempSensor, faux2, engineConfiguration->oilTempSensor, false);

configureTempSensor("fuel temp", fuelTempSensor, ffuel, engineConfiguration->fuelTempSensor, false);

configureTempSensor("ambient temp", ambientTempSensor, fambient, engineConfiguration->ambientTempSensor, false);

configureTempSensor(
"compressor discharge temp",
compressorDischargeTemp,
fcdt,
engineConfiguration->compressorDischargeTemperature,
false);

configureTempSensor("aux1", aux1, faux1, engineConfiguration->auxTempSensor1, false);

configureTempSensor("aux2", aux2, faux2, engineConfiguration->auxTempSensor2, false);
}

void deinitThermistors() {
AdcSubscription::UnsubscribeSensor(clt, engineConfiguration->clt.adcChannel);
AdcSubscription::UnsubscribeSensor(iat, engineConfiguration->iat.adcChannel);
AdcSubscription::UnsubscribeSensor(oilTempSensor, engineConfiguration->oilTempSensor.adcChannel);
AdcSubscription::UnsubscribeSensor(fuelTempSensor, engineConfiguration->fuelTempSensor.adcChannel);
AdcSubscription::UnsubscribeSensor(ambientTempSensor, engineConfiguration->ambientTempSensor.adcChannel);
AdcSubscription::UnsubscribeSensor(
compressorDischargeTemp, engineConfiguration->compressorDischargeTemperature.adcChannel);
AdcSubscription::UnsubscribeSensor(aux1, engineConfiguration->auxTempSensor1.adcChannel);
AdcSubscription::UnsubscribeSensor(aux2, engineConfiguration->auxTempSensor2.adcChannel);
}
