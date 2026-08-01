// bus.h — the shared WCB command bus (UART0).
#pragma once

#include <Arduino.h>

// UART0, shared trunk for WCB commands in, HCR vocalizer commands out (when
// the WCB mesh unicast isn't available, see wcb_hcr_transport.h), and
// periscope commands out (see sendBusCommand()). RGB-DPL panel commands
// (see sendPanelLightCommand()) prefer a direct WCB mesh unicast and only
// fall back to this trunk if that's unreachable.
extern HardwareSerial WCBSerial;
#define COMMAND_SERIAL WCBSerial

// RGB-DPL panel commands (BRIGHTNESS, SCHEME, PERSONALITY, CBIMODE, ALLON/
// ALLOFF, CBION/CBIOFF, DPON/DPOFF, ... see
// https://github.com/thePunderWoman/RGB-DPL-Firmware for the full syntax).
// Tries a direct WCB mesh unicast to the panel lights first (see
// sendPanelCommandViaMesh() in wcb_mesh.h); falls back to sendBusCommand()
// over the shared trunk if the mesh target isn't reachable.
void sendPanelLightCommand(const char* cmd);

// Plain-text commands straight out over the shared WCB trunk — periscope
// commands (the panel lights aren't on this trunk anymore, see
// sendPanelLightCommand() above).
void sendBusCommand(const char* cmd);
