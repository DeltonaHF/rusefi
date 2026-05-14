#pragma once
#include <stdint.h>
#include <stddef.h>

static constexpr int COMBINED_PATTERN_ENTRIES_MAX = 16;

/**
 * Combined crank+cam pattern descriptor for symmetric crank wheels that
 * cannot self-sync on crank signal alone.
 *
 * The event stream is encoded as a shift register: 1=CRANK, 0=CAM.
 * On each relevant event (filtered by edge selection), shift left by 1
 * and OR in the new bit. Compare the shift register against the table of
 * known unique windows using a validity mask that fills with 1s as events
 * arrive. Exactly one match = unique position identified = sync.
 *
 * This directly mirrors the original IAW ECU assembler algorithm.
 */
struct CombinedTriggerPattern {
    // Each entry is one unique 8-bit window into the repeating pattern.
    // 1=CRANK, 0=CAM, MSB=oldest event.
    uint8_t windowTable[COMBINED_PATTERN_ENTRIES_MAX];

    // Sequencer index (0-based position in the 720° cycle) for each
    // window entry — tells the matcher which tooth we are on when matched.
    uint8_t syncIndex[COMBINED_PATTERN_ENTRIES_MAX];

    // Number of valid entries in the table
    uint8_t tableSize = 0;

    // Number of events in each window (how many bits are significant).
    // Must be <= 8. Once collectedBits >= windowBits, matching starts.
    uint8_t windowBits = 8;

    // Total events per full 720° cycle (crank + cam together)
    uint8_t cycleLength = 0;

    // How many crank teeth per 720° — passed as divider to syncEnginePhaseAndReport
    uint8_t crankTeethPerCycle = 0;
};