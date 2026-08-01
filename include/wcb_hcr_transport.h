// wcb_hcr_transport.h — bridges HumanCyborgRelationsAPI's HCRTransport
// interface (hcr.h) to a unicast WCB_Client link, so HCR commands go
// straight to the WCB physically wired to the HCR board's serial input
// (WCB_HCR_TARGET_WCB / WCB_HCR_TARGET_PORT in config.h) over ESP-NOW,
// instead of sharing the WCBSerial trunk with bus/RGB-DPL traffic.
//
// HCRVocalizer::transmit() only skips its local WCBSerial write when
// send() returns true, so returning false here — target not currently
// online, or the raw send itself fails — hands the command back to the
// existing WCBSerial path automatically. See wcb_mesh.cpp for where this
// is wired in via HCR.setExternalTransport().
#pragma once

#include <string.h>
#include <hcr.h>
#include <WCB_Client.h>
#include "config.h"

class WCBHcrTransport : public HCRTransport {
public:
  explicit WCBHcrTransport(WCB_Client *client) : fClient(client) {}

  bool send(const char *command) override {
    if (!fClient->isOnline(WCB_HCR_TARGET_WCB)) return false;
    return fClient->sendRaw(WCB_HCR_TARGET_WCB, WCB_HCR_TARGET_PORT,
                             (const uint8_t *)command, strlen(command));
  }

private:
  WCB_Client *fClient;
};
