#include "sequences.h"
#include "config.h"
#include "bus.h"
#include "maestro.h"
#include "estop.h"
#include "doors.h"
#include "vocalizer.h"

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

  // set the periscope to the up position, lights to sith mode, and rotate
  sendBusCommand(":PL4:PP100:PR30:PW60:PH");

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

  // set the periscope to the up position, lights to sparkle mode, and rotate
  sendBusCommand(":PL7:PP100:PR30:PW14:PH");

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

  // Doors alternate open/closed in time with the beat (130 BPM = ~461 ms/beat).
  // Left-to-right order: LEFT, DATA, CBI, RIGHT.
  // Even positions (LEFT, CBI) open while odd positions (DATA, RIGHT) close, then flip.
  const unsigned long BEAT_MS = 923; // every 2 beats at 130 BPM
  const unsigned long DURATION = 15000; // matches the 15-second clip length
  waitTime(88);
  const int CANTINA_SPEED = 200;        // fast enough to land on the beat

  const int TOP_ARM_HALF   = (TOP_ARM_OPEN    + TOP_ARM_CLOSE)    / 2; // ~1215
  const int BOTTOM_ARM_HALF = (BOTTOM_ARM_OPEN + BOTTOM_ARM_CLOSE) / 2; // ~1275

  sendBusCommand("ALLON");
  sendBusCommand("SCHEME CYBERPUNK");

  playCantina();

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

  sendBusCommand("SCHEME CLASSIC");
  sendBusCommand("ALLOFF");

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

  sendBusCommand("ALLON");
  sendBusCommand("CBIMODE 6"); // first-pass guess: a glitchy/erratic mode

  overloadEmote();

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

  sendBusCommand("CBIMODE 0");
  sendBusCommand("ALLOFF");

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

  // set the periscope to the up position, lights to sith mode, and rotate
  sendBusCommand(":PL4:PP100:PR30:PW9:PH");

  playImperialAlarm();

  digitalWrite(STATUS_LED, LOW);
}

void stepBack() {
  digitalWrite(STATUS_LED, HIGH);

  playStepBack();

  digitalWrite(STATUS_LED, LOW);
}

//-----------------------------------------------------
// Heartbeat Sequence
//-----------------------------------------------------

void heart() {
  digitalWrite(STATUS_LED, HIGH);

  // Heartbeat: thump-thump...thump-thump...thump-thump
  // Door lifts open then snaps closed — the snap IS the thump.
  // 1780=closed against body, 1200=fully open. Lifts ~halfway open for a visible pulse.
  #define HEARTBEAT_LIFT_1    1500  // lift before first thump (bigger)
  #define HEARTBEAT_LIFT_2    1540  // lift before second thump (slightly smaller)
  #define HEARTBEAT_SPEED      200  // fast snap for a crisp thump

  sendBusCommand("CBION");
  sendBusCommand("CBIMODE 4"); // heart shape in red

  playLove();

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

  sendBusCommand("CBIMODE 0");
  sendBusCommand("CBIOFF");

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

  sendBusCommand("ALLON");

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

  sendBusCommand("ALLOFF");

  doorsOpen = false;
  leftDoorOpen = false;
  rightDoorOpen = false;
  cbi_dataOpen = false;
  cbiDoorOpen = false;
  dataDoorOpen = false;

  digitalWrite(STATUS_LED, LOW);
}

//---------------------------------------------
// S C R E A M
//---------------------------------------------
void Scream() {

  digitalWrite(STATUS_LED, HIGH);

  sendBusCommand("ALLON");
  sendBusCommand("PERSONALITY EXCITED");

  playScream();

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

  sendBusCommand("PERSONALITY NORMAL");
  sendBusCommand("ALLOFF");

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
