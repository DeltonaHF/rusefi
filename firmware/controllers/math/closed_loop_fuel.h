// closed_loop_fuel.h

#pragma once

#include "closed_loop_fuel_cell.h"
#include "deadband.h"
#include "short_term_fuel_trim_state_generated.h"

struct stft_s;


struct LambdaActivityMonitor {
    void feed(float smoothedLambda) {
        if (!m_ready) {
            if (m_initialized && absF(smoothedLambda - m_lastValue) > m_threshold) {
                m_crossingCount++;
                if (m_crossingCount >= m_requiredCrossings) {
                    m_ready = true;
                }
            }
            m_lastValue = smoothedLambda;
            m_initialized = true;
        }
    }

    void configure(float threshold, uint8_t requiredCrossings) {
        m_threshold = threshold;
        m_requiredCrossings = requiredCrossings;
    }

    bool isReady() const { return m_ready; }

    void reset() {
        m_ready = false;
        m_initialized = false;
        m_crossingCount = 0;
        m_lastValue = 0;
    }

private:
    float m_threshold = 0.05f;
    float m_lastValue = 0;
    uint8_t m_requiredCrossings = 5;
    uint8_t m_crossingCount = 0;
    bool m_ready = false;
    bool m_initialized = false;
};

struct ClosedLoopFuelResult {
	ClosedLoopFuelResult() {
		// Default is no correction, aka 1.0 multiplier
		for (size_t i = 0; i < FT_BANK_COUNT; i++) {
			banks[i] = 1.0f;
		}
		region = ftRegionIdle;
	}

	float banks[FT_BANK_COUNT];
	ft_region_e region;
};

struct FuelingBank {
	ClosedLoopFuelCellImpl cells[STFT_CELL_COUNT];
};

class ShortTermFuelTrim : public EngineModule, public short_term_fuel_trim_state_s {
public:
	void init(stft_s *stftCfg);
	// EngineModule implementation
	void onSlowCallback() override;
	bool needsDelayedShutoff() override;

	ClosedLoopFuelResult getCorrection(float rpm, float fuelLoad);

#if ! EFI_UNIT_TEST
private:
#endif
	FuelingBank banks[FT_BANK_COUNT];
	LambdaActivityMonitor lambdaActivity[FT_BANK_COUNT];

	Deadband<25> idleDeadband;
	Deadband<2> overrunDeadband;
	Deadband<2> loadDeadband;

	SensorType getSensorForBankIndex(size_t index);
	ft_region_e computeStftBin(float rpm, float load, stft_s& cfg);
	stft_state_e getCorrectionState();
	stft_state_e getLearningState(SensorType sensor);
};

void initStft(void);

/* TODO: move out of here */
bool checkIfTuningVeNow();
