#include "serial_commands.h"
#include <string.h>
#include <stdlib.h>
#include "config.h"
#include "bus.h"
#include "maestro.h"
#include "estop.h"
#include "doors.h"
#include "sequences.h"
#include "vocalizer.h"
#include "web_page.h"

unsigned long loopTime; // Time variable

// CB<val>\n  — charge bay panel command (from WCB, forwarded to RGB-DPL)
// DP<val>\n  — data panel command (from WCB, forwarded to RGB-DPL)
// val encoding (legacy, kept for compatibility with existing WCB behavior):
// (seq * 10000) + (speed * 100) + duration_seconds.
// seq 0 = resume normal animation (bright), seq 1 = go dark/disabled.
// CB/DP address the CBI matrix and Data Panel independently, so they map to
// RGB-DPL's per-zone CBION/CBIOFF and DPON/DPOFF rather than the blanket
// ALLON/ALLOFF. speed/duration still have no direct RGB-DPL equivalent.
static void doCBILEDCommand(long val) {
  uint8_t seq = (uint8_t)(val / 10000);
  sendBusCommand(seq == 1 ? "CBIOFF" : "CBION");
}

static void doDPLEDCommand(long val) {
  uint8_t seq = (uint8_t)(val / 10000);
  sendBusCommand(seq == 1 ? "DPOFF" : "DPON");
}

// Marcduino body panel numbering used here:
// 0=all  1=top arm  2=bottom arm  3=left door  4=right door  5=CBI  6=data panel
// Adjust to match your body master's panel assignment if needed.
static void doMarcduinoOpen(uint8_t panel) {
  switch (panel) {
    case 0:
      moveServo(TOP_UTIL_ARM, TOP_ARM_OPEN, UTILITYARMSSPEED2);
      moveServo(BOTTOM_UTIL_ARM, BOTTOM_ARM_OPEN, UTILITYARMSSPEED2);
      moveServo(LEFT_DOOR, LEFT_DOOR_OPEN, DOOR_OPEN_SPEED);
      moveServo(RIGHT_DOOR, RIGHT_DOOR_OPEN, DOOR_OPEN_SPEED);
      moveServo(CBI_DOOR, CBI_DOOR_OPEN, DOOR_OPEN_SPEED);
      moveServo(DATA_DOOR, DATA_DOOR_OPEN, DOOR_OPEN_SPEED);
      sendBusCommand("ALLON");
      waitTime(900);
      topUtilityArmOpen = true; bottomUtilityArmOpen = true; utilityArmOpen = true;
      leftDoorOpen = true; rightDoorOpen = true;
      cbiDoorOpen = true; dataDoorOpen = true;
      doorsOpen = true; cbi_dataOpen = true;
      break;
    case 1:
      if (topUtilityArmOpen) return;
      topUtilityArmOpen = true;
      moveServo(TOP_UTIL_ARM, TOP_ARM_OPEN, UTILITYARMSSPEED);
      waitTime(900);
      break;
    case 2:
      if (bottomUtilityArmOpen) return;
      bottomUtilityArmOpen = true;
      moveServo(BOTTOM_UTIL_ARM, BOTTOM_ARM_OPEN, UTILITYARMSSPEED);
      waitTime(900);
      break;
    case 3:
      if (leftDoorOpen) return;
      leftDoorOpen = true;
      moveServo(LEFT_DOOR, LEFT_DOOR_OPEN, DOOR_OPEN_SPEED);
      waitTime(900);
      break;
    case 4:
      if (rightDoorOpen) return;
      rightDoorOpen = true;
      moveServo(RIGHT_DOOR, RIGHT_DOOR_OPEN, DOOR_OPEN_SPEED);
      waitTime(900);
      break;
    case 5:
      if (cbiDoorOpen) return;
      cbiDoorOpen = true;
      sendBusCommand("CBION");
      moveServo(CBI_DOOR, CBI_DOOR_OPEN, DOOR_OPEN_SPEED);
      waitTime(900);
      break;
    case 6:
      if (dataDoorOpen) return;
      dataDoorOpen = true;
      sendBusCommand("DPON");
      moveServo(DATA_DOOR, DATA_DOOR_OPEN, DOOR_OPEN_SPEED);
      waitTime(900);
      break;
  }
}

static void doMarcduinoClose(uint8_t panel) {
  switch (panel) {
    case 0:
      moveServo(TOP_UTIL_ARM, TOP_ARM_CLOSE, UTILITYARMSSPEED);
      moveServo(BOTTOM_UTIL_ARM, BOTTOM_ARM_CLOSE, UTILITYARMSSPEED);
      moveServo(LEFT_DOOR, LEFT_DOOR_CLOSE, DOOR_CLOSE_SPEED);
      moveServo(RIGHT_DOOR, RIGHT_DOOR_CLOSE, DOOR_CLOSE_SPEED);
      moveServo(CBI_DOOR, CBI_DOOR_CLOSE, DOOR_CLOSE_SPEED);
      moveServo(DATA_DOOR, DATA_DOOR_CLOSE, DOOR_CLOSE_SPEED);
      sendBusCommand("ALLOFF");
      waitTime(900);
      topUtilityArmOpen = false; bottomUtilityArmOpen = false; utilityArmOpen = false;
      leftDoorOpen = false; rightDoorOpen = false;
      cbiDoorOpen = false; dataDoorOpen = false;
      doorsOpen = false; cbi_dataOpen = false;
      break;
    case 1:
      if (!topUtilityArmOpen) return;
      topUtilityArmOpen = false;
      moveServo(TOP_UTIL_ARM, TOP_ARM_CLOSE, UTILITYARMSSPEED);
      waitTime(900);
      break;
    case 2:
      if (!bottomUtilityArmOpen) return;
      bottomUtilityArmOpen = false;
      moveServo(BOTTOM_UTIL_ARM, BOTTOM_ARM_CLOSE, UTILITYARMSSPEED);
      waitTime(900);
      break;
    case 3:
      if (!leftDoorOpen) return;
      leftDoorOpen = false;
      moveServo(LEFT_DOOR, LEFT_DOOR_CLOSE, DOOR_CLOSE_SPEED);
      waitTime(900);
      break;
    case 4:
      if (!rightDoorOpen) return;
      rightDoorOpen = false;
      moveServo(RIGHT_DOOR, RIGHT_DOOR_CLOSE, DOOR_CLOSE_SPEED);
      waitTime(900);
      break;
    case 5:
      if (!cbiDoorOpen) return;
      cbiDoorOpen = false;
      sendBusCommand("CBIOFF");
      moveServo(CBI_DOOR, CBI_DOOR_CLOSE, DOOR_CLOSE_SPEED);
      waitTime(900);
      break;
    case 6:
      if (!dataDoorOpen) return;
      dataDoorOpen = false;
      sendBusCommand("DPOFF");
      moveServo(DATA_DOOR, DATA_DOOR_CLOSE, DOOR_CLOSE_SPEED);
      waitTime(900);
      break;
  }
}

// :OP<nn>  :CL<nn>  :SE<nn>
static void doMarcduinoCommand(const char* cmd) {
  if (strncmp(cmd, "OP", 2) == 0) {
    doMarcduinoOpen((uint8_t)atoi(cmd + 2));
  } else if (strncmp(cmd, "CL", 2) == 0) {
    doMarcduinoClose((uint8_t)atoi(cmd + 2));
  } else if (strncmp(cmd, "SE", 2) == 0) {
    switch (atoi(cmd + 2)) {
      case 0: resetServos(); break;
      case 1: Scream(); break;
    }
  }
}

void readSerial() {
  static char buf[32];
  static uint8_t idx = 0;

  while (COMMAND_SERIAL.available()) {
    char c = COMMAND_SERIAL.read();
    if (c == '\n' || c == '\r') {
      if (idx > 0) {
        buf[idx] = '\0';
        if (strncmp(buf, "BD:", 3) == 0) {
          doCommand(buf + 3);
        } else if (strncmp(buf, "CB", 2) == 0) {
          doCBILEDCommand(atol(buf + 2));
        } else if (strncmp(buf, "DP", 2) == 0) {
          doDPLEDCommand(atol(buf + 2));
        } else if (buf[0] == ':') {
          doMarcduinoCommand(buf + 1);
        }
        idx = 0;
      }
    } else if (idx < sizeof(buf) - 1) {
      buf[idx++] = c;
    }
  }
}

// Wrapper so RESET fits the plain void() shape the command table expects,
// while keeping its original multi-step behavior (debug log + full reset).
static void doReset() {
  DEBUG_PRINT_LN(F("Got reset message"));
  resetServos();
  resetVocalizer();
  digitalWrite(STATUS_LED, HIGH);
}

// Wrapper so MRESET fits the plain void() shape the command table expects.
static void doMaestroReset() {
  DEBUG_PRINT_LN(F("Maestro hard reset"));
  maestroHardReset();
}

struct SerialCommand {
  const char* name;
  void (*fn)();
};

// BD:COMMAND dispatch table. To add a new command, add one line here — no
// other change needed. WIFI is the one exception, handled separately below
// since it takes a variable suffix (WIFI/WIFI0/WIFI1) rather than matching
// a single fixed string.
static const SerialCommand COMMAND_TABLE[] = {
  {"ESTOP", performEStop},
  {"MRESET", doMaestroReset},
  {"RESET", doReset},
  {"VADER", Vader},
  {"ROCKMARCH", RockMarch},
  {"THEME", Theme},
  {"BATTLEALARM", BattleAlarm},
  {"CLONES", Clones},
  {"DUEL", Duel},
  {"LUKEJABBA", LukeJabba},
  {"THRONE", Throne},
  {"CANTINA", Cantina},
  {"SCREAM", Scream},
  {"LEIA", Leia},
  {"OVERLOAD", overload},
  {"UARMS", UtilityArms},
  {"DOORS", Doors},
  {"LDOOR", openLeftDoor},
  {"RDOOR", openRightDoor},
  {"OPENALL", openEverything},
  {"CBIDOOR", openCBIDoor},
  {"DATADOOR", openDataDoor},
  {"CBIDATADOOR", openCBI_DataDoor},
  {"TOPARM", TopUtilityArm},
  {"BOTARM", BottomUtilityArm},
  {"HELLO", playHello},
  {"ALARM", alarm},
  {"HEART", heart},
  {"DISCO", Disco},
  {"FLUTTER", Flutter},
  {"STEPBACK", stepBack},
};
static const size_t COMMAND_TABLE_LEN = sizeof(COMMAND_TABLE) / sizeof(COMMAND_TABLE[0]);

void doCommand(const char* cmd) {
  loopTime = millis();
  DEBUG_PRINT(F("Serial command: "));
  DEBUG_PRINT_LN(cmd);

  if (strncmp(cmd, "WIFI", 4) == 0) {
    // WIFI0 = off, WIFI1 = on, bare WIFI = toggle — matches the
    // #APWIFI[0|1] / #PWIFI[0|1] convention used by AstroPixelsPlus and the
    // Periscope, just under our own BD: prefix.
    char suffix = cmd[4];
    if (suffix == '0') {
      DEBUG_PRINT_LN(F("WiFi off"));
      stopOTAWebServer();
    } else if (suffix == '1') {
      DEBUG_PRINT_LN(F("WiFi on"));
      startOTAWebServer();
    } else if (otaWebServerRunning) {
      DEBUG_PRINT_LN(F("WiFi off"));
      stopOTAWebServer();
    } else {
      DEBUG_PRINT_LN(F("WiFi on"));
      startOTAWebServer();
    }
    return;
  }

  for (size_t i = 0; i < COMMAND_TABLE_LEN; i++) {
    if (strcmp(cmd, COMMAND_TABLE[i].name) == 0) {
      COMMAND_TABLE[i].fn();
      return;
    }
  }

  digitalWrite(STATUS_LED, LOW);
}
