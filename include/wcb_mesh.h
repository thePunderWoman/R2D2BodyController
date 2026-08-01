// wcb_mesh.h — owns the WCB_Client ESP-NOW mesh connection Body Controller
// uses to unicast commands directly to their destination boards instead of
// sharing the WCBSerial trunk with everything else on it:
//   - HCR audio commands -> WCB_HCR_TARGET_WCB/PORT (see wcb_hcr_transport.h)
//   - RGB-DPL panel commands -> WCB_PANEL_TARGET_ID (see sendPanelCommandViaMesh)
// Both fall back to WCBSerial automatically whenever the mesh isn't joined
// or a send fails, so audio/lighting keep working even with WiFi off.
#pragma once

// Call once from setup(), after startOTAWebServer() -- WCB_Client detects
// the already-live SoftAP and rides its channel instead of force-moving
// the radio.
void beginWCBMesh();

// Call every loop() iteration -- required by WCB_Client's own contract.
void updateWCBMesh();

// Unicasts cmd as a text command to the RGB-DPL panel lights over the mesh.
// Returns true if handed off (caller should skip the WCBSerial write);
// false if the mesh target isn't reachable (caller should fall back to
// WCBSerial).
bool sendPanelCommandViaMesh(const char *cmd);
