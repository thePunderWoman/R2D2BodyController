#include "estop.h"
#include <string.h>
#include "config.h"
#include "bus.h"
#include "maestro.h"

void softReset() { ESP.restart(); }

void performEStop() {
  DEBUG_PRINT_LN(F("!!! ESTOP - releasing all servos !!!"));

  // Set Target 0 tells the Maestro to stop sending pulses on that channel
  // entirely — the servo goes limp/unpowered rather than being held in
  // place. No need to read back position or fight the Maestro's own
  // interpolation; this is just its normal "channel off" behavior.
  for (uint8_t i = 0; i < NBR_SERVOS; i++) {
    maestroSetTarget(i, 0);
  }

  digitalWrite(STATUS_LED, HIGH); // solid on = estopped; send RESET to clear

  char buf[16];
  uint8_t idx = 0;
  while (true) {
    if (COMMAND_SERIAL.available()) {
      char c = COMMAND_SERIAL.read();
      if (c == '\n' || c == '\r') {
        if (idx > 0) {
          buf[idx] = '\0';
          idx = 0;
          if (strcmp(buf, "BD:RESET") == 0) softReset();
        }
      } else if (idx < sizeof(buf) - 1) {
        buf[idx++] = c;
      }
    }
  }
}

bool checkForEstop() {
  static char buf[16];
  static uint8_t idx = 0;

  while (COMMAND_SERIAL.available()) {
    char c = COMMAND_SERIAL.read();
    if (c == '\n' || c == '\r') {
      if (idx > 0) {
        buf[idx] = '\0';
        idx = 0;
        if (strcmp(buf, "BD:ESTOP") == 0) return true;
      }
    } else if (idx < sizeof(buf) - 1) {
      buf[idx++] = c;
    }
  }
  return false;
}

void waitTime(unsigned long duration)
{
  unsigned long endTime = millis() + duration;
  while (millis() < endTime)
  {
    if (checkForEstop()) performEStop(); // never returns
    releaseIdleServos(); // sequences spend most of their time in here, not loop()
  }
}
