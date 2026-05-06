/**
 * @file voltage_table_func.cpp
 */

#include "pch.h"
#include "voltage_table_func.h"

void VoltageTableFunc::configure(const voltage_table_conf_s &cfg) {
    m_count = 0;

    for (int i = 0; i < VOLTAGE_TABLE_SIZE; i++) {
        // Voltage breakpoints must be strictly ascending — stop at first violation
        if (i > 0 && cfg.voltageTable[i] <= cfg.voltageTable[i - 1]) {
            break;
        }
        m_voltage[i] = cfg.voltageTable[i];
        m_tempC[i]   = cfg.tempCTable[i];
        m_count++;
    }
}

SensorResult VoltageTableFunc::convert(float voltage) const {
    if (m_count < 2) {
        // Table not properly configured
        return UnexpectedCode::Low;
    }

    // Clamp to table range — no extrapolation
    if (voltage <= m_voltage[0]) {
        return m_tempC[0];
    }
    if (voltage >= m_voltage[m_count - 1]) {
        return m_tempC[m_count - 1];
    }

    // Binary search for the bracketing interval [lo, lo+1]
    int lo = 0;
    int hi = m_count - 1;
    while (hi - lo > 1) {
        int mid = (lo + hi) / 2;
        if (m_voltage[mid] <= voltage) {
            lo = mid;
        } else {
            hi = mid;
        }
    }

    // Linear interpolation
    float vLo = m_voltage[lo];
    float vHi = m_voltage[hi];
    float tLo = m_tempC[lo];
    float tHi = m_tempC[hi];

    float frac  = (voltage - vLo) / (vHi - vLo);
    float tempC = tLo + frac * (tHi - tLo);

    return tempC;
}

void VoltageTableFunc::showInfo(float testVoltage) const {
    SensorResult result = convert(testVoltage);
    efiPrintf("VoltTable: %.3f V -> valid: %s  %.1f deg C  (%d points loaded)",
              testVoltage,
              boolToString(result.Valid),
              result.Valid ? result.Value : 0.0f,
              m_count);
}
