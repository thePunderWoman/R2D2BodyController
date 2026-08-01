#include "doors.h"
#include "config.h"
#include "bus.h"
#include "maestro.h"
#include "estop.h"

bool utilityArmOpen = false;
bool topUtilityArmOpen = false;
bool bottomUtilityArmOpen = false;
bool leftDoorOpen = false;
bool rightDoorOpen = false;
bool cbiDoorOpen = false;
bool dataDoorOpen = false;
bool doorsOpen = false;
bool cbi_dataOpen = false;

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

  sendPanelLightCommand("ALLOFF");

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

    sendPanelLightCommand("ALLON");

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

    sendPanelLightCommand("ALLOFF");

    waitTime(1000); // wait on arm to reach position

  } else {
    DEBUG_PRINT_LN(F("Open doors"));
    doorsOpen = true;
    leftDoorOpen = true;
    rightDoorOpen = true;
    cbi_dataOpen = true;
    cbiDoorOpen = true;
    dataDoorOpen = true;

    sendPanelLightCommand("ALLON");

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
    sendPanelLightCommand("CBIOFF");
    moveServo(CBI_DOOR, CBI_DOOR_CLOSE, DOOR_CLOSE_SPEED);

    waitTime(1000); // wait on door to reach position

  } else {
    cbiDoorOpen = true;
    DEBUG_PRINT_LN(F("Open Charge Bay Door"));

    sendPanelLightCommand("CBION");
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
    sendPanelLightCommand("DPOFF");
    moveServo(DATA_DOOR, DATA_DOOR_CLOSE, DOOR_CLOSE_SPEED);

    waitTime(1000); // wait on door to reach position

  } else {
    dataDoorOpen = true;
    DEBUG_PRINT_LN(F("Open Data Port Door"));

    sendPanelLightCommand("DPON");
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

    sendPanelLightCommand("ALLOFF");
    moveServo(CBI_DOOR, CBI_DOOR_CLOSE, DOOR_CLOSE_SPEED);
    moveServo(DATA_DOOR, DATA_DOOR_CLOSE, DOOR_CLOSE_SPEED);

    waitTime(1000); // wait on door to reach position

  } else {
    cbi_dataOpen = true;
    cbiDoorOpen = true;
    dataDoorOpen = true;
    DEBUG_PRINT_LN(F("Open Charge Bay & Data Door"));

    sendPanelLightCommand("ALLON");
    moveServo(CBI_DOOR, CBI_DOOR_OPEN, DOOR_OPEN_SPEED);
    moveServo(DATA_DOOR, DATA_DOOR_OPEN, DOOR_OPEN_SPEED);

    waitTime(1000);
  }

  digitalWrite(STATUS_LED, LOW);
}
