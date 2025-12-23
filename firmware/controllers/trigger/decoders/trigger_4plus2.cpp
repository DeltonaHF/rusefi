/**
 * @file trigger_4plus2.cpp
 *
 * @date December 20, 2024
 * @author [Your Name]
 *
 * 4+2 trigger decoder
 * Crank: 4 equally spaced teeth (90 degrees apart)
 * Cam: 2 teeth (180 crank degrees apart, used for sync only)
 * VR sensors - rising edge triggering only
 */

#include "pch.h"
#include "trigger_4plus2.h"

/**
 * Crank wheel: 4 equally spaced teeth at 90 degree intervals
 * Cam wheel: 2 teeth, first at ~50° after TDC, second at 180° after first
 * 
 * Trigger pattern (rising edges only, VR sensors):
 * Crank: |    |    |    |    |    |    |    |    (8 teeth over 720°)
 *        0    90   180  270  360  450  540  630
 * 
 * Cam:   ---|---------|-------------------------  (2 teeth)
 *        50              230 (50+180)
 * 
 * Fast sync strategy:
 * - Count crank teeth between cam teeth
 * - Between cam tooth 1 and cam tooth 2: expect 2 crank teeth (at 90°, 180°)
 * - Between cam tooth 2 and cam tooth 1: expect 6 crank teeth (at 270°, 360°, 450°, 540°, 630°, 720°=0°)
 * - This pattern allows sync after seeing just 1 cam tooth + subsequent crank teeth
 * 
 * Possible sync scenarios:
 * 1) See cam tooth -> count crank teeth -> if 2 teeth then next cam, if 6 teeth then back to first cam
 * 2) This allows position lock within 180° worst case instead of full 720°
 */

void initialize4Plus2(TriggerWaveform *s) {
	s->initialize(FOUR_STROKE_CAM_SENSOR, SyncEdge::RiseOnly);

	// First cam tooth comes approximately 50 degrees after TDC #1
	// Adjust this value based on actual cam sensor position (±15 degrees tolerance)
	float camOffset1 = 50.0f;
	float camOffset2 = camOffset1 + 180.0f; // Second cam tooth 180 crank degrees after first
	
	// TDC position - first cam tooth is our reference point for sync
	s->tdcPosition = 720.0f - camOffset1;

	// This trigger requires both crank and cam inputs
	s->needSecondTriggerInput = true;

	// Crank teeth - secondary wheel for positioning
	// 4 teeth per revolution, 90 degrees apart
	// VR sensor - only rising edges are added as events
	
	int toothWidth = 5;
	
	// First revolution (0-360 degrees)
	s->addToothRiseFall(0.0f, toothWidth, TriggerWheel::T_SECONDARY);
	s->addToothRiseFall(90.0f, toothWidth, TriggerWheel::T_SECONDARY);
	s->addToothRiseFall(180.0f, toothWidth, TriggerWheel::T_SECONDARY);
	s->addToothRiseFall(270.0f, toothWidth, TriggerWheel::T_SECONDARY);
	
	// Second revolution (360-720 degrees)
	s->addToothRiseFall(360.0f, toothWidth, TriggerWheel::T_SECONDARY);
	s->addToothRiseFall(450.0f, toothWidth, TriggerWheel::T_SECONDARY);
	s->addToothRiseFall(540.0f, toothWidth, TriggerWheel::T_SECONDARY);
	s->addToothRiseFall(630.0f, toothWidth, TriggerWheel::T_SECONDARY);

	// Cam teeth - primary wheel for synchronization
	// VR sensor - only rising edges matter, but addToothRiseFall handles this
	
	// Cam tooth 1 at ~50 degrees after TDC
	s->addToothRiseFall(camOffset1, toothWidth, TriggerWheel::T_PRIMARY);
	
	// Cam tooth 2 at 180 crank degrees after first cam tooth
	s->addToothRiseFall(camOffset2, toothWidth, TriggerWheel::T_PRIMARY);

	// Enable fast sync by NOT using only primary for sync
	// This allows the decoder to use the relationship between crank and cam teeth
	s->useOnlyPrimaryForSync = false;
	
	// Set synchronization gaps for cam teeth (primary wheel)
	// Gap between cam teeth: 180 degrees (2 crank teeth)
	// Gap after second cam tooth back to first: 540 degrees (6 crank teeth)
	// Ratio: 540/180 = 3.0
	s->setTriggerSynchronizationGap2(1.5f, 4.5f);
	
	// Set secondary (crank) synchronization gap
	// All crank teeth are equally spaced at 90 degrees
	// This tells decoder: "crank teeth are all equal, use cam for sync"
	// nominal gap = 1.0 (all gaps equal)
	s->setSecondTriggerSynchronizationGap2(0.7f, 1.3f);
	
	// Enable gap detection on third tooth for fast sync
	// This allows pattern recognition: cam -> 2 cranks -> cam (short pattern)
	//                             vs: cam -> 6 cranks -> cam (long pattern)
	// Gap index 0: between cam teeth (short, 2 crank teeth) - nominal ratio ~1.0
	// Gap index 1: long gap with 6 crank teeth - nominal ratio ~3.0
	s->setTriggerSynchronizationGap3(/*gapIndex*/0, 0.5f, 1.5f);   // short cam gap
	s->setTriggerSynchronizationGap3(/*gapIndex*/1, 2.0f, 5.0f);   // long cam gap
}