#pragma once

#include "ignition_state_generated.h"

enum class IgnDiffCorrMode : uint8_t {
    Suppressed,
    Idle,
    OffIdle,
};
  
class IgnitionState : public ignition_state_s {
public:
	void updateDwell(float rpm, bool isCranking);
	void updateAdvanceCorrections(float engineLoad);
  void updateIgnDiffCorrection(uint32_t trgEventIndex);

  floatms_t getDwell() const;
  angle_t getWrappedAdvance(const float rpm, const float engineLoad);
  angle_t getTrailingSparkAngle(const float rpm, const float engineLoad);
	angle_t getSparkHardwareLatencyCorrection();

	static angle_t getInterpolatedIgnitionAngle(float rpm, float ignitionLoad);
	static angle_t getInterpolatedIgnitionTrim(size_t cylinderNumber, float rpm, float ignitionLoad);

	// Runtime state for ignition differential correction — not serialised
	uint8_t ignDiffCorrMapRampCount = 0;

private:
  angle_t getAdvance(float rpm, float engineLoad);
	floatms_t getSparkDwell(float rpm, bool isCranking);
};
