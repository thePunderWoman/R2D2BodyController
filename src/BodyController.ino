// Jessica's Astromech Body Controller Firmware

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "config.h"
#include "web_page.h"
#include <hcr.h>

// Command bus — UART0, shared trunk for WCB commands in, HCR vocalizer
// commands out, and RGB-DPL panel commands out (see sendPanelCommand()).
#define COMMAND_SERIAL WCBSerial

// ESP32 has no AVR-style "jump to address 0" reset vector; ESP.restart()
// triggers a real chip reset, which re-runs setup() the same way — and
// setup() always calls resetServos(). Used by performEStop() as the only
// way out of its RESET wait, so ESTOP never has to unwind back into
// whatever sequence it interrupted.
void softReset() { ESP.restart(); }

unsigned long loopTime; // Time variable

// Some variables to keep track of doors and arms etc.
bool utilityArmOpen = false;
bool topUtilityArmOpen = false;
bool bottomUtilityArmOpen = false;
bool leftDoorOpen = false;
bool rightDoorOpen = false;
bool cbiDoorOpen = false;
bool dataDoorOpen = false;
bool doorsOpen = false;
bool cbi_dataOpen = false;

HCRVocalizer HCR(&WCBSerial, WCB_BAUD);

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

  DEBUG_PRINT_LN(F("Body Controller " FIRMWARE_VERSION " (" MCU_VARIANT ")"));
  DEBUG_PRINT_LN(F("Command serial ready (UART0 @ 9600)"));

  pinMode(STATUS_LED, OUTPUT); // turn status led off
  digitalWrite(STATUS_LED, LOW);

  DEBUG_PRINT(F("Activating Servos"));
  resetServos();

  startOTAWebServer(); // WiFi AP + mDNS + /update, see web_page.h

  DEBUG_PRINT_LN(F("Setup Complete"));
}

void loop() {
  readSerial();
  webServer.handleClient();
}

//----------------------------------------------------------------------------
//  Vocalizer Commands
//----------------------------------------------------------------------------

void playScream() {
  HCR.Stimulate(SCARED, EMOTE_STRONG);
}

/**
 * @brief Plays the Leia audio
 * 
 */
void playLeia() {
  HCR.PlayWAV(CH_A, "0000");
}

void playSWTheme() {
  HCR.PlayWAV(CH_A, "0001");
}

void playSWThemeFull() {
  HCR.PlayWAV(CH_A, "0002");
}

void playCantina() {
  HCR.PlayWAV(CH_A, "0003");
}

void playCantinaFull() {
  HCR.PlayWAV(CH_A, "0004");
}

void playVader() {
  HCR.PlayWAV(CH_A, "0005");
}

void playVaderFull() {
  HCR.PlayWAV(CH_A, "0006");
}

void playDuel() {
  HCR.PlayWAV(CH_A, "0007");
}

void playThrone() {
  HCR.PlayWAV(CH_A, "0008");
}

void playClones() {
  HCR.PlayWAV(CH_A, "0009");
}

void playLukeJabba() {
  HCR.PlayWAV(CH_A, "0010");
}

void playHello() {
  HCR.PlayWAV(CH_A, "0011");
}

void playImperialAlarm() {
  HCR.PlayWAV(CH_A, "0012");
}

void playBattleAlarm() {
  HCR.PlayWAV(CH_A, "0013");
}

void playLove() {
  HCR.PlayWAV(CH_A, "0014");
}

void playRockMarch() {
  HCR.PlayWAV(CH_A, "0015");
}

void playDisco() {
  HCR.PlayWAV(CH_A, "0016");
}

// Emote Events

void enableMuse() {
  HCR.Muse(20,45);
  HCR.SetMuse(1);
}

void disableMuse() {
  HCR.SetMuse(0);
}

void resetVocalizer() {
  DEBUG_PRINT_LN(F("RESETTING VOCALIZER"));
  HCR.ResetEmotions();
  HCR.StopWAV(CH_A);
  enableMuse();
}

void overloadEmote() {
  HCR.Overload();
}

// Events
void Leia() {
  digitalWrite(STATUS_LED, HIGH);
  
  disableMuse();
  HCR.StopEmote();
  
  playLeia();
  
  waitTime(40000); // wait 40 seconds
  enableMuse();
  
  digitalWrite(STATUS_LED, LOW);
}

void Vader() {
  digitalWrite(STATUS_LED, HIGH);

  playVader();
  digitalWrite(STATUS_LED, LOW);
}

void RockMarch() {
  digitalWrite(STATUS_LED, HIGH);

  playRockMarch();
  digitalWrite(STATUS_LED, LOW);
}

void Disco() {
  digitalWrite(STATUS_LED, HIGH);

  playDisco();
  digitalWrite(STATUS_LED, LOW);
}

void Theme() {
  digitalWrite(STATUS_LED, HIGH);

  playSWTheme();
  digitalWrite(STATUS_LED, LOW);
}

void BattleAlarm() {
  digitalWrite(STATUS_LED, HIGH);

  playBattleAlarm();
  digitalWrite(STATUS_LED, LOW);
}

void Clones() {
  digitalWrite(STATUS_LED, HIGH);

  playClones();
  digitalWrite(STATUS_LED, LOW);
}

void Duel() {
  digitalWrite(STATUS_LED, HIGH);

  playDuel();
  digitalWrite(STATUS_LED, LOW);
}

void LukeJabba() {
  digitalWrite(STATUS_LED, HIGH);

  playLukeJabba();
  digitalWrite(STATUS_LED, LOW);
}

void Throne() {
  digitalWrite(STATUS_LED, HIGH);

  playThrone();
  digitalWrite(STATUS_LED, LOW);
}

void Cantina() {
  digitalWrite(STATUS_LED, HIGH);

  playCantina();

  // Doors alternate open/closed in time with the beat (130 BPM = ~461 ms/beat).
  // Left-to-right order: LEFT, DATA, CBI, RIGHT.
  // Even positions (LEFT, CBI) open while odd positions (DATA, RIGHT) close, then flip.
  const unsigned long BEAT_MS = 923; // every 2 beats at 130 BPM
  const unsigned long DURATION = 15000; // matches the 15-second clip length
  waitTime(88);
  const int CANTINA_SPEED = 200;        // fast enough to land on the beat

  const int TOP_ARM_HALF   = (TOP_ARM_OPEN    + TOP_ARM_CLOSE)    / 2; // ~1215
  const int BOTTOM_ARM_HALF = (BOTTOM_ARM_OPEN + BOTTOM_ARM_CLOSE) / 2; // ~1275

  sendPanelCommand("SCHEME CYBERPUNK");

  bool evenOpen = true;
  unsigned long endTime = millis() + DURATION;

  while (millis() < endTime) {
    if (evenOpen) {
      moveServo(LEFT_DOOR, LEFT_DOOR_OPEN, CANTINA_SPEED);
      moveServo(DATA_DOOR, DATA_DOOR_CLOSE, CANTINA_SPEED);
      moveServo(CBI_DOOR, CBI_DOOR_OPEN, CANTINA_SPEED);
      moveServo(RIGHT_DOOR, RIGHT_DOOR_CLOSE, CANTINA_SPEED);
      moveServo(TOP_UTIL_ARM, TOP_ARM_HALF, CANTINA_SPEED);
      moveServo(BOTTOM_UTIL_ARM, BOTTOM_ARM_CLOSE, CANTINA_SPEED);
    } else {
      moveServo(LEFT_DOOR, LEFT_DOOR_CLOSE, CANTINA_SPEED);
      moveServo(DATA_DOOR, DATA_DOOR_OPEN, CANTINA_SPEED);
      moveServo(CBI_DOOR, CBI_DOOR_CLOSE, CANTINA_SPEED);
      moveServo(RIGHT_DOOR, RIGHT_DOOR_OPEN, CANTINA_SPEED);
      moveServo(TOP_UTIL_ARM, TOP_ARM_CLOSE, CANTINA_SPEED);
      moveServo(BOTTOM_UTIL_ARM, BOTTOM_ARM_HALF, CANTINA_SPEED);
    }
    evenOpen = !evenOpen;
    waitTime(BEAT_MS);
  }

  // Close everything and clean up
  moveServo(LEFT_DOOR, LEFT_DOOR_CLOSE, DOOR_CLOSE_SPEED);
  moveServo(DATA_DOOR, DATA_DOOR_CLOSE, DOOR_CLOSE_SPEED);
  moveServo(CBI_DOOR, CBI_DOOR_CLOSE, DOOR_CLOSE_SPEED);
  moveServo(RIGHT_DOOR, RIGHT_DOOR_CLOSE, DOOR_CLOSE_SPEED);
  moveServo(TOP_UTIL_ARM, TOP_ARM_CLOSE, UTILITYARMSSPEED);
  moveServo(BOTTOM_UTIL_ARM, BOTTOM_ARM_CLOSE, UTILITYARMSSPEED);

  waitTime(1000);

  sendPanelCommand("SCHEME CLASSIC");

  leftDoorOpen = false;
  rightDoorOpen = false;
  cbiDoorOpen = false;
  dataDoorOpen = false;
  doorsOpen = false;
  cbi_dataOpen = false;
  utilityArmOpen = false;
  topUtilityArmOpen = false;
  bottomUtilityArmOpen = false;

  digitalWrite(STATUS_LED, LOW);
}

void overload() {
  digitalWrite(STATUS_LED, HIGH);

  overloadEmote();

  randomSeed(analogRead(0));

  const uint8_t NUM_PANELS = 6;
  const uint8_t panels[]    = { TOP_UTIL_ARM,      BOTTOM_UTIL_ARM,      LEFT_DOOR,          RIGHT_DOOR,          CBI_DOOR,          DATA_DOOR          };
  const int panelOpen[]     = { TOP_ARM_OPEN,       BOTTOM_ARM_OPEN,      LEFT_DOOR_OPEN,     RIGHT_DOOR_OPEN,     CBI_DOOR_OPEN,     DATA_DOOR_OPEN     };
  const int panelClose[]    = { TOP_ARM_CLOSE,      BOTTOM_ARM_CLOSE,     LEFT_DOOR_CLOSE,    RIGHT_DOOR_CLOSE,    CBI_DOOR_CLOSE,    DATA_DOOR_CLOSE    };

  // Shuffle panel order, then pick 2 or 3 to participate
  uint8_t order[] = { 0, 1, 2, 3, 4, 5 };
  for (int i = NUM_PANELS - 1; i > 0; i--) {
    int j = random(i + 1);
    uint8_t tmp = order[i]; order[i] = order[j]; order[j] = tmp;
  }
  uint8_t count = random(2, 4); // 2 or 3 panels

  sendPanelCommand("CBIMODE 6"); // first-pass guess: a glitchy/erratic mode

  // Lost-connection drift: each selected panel sluggishly creeps to a random
  // position up to halfway open, then stops as if it lost signal.
  for (uint8_t i = 0; i < count; i++) {
    uint8_t pi = order[i];
    int pos = panelClose[pi] + (long)(panelOpen[pi] - panelClose[pi]) * random(10, 51) / 100;
    moveServo(panels[pi], pos, OVERLOAD_DRIFT_SPEED);
    waitTime(random(400, 900));
  }

  waitTime(2500); // hold the glitched pose

  // Snap closed
  for (uint8_t i = 0; i < count; i++) {
    moveServo(panels[order[i]], panelClose[order[i]], SCREAM_SPEED);
  }

  sendPanelCommand("CBIMODE 0");

  waitTime(800);

  doorsOpen = false;
  leftDoorOpen = false;
  rightDoorOpen = false;
  cbi_dataOpen = false;
  cbiDoorOpen = false;
  dataDoorOpen = false;
  utilityArmOpen = false;
  topUtilityArmOpen = false;
  bottomUtilityArmOpen = false;

  digitalWrite(STATUS_LED, LOW);
}

void alarm() {
  digitalWrite(STATUS_LED, HIGH);

  playImperialAlarm();
  digitalWrite(STATUS_LED, LOW);
}

//----------------------------------------------------------------------------
//  E S T O P  —  emergency stop
//----------------------------------------------------------------------------
// Freezes every attached servo exactly where it is, right now, and then
// blocks forever — no other command, sequence, or LED update runs again.
// The only way out is "BD:RESET", which reboots the board (see softReset
// above) rather than trying to resume whatever was interrupted. Never
// returns.
void performEStop() {
  DEBUG_PRINT_LN(F("!!! ESTOP - freezing all servos in place !!!"));

  // Unlike VarSpeedServo/ReelTwo, the Maestro doesn't hold position just
  // because we stop commanding it — a channel keeps ramping toward whatever
  // target/speed it was last given. So freezing here means explicitly
  // reading back each channel's exact current position and re-targeting it
  // there with speed 0 (uncapped), which snaps it to a dead stop right where
  // it is and holds it under power indefinitely.
  for (uint8_t i = 0; i < NBR_SERVOS; i++) {
    uint16_t pos = maestroGetPosition(i);
    if (pos > 0) {
      maestroSetSpeed(i, 0);
      maestroSetTarget(i, pos);
    }
  }

  digitalWrite(STATUS_LED, HIGH); // solid on = estopped; send RESET to clear

  char buf[16];
  uint8_t idx = 0;
  while (true) {
    if (COMMAND_SERIAL.available()) {
      char c = COMMAND_SERIAL.read();
      if (c == '\n' || c == '\r') {
        if (idx > 0) {
          buf[idx] = '\0';
          idx = 0;
          if (strcmp(buf, "BD:RESET") == 0) softReset();
        }
      } else if (idx < sizeof(buf) - 1) {
        buf[idx++] = c;
      }
    }
  }
}

// Watches for "BD:ESTOP" arriving mid-sequence, using its own local buffer —
// deliberately separate from readSerial()'s, so this can't affect the
// timing of any other command. Any other completed line seen here is just
// discarded; ESTOP needs to be instant, everything else can wait its turn.
bool checkForEstop() {
  static char buf[16];
  static uint8_t idx = 0;

  while (COMMAND_SERIAL.available()) {
    char c = COMMAND_SERIAL.read();
    if (c == '\n' || c == '\r') {
      if (idx > 0) {
        buf[idx] = '\0';
        idx = 0;
        if (strcmp(buf, "BD:ESTOP") == 0) return true;
      }
    } else if (idx < sizeof(buf) - 1) {
      buf[idx++] = c;
    }
  }
  return false;
}

//----------------------------------------------------------------------------
//  Sequences
//----------------------------------------------------------------------------

//-----------------------------------------------------
// Heartbeat Sequence
//-----------------------------------------------------

void heart() {
  digitalWrite(STATUS_LED, HIGH);

  playLove();

  // Heartbeat: thump-thump...thump-thump...thump-thump
  // Door lifts open then snaps closed — the snap IS the thump.
  // 1780=closed against body, 1200=fully open. Lifts ~halfway open for a visible pulse.
  #define HEARTBEAT_LIFT_1    1500  // lift before first thump (bigger)
  #define HEARTBEAT_LIFT_2    1540  // lift before second thump (slightly smaller)
  #define HEARTBEAT_SPEED      200  // fast snap for a crisp thump

  sendPanelCommand("CBIMODE 1"); // first-pass guess: a slow pulsing mode

  for (int i = 0; i < 3; i++) {
    // lift then snap shut — first thump; flash heart on the snap
    moveServo(CBI_DOOR, HEARTBEAT_LIFT_1, HEARTBEAT_SPEED);
    waitTime(120);
    moveServo(CBI_DOOR, CBI_DOOR_CLOSE, HEARTBEAT_SPEED);
    waitTime(100);

    // lift then snap shut — second thump (softer); flash heart on the snap
    moveServo(CBI_DOOR, HEARTBEAT_LIFT_2, HEARTBEAT_SPEED);
    waitTime(100);
    moveServo(CBI_DOOR, CBI_DOOR_CLOSE, HEARTBEAT_SPEED);

    // pause between heartbeat pairs
    waitTime(700);
  }

  sendPanelCommand("CBIMODE 0");

  digitalWrite(STATUS_LED, LOW);
}

//-----------------------------------------------------
// Flutter Sequence
//-----------------------------------------------------

void Flutter() {
  digitalWrite(STATUS_LED, HIGH);

  const int RIGHT_DOOR_HALF = RIGHT_DOOR_CLOSE + (RIGHT_DOOR_OPEN - RIGHT_DOOR_CLOSE) / 2;
  const int CBI_DOOR_HALF   = CBI_DOOR_CLOSE   + (CBI_DOOR_OPEN   - CBI_DOOR_CLOSE)   / 2;
  const int DATA_DOOR_HALF  = DATA_DOOR_CLOSE  + (DATA_DOOR_OPEN  - DATA_DOOR_CLOSE)  / 2;
  const int LEFT_DOOR_HALF  = LEFT_DOOR_CLOSE  + (LEFT_DOOR_OPEN  - LEFT_DOOR_CLOSE)  / 2;

  // Right-to-left order across the body: RIGHT, CBI, DATA, LEFT
  const uint8_t doors[]    = { RIGHT_DOOR,           CBI_DOOR,           DATA_DOOR,           LEFT_DOOR           };
  const int doorHalf[]     = { RIGHT_DOOR_HALF,      CBI_DOOR_HALF,      DATA_DOOR_HALF,      LEFT_DOOR_HALF      };
  const int doorClose[]    = { RIGHT_DOOR_CLOSE,     CBI_DOOR_CLOSE,     DATA_DOOR_CLOSE,     LEFT_DOOR_CLOSE     };

  // Wave open, right to left, each door lifting halfway
  for (uint8_t i = 0; i < 4; i++) {
    moveServo(doors[i], doorHalf[i], FLUTTER_SPEED);
    waitTime(FLUTTER_STAGGER_MS);
  }

  waitTime(FLUTTER_HOLD_MS); // hold halfway open

  // Wave close, right to left
  for (uint8_t i = 0; i < 4; i++) {
    moveServo(doors[i], doorClose[i], FLUTTER_SPEED);
    waitTime(FLUTTER_STAGGER_MS);
  }

  waitTime(500); // wait on last door to reach position

  doorsOpen = false;
  leftDoorOpen = false;
  rightDoorOpen = false;
  cbi_dataOpen = false;
  cbiDoorOpen = false;
  dataDoorOpen = false;

  digitalWrite(STATUS_LED, LOW);
}

//-----------------------------------------------------
// Reset/Close All
//-----------------------------------------------------

void resetServos() {

  moveServo(TOP_UTIL_ARM, TOP_ARM_CLOSE, UTILITYARMSSPEED3);

  moveServo(BOTTOM_UTIL_ARM, BOTTOM_ARM_CLOSE, UTILITYARMSSPEED3);

  moveServo(LEFT_DOOR, LEFT_DOOR_CLOSE, DOOR_CLOSE_SPEED);

  moveServo(RIGHT_DOOR, RIGHT_DOOR_CLOSE, DOOR_CLOSE_SPEED);

  moveServo(CBI_DOOR, CBI_DOOR_CLOSE, DOOR_CLOSE_SPEED);

  moveServo(DATA_DOOR, DATA_DOOR_CLOSE, DOOR_CLOSE_SPEED);

  sendPanelCommand("SEQUENCE ON");

  waitTime(600); // wait on servos

  doorsOpen = false;
  leftDoorOpen = false;
  rightDoorOpen = false;
  cbi_dataOpen = false;
  cbiDoorOpen = false;
  dataDoorOpen = false;
  utilityArmOpen = false;
  topUtilityArmOpen = false;
  bottomUtilityArmOpen = false;
}
//-----------------------------------------------------
// Open/Close Everything
//-----------------------------------------------------

void openEverything() {

  digitalWrite(STATUS_LED, HIGH);

  //If everything is open, close everything.
  if (doorsOpen && cbi_dataOpen && utilityArmOpen) {
    resetServos();

  } else { //Open everything
    moveServo(TOP_UTIL_ARM, TOP_ARM_OPEN, UTILITYARMSSPEED2);

    moveServo(BOTTOM_UTIL_ARM, BOTTOM_ARM_OPEN, UTILITYARMSSPEED2);

    moveServo(LEFT_DOOR, LEFT_DOOR_OPEN, DOOR_OPEN_SPEED);

    moveServo(RIGHT_DOOR, RIGHT_DOOR_OPEN, DOOR_OPEN_SPEED);

    moveServo(CBI_DOOR, CBI_DOOR_OPEN, DOOR_OPEN_SPEED);

    moveServo(DATA_DOOR, DATA_DOOR_OPEN, DOOR_OPEN_SPEED);

    sendPanelCommand("BRIGHTNESS 100");

    waitTime(1000); // wait on servos

    doorsOpen = true;
    leftDoorOpen = true;
    rightDoorOpen = true;
    cbi_dataOpen = true;
    cbiDoorOpen = true;
    dataDoorOpen = true;
    utilityArmOpen = true;
    topUtilityArmOpen = true;
    bottomUtilityArmOpen = true;
  }

  digitalWrite(STATUS_LED, LOW);
}

//------------------------------------------------------------------
// Open/Close both Utility Arms
void UtilityArms() {

  digitalWrite(STATUS_LED, HIGH);

  //if both arms were opened individually, utilityArmOpen is true
  if (topUtilityArmOpen && bottomUtilityArmOpen) {
    utilityArmOpen = true;
  }

  // If the Arms are open then close them
  if (utilityArmOpen) {
    DEBUG_PRINT_LN(F("Close utility arms"));
    utilityArmOpen = false;
    topUtilityArmOpen = false;
    bottomUtilityArmOpen = false;

    moveServo(TOP_UTIL_ARM, TOP_ARM_CLOSE, UTILITYARMSSPEED);
    moveServo(BOTTOM_UTIL_ARM, BOTTOM_ARM_CLOSE, UTILITYARMSSPEED);

    waitTime(1000);  // wait on arm to reach position

  } else if (topUtilityArmOpen) { //if top arm is open, open bottom
    DEBUG_PRINT_LN(F("Open bottom arm"));
    utilityArmOpen = true;
    bottomUtilityArmOpen = true;

    moveServo(BOTTOM_UTIL_ARM, BOTTOM_ARM_OPEN, UTILITYARMSSPEED);

    waitTime(1000);  // wait on arm to reach position

  } else if (bottomUtilityArmOpen) { //if bottom arm is open, open top
    DEBUG_PRINT_LN(F("Open top arm"));
    utilityArmOpen = true;
    topUtilityArmOpen = true;

    moveServo(TOP_UTIL_ARM, TOP_ARM_OPEN, UTILITYARMSSPEED);

    waitTime(1000);  // wait on arm to reach position

  } else { // Open both arms if closed
    DEBUG_PRINT_LN(F("Open utility arms"));
    utilityArmOpen = true;
    topUtilityArmOpen = true;
    bottomUtilityArmOpen = true;

    moveServo(TOP_UTIL_ARM, TOP_ARM_OPEN, UTILITYARMSSPEED);
    moveServo(BOTTOM_UTIL_ARM, BOTTOM_ARM_OPEN, UTILITYARMSSPEED);

    waitTime(1000);  // wait on arm to reach position

  }

  digitalWrite(STATUS_LED, LOW);
}

//------------------------------------------------------------------
// Open/Close Top Utility Arm
void TopUtilityArm() {

  digitalWrite(STATUS_LED, HIGH);

  if (utilityArmOpen) { // If both the Arms are open, close the bottom one
    DEBUG_PRINT_LN(F("Close bottom utility arm"));
    topUtilityArmOpen = true;
    bottomUtilityArmOpen = false;
    utilityArmOpen = false;

    // pull arms slightly beyond closed to make sure they're really closed. Will vary on your droid
    moveServo(BOTTOM_UTIL_ARM, BOTTOM_ARM_CLOSE, UTILITYARMSSPEED);

    waitTime(1000);  // wait on arm to reach position

  } else if (topUtilityArmOpen) { // If the top arm is open, close it
    DEBUG_PRINT_LN(F("Close top utility arm"));
    topUtilityArmOpen = false;

    // Set Servo position to Close.
    moveServo(TOP_UTIL_ARM, TOP_ARM_CLOSE, UTILITYARMSSPEED); // close at moderate speed

    waitTime(1000);  // wait on arm to reach position

  } else { // Open the Top Arm Only
    DEBUG_PRINT_LN(F("Open top utility arm"));
    topUtilityArmOpen = true;

    // Set Servo position to Open.
    moveServo(TOP_UTIL_ARM, TOP_ARM_OPEN, UTILITYARMSSPEED); // open at moderate speed

    waitTime(1000);  // wait on arm to reach position

  }

  digitalWrite(STATUS_LED, LOW);
}

//------------------------------------------------------------------
// Open/Close Bottom Utility Arm
void BottomUtilityArm() {

  digitalWrite(STATUS_LED, HIGH);

  if (utilityArmOpen) { // If both the Arms are open, close the top one
    DEBUG_PRINT_LN(F("Close top utility arm"));
    topUtilityArmOpen = false;
    bottomUtilityArmOpen = true;
    utilityArmOpen = false;

    // pull arms slightly beyond closed to make sure they're really closed. Will vary on your droid
    moveServo(TOP_UTIL_ARM, TOP_ARM_CLOSE, UTILITYARMSSPEED);

    waitTime(1000);  // wait on arm to reach position

  } else if (bottomUtilityArmOpen) { // If the bottom arm is open, close it
    DEBUG_PRINT_LN(F("Close bottom utility arm"));
    bottomUtilityArmOpen = false;

    // Set Servo position to Close.
    moveServo(BOTTOM_UTIL_ARM, BOTTOM_ARM_CLOSE, UTILITYARMSSPEED); // close at moderate speed

    waitTime(1000);  // wait on arm to reach position

  } else { // Open the Bottom Arm Only
    DEBUG_PRINT_LN(F("Open bottom utility arm"));
    bottomUtilityArmOpen = true;

    // Set Servo position to Open.
    moveServo(BOTTOM_UTIL_ARM, BOTTOM_ARM_OPEN, UTILITYARMSSPEED); // open at moderate speed

    waitTime(1000);  // wait on arm to reach position

  }

  digitalWrite(STATUS_LED, LOW);
}

//---------------------------------------------
// S C R E A M
//---------------------------------------------
void Scream() {

  digitalWrite(STATUS_LED, HIGH);

  playScream();

  sendPanelCommand("PERSONALITY EXCITED");

  for (int i = 0; i < 7; i++) {

    DEBUG_PRINT(F("Loop:"));
    DEBUG_PRINT_LN(i + 1);
    moveServo(LEFT_DOOR, LEFT_DOOR_OPEN, SCREAM_SPEED);
    moveServo(DATA_DOOR, DATA_DOOR_OPEN, SCREAM_SPEED);

    moveServo(RIGHT_DOOR, RIGHT_DOOR_CLOSE, SCREAM_SPEED);
    moveServo(CBI_DOOR, CBI_DOOR_CLOSE, SCREAM_SPEED);

    moveServo(TOP_UTIL_ARM, 1250, 255);
    moveServo(BOTTOM_UTIL_ARM, BOTTOM_ARM_CLOSE, 255);

    waitTime(150);

    moveServo(LEFT_DOOR, LEFT_DOOR_CLOSE, SCREAM_SPEED);
    moveServo(DATA_DOOR, DATA_DOOR_CLOSE, SCREAM_SPEED);

    moveServo(RIGHT_DOOR, RIGHT_DOOR_OPEN, SCREAM_SPEED);
    moveServo(CBI_DOOR, CBI_DOOR_OPEN, SCREAM_SPEED);

    moveServo(TOP_UTIL_ARM, TOP_ARM_CLOSE, 255); // open at moderate speed
    moveServo(BOTTOM_UTIL_ARM, 1250, 255); // 0=open all the way

    waitTime(150);

  }

  DEBUG_PRINT_LN(F("Close everything"));

  moveServo(TOP_UTIL_ARM, TOP_ARM_CLOSE, UTILITYARMSSPEED);
  moveServo(BOTTOM_UTIL_ARM, BOTTOM_ARM_CLOSE, UTILITYARMSSPEED);
  moveServo(LEFT_DOOR, LEFT_DOOR_CLOSE, DOOR_CLOSE_SPEED);
  moveServo(RIGHT_DOOR, RIGHT_DOOR_CLOSE, DOOR_CLOSE_SPEED);
  moveServo(CBI_DOOR, CBI_DOOR_CLOSE, DOOR_CLOSE_SPEED);
  moveServo(DATA_DOOR, DATA_DOOR_CLOSE, DOOR_CLOSE_SPEED);

  sendPanelCommand("PERSONALITY NORMAL");

  waitTime(1000); // wait on arm to reach position

  doorsOpen = false;
  leftDoorOpen = false;
  rightDoorOpen = false;
  cbi_dataOpen = false;
  cbiDoorOpen = false;
  dataDoorOpen = false;
  utilityArmOpen = false;
  topUtilityArmOpen = false;
  bottomUtilityArmOpen = false;

  digitalWrite(STATUS_LED, LOW);
}

//-----------------------------------------------------
// D O O R S
//-----------------------------------------------------

void Doors() {

  digitalWrite(STATUS_LED, HIGH);

  //if doors were opened individually, doorsOpen is true
  if (leftDoorOpen && rightDoorOpen && cbiDoorOpen && dataDoorOpen) {
    doorsOpen = true;
  }

  // If the Doors are open then close them
  if (doorsOpen) {
    DEBUG_PRINT_LN(F("Close doors"));
    doorsOpen = false;
    leftDoorOpen = false;
    rightDoorOpen = false;
    cbiDoorOpen = false;
    dataDoorOpen = false;
    cbi_dataOpen = false;

    moveServo(LEFT_DOOR, LEFT_DOOR_CLOSE, DOOR_CLOSE_SPEED);
    moveServo(RIGHT_DOOR, RIGHT_DOOR_CLOSE, DOOR_CLOSE_SPEED);
    moveServo(CBI_DOOR, CBI_DOOR_CLOSE, DOOR_CLOSE_SPEED);
    moveServo(DATA_DOOR, DATA_DOOR_CLOSE, DOOR_CLOSE_SPEED);

    sendPanelCommand("SEQUENCE ON");

    waitTime(1000); // wait on arm to reach position

  } else {
    DEBUG_PRINT_LN(F("Open doors"));
    doorsOpen = true;
    leftDoorOpen = true;
    rightDoorOpen = true;
    cbi_dataOpen = true;
    cbiDoorOpen = true;
    dataDoorOpen = true;

    sendPanelCommand("BRIGHTNESS 100");

    moveServo(LEFT_DOOR, LEFT_DOOR_OPEN, DOOR_OPEN_SPEED);
    moveServo(RIGHT_DOOR, RIGHT_DOOR_OPEN, DOOR_OPEN_SPEED);
    moveServo(CBI_DOOR, CBI_DOOR_OPEN, DOOR_OPEN_SPEED);
    moveServo(DATA_DOOR, DATA_DOOR_OPEN, DOOR_OPEN_SPEED);

    waitTime(1000);

  }

  digitalWrite(STATUS_LED, LOW);
}

void openLeftDoor() {

  digitalWrite(STATUS_LED, HIGH);

  if (leftDoorOpen) {
    DEBUG_PRINT_LN(F("Close Left Door"));
    leftDoorOpen = false;
    moveServo(LEFT_DOOR, LEFT_DOOR_CLOSE, DOOR_CLOSE_SPEED);
    waitTime(1000); // wait on door to reach position

  } else {
    leftDoorOpen = true;
    DEBUG_PRINT_LN(F("Open Left Door"));
    moveServo(LEFT_DOOR, LEFT_DOOR_OPEN, DOOR_OPEN_SPEED);
    waitTime(1000);
  }

  digitalWrite(STATUS_LED, LOW);
}

void openRightDoor() {
  digitalWrite(STATUS_LED, HIGH);

  if (rightDoorOpen) {
    DEBUG_PRINT_LN(F("Close Right Door"));
    rightDoorOpen = false;
    moveServo(RIGHT_DOOR, RIGHT_DOOR_CLOSE, DOOR_CLOSE_SPEED);
    waitTime(1000); // wait on door to reach position

  } else {
    rightDoorOpen = true;
    DEBUG_PRINT_LN(F("Open Right Door"));
    moveServo(RIGHT_DOOR, RIGHT_DOOR_OPEN, DOOR_OPEN_SPEED);
    waitTime(1000);
  }

  digitalWrite(STATUS_LED, LOW);
}

void openCBIDoor() {
  digitalWrite(STATUS_LED, HIGH);

  if (cbiDoorOpen) {
    DEBUG_PRINT_LN(F("Close Charge Bay Door"));
    cbiDoorOpen = false;
    moveServo(CBI_DOOR, CBI_DOOR_CLOSE, DOOR_CLOSE_SPEED);

    waitTime(1000); // wait on door to reach position

  } else {
    cbiDoorOpen = true;
    DEBUG_PRINT_LN(F("Open Charge Bay Door"));

    moveServo(CBI_DOOR, CBI_DOOR_OPEN, DOOR_OPEN_SPEED);

    waitTime(1000);
  }

  digitalWrite(STATUS_LED, LOW);
}

void openDataDoor() {
  digitalWrite(STATUS_LED, HIGH);

  if (dataDoorOpen) {
    DEBUG_PRINT_LN(F("Close Data Port Door"));
    dataDoorOpen = false;
    moveServo(DATA_DOOR, DATA_DOOR_CLOSE, DOOR_CLOSE_SPEED);

    waitTime(1000); // wait on door to reach position

  } else {
    dataDoorOpen = true;
    DEBUG_PRINT_LN(F("Open Data Port Door"));

    moveServo(DATA_DOOR, DATA_DOOR_OPEN, DOOR_OPEN_SPEED);

    waitTime(1000);
  }

  digitalWrite(STATUS_LED, LOW);
}

void openCBI_DataDoor() {
  digitalWrite(STATUS_LED, HIGH);

  //if doors were opened individually, cbi_dataOpen is true
  if (cbiDoorOpen && dataDoorOpen) {
    cbi_dataOpen = true;
  }

  if (cbi_dataOpen) {
    DEBUG_PRINT_LN(F("Close Charge Bay & Data Door"));
    cbi_dataOpen = false;
    cbiDoorOpen = false;
    dataDoorOpen = false;

    moveServo(CBI_DOOR, CBI_DOOR_CLOSE, DOOR_CLOSE_SPEED);
    moveServo(DATA_DOOR, DATA_DOOR_CLOSE, DOOR_CLOSE_SPEED);

    waitTime(1000); // wait on door to reach position

  } else {
    cbi_dataOpen = true;
    cbiDoorOpen = true;
    dataDoorOpen = true;
    DEBUG_PRINT_LN(F("Open Charge Bay & Data Door"));

    moveServo(CBI_DOOR, CBI_DOOR_OPEN, DOOR_OPEN_SPEED);
    moveServo(DATA_DOOR, DATA_DOOR_OPEN, DOOR_OPEN_SPEED);

    waitTime(1000);
  }

  digitalWrite(STATUS_LED, LOW);
}

//----------------------------------------------------------------------------
//  Delay function
//----------------------------------------------------------------------------
void waitTime(unsigned long duration)
{
  unsigned long endTime = millis() + duration;
  while (millis() < endTime)
  {
    if (checkForEstop()) performEStop(); // never returns
  }
}

// Serial Command Functions

// ---------------------------------------------------------------------------
// RGB-DPL panel commands
// ---------------------------------------------------------------------------
// RGB-DPL (https://github.com/thePunderWoman/RGB-DPL-Firmware) shares the
// WCB serial trunk — its plain-text command syntax (BRIGHTNESS, SCHEME,
// PERSONALITY, CBIMODE, SEQUENCE, ...) is documented in that repo's README
// and manual.
void sendPanelCommand(const char* cmd) {
  WCBSerial.println(cmd);
}

// CB<val>\n  — charge bay panel command (from WCB, forwarded to RGB-DPL)
// DP<val>\n  — data panel command (from WCB, forwarded to RGB-DPL)
// val encoding (legacy, kept for compatibility with existing WCB behavior):
// (seq * 10000) + (speed * 100) + duration_seconds.
// seq 0 = resume normal animation, seq 1 = go dark/disabled.
// First-pass mapping: both collapse to RGB-DPL's global SEQUENCE toggle,
// since CB/DP no longer address two separate physical displays the way the
// old MAX7219 matrices did, and speed/duration have no direct RGB-DPL
// equivalent yet. Revisit once you've been through the full RGB-DPL manual
// for finer per-panel control (CBIMODE/LDPLMODE/SCHEME).
void doCBILEDCommand(long val) {
  uint8_t seq = (uint8_t)(val / 10000);
  sendPanelCommand(seq == 1 ? "SEQUENCE OFF" : "SEQUENCE ON");
}

void doDPLEDCommand(long val) {
  uint8_t seq = (uint8_t)(val / 10000);
  sendPanelCommand(seq == 1 ? "SEQUENCE OFF" : "SEQUENCE ON");
}

// Marcduino body panel numbering used here:
// 0=all  1=top arm  2=bottom arm  3=left door  4=right door  5=CBI  6=data panel
// Adjust to match your body master's panel assignment if needed.
void doMarcduinoOpen(uint8_t panel) {
  switch (panel) {
    case 0:
      moveServo(TOP_UTIL_ARM, TOP_ARM_OPEN, UTILITYARMSSPEED2);
      moveServo(BOTTOM_UTIL_ARM, BOTTOM_ARM_OPEN, UTILITYARMSSPEED2);
      moveServo(LEFT_DOOR, LEFT_DOOR_OPEN, DOOR_OPEN_SPEED);
      moveServo(RIGHT_DOOR, RIGHT_DOOR_OPEN, DOOR_OPEN_SPEED);
      moveServo(CBI_DOOR, CBI_DOOR_OPEN, DOOR_OPEN_SPEED);
      moveServo(DATA_DOOR, DATA_DOOR_OPEN, DOOR_OPEN_SPEED);
      sendPanelCommand("BRIGHTNESS 100");
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
      moveServo(CBI_DOOR, CBI_DOOR_OPEN, DOOR_OPEN_SPEED);
      waitTime(900);
      break;
    case 6:
      if (dataDoorOpen) return;
      dataDoorOpen = true;
      moveServo(DATA_DOOR, DATA_DOOR_OPEN, DOOR_OPEN_SPEED);
      waitTime(900);
      break;
  }
}

void doMarcduinoClose(uint8_t panel) {
  switch (panel) {
    case 0:
      moveServo(TOP_UTIL_ARM, TOP_ARM_CLOSE, UTILITYARMSSPEED);
      moveServo(BOTTOM_UTIL_ARM, BOTTOM_ARM_CLOSE, UTILITYARMSSPEED);
      moveServo(LEFT_DOOR, LEFT_DOOR_CLOSE, DOOR_CLOSE_SPEED);
      moveServo(RIGHT_DOOR, RIGHT_DOOR_CLOSE, DOOR_CLOSE_SPEED);
      moveServo(CBI_DOOR, CBI_DOOR_CLOSE, DOOR_CLOSE_SPEED);
      moveServo(DATA_DOOR, DATA_DOOR_CLOSE, DOOR_CLOSE_SPEED);
      sendPanelCommand("SEQUENCE ON");
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
      moveServo(CBI_DOOR, CBI_DOOR_CLOSE, DOOR_CLOSE_SPEED);
      waitTime(900);
      break;
    case 6:
      if (!dataDoorOpen) return;
      dataDoorOpen = false;
      moveServo(DATA_DOOR, DATA_DOOR_CLOSE, DOOR_CLOSE_SPEED);
      waitTime(900);
      break;
  }
}

// :OP<nn>  :CL<nn>  :SE<nn>
void doMarcduinoCommand(const char* cmd) {
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

void doCommand(const char* cmd) {
  loopTime = millis();
  DEBUG_PRINT(F("Serial command: "));
  DEBUG_PRINT_LN(cmd);

  if (strcmp(cmd, "ESTOP") == 0) {
    performEStop(); // never returns
  } else if (strcmp(cmd, "RESET") == 0) {
    DEBUG_PRINT_LN(F("Got reset message"));
    resetServos();
    resetVocalizer();
    digitalWrite(STATUS_LED, HIGH);
  } else if (strcmp(cmd, "VADER") == 0) {
    Vader();
  } else if (strcmp(cmd, "ROCKMARCH") == 0) {
    RockMarch();
  } else if (strcmp(cmd, "THEME") == 0) {
    Theme();
  } else if (strcmp(cmd, "BATTLEALARM") == 0) {
    BattleAlarm();
  } else if (strcmp(cmd, "CLONES") == 0) {
    Clones();
  } else if (strcmp(cmd, "DUEL") == 0) {
    Duel();
  } else if (strcmp(cmd, "LUKEJABBA") == 0) {
    LukeJabba();
  } else if (strcmp(cmd, "THRONE") == 0) {
    Throne();
  } else if (strcmp(cmd, "CANTINA") == 0) {
    Cantina();
  } else if (strcmp(cmd, "SCREAM") == 0) {
    Scream();
  } else if (strcmp(cmd, "LEIA") == 0) {
    Leia();
  } else if (strcmp(cmd, "OVERLOAD") == 0) {
    overload();
  } else if (strcmp(cmd, "UARMS") == 0) {
    UtilityArms();
  } else if (strcmp(cmd, "DOORS") == 0) {
    Doors();
  } else if (strcmp(cmd, "LDOOR") == 0) {
    openLeftDoor();
  } else if (strcmp(cmd, "RDOOR") == 0) {
    openRightDoor();
  } else if (strcmp(cmd, "OPENALL") == 0) {
    openEverything();
  } else if (strcmp(cmd, "CBIDOOR") == 0) {
    openCBIDoor();
  } else if (strcmp(cmd, "DATADOOR") == 0) {
    openDataDoor();
  } else if (strcmp(cmd, "CBIDATADOOR") == 0) {
    openCBI_DataDoor();
  } else if (strcmp(cmd, "TOPARM") == 0) {
    TopUtilityArm();
  } else if (strcmp(cmd, "BOTARM") == 0) {
    BottomUtilityArm();
  } else if (strcmp(cmd, "HELLO") == 0) {
    playHello();
  } else if (strcmp(cmd, "ALARM") == 0) {
    alarm();
  } else if (strcmp(cmd, "HEART") == 0) {
    heart();
  } else if (strcmp(cmd, "DISCO") == 0) {
    Disco();
  } else if (strcmp(cmd, "FLUTTER") == 0) {
    Flutter();
  } else {
    digitalWrite(STATUS_LED, LOW);
  }
}

