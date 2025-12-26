/**
 * @file trigger_4plus2.1
 *
 * @date December 20, 2024
 * @author [Your Name]
 *
 * 4+1 crank trigger decoder
 * Crank only: 4 equally spaced teeth (90 degrees apart) and one sync teeth 5 deg BTDC 1&4
 * VR sensors - rising edge triggering
 */

#pragma once

class TriggerWaveform;

void initialize4Plus1(TriggerWaveform *s);