#include "bus.h"

#include "wcb_mesh.h"

HardwareSerial WCBSerial(0);

void sendPanelLightCommand(const char* cmd) {
  // Prefer unicasting straight to the panel lights over the mesh -- keeps
  // this traffic off the shared WCBSerial trunk entirely. Falls back to
  // WCBSerial below whenever the mesh target isn't reachable.
  if (sendPanelCommandViaMesh(cmd)) return;

  sendBusCommand(cmd);
}

void sendBusCommand(const char* cmd) {
  // ESP32 UART can corrupt or drop the first byte of a transmission after
  // the line's sat idle for a bit -- prime it with a throwaway newline so
  // that's what gets eaten instead of the first character of a real
  // command (which is what was showing up as a stray leading byte on
  // RGB-DPL's console, e.g. "ALLON" arriving as garbage+"ALLON").
  COMMAND_SERIAL.printf("\n%s\r\n", cmd);
}
