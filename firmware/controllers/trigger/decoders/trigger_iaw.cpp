/*
 * @file trigger_iaw.cpp
 *
 * IAW 4+2 trigger decoder for Magneti Marelli IAW ECU.
 *
 * Crank wheel: 4 equally spaced teeth per revolution (90° apart),
 * disambiguated by a 2-tooth cam wheel with unequal spacing:
 *   - short gap: 2 crank teeth (180° crank = 90° cam)
 *   - long gap:  6 crank teeth (540° crank = 270° cam)
 *
 * Full combined event sequence per 720° cycle (10 events total):
 *   pos: 0    1   2    3    4   5    6    7    8    9
 *        CR   CA  CR   CR   CA  CR   CR   CR   CR   CR
 *         1    0   1    1    0   1    1    1    1    1
 *
 * Pattern matching uses an 8-bit shift register (1=CRANK, 0=CAM, MSB=oldest).
 * All 10 possible 8-event windows are unique except start=8 and start=9
 * (both produce 0xDB), so 9 entries cover all unambiguous positions.
 *
 * syncIndex stores the number of crank teeth seen before the window start
 * position — this is passed directly as the remainder to syncEnginePhaseAndReport.
 * The correct remainder value for TDC alignment must be verified from logs
 * (look for totalShift=0 or a consistent known offset).
 */

#include "pch.h"
#include "trigger_iaw.h"
#include "trigger_universal.h"


/**
 * Combined crank+cam pattern for IAW 4+2.
 *
 * windowTable: 8-bit shift register snapshot at each unambiguous position.
 *   Shift left on each new event, OR new bit into LSB (1=CRANK, 0=CAM).
 *   MSB holds the oldest event.
 *
 * syncIndex: number of crank teeth elapsed before the window start position.
 *   Used directly as remainder in syncEnginePhaseAndReport(8, remainder).
 *   Needs final calibration from log (totalShift should be stable and known).
 *
 * Excluded: start=8 and start=9 both produce 0xDB (ambiguous — two consecutive
 *   long runs of crank teeth make these windows identical).
 */
static const CombinedTriggerPattern iaw4Plus2Pattern = {
    //                                    pos sequence entering window (MSB→LSB)
    .windowTable = {
//      0b01111110,  // 0x7E  start=x: CA,CR,CR,CR,CR,CR,CR,CA // not used, as last event needs to be CR
        0b11111101,  // 0xFD  start=1: CR,CR,CR,CR,CR,CR,CA,CR
        0b11111011,  // 0xFB  start=2: CR,CR,CR,CR,CR,CA,CR,CR
//      0b11110110,  // 0xF6  start=x: CR,CR,CR,CR,CA,CR,CR,CA  // not used, as last event needs to be CR
        0b11101101,  // 0xED  start=3: CR,CR,CR,CA,CR,CR,CA,CR  
        0b11011011,  // 0xDB  start=4: CR,CR,CA,CR,CR,CA,CR,CR  
        0b10110111,  // 0xB7  start=5: CR,CA,CR,CR,CA,CR,CR,CR
        0b01101111,  // 0x6F  start=6: CA,CR,CR,CA,CR,CR,CR,CR
        0b11011111,  // 0xDF  start=7: CR,CR,CA,CR,CR,CR,CR,CR  (was missing 0b11011111)
        0b10111111,  // 0xBF  start=0: CR,CA,CR,CR,CR,CR,CR,CR // last CR is TDC firing #1
    },

    // Crank teeth elapsed before each window's start position:
    //   pos 0→CR(0), pos 1→CA(1cr), pos 2→CR(1cr+ca=1), pos 3→CR(2),
    //   pos 4→CA(3cr), pos 5→CR(3cr+ca=3), pos 6→CR(4), pos 7→CR(5)
    .syncIndex = {
        0,  // start=0: 0 crank teeth before pos 0
        1,  // start=1: 1 crank tooth  before pos 1 (pos 0 = CR)
        2,  // start=2: 2 crank teeth  before pos 3
        3,  // start=3: 3 crank teeth  before pos 4
        4,  // start=4: 4 crank teeth  before pos 6
        5,  // start=5: 5 crank teeth  before pos 7
        6,  // start=6: 4 crank teeth  before pos 6
        7,  // start=7: 5 crank teeth  before pos 7
    },

    .tableSize          = 8,   // 8 entries cover all 10 possible windows (cam events at the head of buffer not used)
    .windowBits         = 8,   // need 8 events before first match attempt
    .cycleLength        = 10,  // 8 crank + 2 cam events per 720°
    .crankTeethPerCycle = 8,   // divider passed to syncEnginePhaseAndReport
};

void configureIaw4Plus2Crank(TriggerWaveform* s) {
    // Configure the crank wheel as 4 equally spaced teeth per revolution.
    // Phase disambiguation is impossible from crank alone — the combined
    // crank+cam pattern matcher in trigger_central.cpp handles it.

  configure4ToothCrank(s);

    // Register the combined pattern. getTriggerCentral() is valid here
    // since this is called from applyNonPersistentConfiguration().
  getTriggerCentral()->combinedPattern = &iaw4Plus2Pattern;

    // Suppress crank sync until combined pattern match fires
  getTriggerCentral()->triggerState.setCombinedPatternReady(false);
}