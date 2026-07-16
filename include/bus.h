// bus.h — the shared WCB command bus (UART0).
#pragma once

#include <Arduino.h>

// UART0, shared trunk for WCB commands in, HCR vocalizer commands out, and
// RGB-DPL panel / periscope commands out (see sendBusCommand()).
extern HardwareSerial WCBSerial;
#define COMMAND_SERIAL WCBSerial

// Plain-text commands out over the shared WCB trunk — RGB-DPL panel commands
// today (BRIGHTNESS, SCHEME, PERSONALITY, CBIMODE, ALLON/ALLOFF, CBION/
// CBIOFF, DPON/DPOFF, ... see https://github.com/thePunderWoman/RGB-DPL-Firmware
// for the full syntax), and periscope commands once those are added — both
// devices listen on the same trunk, so one generic sender covers either.
void sendBusCommand(const char* cmd);
