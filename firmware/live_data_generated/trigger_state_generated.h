// this section was generated automatically by rusEFI tool config_definition_base-all.jar based on (unknown script) controllers/trigger/trigger_state.txt
// by class com.rusefi.output.CHeaderConsumer
// begin
#pragma once
#include "rusefi_types.h"
// start of trigger_state_s
struct trigger_state_s {
	/**
	 * sync: wheel sync counter
	 * offset 0
	 */
	uint32_t synchronizationCounter = (uint32_t)0;
	/**
	 * sync: shift reg 1=crank 0=cam
	 * offset 4
	 */
	uint8_t combinedShiftReg = (uint8_t)0;
	/**
	 * sync: #bits combinedShiftReg
	 * offset 5
	 */
	uint8_t combinedBitsCollected = (uint8_t)0;
	/**
	 * need 4 byte alignment
	 * units: units
	 * offset 6
	 */
	uint8_t alignmentFill_at_6[2] = {};
	/**
	 * units: us
	 * offset 8
	 */
	uint32_t vvtToothDurations0 = (uint32_t)0;
	/**
	 * "sync: Primary Position"
	 * offset 12
	 */
	float vvtCurrentPosition = (float)0;
	/**
	 * "sync: Cam Position"
	 * offset 16
	 */
	float vvtToothPosition[4] = {};
	/**
	 * @@GAUGE_NAME_TRG_GAP@@
	 * offset 32
	 */
	float triggerSyncGapRatio = (float)0;
	/**
	 * offset 36
	 */
	uint8_t triggerStateIndex = (uint8_t)0;
	/**
	 * offset 37
	 */
	int8_t triggerCountersError = (int8_t)0;
	/**
	 * need 4 byte alignment
	 * units: units
	 * offset 38
	 */
	uint8_t alignmentFill_at_38[2] = {};
};
static_assert(sizeof(trigger_state_s) == 40);

// end
// this section was generated automatically by rusEFI tool config_definition_base-all.jar based on (unknown script) controllers/trigger/trigger_state.txt
