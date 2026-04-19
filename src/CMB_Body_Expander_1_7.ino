// CMB Servo Expander Body v 1.8
// Based on Work by Chris James (v1.14) 3-10-2017
// Based on work by Chris Bozzoli (DBoz) (v1.7)
// Modified by Jessica Janiuk (thePunderWoman)
//
//v1.2 Added Individual Utility Arm Movement 7-10-18
//v1.3 Replaced all delays with waitTime funciton 4-12-20
//v1.4 Rearranged Switch/Case Numbers to align with Dome 4-21-20
//v1.5 Removed Back Door code, renumbered servo pins, Added Open Everything Funciton 5-21-20
//V1.6 Optimized I2C function 8-20-20
//V1.7 Integrated CBI_DataPanel_SA_Breakout 1.0 code 2/24/21
//V1.8 Integrated HCR Vocalizer APIs
// -------------------------------------------------

//Variable Speed Servo Library with sequencing ability
//https://github.com/netlabtoolkit/VarSpeedServo
#include <VarSpeedServo.h>

#include <LedControl.h>
#include "config.h"
#include <hcr.h>

// Serial command port — hardware Serial (the only port on a Pro Mini)
#define COMMAND_SERIAL Serial
#define COMMAND_BAUD   9600

float vout = 0.0;       // for voltage out measured analog input
int value = 0;          // used to hold the analog value coming out of the voltage divider
float vin = 0.0;        // voltage calculated... since the divider allows for 15 volts

#define VOLUME_HIGH 2
#define VOLUME_LOW 1
#define VOLUME_MUTE 0

int voiceVolumeHigh = 90;
int chaVolumeHigh = 50;
int chbVolumeHigh = 50;
int voiceVolumeLow = 50;
int chaVolumeLow = 25;
int chbVolumeLow = 25;
int volumeLevel = VOLUME_HIGH; 
bool muse = true;

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

// Instantiate LedControl driver
LedControl lc = LedControl(DATAIN_PIN, CLOCK_PIN, LOAD_PIN, NUMDEV);

HCRVocalizer HCR(&Serial,9600);

void setup()
{
  COMMAND_SERIAL.begin(COMMAND_BAUD);

  // initialize Maxim driver chips
  lc.shutdown(DATAPORT, false);                 // take out of shutdown
  lc.clearDisplay(DATAPORT);                    // clear
  lc.setIntensity(DATAPORT, DATAPORTINTENSITY); // set intensity

  lc.shutdown(CBI, false);                      // take out of shutdown
  lc.clearDisplay(CBI);                         // clear
  lc.setIntensity(CBI, CBIINTENSITY);           // set intensity

#ifdef  TEST// test LEDs
  singleTest();
  waitTime(2000);
#endif

#ifndef monitorVCC
  pinMode(analoginput, INPUT);
#endif

  DEBUG_PRINT_LN(F("Amidala RC - Body Controller v1.7 (3-17-21)"));

  DEBUG_PRINT_LN(F("Command serial ready (Serial @ 9600)"));

  pinMode(STATUS_LED, OUTPUT); // turn status led off
  digitalWrite(STATUS_LED, LOW);

  pinMode(VM_SWITCH_PIN, OUTPUT); //Sets digital pin as an output
  pinMode(CBI_SWITCH_PIN, OUTPUT); //Sets digital pin as an output
  pinMode(DP_SWITCH_PIN, OUTPUT); //Sets digital pin as an output

  digitalWrite(CBI_SWITCH_PIN, LOW); //Sets output of digital pin low to turn off lights
  digitalWrite(DP_SWITCH_PIN, LOW);
  digitalWrite(VM_SWITCH_PIN, LOW);

  DEBUG_PRINT(F("Activating Servos"));
  resetServos();
  
  DEBUG_PRINT_LN(F("Setup Complete"));
}

void loop() {
  if (digitalRead(CBI_SWITCH_PIN) == LOW)
  {
    lc.shutdown(CBI, true);
  }
  else
  {
    lc.shutdown(CBI, false);
  }
  if (digitalRead(DP_SWITCH_PIN) == LOW)
  {
    lc.shutdown(DATAPORT, true);
  }
  else
  {
    lc.shutdown(DATAPORT, false);
  }

  // this is the new code. Every block of LEDs is handled independently
  updateTopBlocks();
  bargraphDisplay(0);
  updatebottomLEDs();
  updateRedLEDs();
  updateCBILEDs();
#ifdef monitorVCC
  getVCC();
#endif
#ifndef BLUELEDTRACKGRAPH
  updateBlueLEDs();
#endif
  readSerial();
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


// Emote Events

void enableMuse() {
  HCR.Muse(20,45);
  HCR.SetMuse(1);
  muse = true;
}

void disableMuse() {
  HCR.SetMuse(0);
  muse = false;
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

void Theme() {
  digitalWrite(STATUS_LED, HIGH);

  playSWTheme();
  digitalWrite(STATUS_LED, LOW);
}

void Cantina() {
  digitalWrite(STATUS_LED, HIGH);

  playCantina();
  digitalWrite(STATUS_LED, LOW);
}

void overload() {
  digitalWrite(STATUS_LED, HIGH);

  overloadEmote();

  randomSeed(analogRead(0));

  const uint8_t NUM_PANELS = 6;
  const uint8_t panels[]    = { TOP_UTIL_ARM,      BOTTOM_UTIL_ARM,      LEFT_DOOR,          RIGHT_DOOR,          CBI_DOOR,          DATA_DOOR          };
  const int panelPin[]      = { TOP_UTIL_ARM_SERVO_PIN, BOTTOM_UTIL_ARM_SERVO_PIN, LEFT_DOOR_SERVO_PIN, RIGHT_DOOR_SERVO_PIN, CBI_DOOR_SERVO_PIN, DATA_DOOR_SERVO_PIN };
  const int panelMinPulse[] = { ARMMINPULSE,        ARMMINPULSE,          LEFT_DOOR_MINPULSE, RIGHT_DOOR_MINPULSE, CBI_DOOR_MINPULSE, DATA_DOOR_MINPULSE };
  const int panelMaxPulse[] = { ARMMAXPULSE,        ARMMAXPULSE,          LEFT_DOOR_MAXPULSE, RIGHT_DOOR_MAXPULSE, CBI_DOOR_MAXPULSE, DATA_DOOR_MAXPULSE };
  const int panelOpen[]     = { TOP_ARM_OPEN,       BOTTOM_ARM_OPEN,      LEFT_DOOR_OPEN,     RIGHT_DOOR_OPEN,     CBI_DOOR_OPEN,     DATA_DOOR_OPEN     };
  const int panelClose[]    = { TOP_ARM_CLOSE,      BOTTOM_ARM_CLOSE,     LEFT_DOOR_CLOSE,    RIGHT_DOOR_CLOSE,    CBI_DOOR_CLOSE,    DATA_DOOR_CLOSE    };

  for (uint8_t i = 0; i < NUM_PANELS; i++) {
    Servos[panels[i]].attach(panelPin[i], panelMinPulse[i], panelMaxPulse[i]);
  }

  digitalWrite(CBI_SWITCH_PIN, HIGH);
  digitalWrite(DP_SWITCH_PIN, HIGH);
  digitalWrite(VM_SWITCH_PIN, HIGH);

  // Spasm burst: fling everything open in a randomized scrambled order
  uint8_t order[] = { 0, 1, 2, 3, 4, 5 };
  for (int i = NUM_PANELS - 1; i > 0; i--) {
    int j = random(i + 1);
    uint8_t tmp = order[i]; order[i] = order[j]; order[j] = tmp;
  }
  for (uint8_t i = 0; i < NUM_PANELS; i++) {
    uint8_t pi = order[i];
    Servos[panels[pi]].write(panelOpen[pi], SCREAM_SPEED);
    waitTime(random(10, 35));
  }

  // Chaos loop: ~8 seconds of random panels snapping to random positions
  unsigned long chaosEnd = millis() + 8000;
  while (millis() < chaosEnd) {
    uint8_t pi = random(NUM_PANELS);
    int t = random(4); // 0=closed, 3=fully open, 1-2 are intermediate
    int pos = panelClose[pi] + (long)(panelOpen[pi] - panelClose[pi]) * t / 3;
    Servos[panels[pi]].write(pos, SCREAM_SPEED);
    waitTime(random(40, 180));
  }

  // Slam everything closed
  for (uint8_t i = 0; i < NUM_PANELS; i++) {
    Servos[panels[i]].write(panelClose[i], SCREAM_SPEED);
  }

  digitalWrite(CBI_SWITCH_PIN, LOW);
  digitalWrite(DP_SWITCH_PIN, LOW);
  digitalWrite(VM_SWITCH_PIN, LOW);

  waitTime(1000);

  for (uint8_t i = 0; i < NUM_PANELS; i++) {
    Servos[panels[i]].detach();
  }

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
//  Sequences
//----------------------------------------------------------------------------

//-----------------------------------------------------
// Love
//-----------------------------------------------------

void love() {
  digitalWrite(STATUS_LED, HIGH);

  playLove();

  // Heartbeat: thump-thump...thump-thump...thump-thump
  // Door lifts slightly open then snaps closed — the snap IS the thump.
  // 1780=closed against body, 1200=fully open. Lifts only ~1/4 open so it's a subtle pulse.
  #define HEARTBEAT_LIFT_1    1620  // lift before first thump (slightly bigger)
  #define HEARTBEAT_LIFT_2    1650  // lift before second thump (slightly smaller)
  #define HEARTBEAT_SPEED      200  // fast snap for a crisp thump

  Servos[CBI_DOOR].attach(CBI_DOOR_SERVO_PIN, CBI_DOOR_MINPULSE, CBI_DOOR_MAXPULSE);

  for (int i = 0; i < 3; i++) {
    // lift then snap shut — first thump
    Servos[CBI_DOOR].write(HEARTBEAT_LIFT_1, HEARTBEAT_SPEED);
    waitTime(120);
    Servos[CBI_DOOR].write(CBI_DOOR_CLOSE, HEARTBEAT_SPEED);
    waitTime(100);

    // lift then snap shut — second thump (softer)
    Servos[CBI_DOOR].write(HEARTBEAT_LIFT_2, HEARTBEAT_SPEED);
    waitTime(100);
    Servos[CBI_DOOR].write(CBI_DOOR_CLOSE, HEARTBEAT_SPEED);

    // pause between heartbeat pairs
    waitTime(700);
  }

  Servos[CBI_DOOR].detach();

  digitalWrite(STATUS_LED, LOW);
}

//-----------------------------------------------------
// Reset/Close All
//-----------------------------------------------------

void resetServos() {

  Servos[TOP_UTIL_ARM].attach(TOP_UTIL_ARM_SERVO_PIN, ARMMINPULSE, ARMMAXPULSE);
  Servos[TOP_UTIL_ARM].write(TOP_ARM_CLOSE, UTILITYARMSSPEED3);

  Servos[BOTTOM_UTIL_ARM].attach(BOTTOM_UTIL_ARM_SERVO_PIN, ARMMINPULSE, ARMMAXPULSE);
  Servos[BOTTOM_UTIL_ARM].write(BOTTOM_ARM_CLOSE, UTILITYARMSSPEED3);

  Servos[LEFT_DOOR].attach(LEFT_DOOR_SERVO_PIN, LEFT_DOOR_MINPULSE, LEFT_DOOR_MAXPULSE);
  Servos[LEFT_DOOR].write(LEFT_DOOR_CLOSE, DOOR_CLOSE_SPEED);

  Servos[RIGHT_DOOR].attach(RIGHT_DOOR_SERVO_PIN, RIGHT_DOOR_MINPULSE, RIGHT_DOOR_MAXPULSE);
  Servos[RIGHT_DOOR].write(RIGHT_DOOR_CLOSE, DOOR_CLOSE_SPEED);

  Servos[CBI_DOOR].attach(CBI_DOOR_SERVO_PIN, CBI_DOOR_MINPULSE, CBI_DOOR_MAXPULSE);
  Servos[CBI_DOOR].write(CBI_DOOR_CLOSE, DOOR_CLOSE_SPEED);

  Servos[DATA_DOOR].attach(DATA_DOOR_SERVO_PIN, DATA_DOOR_MINPULSE, DATA_DOOR_MAXPULSE);
  Servos[DATA_DOOR].write(DATA_DOOR_CLOSE, DOOR_CLOSE_SPEED);

  digitalWrite(CBI_SWITCH_PIN, LOW);
  digitalWrite(DP_SWITCH_PIN, LOW);
  digitalWrite(VM_SWITCH_PIN, LOW);

  waitTime(600); // wait on servos

  // Detach from the servo to save power and reduce jitter
  Servos[TOP_UTIL_ARM].detach();
  Servos[BOTTOM_UTIL_ARM].detach();
  Servos[LEFT_DOOR].detach();
  Servos[RIGHT_DOOR].detach();
  Servos[CBI_DOOR].detach();
  Servos[DATA_DOOR].detach();

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
    Servos[TOP_UTIL_ARM].attach(TOP_UTIL_ARM_SERVO_PIN, ARMMINPULSE, ARMMAXPULSE);
    Servos[TOP_UTIL_ARM].write(TOP_ARM_OPEN, UTILITYARMSSPEED2);

    Servos[BOTTOM_UTIL_ARM].attach(BOTTOM_UTIL_ARM_SERVO_PIN, ARMMINPULSE, ARMMAXPULSE);
    Servos[BOTTOM_UTIL_ARM].write(BOTTOM_ARM_OPEN, UTILITYARMSSPEED2);

    Servos[LEFT_DOOR].attach(LEFT_DOOR_SERVO_PIN, LEFT_DOOR_MINPULSE, LEFT_DOOR_MAXPULSE);
    Servos[LEFT_DOOR].write(LEFT_DOOR_OPEN, DOOR_OPEN_SPEED);

    Servos[RIGHT_DOOR].attach(RIGHT_DOOR_SERVO_PIN, RIGHT_DOOR_MINPULSE, RIGHT_DOOR_MAXPULSE);
    Servos[RIGHT_DOOR].write(RIGHT_DOOR_OPEN, DOOR_OPEN_SPEED);

    Servos[CBI_DOOR].attach(CBI_DOOR_SERVO_PIN, CBI_DOOR_MINPULSE, CBI_DOOR_MAXPULSE);
    Servos[CBI_DOOR].write(CBI_DOOR_OPEN, DOOR_OPEN_SPEED);

    Servos[DATA_DOOR].attach(DATA_DOOR_SERVO_PIN, DATA_DOOR_MINPULSE, DATA_DOOR_MAXPULSE);
    Servos[DATA_DOOR].write(DATA_DOOR_OPEN, DOOR_OPEN_SPEED);

    digitalWrite(CBI_SWITCH_PIN, HIGH);
    digitalWrite(DP_SWITCH_PIN, HIGH);
    digitalWrite(VM_SWITCH_PIN, HIGH);

    waitTime(1000); // wait on servos

    // Detach from the servo to save power and reduce jitter
    Servos[TOP_UTIL_ARM].detach();
    Servos[BOTTOM_UTIL_ARM].detach();
    Servos[LEFT_DOOR].detach();
    Servos[RIGHT_DOOR].detach();
    Servos[CBI_DOOR].detach();
    Servos[DATA_DOOR].detach();

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

    Servos[TOP_UTIL_ARM].attach(TOP_UTIL_ARM_SERVO_PIN, ARMMINPULSE, ARMMAXPULSE);
    Servos[BOTTOM_UTIL_ARM].attach(BOTTOM_UTIL_ARM_SERVO_PIN, ARMMINPULSE, ARMMAXPULSE);

    Servos[TOP_UTIL_ARM].write(TOP_ARM_CLOSE, UTILITYARMSSPEED);
    Servos[BOTTOM_UTIL_ARM].write(BOTTOM_ARM_CLOSE, UTILITYARMSSPEED);

    waitTime(1000);  // wait on arm to reach position

    Servos[TOP_UTIL_ARM].detach(); // detach so we can move the arms freely
    Servos[BOTTOM_UTIL_ARM].detach();

  } else if (topUtilityArmOpen) { //if top arm is open, open bottom
    DEBUG_PRINT_LN(F("Open bottom arm"));
    utilityArmOpen = true;
    bottomUtilityArmOpen = true;

    Servos[BOTTOM_UTIL_ARM].attach(BOTTOM_UTIL_ARM_SERVO_PIN, ARMMINPULSE, ARMMAXPULSE);

    Servos[BOTTOM_UTIL_ARM].write(BOTTOM_ARM_OPEN, UTILITYARMSSPEED);

    waitTime(1000);  // wait on arm to reach position

    Servos[BOTTOM_UTIL_ARM].detach();

  } else if (bottomUtilityArmOpen) { //if bottom arm is open, open top
    DEBUG_PRINT_LN(F("Open top arm"));
    utilityArmOpen = true;
    topUtilityArmOpen = true;

    Servos[TOP_UTIL_ARM].attach(TOP_UTIL_ARM_SERVO_PIN, ARMMINPULSE, ARMMAXPULSE);

    Servos[TOP_UTIL_ARM].write(TOP_ARM_OPEN, UTILITYARMSSPEED);

    waitTime(1000);  // wait on arm to reach position

    Servos[TOP_UTIL_ARM].detach();

  } else { // Open both arms if closed
    DEBUG_PRINT_LN(F("Open utility arms"));
    utilityArmOpen = true;
    topUtilityArmOpen = true;
    bottomUtilityArmOpen = true;

    Servos[TOP_UTIL_ARM].attach(TOP_UTIL_ARM_SERVO_PIN, ARMMINPULSE, ARMMAXPULSE);
    Servos[BOTTOM_UTIL_ARM].attach(BOTTOM_UTIL_ARM_SERVO_PIN, ARMMINPULSE, ARMMAXPULSE);

    Servos[TOP_UTIL_ARM].write(TOP_ARM_OPEN, UTILITYARMSSPEED);
    Servos[BOTTOM_UTIL_ARM].write(BOTTOM_ARM_OPEN, UTILITYARMSSPEED);

    waitTime(1000);  // wait on arm to reach position

    Servos[TOP_UTIL_ARM].detach(); // detach so we can move the arms freely
    Servos[BOTTOM_UTIL_ARM].detach();
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

    Servos[BOTTOM_UTIL_ARM].attach(BOTTOM_UTIL_ARM_SERVO_PIN, ARMMINPULSE, ARMMAXPULSE);

    // pull arms slightly beyond closed to make sure they're really closed. Will vary on your droid
    Servos[BOTTOM_UTIL_ARM].write(BOTTOM_ARM_CLOSE, UTILITYARMSSPEED);

    waitTime(1000);  // wait on arm to reach position

    Servos[BOTTOM_UTIL_ARM].detach();

  } else if (topUtilityArmOpen) { // If the top arm is open, close it
    DEBUG_PRINT_LN(F("Close top utility arm"));
    topUtilityArmOpen = false;

    Servos[TOP_UTIL_ARM].attach(TOP_UTIL_ARM_SERVO_PIN, ARMMINPULSE, ARMMAXPULSE);

    // Set Servo position to Close.
    Servos[TOP_UTIL_ARM].write(TOP_ARM_CLOSE, UTILITYARMSSPEED); // close at moderate speed

    waitTime(1000);  // wait on arm to reach position

    Servos[TOP_UTIL_ARM].detach(); // detach so we can move the arms freely

  } else { // Open the Top Arm Only
    DEBUG_PRINT_LN(F("Open top utility arm"));
    topUtilityArmOpen = true;

    Servos[TOP_UTIL_ARM].attach(TOP_UTIL_ARM_SERVO_PIN, ARMMINPULSE, ARMMAXPULSE);

    // Set Servo position to Open.
    Servos[TOP_UTIL_ARM].write(TOP_ARM_OPEN, UTILITYARMSSPEED); // open at moderate speed

    waitTime(1000);  // wait on arm to reach position

    Servos[TOP_UTIL_ARM].detach(); // detach so we can move the arms freely

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

    Servos[TOP_UTIL_ARM].attach(TOP_UTIL_ARM_SERVO_PIN, ARMMINPULSE, ARMMAXPULSE);

    // pull arms slightly beyond closed to make sure they're really closed. Will vary on your droid
    Servos[TOP_UTIL_ARM].write(TOP_ARM_CLOSE, UTILITYARMSSPEED);

    waitTime(1000);  // wait on arm to reach position

    Servos[TOP_UTIL_ARM].detach();

  } else if (bottomUtilityArmOpen) { // If the bottom arm is open, close it
    DEBUG_PRINT_LN(F("Close bottom utility arm"));
    bottomUtilityArmOpen = false;

    Servos[BOTTOM_UTIL_ARM].attach(BOTTOM_UTIL_ARM_SERVO_PIN, ARMMINPULSE, ARMMAXPULSE);

    // Set Servo position to Close.
    Servos[BOTTOM_UTIL_ARM].write(BOTTOM_ARM_CLOSE, UTILITYARMSSPEED); // close at moderate speed

    waitTime(1000);  // wait on arm to reach position

    Servos[BOTTOM_UTIL_ARM].detach(); // detach so we can move the arms freely

  } else { // Open the Bottom Arm Only
    DEBUG_PRINT_LN(F("Open bottom utility arm"));
    bottomUtilityArmOpen = true;

    Servos[BOTTOM_UTIL_ARM].attach(BOTTOM_UTIL_ARM_SERVO_PIN, ARMMINPULSE, ARMMAXPULSE);

    // Set Servo position to Open.
    Servos[BOTTOM_UTIL_ARM].write(BOTTOM_ARM_OPEN, UTILITYARMSSPEED); // open at moderate speed

    waitTime(1000);  // wait on arm to reach position

    Servos[BOTTOM_UTIL_ARM].detach(); // detach so we can move the arms freely

  }

  digitalWrite(STATUS_LED, LOW);
}

//---------------------------------------------
// S C R E A M
//---------------------------------------------
void Scream() {

  digitalWrite(STATUS_LED, HIGH);

  playScream();
  Servos[TOP_UTIL_ARM].attach(TOP_UTIL_ARM_SERVO_PIN, ARMMINPULSE, ARMMAXPULSE);
  Servos[BOTTOM_UTIL_ARM].attach(BOTTOM_UTIL_ARM_SERVO_PIN, ARMMINPULSE, ARMMAXPULSE);
  Servos[LEFT_DOOR].attach(LEFT_DOOR_SERVO_PIN, LEFT_DOOR_MINPULSE, LEFT_DOOR_MAXPULSE);
  Servos[RIGHT_DOOR].attach(RIGHT_DOOR_SERVO_PIN, RIGHT_DOOR_MINPULSE, RIGHT_DOOR_MAXPULSE);
  Servos[CBI_DOOR].attach(CBI_DOOR_SERVO_PIN, CBI_DOOR_MINPULSE, CBI_DOOR_MAXPULSE);
  Servos[DATA_DOOR].attach(DATA_DOOR_SERVO_PIN, DATA_DOOR_MINPULSE, DATA_DOOR_MAXPULSE);

  for (int i = 0; i < 7; i++) {

    DEBUG_PRINT(F("Loop:"));
    DEBUG_PRINT_LN(i + 1);
    Servos[LEFT_DOOR].write(LEFT_DOOR_OPEN, SCREAM_SPEED);
    Servos[DATA_DOOR].write(DATA_DOOR_OPEN, SCREAM_SPEED);
    digitalWrite(DP_SWITCH_PIN, HIGH); //Turns on Data Panel lights

    Servos[RIGHT_DOOR].write(RIGHT_DOOR_CLOSE, SCREAM_SPEED);
    Servos[CBI_DOOR].write(CBI_DOOR_CLOSE, SCREAM_SPEED);
    digitalWrite(CBI_SWITCH_PIN, LOW); //Turns off CBI lights

    Servos[TOP_UTIL_ARM].write(1250, 255);
    Servos[BOTTOM_UTIL_ARM].write(BOTTOM_ARM_CLOSE, 255);

    waitTime(150);

    Servos[LEFT_DOOR].write(LEFT_DOOR_CLOSE, SCREAM_SPEED);
    Servos[DATA_DOOR].write(DATA_DOOR_CLOSE, SCREAM_SPEED);
    digitalWrite(DP_SWITCH_PIN, LOW); //Turns off Data Panel lights

    Servos[RIGHT_DOOR].write(RIGHT_DOOR_OPEN, SCREAM_SPEED);
    Servos[CBI_DOOR].write(CBI_DOOR_OPEN, SCREAM_SPEED);
    digitalWrite(CBI_SWITCH_PIN, HIGH); //Turns on CBI lights

    Servos[TOP_UTIL_ARM].write(TOP_ARM_CLOSE, 255); // open at moderate speed
    Servos[BOTTOM_UTIL_ARM].write(1250, 255); // 0=open all the way

    waitTime(150);

  }

  DEBUG_PRINT_LN(F("Close everything"));

  Servos[TOP_UTIL_ARM].write(TOP_ARM_CLOSE, UTILITYARMSSPEED);
  Servos[BOTTOM_UTIL_ARM].write(BOTTOM_ARM_CLOSE, UTILITYARMSSPEED);
  Servos[LEFT_DOOR].write(LEFT_DOOR_CLOSE, DOOR_CLOSE_SPEED);
  Servos[RIGHT_DOOR].write(RIGHT_DOOR_CLOSE, DOOR_CLOSE_SPEED);
  Servos[CBI_DOOR].write(CBI_DOOR_CLOSE, DOOR_CLOSE_SPEED);
  Servos[DATA_DOOR].write(DATA_DOOR_CLOSE, DOOR_CLOSE_SPEED);

  digitalWrite(CBI_SWITCH_PIN, LOW); //Turns off CBI lights
  digitalWrite(DP_SWITCH_PIN, LOW); //Turns off Data Panel lights
  digitalWrite(VM_SWITCH_PIN, LOW);  //Turns off Voltmeter

  waitTime(1000); // wait on arm to reach position

  DEBUG_PRINT_LN(F("Detach"));

  Servos[LEFT_DOOR].detach();
  Servos[RIGHT_DOOR].detach();
  Servos[CBI_DOOR].detach();
  Servos[DATA_DOOR].detach();
  Servos[TOP_UTIL_ARM].detach();
  Servos[BOTTOM_UTIL_ARM].detach();

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

    Servos[LEFT_DOOR].attach(LEFT_DOOR_SERVO_PIN, LEFT_DOOR_MINPULSE, LEFT_DOOR_MAXPULSE);
    Servos[RIGHT_DOOR].attach(RIGHT_DOOR_SERVO_PIN, RIGHT_DOOR_MINPULSE, RIGHT_DOOR_MAXPULSE);
    Servos[CBI_DOOR].attach(CBI_DOOR_SERVO_PIN, CBI_DOOR_MINPULSE, CBI_DOOR_MAXPULSE);
    Servos[DATA_DOOR].attach(DATA_DOOR_SERVO_PIN, DATA_DOOR_MINPULSE, DATA_DOOR_MAXPULSE);

    Servos[LEFT_DOOR].write(LEFT_DOOR_CLOSE, DOOR_CLOSE_SPEED);
    Servos[RIGHT_DOOR].write(RIGHT_DOOR_CLOSE, DOOR_CLOSE_SPEED);
    Servos[CBI_DOOR].write(CBI_DOOR_CLOSE, DOOR_CLOSE_SPEED);
    Servos[DATA_DOOR].write(DATA_DOOR_CLOSE, DOOR_CLOSE_SPEED);

    digitalWrite(CBI_SWITCH_PIN, LOW); //Turns off CBI lights
    digitalWrite(DP_SWITCH_PIN, LOW); //Turns off Data Panel lights
    digitalWrite(VM_SWITCH_PIN, LOW);  //Turns off Voltmeter

    waitTime(1000); // wait on arm to reach position

    Servos[LEFT_DOOR].detach();
    Servos[RIGHT_DOOR].detach();
    Servos[CBI_DOOR].detach();
    Servos[DATA_DOOR].detach();

  } else {
    DEBUG_PRINT_LN(F("Open doors"));
    doorsOpen = true;
    leftDoorOpen = true;
    rightDoorOpen = true;
    cbi_dataOpen = true;
    cbiDoorOpen = true;
    dataDoorOpen = true;

    Servos[LEFT_DOOR].attach(LEFT_DOOR_SERVO_PIN, LEFT_DOOR_MINPULSE, LEFT_DOOR_MAXPULSE);
    Servos[RIGHT_DOOR].attach(RIGHT_DOOR_SERVO_PIN, RIGHT_DOOR_MINPULSE, RIGHT_DOOR_MAXPULSE);
    Servos[CBI_DOOR].attach(CBI_DOOR_SERVO_PIN, CBI_DOOR_MINPULSE, CBI_DOOR_MAXPULSE);
    Servos[DATA_DOOR].attach(DATA_DOOR_SERVO_PIN, DATA_DOOR_MINPULSE, DATA_DOOR_MAXPULSE);

    Servos[LEFT_DOOR].write(LEFT_DOOR_OPEN, DOOR_OPEN_SPEED);
    Servos[RIGHT_DOOR].write(RIGHT_DOOR_OPEN, DOOR_OPEN_SPEED);
    Servos[CBI_DOOR].write(CBI_DOOR_OPEN, DOOR_OPEN_SPEED);
    Servos[DATA_DOOR].write(DATA_DOOR_OPEN, DOOR_OPEN_SPEED);

    digitalWrite(CBI_SWITCH_PIN, HIGH); //Turns on CBI lights
    digitalWrite(DP_SWITCH_PIN, HIGH); //Turns on Data Panel lights
    digitalWrite(VM_SWITCH_PIN, HIGH);  //Turns on Voltmeter

    waitTime(1000);

    Servos[LEFT_DOOR].detach();
    Servos[RIGHT_DOOR].detach();
    Servos[CBI_DOOR].detach();
    Servos[DATA_DOOR].detach();

  }

  digitalWrite(STATUS_LED, LOW);
}

void openLeftDoor() {

  digitalWrite(STATUS_LED, HIGH);

  if (leftDoorOpen) {
    DEBUG_PRINT_LN(F("Close Left Door"));
    leftDoorOpen = false;
    Servos[LEFT_DOOR].attach(LEFT_DOOR_SERVO_PIN, LEFT_DOOR_MINPULSE, LEFT_DOOR_MAXPULSE);
    Servos[LEFT_DOOR].write(LEFT_DOOR_CLOSE, DOOR_CLOSE_SPEED);
    waitTime(1000); // wait on door to reach position
    Servos[LEFT_DOOR].detach();

  } else {
    leftDoorOpen = true;
    DEBUG_PRINT_LN(F("Open Left Door"));
    Servos[LEFT_DOOR].attach(LEFT_DOOR_SERVO_PIN, LEFT_DOOR_MINPULSE, LEFT_DOOR_MAXPULSE);
    Servos[LEFT_DOOR].write(LEFT_DOOR_OPEN, DOOR_OPEN_SPEED);
    waitTime(1000);
    Servos[LEFT_DOOR].detach();
  }

  digitalWrite(STATUS_LED, LOW);
}

void openRightDoor() {
  digitalWrite(STATUS_LED, HIGH);

  if (rightDoorOpen) {
    DEBUG_PRINT_LN(F("Close Right Door"));
    rightDoorOpen = false;
    Servos[RIGHT_DOOR].attach(RIGHT_DOOR_SERVO_PIN, RIGHT_DOOR_MINPULSE, RIGHT_DOOR_MAXPULSE);
    Servos[RIGHT_DOOR].write(RIGHT_DOOR_CLOSE, DOOR_CLOSE_SPEED);
    waitTime(1000); // wait on door to reach position
    Servos[RIGHT_DOOR].detach();

  } else {
    rightDoorOpen = true;
    DEBUG_PRINT_LN(F("Open Right Door"));
    Servos[RIGHT_DOOR].attach(RIGHT_DOOR_SERVO_PIN, RIGHT_DOOR_MINPULSE, RIGHT_DOOR_MAXPULSE);
    Servos[RIGHT_DOOR].write(RIGHT_DOOR_OPEN, DOOR_OPEN_SPEED);
    waitTime(1000);
    Servos[RIGHT_DOOR].detach();
  }

  digitalWrite(STATUS_LED, LOW);
}

void openCBIDoor() {
  digitalWrite(STATUS_LED, HIGH);

  if (cbiDoorOpen) {
    DEBUG_PRINT_LN(F("Close Charge Bay Door"));
    cbiDoorOpen = false;
    Servos[CBI_DOOR].attach(CBI_DOOR_SERVO_PIN, CBI_DOOR_MINPULSE, CBI_DOOR_MAXPULSE);
    Servos[CBI_DOOR].write(CBI_DOOR_CLOSE, DOOR_CLOSE_SPEED);

    digitalWrite(CBI_SWITCH_PIN, LOW); //Turns off CBI lights

    waitTime(1000); // wait on door to reach position
    Servos[CBI_DOOR].detach();

  } else {
    cbiDoorOpen = true;
    DEBUG_PRINT_LN(F("Open Charge Bay Door"));
    Servos[CBI_DOOR].attach(CBI_DOOR_SERVO_PIN, CBI_DOOR_MINPULSE, CBI_DOOR_MAXPULSE);
    Servos[CBI_DOOR].write(CBI_DOOR_OPEN, DOOR_OPEN_SPEED);

    digitalWrite(CBI_SWITCH_PIN, HIGH); //Turns on CBI lights

    waitTime(1000);
    Servos[CBI_DOOR].detach();
  }

  digitalWrite(STATUS_LED, LOW);
}

void openDataDoor() {
  digitalWrite(STATUS_LED, HIGH);

  if (dataDoorOpen) {
    DEBUG_PRINT_LN(F("Close Data Port Door"));
    dataDoorOpen = false;
    Servos[DATA_DOOR].attach(DATA_DOOR_SERVO_PIN, DATA_DOOR_MINPULSE, DATA_DOOR_MAXPULSE);
    Servos[DATA_DOOR].write(DATA_DOOR_CLOSE, DOOR_CLOSE_SPEED);

    digitalWrite(DP_SWITCH_PIN, LOW); //Turns off Data Panel lights
    digitalWrite(VM_SWITCH_PIN, LOW);  //Turns off Voltmeter

    waitTime(1000); // wait on door to reach position
    Servos[DATA_DOOR].detach();

  } else {
    dataDoorOpen = true;
    DEBUG_PRINT_LN(F("Open Data Port Door"));
    Servos[DATA_DOOR].attach(DATA_DOOR_SERVO_PIN, DATA_DOOR_MINPULSE, DATA_DOOR_MAXPULSE);
    Servos[DATA_DOOR].write(DATA_DOOR_OPEN, DOOR_OPEN_SPEED);

    digitalWrite(DP_SWITCH_PIN, HIGH); //Turns on Data Panel lights
    digitalWrite(VM_SWITCH_PIN, HIGH);  //Turns on Voltmeter

    waitTime(1000);
    Servos[DATA_DOOR].detach();
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
    Servos[CBI_DOOR].attach(CBI_DOOR_SERVO_PIN, CBI_DOOR_MINPULSE, CBI_DOOR_MAXPULSE);
    Servos[DATA_DOOR].attach(DATA_DOOR_SERVO_PIN, DATA_DOOR_MINPULSE, DATA_DOOR_MAXPULSE);

    Servos[CBI_DOOR].write(CBI_DOOR_CLOSE, DOOR_CLOSE_SPEED);
    Servos[DATA_DOOR].write(DATA_DOOR_CLOSE, DOOR_CLOSE_SPEED);

    digitalWrite(CBI_SWITCH_PIN, LOW); //Turns off CBI lights
    digitalWrite(DP_SWITCH_PIN, LOW); //Turns off Data Panel lights
    digitalWrite(VM_SWITCH_PIN, LOW);  //Turns off Voltmeter

    waitTime(1000); // wait on door to reach position
    Servos[CBI_DOOR].detach();
    Servos[DATA_DOOR].detach();

  } else {
    cbi_dataOpen = true;
    cbiDoorOpen = true;
    dataDoorOpen = true;
    DEBUG_PRINT_LN(F("Open Charge Bay & Data Door"));
    Servos[CBI_DOOR].attach(CBI_DOOR_SERVO_PIN, CBI_DOOR_MINPULSE, CBI_DOOR_MAXPULSE);
    Servos[DATA_DOOR].attach(DATA_DOOR_SERVO_PIN, DATA_DOOR_MINPULSE, DATA_DOOR_MAXPULSE);

    Servos[CBI_DOOR].write(CBI_DOOR_OPEN, DOOR_OPEN_SPEED);
    Servos[DATA_DOOR].write(DATA_DOOR_OPEN, DOOR_OPEN_SPEED);

    digitalWrite(CBI_SWITCH_PIN, HIGH); //Turns on CBI lights
    digitalWrite(DP_SWITCH_PIN, HIGH); //Turns on Data Panel lights
    digitalWrite(VM_SWITCH_PIN, HIGH);  //Turns on Voltmeter

    waitTime(1000);
    Servos[CBI_DOOR].detach();
    Servos[DATA_DOOR].detach();
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
  {}// do nothing
}

// Serial Command Functions

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

  if (strcmp(cmd, "RESET") == 0) {
    DEBUG_PRINT_LN(F("Got reset message"));
    resetServos();
    resetVocalizer();
    digitalWrite(STATUS_LED, HIGH);
  } else if (strcmp(cmd, "VADER") == 0) {
    Vader();
  } else if (strcmp(cmd, "THEME") == 0) {
    Theme();
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
  } else if (strcmp(cmd, "LOVE") == 0) {
    love();
  } else {
    digitalWrite(STATUS_LED, LOW);
  }
}

///////////////////////////////////////////////////
// Test LEDs, each Maxim driver row in turn
// Each LED blinks according to the col number
// Col 0 is just on
// Col 1 blinks twice
// col 2 blinks 3 times, etc...
//

#define TESTDELAY 30
void singleTest()
{
  for (int row = 0; row < 6; row++)
  {
    for (int col = 0; col < 7; col++)
    {
      waitTime(TESTDELAY);
      lc.setLed(DATAPORT, row, col, true);
      waitTime(TESTDELAY);
      for (int i = 0; i < col; i++)
      {
        lc.setLed(DATAPORT, row, col, false);
        waitTime(TESTDELAY);
        lc.setLed(DATAPORT, row, col, true);
        waitTime(TESTDELAY);
      }
    }
  }


  for (int row = 0; row < 4; row++)
  {
    for (int col = 0; col < 5; col++)
    {
      waitTime(TESTDELAY);
      lc.setLed(CBI, row, col, true);
      waitTime(TESTDELAY);
      for (int i = 0; i < col; i++)
      {
        lc.setLed(CBI, row, col, false);
        waitTime(TESTDELAY);
        lc.setLed(CBI, row, col, true);
        waitTime(TESTDELAY);
      }
    }
  }

  lc.setLed(CBI, 4, 5, true);
  waitTime(TESTDELAY);
  lc.setLed(CBI, 5, 5, true);
  waitTime(TESTDELAY);
  lc.setLed(CBI, 6, 5, true);
  waitTime(TESTDELAY);
}

///////////////////////////////////
// animates the two top left blocks
// (green and yellow blocks)
void updateTopBlocks()
{
  static unsigned long timeLast = 0;
  unsigned long elapsed;
  elapsed = millis();
  if ((elapsed - timeLast) < TOPBLOCKSPEED) return;
  timeLast = elapsed;

  lc.setRow(DATAPORT, 4, randomRow(4)); // top yellow blocks
  lc.setRow(DATAPORT, 5, randomRow(4)); // top green blocks

}

///////////////////////////////////
// animates the CBI
//
void updateCBILEDs()
{
  static unsigned long timeLast = 0;
  unsigned long elapsed;
  elapsed = millis();
  if ((elapsed - timeLast) < CBISPEED) return;
  timeLast = elapsed;

#ifdef monitorVCC
  lc.setRow(CBI, random(4), randomRow(random(4)));
#else
  lc.setRow(CBI, random(7), randomRow(random(4)));
#endif
}

////////////////////////////////////
// Utility to generate random LED patterns
// Mode goes from 0 to 6. The lower the mode
// the less the LED density that's on.
// Modes 4 and 5 give the most organic feel
byte randomRow(byte randomMode)
{
  switch (randomMode)
  {
    case 0:  // stage -3
      return (random(256)&random(256)&random(256)&random(256));
      break;
    case 1:  // stage -2
      return (random(256)&random(256)&random(256));
      break;
    case 2:  // stage -1
      return (random(256)&random(256));
      break;
    case 3: // legacy "blocky" mode
      return random(256);
      break;
    case 4:  // stage 1
      return (random(256) | random(256));
      break;
    case 5:  // stage 2
      return (random(256) | random(256) | random(256));
      break;
    case 6:  // stage 3
      return (random(256) | random(256) | random(256) | random(256));
      break;
    default:
      return random(256);
      break;
  }
}

//////////////////////
// bargraph for the right column
// disp 0: Row 2 Col 5 to 0 (left bar) - 6 to 0 if including lower red LED,
// disp 1: Row 3 Col 5 to 0 (right bar)

#define MAXGRAPH 2

void bargraphDisplay(byte disp)
{
  static byte bargraphdata[MAXGRAPH]; // status of bars

  if (disp >= MAXGRAPH) return;

  // speed control
  static unsigned long previousDisplayUpdate[MAXGRAPH] = {0, 0};

  unsigned long currentMillis = millis();
  if (currentMillis - previousDisplayUpdate[disp] < BARGRAPHSPEED) return;
  previousDisplayUpdate[disp] = currentMillis;

  // adjust to max numbers of LED available per bargraph
  byte maxcol;
  if (disp == 0 || disp == 1) maxcol = 6;
  else maxcol = 3; // for smaller graph bars, not defined yet

  // use utility to update the value of the bargraph  from it's previous value
  byte value = updatebar(disp, &bargraphdata[disp], maxcol);
  byte data = 0;
  // transform value into byte representing of illuminated LEDs
  // start at 1 so it can go all the way to no illuminated LED
  for (int i = 1; i <= value; i++)
  {
    data |= 0x01 << (i - 1);
  }
  // transfer the byte column wise to the video grid
  fillBar(disp, data, value, maxcol);
}

/////////////////////////////////
// helper for updating bargraph values, to imitate bargraph movement
byte updatebar(byte disp, byte* bargraphdata, byte maxcol)
{
  // bargraph values go up or down one pixel at a time
  int variation = random(0, 3);           // 0= move down, 1= stay, 2= move up
  int value = (int)(*bargraphdata);       // get the previous value
  //if (value==maxcol) value=maxcol-2; else      // special case, staying stuck at maximum does not look realistic, knock it down
  value += (variation - 1);               // grow or shring it by one step
#ifndef BLUELEDTRACKGRAPH
  if (value <= 0) value = 0;              // can't be lower than 0
#else
  if (value <= 1) value = 1;              // if blue LED tracks, OK to keep lower LED always on
#endif
  if (value > maxcol) value = maxcol;     // can't be higher than max
  (*bargraphdata) = (byte)value;          // store new value, use byte type to save RAM
  return (byte)value;                     // return new value
}

/////////////////////////////////////////
// helper for lighting up a bar of LEDs based on a value
void fillBar(byte disp, byte data, byte value, byte maxcol)
{
  for (byte col = 0; col < maxcol; col++)
  {
    // test state of LED
    byte LEDon = (data & 1 << col);
    if (LEDon)
    {
      //lc.setLed(DATAPORT,row,maxcol-col-1,true);  // set column bit
      lc.setLed(DATAPORT, 2, maxcol - col - 1, true); // set column bit
      lc.setLed(DATAPORT, 3, maxcol - col - 1, true); // set column bit
      //lc.setLed(DATAPORT,0,maxcol-col-1,true);      // set blue column bit
    }
    else
    {
      //lc.setLed(DATAPORT,row,maxcol-col-1,false); // reset column bit
      lc.setLed(DATAPORT, 2, maxcol - col - 1, false); // reset column bit
      lc.setLed(DATAPORT, 3, maxcol - col - 1, false); // reset column bit
      //lc.setLed(DATAPORT,0,maxcol-col-1,false);     // set blue column bit
    }
  }
#ifdef BLUELEDTRACKGRAPH
  // do blue tracking LED
  byte blueLEDrow = B00000010;
  blueLEDrow = blueLEDrow << value;
  lc.setRow(DATAPORT, 0, blueLEDrow);
#endif
}

/////////////////////////////////
// This animates the bottom white LEDs
void updatebottomLEDs()
{
  static unsigned long timeLast = 0;
  unsigned long elapsed = millis();
  if ((elapsed - timeLast) < BOTTOMLEDSPEED) return;
  timeLast = elapsed;

  // bottom LEDs are row 1,
  lc.setRow(DATAPORT, 1, randomRow(4));
}

////////////////////////////////
// This is for the two red LEDs
void updateRedLEDs()
{
  static unsigned long timeLast = 0;
  unsigned long elapsed = millis();
  if ((elapsed - timeLast) < REDLEDSPEED) return;
  timeLast = elapsed;

  // red LEDs are row 2 and 3, col 6,
  lc.setLed(DATAPORT, 2, 6, random(0, 2));
  lc.setLed(DATAPORT, 3, 6, random(0, 2));
}

//////////////////////////////////
// This animates the blue LEDs
// Uses a random delay, which never exceeds BLUELEDSPEED
void updateBlueLEDs()
{
  static unsigned long timeLast = 0;
  static unsigned long variabledelay = BLUELEDSPEED;
  unsigned long elapsed = millis();
  if ((elapsed - timeLast) < variabledelay) return;
  timeLast = elapsed;
  variabledelay = random(10, BLUELEDSPEED);

  /*********experimental, moving dots animation
    static byte stage=0;
    stage++;
    if (stage>7) stage=0;
    byte LEDstate=B00000011;
    // blue LEDs are row 0 col 0-5
    lc.setRow(DATAPORT,0,LEDstate<<stage);
  *********************/

  // random
  lc.setRow(DATAPORT, 0, randomRow(4));
}


void getVCC()
{
  value = analogRead(analoginput); // this must be between 0.0 and 5.0 - otherwise you'll let the blue smoke out of your arduino
  vout = (value * 5.0) / 1024.0; //voltage coming out of the voltage divider
  vin = vout / (R2 / (R1 + R2)); //voltage to display
  if (vin < 0.09) {
    vin = 0.0; //avoids undesirable reading
  }

  lc.setLed(CBI, 6, 5, (vin >= greenVCC));
  lc.setLed(CBI, 5, 5, (vin >= yellowVCC));
  lc.setLed(CBI, 4, 5, (vin >= redVCC));
#ifdef DEBUG_VM
  DEBUG_PRINT(F("Volt Out = "));
  DEBUG_PRINT_DEC(vout, 1);   //Print float "vout" with 1 decimal place
  DEBUG_PRINT(F("\tVolts Calc = "));
  DEBUG_PRINT_LN_DEC(vin, 1);   //Print float "vin" with 1 decimal place
#endif
}
