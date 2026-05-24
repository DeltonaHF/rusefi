/**
 * instant_rpm_calculator.h
 */

#pragma once
#include "trigger_structure.h"

class InstantRpmCalculator {
public:
	InstantRpmCalculator();
	float getInstantRpm() const {
		return m_instantRpm;
	}

#if EFI_ENGINE_CONTROL && EFI_SHAFT_POSITION_INPUT
	void updateInstantRpm(
			uint32_t current_index,
		TriggerWaveform const & triggerShape, TriggerFormDetails *triggerFormDetails,
		uint32_t index, efitick_t nowNt);
#endif
	/**
	 * Update timeOfLastEvent[] on every trigger event - even without synchronization
	 * Needed for early spin-up RPM detection.
	 */
	void setLastEventTimeForInstantRpm(efitick_t nowNt);

	void movePreSynchTimestamps();

	void resetInstantRpm() {
		setArrayValues(timeOfLastEvent, 0);
		setArrayValues(spinningEvents, 0);
		spinningEventIndex = 0;
		prevInstantRpmValue = 0;
		dInstantRpm = NAN;
		m_instantRpm = 0;
		setArrayValues(indexOfPreviousEvent, (uint16_t)-1);
	}

	/**
	 * timestamp of each trigger wheel tooth
	 */
	uint32_t timeOfLastEvent[PWM_PHASE_MAX_COUNT];

  /**
   * Instant RPM is calculated based on the time difference 
   * between current tooth and the tooth which is located 
   * 'stepBack' degrees before the current tooth.
   * Index of that tooth is stored in 'indexOfPreviousEvent' array.
   */
  uint16_t indexOfPreviousEvent[PWM_PHASE_MAX_COUNT];

	size_t spinningEventIndex = 0;

	// we might need up to one full trigger cycle of events - which on 60-2 means storage for ~120
	// todo: change the implementation to reuse 'timeOfLastEvent'
	uint32_t spinningEvents[120];
	/**
	 * instant RPM calculated at this trigger wheel tooth
	 */
	float instantRpmValue[PWM_PHASE_MAX_COUNT];
	/**
	 * Stores last non-zero instant RPM value to fix early instability
	 */
	float prevInstantRpmValue = 0;
	/**
	 * difference between current instant RPM and previous instant RPM
	 */
	float dInstantRpm = NAN;


	float m_instantRpm = 0;
  angle_t stepBack = 90; // degrees, will get set to an engine specific value by updateTriggerConfiguration()

private:
	float calculateInstantRpm(
		TriggerWaveform const & triggerShape, TriggerFormDetails *triggerFormDetails,
		uint32_t index, efitick_t nowNt);

	float m_instantRpmRatio = 0;
};
