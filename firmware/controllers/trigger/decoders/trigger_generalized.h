/**
 * @file trigger_generalized.h
 *
 * @date December 27, 2025
 * @author Nik (Deltona HF)
 *
 * Generalized N-tooth trigger decoder (VR sensors, rising edge only)
 * 
 * Accepts arbitrary tooth angles and automatically detects synchronization gaps
 * Optimizes starting position to maximize detectable sync gaps
 * 
 * Applications:
 * - Fiat Tipo/Tempra 1.6 (Digiplex 2): 4+1 pattern
 * - Lancia Thema 16v (Digiplex 2s): 4+1 pattern  
 * - Custom trigger wheels with arbitrary tooth placement
 */

#pragma once

class TriggerWaveform;

void initializeGeneralizedTrigger(TriggerWaveform *s, 
                                   const float *toothAngles, 
                                   unsigned short numTeeth,
                                   unsigned short toothWidth);

void initialize4Plus1Generalized(TriggerWaveform *s, short syncToothOffset);




void initialize4Plus1(TriggerWaveform *s);