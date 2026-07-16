// web_page.h — wireless firmware update page + OTA plumbing.
//
// Single self-hosted page (no external CSS/JS/font dependencies — the
// browser is connected to this device's own WiFi AP, which has no internet
// route). Pattern ported from ~/repos/Amidala's web/update.html and
// src/wifi_ap.cpp OTA handlers, trimmed down since Body Controller only
// needs this one page (Amidala's ~18-page setup uses a build-time embed
// script that isn't worth the overhead here).
#pragma once

#include <WiFi.h>
#include <WebServer.h>

extern WebServer webServer;
extern bool otaWebServerRunning;

// Route handlers only need registering once; WiFi radio and the server
// socket are what actually get started/stopped by WIFION/WIFIOFF.
void setupOTARoutes();

// Self-hosted AP (no external router dependency) + mDNS + web server. Call
// once from setup(), and again any time WiFi is re-enabled after
// stopOTAWebServer() (see "BD:WIFION" / "BD:WIFIOFF" in doCommand()).
void startOTAWebServer();

// Fully powers down the WiFi radio — for cutting RF noise during normal
// operation. OTA updates are unavailable until "BD:WIFION" brings it back.
void stopOTAWebServer();
