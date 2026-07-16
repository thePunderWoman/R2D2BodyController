// maestro.h — Pololu Maestro servo driver (Compact Protocol over
// MaestroSerial). The Maestro does its own speed-limited interpolation in
// hardware, so unlike the ReelTwo ServoDispatchDirect this replaced, nothing
// here needs AnimatedEvent::process() pumped to make progress — sequences
// just fire a Set Speed + Set Target pair and the Maestro takes it from
// there.
#pragma once

#include <Arduino.h>

extern HardwareSerial MaestroSerial;

// Sends a raw Set Target (0x84) — used directly (not via moveServo) by
// performEStop() to zero every channel without going through the
// speed/position bookkeeping moveServo() does.
void maestroSetTarget(uint8_t channel, uint16_t pulseUs);

void moveServo(uint8_t servoIndex, uint16_t pos, uint8_t speed);

// Called from loop() (and waitTime(), since sequences spend most of their
// time blocked there); releases any channel whose commanded move should
// have finished at least SERVO_RELEASE_MARGIN_MS ago and hasn't been
// re-commanded since. A channel mid-sequence (repeatedly re-targeted) never
// hits this — only ones that have gone quiet while holding position.
void releaseIdleServos();

// Pulses RST low briefly then releases it — hard-resets just the Maestro's
// own microcontroller (not its stored channel config) without touching
// servos, lights, or vocalizer elsewhere on the droid. For recovering a
// Maestro that's stopped responding to Compact Protocol entirely; a normal
// "BD:RESET" can't fix that since it only works by sending more serial
// commands. See "BD:MRESET" in doCommand().
void maestroHardReset();

// Polled from loop() whenever MAESTRO_ERR_PIN reads HIGH; decodes the
// bitmask per the Maestro user's guide section 4.e for DEBUG_PRINT output.
void maestroReportErrors();
