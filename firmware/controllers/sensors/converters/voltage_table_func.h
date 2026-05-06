/**
 * @file voltage_table_func.h
 *
 * A SensorConverter that maps ADC voltage directly to temperature using a
 * user-supplied 16-point lookup table with linear interpolation.
 *
 * Intended for CLT/IAT sensors whose voltage→temperature relationship cannot
 * be described by the Steinhart-Hart equation (e.g. when conditioning
 * circuitry precedes the ADC input and deforms the original NTC curve).
 *
 * Input:  ADC voltage (volts, 0..5 V) — received at the same stage where
 *         ResistanceFunc normally receives it, but bypassing resistance math.
 * Output: temperature in °C
 */

#pragma once

#include "sensor_converter_func.h"

class VoltageTableFunc final : public SensorConverter {
public:
    void configure(const voltage_table_conf_s &cfg);
    SensorResult convert(float voltage) const override;
    void showInfo(float testVoltage) const override;

    int m_count = 0;

private:
    float m_voltage[VOLTAGE_TABLE_SIZE] = {};
    float m_tempC[VOLTAGE_TABLE_SIZE]   = {};
};