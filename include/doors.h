// doors.h — door / utility-arm open-close toggles and their shared state.
#pragma once

#include <Arduino.h>

// Some variables to keep track of doors and arms etc. Shared with
// sequences.cpp, which reads and resets these directly (Cantina, Scream,
// Flutter, overload all move the same servos and need to leave state
// consistent when they finish).
extern bool utilityArmOpen;
extern bool topUtilityArmOpen;
extern bool bottomUtilityArmOpen;
extern bool leftDoorOpen;
extern bool rightDoorOpen;
extern bool cbiDoorOpen;
extern bool dataDoorOpen;
extern bool doorsOpen;
extern bool cbi_dataOpen;

void resetServos();
void openEverything();
void UtilityArms();
void TopUtilityArm();
void BottomUtilityArm();
void Doors();
void openLeftDoor();
void openRightDoor();
void openCBIDoor();
void openDataDoor();
void openCBI_DataDoor();
