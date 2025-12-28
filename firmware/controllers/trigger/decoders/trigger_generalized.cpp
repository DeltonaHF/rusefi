/**
* @file trigger_generalized.c
*
* @date December 27, 2025
* @author Nik (DeltonaHF)
*
* Generalized N-tooth trigger decoder (VR sensors, rising edge only)
*
* Accepts arbitrary tooth angles and automatically detects synchronization gaps
*
* Applications:
* - Fiat Tipo/Tempra 1.6 (Digiplex 2): 4+1 pattern
* - Lancia Thema 16v (Digiplex 2s): 4+1 pattern
* - Custom trigger wheels with arbitrary tooth placement
*/

#include "pch.h"
#include "trigger_generalized.h"

#if EFI_UNIT_TEST
extern bool printTriggerDebug;
#endif

#define MAX_TEETH 60
#define MIN_GAP_RATIO 0.5f
#define MAX_GAP_RATIO 2.0f

typedef struct
{
	float gapSize;
	float ratio;
	bool isUnusual;
} GapInfo;

// Comparison function for qsort
static int compareFloat(const void *a, const void *b)
{
	float fa = *(const float *)a;
	float fb = *(const float *)b;
	if (fa < fb) return -1;
	if (fa > fb) return 1;
	return 0;
}

// Normalize angle to (0, 360] range
static float normalizeAngle(float angle, float cycleDuration)
{
	while (angle <= 0.0f)
	angle += cycleDuration;
	while (angle > cycleDuration)
	angle -= cycleDuration;
	return angle;
}

void initializeGeneralizedTrigger(TriggerWaveform *s,
const float *toothAngles,
unsigned short numTeeth,
unsigned short toothWidth)
{
	float sortedAngles[MAX_TEETH];
	GapInfo gaps[MAX_TEETH];
	unsigned short unusualIndices[MAX_TEETH];
	unsigned short numUnusual = 0;
	unsigned short i;
	float cycleDuration;
	bool foundTwoConsecutive = false;
	bool foundTwoNonConsecutive = false;
	unsigned short gapIdx1 = 0;
	unsigned short gapIdx2 = 0;
	unsigned short firstSyncGapIdx = 0;
	
	s->initialize(FOUR_STROKE_CRANK_SENSOR, SyncEdge::RiseOnly);
	cycleDuration = s->getCycleDuration();

	// Validate inputs
	if (numTeeth == 0)
	{
		firmwareError(ObdCode::CUSTOM_OBD_TRIGGER_WAVEFORM,
		"Generalized Trigger: No tooth angles provided");
		s->setShapeDefinitionError(true);
		return;
	}
	
	if (numTeeth > MAX_TEETH)
	{
		firmwareError(ObdCode::CUSTOM_OBD_TRIGGER_WAVEFORM,
		"Generalized Trigger: Too many teeth (%d > %d)", numTeeth, MAX_TEETH);
		s->setShapeDefinitionError(true);
		return;
	}

	// Normalize and sort tooth angles
	for (i = 0; i < numTeeth; i++)
	{
		sortedAngles[i] = normalizeAngle(toothAngles[i], cycleDuration);
	}
	qsort(sortedAngles, numTeeth, sizeof(float), compareFloat);
	
	// Last event has to end on cycleDuration (full turn)
	float lastTeethFullTurnOffset = cycleDuration - sortedAngles[numTeeth-1];
	for (i = 0; i < numTeeth; i++)
	{
		sortedAngles[i] += lastTeethFullTurnOffset;
	}

	#if EFI_UNIT_TEST
	if (printTriggerDebug)
	{
		printf("Sorted tooth angles:\n");
		for (i = 0; i < numTeeth; i++)
		{
			printf("  tooth[%d]: %.1f°\n", i, sortedAngles[i]);
		}
	}
	#endif

	// Add teeth to waveform
	for (i = 0; i < numTeeth; i++)
	{
		s->addToothRiseFall(sortedAngles[i], toothWidth);
	}

	// Calculate gaps: gap[i] = distance FROM tooth[i-1] TO tooth[i]
	gaps[0].gapSize = cycleDuration - sortedAngles[numTeeth - 1] + sortedAngles[0];
	for (i = 1; i < numTeeth; i++)
	{
		gaps[i].gapSize = sortedAngles[i] - sortedAngles[i - 1];
	}

	// Calculate ratios: ratio[i] = gap[i] / gap[i-1]
	for (i = 0; i < numTeeth; i++)
	{
		unsigned short prevIdx = (i == 0) ? numTeeth - 1 : i - 1;
		gaps[i].ratio = gaps[i].gapSize / gaps[prevIdx].gapSize;
		gaps[i].isUnusual = (gaps[i].ratio <= MIN_GAP_RATIO || gaps[i].ratio >= MAX_GAP_RATIO);
		
		if (gaps[i].isUnusual)
		{
			unusualIndices[numUnusual++] = i;
		}
	}

	#if EFI_UNIT_TEST
	if (printTriggerDebug)
	{
		printf("Gap analysis:\n");
		for (i = 0; i < numTeeth; i++)
		{
			printf("  gap[%2d]: %5.1f° (ratio=%.3f)%s\n",
			i, gaps[i].gapSize, gaps[i].ratio,
			gaps[i].isUnusual ? " [UNUSUAL]" : "");
		}
	}
	#endif

	if (numUnusual == 0)
	{
		firmwareError(ObdCode::CUSTOM_OBD_TRIGGER_WAVEFORM,
		"Generalized Trigger: No sync gaps found - all gaps similar");
		s->setShapeDefinitionError(true);
		return;
	}

	// Check for two consecutive unusual gaps (including wraparound)
	for (i = 0; i < numUnusual - 1; i++)
	{
		unsigned short nextIdx = (unusualIndices[i] + 1) % numTeeth;
		if (unusualIndices[i + 1] == nextIdx)
		{
			foundTwoConsecutive = true;
			gapIdx1 = unusualIndices[i];
			gapIdx2 = unusualIndices[i + 1];
			break;
		}
	}
	
	// Check wraparound: last and first unusual gaps
	if (!foundTwoConsecutive && numUnusual >= 2)
	{
		unsigned short nextIdx = (unusualIndices[numUnusual - 1] + 1) % numTeeth;
		if (unusualIndices[0] == nextIdx)
		{
			foundTwoConsecutive = true;
			gapIdx1 = unusualIndices[numUnusual - 1];
			gapIdx2 = unusualIndices[0];
		}
	}

	// Check for two non-consecutive unusual gaps
	if (!foundTwoConsecutive && numUnusual >= 2)
	{
		foundTwoNonConsecutive = true;
		gapIdx1 = unusualIndices[0];
		gapIdx2 = unusualIndices[1];
	}

	#if EFI_UNIT_TEST
	if (printTriggerDebug)
	{
		printf("Sync gap selection:\n");
		printf("  Found %d unusual gaps\n", numUnusual);
		if (foundTwoConsecutive)
		{
			printf("  Two consecutive: gap[%d] and gap[%d]\n", gapIdx1, gapIdx2);
		}
		else if (foundTwoNonConsecutive)
		{
			printf("  Two non-consecutive: gap[%d] and gap[%d]\n", gapIdx1, gapIdx2);
		}
		else
		{
			printf("  Single gap: gap[%d]\n", unusualIndices[0]);
		}
	}
	#endif

	// Configure synchronization
	if (foundTwoConsecutive || foundTwoNonConsecutive)
	{
		// Two-gap synchronization
		if (gapIdx2 < gapIdx1)
		{
			// Wraparound consecutive case (e.g., 4→0)
			s->setTriggerSynchronizationGap3(0,
			gaps[gapIdx2].ratio * 0.66f, gaps[gapIdx2].ratio * 1.5f);
			s->setTriggerSynchronizationGap3(numTeeth - gapIdx1,
			gaps[gapIdx1].ratio * 0.66f, gaps[gapIdx1].ratio * 1.5f);
			firstSyncGapIdx = gapIdx2;
		}
		else
		{
			// Normal consecutive case (e.g., 0→1, 1→2)
			// Gap index 0 = current gap (gapIdx2), Gap index 1 = previous gap (gapIdx1)
			s->setTriggerSynchronizationGap3(0,
			gaps[gapIdx2].ratio * 0.66f, gaps[gapIdx2].ratio * 1.5f);
			s->setTriggerSynchronizationGap3(gapIdx2 - gapIdx1,
			gaps[gapIdx1].ratio * 0.66f, gaps[gapIdx1].ratio * 1.5f);
			firstSyncGapIdx = gapIdx2;
		}
		
		#if EFI_UNIT_TEST
		if (printTriggerDebug)
		{
			printf("Two-gap sync configured:\n");
			printf("  Gap index 0: ratio=%.3f\n", gaps[firstSyncGapIdx].ratio);
			if (gapIdx2 < gapIdx1)
			{
				printf("  Gap index %d: ratio=%.3f\n", numTeeth - gapIdx1, gaps[gapIdx1].ratio);
			}
			else
			{
				printf("  Gap index %d: ratio=%.3f\n", gapIdx2 - gapIdx1, gaps[gapIdx1].ratio);
			}
		}
		#endif
	}
	else
	{
		// Single-gap synchronization - find most unusual gap
		float maxDeviation = 0.0f;
		unsigned short syncGapIdx = unusualIndices[0];
		
		for (i = 0; i < numUnusual; i++)
		{
			float deviation = (gaps[unusualIndices[i]].ratio > 1.0f)
			? gaps[unusualIndices[i]].ratio
			: 1.0f / gaps[unusualIndices[i]].ratio;
			
			if (deviation > maxDeviation)
			{
				maxDeviation = deviation;
				syncGapIdx = unusualIndices[i];
			}
		}
		
		float ratio = gaps[syncGapIdx].ratio;
		
		s->setTriggerSynchronizationGap(ratio);
		firstSyncGapIdx = syncGapIdx;

		#if EFI_UNIT_TEST
		if (printTriggerDebug)
		{
			printf("Single-gap sync: gap[%d] ratio=%.3f\n", syncGapIdx, ratio);
		}
		#endif
	}

	// Calculate TDC position: 360 - tooth[firstSyncGapIdx]
	s->tdcPosition = cycleDuration - (sortedAngles[firstSyncGapIdx] - lastTeethFullTurnOffset);
	while(s->tdcPosition > cycleDuration)
		s->tdcPosition -= cycleDuration;
	while(s->tdcPosition < cycleDuration)
	s->tdcPosition += cycleDuration;

	#if EFI_UNIT_TEST
	if (printTriggerDebug)
	{
		printf("Generalized Trigger initialized:\n");
		printf("  Teeth: %d\n", numTeeth);
		printf("  First sync gap index: %d\n", firstSyncGapIdx);
		printf("  Tooth at first sync gap: %.1f°\n", sortedAngles[firstSyncGapIdx]);
		printf("  TDC position: %.1f°\n", s->tdcPosition);
	}
	#endif
}

// Convenience function for 4+1 pattern (backwards compatibility)
void initialize4Plus1Generalized(TriggerWaveform *s, short syncToothOffset)
{
	const float REGULAR_TOOTH_ANGLE = 90.0f;
	float toothAngles[5];
	unsigned short numTeeth = 0;
	
	if (syncToothOffset == 0)
	{
	#if EFI_UNIT_TEST
		// Test pattern with single sync gap
		// Pattern: 15°, 25°, 35°, 45°, 55°
		// Gaps: 10° - 10° - 10° - 10° - 320° (wraparound)
//	one unusual gap with ratio (360-325) / (325-190)
/*		toothAngles[0] = 0;
		toothAngles[1] = 40;
		toothAngles[2] = 100;
		toothAngles[3] = 190;
		toothAngles[4] = 325;
		numTeeth = 5;
*/

//	one unusual gap with ratio (135-0) / (360-325)
/*		toothAngles[0] = 135;
		toothAngles[1] = 225;
		toothAngles[2] = 285;
		toothAngles[3] = 325;
		toothAngles[4] = 0;
		numTeeth = 5;
*/

//	symmetrical teeth
/*		toothAngles[0] = 90;
		toothAngles[1] = 180;
		toothAngles[2] = 270;
		toothAngles[3] = 360;
		numTeeth = 4;
*/
	#else
		firmwareError(ObdCode::CUSTOM_OBD_TRIGGER_WAVEFORM,
			"Generalized Trigger: Sync offset must be non zero");
			s->setShapeDefinitionError(true);
			return;
	#endif

	}
	else if (syncToothOffset > 0)
	{
		// Tipo/Tempra: Regular tooth 5° BTDC, sync at TDC
		// Pattern: 0°, 85°, 175°, 265°, 355°
		// Gaps: 5° - 85° - 90° - 90° - 90°
		toothAngles[0] = 0.0f;
		toothAngles[1] = REGULAR_TOOTH_ANGLE - syncToothOffset;
		toothAngles[2] = REGULAR_TOOTH_ANGLE + (REGULAR_TOOTH_ANGLE - syncToothOffset);
		toothAngles[3] = 2 * REGULAR_TOOTH_ANGLE + (REGULAR_TOOTH_ANGLE - syncToothOffset);
		toothAngles[4] = 3 * REGULAR_TOOTH_ANGLE + (REGULAR_TOOTH_ANGLE - syncToothOffset);
		numTeeth = 5;
	}
	else
	{
		// Thema: Sync at TDC, regular tooth 5° ATDC
		// Pattern: 0°, 5°, 95°, 185°, 275°
		// Gaps: 85° - 5° - 90° - 90° - 90°
		float offset = -syncToothOffset;
		toothAngles[0] = 0.0f;
		toothAngles[1] = offset;
		toothAngles[2] = REGULAR_TOOTH_ANGLE + offset;
		toothAngles[3] = 2 * REGULAR_TOOTH_ANGLE + offset;
		toothAngles[4] = 3 * REGULAR_TOOTH_ANGLE + offset;
		numTeeth = 5;
	}
	
	initializeGeneralizedTrigger(s, toothAngles, numTeeth, 3);
}