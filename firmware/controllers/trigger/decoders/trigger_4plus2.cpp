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

#if EFI_UNIT_TEST
	extern bool printTriggerDebug;
#endif

void _4PLUS2_CRANK_PULSE(TriggerWaveform *s, unsigned idx, unsigned crankToothAngle, unsigned toothWidth) 
{
	s->addEvent720((((crankToothAngle) * (idx)) - (toothWidth)), TriggerValue::RISE, TriggerWheel::T_SECONDARY);
	s->addEvent720(((crankToothAngle) * (idx)), TriggerValue::FALL, TriggerWheel::T_SECONDARY);
#if EFI_UNIT_TEST
	if (printTriggerDebug)
	{
		printf(("Crank tooth #%02d, Angle %3.u°\n"), (idx), (crankToothAngle * (idx)));
	}
#endif
}

void _4PLUS2_CAM_PULSE(TriggerWaveform *s, unsigned idx, unsigned offset, unsigned camToothAngle, unsigned toothWidth)
	{
		s->addEvent720((((camToothAngle) * (idx)) + (offset) - (toothWidth)), TriggerValue::RISE, TriggerWheel::T_PRIMARY);
		s->addEvent720((((camToothAngle) * (idx)) + (offset)), TriggerValue::FALL, TriggerWheel::T_PRIMARY);
#if EFI_UNIT_TEST
		if (printTriggerDebug) 
		{
			printf("Cam   tooth #%02d, Angle %3.u°\n", idx, ((camToothAngle * (idx)) + (offset)));
		}
#endif
					
	}

void initialize4Plus2(TriggerWaveform *s) {
	s->initialize(FOUR_STROKE_CAM_SENSOR, SyncEdge::RiseOnly);

	// First cam tooth comes approximately 50 degrees after TDC #1
	// Adjust this value based on actual cam sensor position (±15 degrees tolerance)
	unsigned camOffset1 = 50;
	unsigned camOffset2 = camOffset1 + 180; // Second cam tooth 180 crank degrees after first	

	// Crank teeth - secondary wheel for positioning
	// 4 teeth per revolution, 90 degrees apart
	// VR sensor - only rising edges are added as events
	int countCrankTooth = 4;
	unsigned crankToothAngle = (s->getCycleDuration()/2)/countCrankTooth;
	unsigned camToothAngle = 180;
	unsigned toothWidth = 5;

	// TDC position - second cam tooth is our reference point for sync
	// we are specifying the distance from reference point for sync to next TDC (in engine rotation direction)
	s->tdcPosition = 720 - camOffset2 /* - toothWidth */;

	// This trigger requires both crank and cam inputs
	s->needSecondTriggerInput = true;

	// Enable fast sync by NOT using only primary for sync
	// This allows the decoder to use the relationship between crank and cam teeth
	s->useOnlyPrimaryForSync = false;
	
	// note: doublecheck this, although set to false, triggers.txt states the sync is needed
	// may also started without engine phase being identified (wasted spark instead of full seq.) 
	s->isSynchronizationNeeded = false; 	

	int i;
	// Cam tooth 1 at ~50 degrees after TDC
	_4PLUS2_CAM_PULSE(s, 0, camOffset1, camToothAngle, toothWidth);
	
	for(i=1; i<=2; i++)
		_4PLUS2_CRANK_PULSE(s, i, crankToothAngle, toothWidth);

	_4PLUS2_CAM_PULSE(s, 1, camOffset1, camToothAngle, toothWidth);
	
	for(i=3; i<=8; i++)
		_4PLUS2_CRANK_PULSE(s, i, crankToothAngle, toothWidth);

	// Cam teeth - primary wheel for synchronization
	// VR sensor - only rising edges matter for sync
	
	// Set synchronization gaps for cam teeth (primary wheel)
	// Gap between cam teeth: 180 degrees (2 crank teeth)
	// Gap after second cam tooth back to first: 540 degrees (6 crank teeth)
	// Ratio: 540/180 = 3.0
//	s->setTriggerSynchronizationGap2(1.5f, 4.5f);
	
	// Set secondary (crank) synchronization gap
	// All crank teeth are equally spaced at 90 degrees
	// This tells decoder: "crank teeth are all equal, use cam for sync"
	// nominal gap = 1.0 (all gaps equal)
//	s->setSecondTriggerSynchronizationGap2(0.6f, 1.4f);
	
	// Enable gap detection on third tooth for fast sync
	// This allows pattern recognition: cam -> 2 cranks -> cam (short pattern)
	//                             vs: cam -> 6 cranks -> cam (long pattern)
	// Gap index 0: between cam teeth (short, 2 crank teeth) - nominal ratio ~1.0
	// Gap index 1: long gap with 6 crank teeth - nominal ratio ~3.0
	s->setTriggerSynchronizationGap3(/*gapIndex*/0, 0.25f, 0.75f);   // short cam gap
	s->setTriggerSynchronizationGap3(/*gapIndex*/1, 1.5f, 3.5f);   // long cam gap
}