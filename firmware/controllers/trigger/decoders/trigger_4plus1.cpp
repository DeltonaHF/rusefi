/**
 * @file trigger_4plus1.cpp
 *
 * @date December 26, 2025
 * @author Nik
 *
 * 4+1 crank trigger decoder (VR sensors, rising edge only)
 * 
 * Applications:
 * - Fiat Tipo/Tempra 1.6 (Digiplex 2): Regular tooth 5° BTDC → Sync at TDC → 3 regular teeth
 * - Lancia Thema 16v (Digiplex 2s): Sync at TDC → Regular tooth 5° ATDC → 3 regular teeth
 * 
 * Trigger pattern: 4 equally spaced teeth (90° apart) + 1 sync tooth near TDC
 * 
 * Crank wheel physical layout:
 * - 4 regular teeth at 90° intervals
 * - 1 additional sync tooth offset ±5° from TDC
 * 
 * Tipo/Tempra (positive offset):
 *   Pattern: |   |          |	         |            |    
 *   Angles:  -5° 0°        85°         175°         265°
 *   Gaps:    short(5°)-long(85°)-regular(90°)-regular(90°)-regular(90°)
 * 
 * Thema (negative offset):
 *   Pattern: |   |           |            |           |
 *   Angles:  0°  5°         95°         185°         275°
 *   Gaps:    short(5°)-regular(90°)-regular(90°)-regular(90°)-long(85°)
 */

#include "pch.h"
#include "trigger_4plus1.h"

#if EFI_UNIT_TEST
	extern bool printTriggerDebug;
#endif

void initialize4Plus1(TriggerWaveform *s) {
	s->initialize(FOUR_STROKE_CRANK_SENSOR, SyncEdge::RiseOnly);

	// Configuration constants
	const unsigned short NUM_REGULAR_TEETH = 4;

	// Validate that cycle duration is evenly divisible by number of teeth
	if ((unsigned)s->getCycleDuration() % NUM_REGULAR_TEETH != 0) {
		firmwareError(ObdCode::CUSTOM_OBD_TRIGGER_WAVEFORM,
			"4+1 Trigger: Cycle duration %d must be evenly divisible by %d teeth",
			s->getCycleDuration(), NUM_REGULAR_TEETH);
		s->setShapeDefinitionError(true);
		return;
	}

	const unsigned REGULAR_TOOTH_ANGLE = s->getCycleDuration() / NUM_REGULAR_TEETH;  // 90°
	
	// TDC/Sync tooth offset from "regular" tooth
	// Positive: Tipo/Tempra 1.6 - regular tooth 5° BTDC, sync tooth at TDC
	// Negative: Thema 16v - sync tooth at TDC, regular tooth 5° ATDC
	// Must not exceed ±45° (half of regular tooth angle) for reliable detection
	const short SYNC_TOOTH_OFFSET = 5;
	
	// Validate sync offset at runtime
	if (abs(SYNC_TOOTH_OFFSET) >= REGULAR_TOOTH_ANGLE / 2) {
		firmwareError(ObdCode::CUSTOM_OBD_TRIGGER_WAVEFORM,
		"4+1 Trigger: SYNC_TOOTH_OFFSET=%d must be within ±%d° (half of tooth spacing)",
		SYNC_TOOTH_OFFSET, REGULAR_TOOTH_ANGLE / 2);
		s->setShapeDefinitionError(true); 
		return;
	}	
	
	const unsigned short TOOTH_WIDTH = 3;
	const unsigned short SYNC_TOOTH_INDEX = 2;  // Insert sync around 2nd regular tooth
	const float MIN_GAP_RATIO = 3.0f;  // Minimum ratio for reliable two-gap detection
	
	// Calculate sync tooth position
	const unsigned SYNC_TOOTH_ANGLE = SYNC_TOOTH_INDEX * REGULAR_TOOTH_ANGLE + SYNC_TOOTH_OFFSET;

	int tdcPositionOffset = 0;

	// Add teeth to waveform
	for (unsigned short i = 1; i <= NUM_REGULAR_TEETH; i++) {
		unsigned currToothAngle = i * REGULAR_TOOTH_ANGLE;
		
		// Insert sync tooth before this regular tooth if it falls in this interval
		if (SYNC_TOOTH_ANGLE < currToothAngle && 
		    SYNC_TOOTH_ANGLE > currToothAngle - REGULAR_TOOTH_ANGLE) {
			s->addToothRiseFall(SYNC_TOOTH_ANGLE, TOOTH_WIDTH);
		}
		
		s->addToothRiseFall(currToothAngle, TOOTH_WIDTH);
	}

	// Configure gap-based synchronization
	// The gap patterns differ fundamentally between positive and negative offsets
	
	if (SYNC_TOOTH_OFFSET > 0) {
		// Tipo/Tempra 1.6 (Digiplex 2)
		// Pattern: [regular tooth] --5°-- [TDC/sync] --85°-- [regular tooth] --90°-- [regular tooth] --90°-- [regular tooth] --90°-- 
		// Gap sequence: 
		// 1. short(5°) followed by long(85°)
		// 2. long(90°) followed by short(5°)
		
		unsigned short gapIndex = 0;
		unsigned shortGap = SYNC_TOOTH_OFFSET;
		unsigned longGap = REGULAR_TOOTH_ANGLE - SYNC_TOOTH_OFFSET;
		
		if (longGap > MIN_GAP_RATIO * shortGap) {
			// Use two-gap detection for robust synchronization
			// Primary: long gap after short gap (85°/5° ≈ 17:1 ratio)
			s->setTriggerSynchronizationGap3(gapIndex++,
				(float)longGap / shortGap * 0.66f, 
				(float)longGap / shortGap * 1.5f);
		} else {
			// Offset too large for reliable two-gap detection
			// Fall back to single-gap sync at a different tooth event
			// Compensate TDC position since sync occurs at different physical location
			tdcPositionOffset = SYNC_TOOTH_OFFSET - REGULAR_TOOTH_ANGLE;
		}

		// Secondary: short gap relative to regular tooth spacing (5°/90° ≈ 0.056)
		// Used as confirmation of sync position
		s->setTriggerSynchronizationGap3(gapIndex++,
			(float)shortGap / REGULAR_TOOTH_ANGLE * 0.66f,
			(float)shortGap / REGULAR_TOOTH_ANGLE * 1.5f);
			
	} else {
		// Lancia Thema 16v (Digiplex 2s)
		// Pattern: [TDC/sync] --5°-- [regular tooth] --90°-- [regular tooth] --90°-- [regular tooth] --90°-- [regular tooth] --85°--  
		// Gap sequence: 
		// 1. short(5°) followed by long(90°)
		// 2. long(85°) followed by short(5°)
		
		unsigned shortGap = abs(SYNC_TOOTH_OFFSET);
		unsigned longGap = REGULAR_TOOTH_ANGLE;
		
		// Primary: long gap after short gap (90°/5° = 18:1 ratio)
		s->setTriggerSynchronizationGap((float)longGap / shortGap);
		
		if (longGap + SYNC_TOOTH_OFFSET > MIN_GAP_RATIO * shortGap) {
			// Secondary: short gap relative to preceding interval
			// Used as confirmation of sync position
			s->setTriggerSynchronizationGap3(1,
				(float)shortGap / (longGap + SYNC_TOOTH_OFFSET) * 0.66f, 
				(float)shortGap / (longGap + SYNC_TOOTH_OFFSET) * 1.5f);
		}
	}

	// Set TDC position
	// Reference point is the sync tooth, with compensation for fallback mode if needed
	s->tdcPosition = s->getCycleDuration() - REGULAR_TOOTH_ANGLE + SYNC_TOOTH_OFFSET - tdcPositionOffset;

#if EFI_UNIT_TEST
	if (printTriggerDebug) {
		printf("4+1 Trigger initialized: offset=%d°, sync@%d°, TDC@%.1f°\n", 
			SYNC_TOOTH_OFFSET, SYNC_TOOTH_ANGLE, s->tdcPosition);
	}
#endif
}