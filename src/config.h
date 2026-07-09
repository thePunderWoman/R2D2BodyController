// Jessica's Astromech Body Controller Firmware — ESP32-C3 SuperMini + Pololu Maestro + RGB-DPL
// Configuration Header

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
// RGB-DPL panel commands out (sendPanelCommand() in the .ino). These are the
// conventional default UART0 pins on ESP32-C3 boards; verify against your
// SuperMini's silkscreen.
#define WCB_RX_PIN 20
#define WCB_TX_PIN 21
#define WCB_BAUD   9600

// UART1 — Pololu Mini Maestro 12, Compact Protocol only. Configure the
// Maestro itself (via Maestro Control Center) to "Fixed Baud Rate" at
// MAESTRO_BAUD to match — it won't talk back until that's set.
#define MAESTRO_RX_PIN 5
#define MAESTRO_TX_PIN 4
#define MAESTRO_BAUD   115200

HardwareSerial WCBSerial(0);
HardwareSerial MaestroSerial(1);

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

// ---------------------------------------------------------
// Pololu Maestro servo driver (Compact Protocol over MaestroSerial)
// ---------------------------------------------------------
// The Maestro does its own speed-limited interpolation in hardware, so
// unlike the ReelTwo ServoDispatchDirect this replaced, nothing here needs
// AnimatedEvent::process() pumped to make progress — sequences just fire a
// Set Speed + Set Target pair and the Maestro takes it from there.

static const uint16_t servoMinPulse[NBR_SERVOS] = {
  LEFT_DOOR_MINPULSE, RIGHT_DOOR_MINPULSE, CBI_DOOR_MINPULSE, DATA_DOOR_MINPULSE, ARMMINPULSE, ARMMINPULSE
};
static const uint16_t servoMaxPulse[NBR_SERVOS] = {
  LEFT_DOOR_MAXPULSE, RIGHT_DOOR_MAXPULSE, CBI_DOOR_MAXPULSE, DATA_DOOR_MAXPULSE, ARMMAXPULSE, ARMMAXPULSE
};

// A couple of these pairs are stored max<min (e.g. DATA_DOOR), so clamp
// against the numeric min/max of the pair rather than assuming order.
static inline uint16_t clampServoPulse(uint8_t servoIndex, uint16_t pos)
{
  uint16_t a = servoMinPulse[servoIndex];
  uint16_t b = servoMaxPulse[servoIndex];
  uint16_t lo = min(a, b);
  uint16_t hi = max(a, b);
  if (pos < lo) pos = lo;
  if (pos > hi) pos = hi;
  return pos;
}

static inline void maestroSetTarget(uint8_t channel, uint16_t pulseUs)
{
  uint16_t target = pulseUs * 4; // Maestro units are quarter-microseconds
  uint8_t buf[4] = { 0x84, channel, (uint8_t)(target & 0x7F), (uint8_t)((target >> 7) & 0x7F) };
  MaestroSerial.write(buf, 4);
}

static inline void maestroSetSpeed(uint8_t channel, uint16_t speed)
{
  uint8_t buf[4] = { 0x87, channel, (uint8_t)(speed & 0x7F), (uint8_t)((speed >> 7) & 0x7F) };
  MaestroSerial.write(buf, 4);
}

// Get Position (0x90) — blocks briefly for the Maestro's 2-byte reply. Only
// used by performEStop() to read exactly where a channel is before locking
// it there; nothing else in normal sequence playback needs a round trip.
static inline uint16_t maestroGetPosition(uint8_t channel)
{
  uint8_t cmd[2] = { 0x90, channel };
  MaestroSerial.write(cmd, 2);
  unsigned long start = millis();
  while (MaestroSerial.available() < 2) {
    if (millis() - start > 50) return 0; // timeout - treat as "unknown"
  }
  uint8_t lo = MaestroSerial.read();
  uint8_t hi = MaestroSerial.read();
  return (((uint16_t)hi << 7) | lo) / 4; // quarter-microseconds -> microseconds
}

static inline void moveServo(uint8_t servoIndex, uint16_t pos, uint8_t speed)
{
  pos = clampServoPulse(servoIndex, pos);
  maestroSetSpeed(servoIndex, speed);
  maestroSetTarget(servoIndex, pos);
}

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
#define DATA_DOOR_CLOSE 1650

// Uncomment this if you want Debug output.
#define DEBUG

//Setup Debug Switch
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
