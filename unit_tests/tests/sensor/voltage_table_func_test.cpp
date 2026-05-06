/**
 * @file voltage_table_func_test.cpp
 *
 * Unit tests for VoltageTableFunc — the voltage-to-temperature lookup
 * table sensor converter used when conditioning circuitry deforms the
 * original NTC curve.
 */

#include "pch.h"
#include "voltage_table_func.h"

// ---------------------------------------------------------------------------
// Helper: build a voltage_table_conf_s from parallel C arrays
// ---------------------------------------------------------------------------
static voltage_table_conf_s makeConf(
    std::initializer_list<float> voltages,
    std::initializer_list<float> temps)
{
    voltage_table_conf_s cfg{};
    int i = 0;
    for (float v : voltages) { cfg.voltageTable[i++] = v; }
    i = 0;
    for (float t : temps)    { cfg.tempCTable[i++]   = t; }
    return cfg;
}

// ---------------------------------------------------------------------------
// Basic interpolation — NTC-style (voltage up = temperature down)
// ---------------------------------------------------------------------------
TEST(VoltageTableFunc, InterpolatesCorrectly) {
    // Simple 4-point table: 0V→100°C, 1V→80°C, 2V→40°C, 5V→-20°C
    voltage_table_conf_s cfg = makeConf(
        {0.0f, 1.0f, 2.0f, 5.0f},
        {100.f, 80.f, 40.f, -20.f}
    );
    VoltageTableFunc f;
    f.configure(cfg);
    ASSERT_EQ(4, f.m_count);

    // Exact breakpoints
    EXPECT_NEAR(100.f, f.convert(0.0f).Value, 0.01f);
    EXPECT_NEAR( 80.f, f.convert(1.0f).Value, 0.01f);
    EXPECT_NEAR( 40.f, f.convert(2.0f).Value, 0.01f);
    EXPECT_NEAR(-20.f, f.convert(5.0f).Value, 0.01f);

    // Midpoints
    EXPECT_NEAR( 90.f, f.convert(0.5f).Value, 0.01f);  // between 0V and 1V
    EXPECT_NEAR( 60.f, f.convert(1.5f).Value, 0.01f);  // between 1V and 2V
}

// ---------------------------------------------------------------------------
// PTC-style (voltage up = temperature up) also works
// ---------------------------------------------------------------------------
TEST(VoltageTableFunc, AscendingTempTable) {
    voltage_table_conf_s cfg = makeConf(
        {0.5f, 1.0f, 2.0f, 4.0f},
        {-40.f, 0.f, 50.f, 120.f}
    );
    VoltageTableFunc f;
    f.configure(cfg);
    ASSERT_EQ(4, f.m_count);

    EXPECT_NEAR(0.f,  f.convert(1.0f).Value, 0.01f);
    EXPECT_NEAR(25.f, f.convert(1.5f).Value, 0.01f);  // midpoint between 0°C and 50°C
}

// ---------------------------------------------------------------------------
// Clamping: values outside range clamp to endpoint temperatures
// ---------------------------------------------------------------------------
TEST(VoltageTableFunc, ClampsLow) {
    voltage_table_conf_s cfg = makeConf(
        {1.0f, 2.0f, 3.0f},
        {80.f, 40.f, 20.f}
    );
    VoltageTableFunc f;
    f.configure(cfg);

    // Below lowest breakpoint → clamp to first temp
    SensorResult r = f.convert(0.0f);
    ASSERT_TRUE(r.Valid);
    EXPECT_NEAR(80.f, r.Value, 0.01f);
}

TEST(VoltageTableFunc, ClampsHigh) {
    voltage_table_conf_s cfg = makeConf(
        {1.0f, 2.0f, 3.0f},
        {80.f, 40.f, 20.f}
    );
    VoltageTableFunc f;
    f.configure(cfg);

    // Above highest breakpoint → clamp to last temp
    SensorResult r = f.convert(5.0f);
    ASSERT_TRUE(r.Valid);
    EXPECT_NEAR(20.f, r.Value, 0.01f);
}

// ---------------------------------------------------------------------------
// Monotonicity validation: truncates at first non-ascending voltage
// ---------------------------------------------------------------------------
TEST(VoltageTableFunc, TruncatesAtNonMonotonicPoint) {
    // Points 0..2 are good; point 3 breaks monotonicity
    voltage_table_conf_s cfg = makeConf(
        {0.5f, 1.0f, 2.0f, 1.5f, 3.0f},  // 1.5 <= 2.0 → stop here
        {90.f, 70.f, 40.f, 30.f, 10.f}
    );
    VoltageTableFunc f;
    f.configure(cfg);

    // Should have loaded only 3 valid points
    EXPECT_EQ(3, f.m_count);

    // Behaviour uses only the 3 good points
    EXPECT_NEAR(40.f, f.convert(2.0f).Value, 0.01f);
}

// ---------------------------------------------------------------------------
// Not configured: fewer than 2 points → Low error
// ---------------------------------------------------------------------------
TEST(VoltageTableFunc, ReturnsErrorWhenNotConfigured) {
    VoltageTableFunc f;
    // configure() never called — m_count stays 0
    SensorResult r = f.convert(2.5f);
    ASSERT_FALSE(r.Valid);
    EXPECT_EQ(UnexpectedCode::Low, r.Code);
}

TEST(VoltageTableFunc, ReturnsErrorWithOnlyOnePoint) {
    voltage_table_conf_s cfg = makeConf({1.0f}, {50.f});
    VoltageTableFunc f;
    f.configure(cfg);
    EXPECT_EQ(1, f.m_count);

    SensorResult r = f.convert(1.0f);
    ASSERT_FALSE(r.Valid);
}

// ---------------------------------------------------------------------------
// Full 16-point table loads completely
// ---------------------------------------------------------------------------
TEST(VoltageTableFunc, AcceptsFullSixteenPointTable) {
    voltage_table_conf_s cfg{};
    for (int i = 0; i < VOLTAGE_TABLE_POINTS; i++) {
        cfg.voltageTable[i] = 0.3f * i;          // 0.0, 0.3, 0.6 … 4.5 V
        cfg.tempCTable[i]   = 100.f - 10.f * i;  // 100, 90, 80 … -50 °C
    }
    VoltageTableFunc f;
    f.configure(cfg);
    EXPECT_EQ(VOLTAGE_TABLE_POINTS, f.m_count);

    // Mid-table check at 1.5 V (index 5 exactly)
    EXPECT_NEAR(50.f, f.convert(1.5f).Value, 0.01f);
}
