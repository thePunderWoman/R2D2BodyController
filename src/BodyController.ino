// Jessica's Astromech Body Controller Firmware

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "config.h"
#include "bus.h"
#include "maestro.h"
#include "vocalizer.h"
#include "doors.h"
#include "estop.h"
#include "sequences.h"
#include "serial_commands.h"
#include "web_page.h"
#include "wcb_mesh.h"

void setup()
{
  // Native USB CDC console (separate from the WCB/Maestro hardware UARTs).
  // Its write() is a silent no-op until begin() initializes the CDC tx
  // buffers, so without this every DEBUG_PRINT call vanishes with no error.
  Serial.begin(9600);
  unsigned long usbWaitStart = millis();
  while (!Serial && millis() - usbWaitStart < 2000) { delay(10); } // give a monitor a moment to attach, but don't hang if unattended

  WCBSerial.begin(WCB_BAUD, SERIAL_8N1, WCB_RX_PIN, WCB_TX_PIN);
  MaestroSerial.begin(MAESTRO_BAUD, SERIAL_8N1, MAESTRO_RX_PIN, MAESTRO_TX_PIN);

  pinMode(MAESTRO_RST_PIN, OUTPUT);
  digitalWrite(MAESTRO_RST_PIN, HIGH); // idle high; active low, see maestroHardReset()
  pinMode(MAESTRO_ERR_PIN, INPUT);

  DEBUG_PRINT_LN(F("Body Controller " FIRMWARE_VERSION " (" MCU_VARIANT ")"));
  DEBUG_PRINT_LN(F("Command serial ready (UART0 @ 9600)"));

  pinMode(STATUS_LED, OUTPUT); // turn status led off
  digitalWrite(STATUS_LED, LOW);

  DEBUG_PRINT(F("Activating Servos"));
  resetServos();

  setupOTARoutes(); // registers / , /api/info, /update — see web_page.h
  startOTAWebServer(); // WiFi AP + mDNS + web server; "BD:WIFIOFF" tears this back down

  beginWCBMesh(); // after the OTA AP is up so ESP-NOW can ride its channel

  DEBUG_PRINT_LN(F("Setup Complete"));
}

void loop() {
  readSerial();
  updateWCBMesh();
  if (otaWebServerRunning) webServer.handleClient();
  releaseIdleServos(); // stops holding a channel under power once its move has settled

  // Cheap digitalRead every loop; the actual Get Errors round-trip only
  // happens (at most once/sec) while the Maestro is actively flagging one,
  // so a persistent error can't flood the serial line or the debug console.
  static unsigned long lastErrorCheck = 0;
  if (digitalRead(MAESTRO_ERR_PIN) == HIGH && millis() - lastErrorCheck > 1000) {
    lastErrorCheck = millis();
    maestroReportErrors();
  }
}
