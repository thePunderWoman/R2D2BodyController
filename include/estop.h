// estop.h — E S T O P: emergency stop, and the blocking-delay primitive that
// watches for it.
#pragma once

#include <Arduino.h>

// ESP32 has no AVR-style "jump to address 0" reset vector; ESP.restart()
// triggers a real chip reset, which re-runs setup() the same way — and
// setup() always calls resetServos(). Used by performEStop() as the only
// way out of its RESET wait, so ESTOP never has to unwind back into
// whatever sequence it interrupted.
void softReset();

// Releases every servo (goes limp/unpowered, no fighting the Maestro to hold
// a position) and then blocks forever — no other command, sequence, or panel
// update runs again. The only way out is "BD:RESET", which reboots the board
// (see softReset above) rather than trying to resume whatever was
// interrupted. Never returns.
void performEStop();

// Watches for "BD:ESTOP" arriving mid-sequence, using its own local buffer —
// deliberately separate from readSerial()'s, so this can't affect the
// timing of any other command. Any other completed line seen here is just
// discarded; ESTOP needs to be instant, everything else can wait its turn.
bool checkForEstop();

// Blocking delay used throughout the sequences — checks for ESTOP and
// releases idle servos while it waits, since sequences spend most of their
// time in here rather than in loop().
void waitTime(unsigned long duration);
