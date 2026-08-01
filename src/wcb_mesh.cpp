#include "wcb_mesh.h"

#include <WCB_Client.h>

#include "config.h"
#include "vocalizer.h"
#include "wcb_hcr_transport.h"
#include "wcb_secrets.h"

static WCB_Client WCBMesh(WCB_MESH_OCT2, WCB_MESH_OCT3, WCB_MESH_PASSWORD,
                           WCB_MESH_QUANTITY, WCB_MESH_DEVICE_ID);
static WCBHcrTransport HCRMeshTransport(&WCBMesh);
static bool meshReady = false;

void beginWCBMesh() {
  // Body Controller's OTA SoftAP (see startOTAWebServer(), called before
  // this in setup()) is brought up on WiFi's default AP channel (1) and
  // must be preserved for firmware updates -- WCB_Client detects a live
  // SoftAP and switches to WIFI_AP_STA instead of taking over the radio, but
  // only warns (doesn't force) if the mesh channel disagrees with the AP's.
  // Pin it explicitly so the two are a verified match instead of an
  // implicit "both happen to default the same way".
  WCBMesh.setMeshChannel(1);
  if (!WCBMesh.begin()) {
    DEBUG_PRINT_LN(F("WCB mesh: begin() failed -- HCR/panel commands stay on WCBSerial"));
    return;
  }
  meshReady = true;
  HCR.setExternalTransport(&HCRMeshTransport);
}

void updateWCBMesh() {
  if (meshReady) WCBMesh.update();
}

bool sendPanelCommandViaMesh(const char *cmd) {
  if (!meshReady || !WCBMesh.isOnline(WCB_PANEL_TARGET_ID)) return false;
  return WCBMesh.send(WCB_PANEL_TARGET_ID, cmd);
}
