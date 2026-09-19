// wcb_hcr_transport.h — bridges HumanCyborgRelationsAPI's HCRTransport
// interface (hcr.h) to a WCB that owns the HCR board natively (WCB firmware
// 6.1.0+, configured on the WCB with ?HCR,PORT,Sx:baud).
//
// A WCB running native HCR removes that port from its broadcast loop, so the
// library's bare "<...>" frames -- which a WCB treats as broadcast strings --
// can never reach the HCR, whether they arrive over the mesh or over UART0.
// The WCB's own verb for this is ";H,RAW,<frame>", which puts the identical
// bytes on the HCR's wire. Every frame is wrapped that way here.
//
// Two ways out, both carrying the wrapped line:
//   1. Unicast over the mesh to WCB_HCR_TARGET_WCB (see sendHcrViaMesh()).
//   2. UART0 to the wired WCB, when the mesh isn't joined or that WCB is
//      offline. A command entering a WCB on a serial port gets forwarded to
//      its HCR host, so this needs no WCB number or port.
//
// send() ALWAYS returns true. HCRVocalizer::transmit() falls back to writing
// the bare frame to WCBSerial whenever it gets false, and a bare frame is
// exactly what a native WCB drops -- so the UART0 fallback lives here instead
// of in the library. See wcb_mesh.cpp for where this is wired in via
// HCR.setExternalTransport() (unconditionally, even if the mesh fails to come
// up, for the same reason).
#pragma once

#include <stdio.h>
#include <hcr.h>
#include "bus.h"
#include "config.h"
#include "wcb_mesh.h"

// WCB command character (';', the WCB default -- ?CMDCHAR can change it),
// HCR device 'H', RAW verb.
#define HCR_NATIVE_PREFIX ";H,RAW,"

// One mesh packet holds 199 chars, 187 once the checksum suffix is on.
#define HCR_NATIVE_LINE_MAX 187

class WCBHcrTransport : public HCRTransport {
public:
  bool send(const char *command) override {
    char line[HCR_NATIVE_LINE_MAX + 1];
    int n = snprintf(line, sizeof(line), HCR_NATIVE_PREFIX "%s", command);
    if (n <= 0 || n > HCR_NATIVE_LINE_MAX) {
      // Unsendable (empty/oversize): swallow it rather than leak a bare
      // frame the WCB would just drop anyway.
      DEBUG_PRINT_LN(F("HCR: command too long for one WCB line, dropped"));
      return true;
    }

    if (sendHcrViaMesh(line)) return true;

    // Same idle-line priming newline HCRVocalizer prepends on its own serial
    // path, in one write like sendBusCommand() (the WCB ignores empty lines).
    COMMAND_SERIAL.printf("\n%s\n", line);
    return true;
  }
};
