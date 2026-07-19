#include "bus.h"

HardwareSerial WCBSerial(0);

void sendBusCommand(const char* cmd) {
  // ESP32 UART can corrupt or drop the first byte of a transmission after
  // the line's sat idle for a bit -- prime it with a throwaway newline so
  // that's what gets eaten instead of the first character of a real
  // command (which is what was showing up as a stray leading byte on
  // RGB-DPL's console, e.g. "ALLON" arriving as garbage+"ALLON").
  COMMAND_SERIAL.printf("\n%s\r\n", cmd);
}
