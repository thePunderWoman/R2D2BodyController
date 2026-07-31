// Jessica's Astromech Body Controller Firmware — ESP32-C3 SuperMini + Pololu Maestro + RGB-DPL
// Configuration Header
#pragma once

#include <Arduino.h>

// -------------------------------------------------
// Debug output — defined first since driver helpers further down use it.
// Comment out DEBUG to silence all DEBUG_PRINT*/DEBUG_PRINT_LN* output.
// -------------------------------------------------
// #define DEBUG

#ifdef DEBUG
#define DEBUG_PRINT_LN(msg)  Serial.println(msg)
#define DEBUG_PRINT(msg)  Serial.print(msg)
#define DEBUG_PRINT_LN_DEC(msg, dec_places) Serial.println(msg, dec_places)
#define DEBUG_PRINT_DEC(msg, dec_places) Serial.print(msg, dec_places)
#else
#define DEBUG_PRINT_LN(msg)
#define DEBUG_PRINT(msg)
#define DEBUG_PRINT_LN_DEC(msg, dec_places)
#define DEBUG_PRINT_DEC(msg, dec_places)
#endif

// -------------------------------------------------
// I2C Address (kept for reference / parity with sibling boards on the bus)
// -------------------------------------------------
#define I2CAddress 9      //I2C Address for Body Controller

// Destination I2C addresses
#define STEALTH_CNTL 0    // Main Stealth Controller (Master for the Body I2C bus)
#define BODY_EXP 9        // Body Controller
#define DOME_EXP 12       // Dome Astropixels Plus
#define LOGICS 10         // Rseries Logics (Teensy version)
#define MAGIC_PNL 20      // Magic Panel
#define PSI_FRNT 22       // Front PSI
#define PSI_REAR 23       // Rear PSI
#define FLTHY_HP 25       // FlthyHP Breakout Board
#define PERI_LIFT 32      // Periscope Lifter

// -------------------------------------------------
// Firmware identity — shown on the OTA update page (see web_page.h). Bump
// FIRMWARE_VERSION by hand each release.
// -------------------------------------------------
#define FIRMWARE_VERSION "2.0.0"
#define BOARD_REV        "1.0"
#define MCU_VARIANT      "ESP32-C3 SuperMini"

// -------------------------------------------------
// WiFi AP for wireless firmware updates. Connect to this network, then
// browse to http://bodycontroller.local/ (or the IP shown on boot).
// -------------------------------------------------
#define OTA_AP_SSID     "BodyController"
#define OTA_AP_PASSWORD "Astromech"   // CHANGE ME before flashing

// Onboard LED on the SuperMini board (nologo_esp32c3_super_mini variant)
#define STATUS_LED 8

// -------------------------------------------------
// Serial buses
// -------------------------------------------------
// UART0 — shared trunk: WCB commands in, HCR vocalizer commands out, and
// RGB-DPL panel / periscope commands out (sendBusCommand() in bus.cpp).
// These are the conventional default UART0 pins on ESP32-C3 boards; verify
// against your SuperMini's silkscreen. The HardwareSerial object itself
// lives in bus.h/bus.cpp.
#define WCB_RX_PIN 20
#define WCB_TX_PIN 21
#define WCB_BAUD   9600

// UART1 — Pololu Mini Maestro 12, Compact Protocol only. Configure the
// Maestro itself (via Maestro Control Center) to "Fixed Baud Rate" at
// MAESTRO_BAUD to match — it won't talk back until that's set. The
// HardwareSerial object itself lives in maestro.h/maestro.cpp.
#define MAESTRO_RX_PIN 3
#define MAESTRO_TX_PIN 4
#define MAESTRO_BAUD   115200

// Optional hardware lines, not required for basic operation but wired up
// for resilience/diagnostics. Neither GPIO5 nor GPIO6 is an ESP32-C3
// strapping pin, so they're safe regardless of what the Maestro is doing
// to them at boot (GPIO2/8/9 are the ones to avoid — see the note on
// MAESTRO_ERR_PIN below for why that mattered here).
#define MAESTRO_RST_PIN 5  // ESP32 output -> Maestro RST (active low; Maestro pulls it up internally, so idle-high here is safe)
#define MAESTRO_ERR_PIN 6  // Maestro ERR -> ESP32 input (idles LOW when no error, which is why this can't be a strapping pin)

#define NBR_SERVOS 6  // Number of servos == Maestro channels 0-5

// Easy names for the servo index / Maestro channel number — same value
// serves both roles, no separate pin table needed now that the Maestro
// generates the actual PWM.
#define LEFT_DOOR 0
#define RIGHT_DOOR 1
#define CBI_DOOR 2
#define DATA_DOOR 3
#define BOTTOM_UTIL_ARM 4
#define TOP_UTIL_ARM 5

//Servo Speeds — Maestro Set Speed units, (0.25us)/(10ms). This works out to
// the exact same us/sec-per-unit rate as VarSpeedServo's old speed units
// (both are "N minimal-time-units per fixed tick" designs that happen to
// reduce to the same 25 us/sec-per-unit constant), so the original 0-255
// values carry over unscaled. Still just a starting point — retune by feel.
#define UTILITYARMSSPEED 100 // servo speed for opening utility arms. 1=super slow, 255=fastest
#define UTILITYARMSSPEED2 45 // servo speed for opening utility arms. 1=super slow, 255=fastest
#define UTILITYARMSSPEED3 35 // servo speed for opening utility arms. 1=super slow, 255=fastest

#define DOOR_OPEN_SPEED 120 // servo speed for opening door
#define DOOR_CLOSE_SPEED 100 // servo speed for opening door

#define SCREAM_SPEED 250

#define OVERLOAD_DRIFT_SPEED 30

#define FLUTTER_SPEED 150       // servo speed for the flutter wave open/close
#define FLUTTER_STAGGER_MS 120  // delay between each door starting its move, right to left
#define FLUTTER_HOLD_MS 500     // how long doors hold halfway open before closing wave

//Tweaked Pulse Widths. Usually 1000-2000 or 500-2500. These clamp
// moveServo() targets in firmware as a software safety net — also set
// matching per-channel min/max limits on the Maestro itself via Maestro
// Control Center for a hardware-level backstop.
#define ARMMINPULSE 600
#define ARMMAXPULSE 2400

#define RIGHT_DOOR_MINPULSE 650
#define RIGHT_DOOR_MAXPULSE 2200

#define LEFT_DOOR_MINPULSE 650
#define LEFT_DOOR_MAXPULSE 2200

#define CBI_DOOR_MINPULSE 650
#define CBI_DOOR_MAXPULSE 2200

#define DATA_DOOR_MINPULSE 2200
#define DATA_DOOR_MAXPULSE 650

// How long after a commanded move should finish before its channel gets
// released (Set Target 0) if nothing re-commands it first. The Maestro
// holds a channel under power indefinitely once it reaches a target --
// unlike VarSpeedServo/ReelTwo, which auto-detached after settling -- and
// a servo held under power while stationary is exactly what causes the
// hunting/buzz noise. This margin is padding on top of the estimated move
// time, to allow for the Maestro's own settle/overshoot.
#define SERVO_RELEASE_MARGIN_MS 300

//Servo Positions
#define TOP_ARM_OPEN 650
#define BOTTOM_ARM_OPEN 750
#define LEFT_DOOR_OPEN 2200
#define RIGHT_DOOR_OPEN 1430
#define CBI_DOOR_OPEN 1100
#define DATA_DOOR_OPEN 950

#define TOP_ARM_CLOSE 1780
#define BOTTOM_ARM_CLOSE 1800
#define LEFT_DOOR_CLOSE 1500
#define RIGHT_DOOR_CLOSE 2000
#define CBI_DOOR_CLOSE 1780
#define DATA_DOOR_CLOSE 1950
