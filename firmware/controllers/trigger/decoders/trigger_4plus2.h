/**
 * @file trigger_4plus2.h
 *
 * @date December 20, 2024
 * @author [Your Name]
 *
 * 4+2 trigger decoder
 * Crank: 4 equally spaced teeth (90 degrees apart)
 * Cam: 2 teeth (90 degrees apart, used for sync only)
 * VR sensors - rising edge triggering
 */

#pragma once

class TriggerWaveform;

void initialize4Plus2(TriggerWaveform *s);