#include "maestro.h"
#include "config.h"

HardwareSerial MaestroSerial(1);

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

void maestroSetTarget(uint8_t channel, uint16_t pulseUs)
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

static uint16_t servoLastPos[NBR_SERVOS] = {
  LEFT_DOOR_MINPULSE, RIGHT_DOOR_MINPULSE, CBI_DOOR_MINPULSE, DATA_DOOR_MINPULSE, ARMMINPULSE, ARMMINPULSE
};
static unsigned long servoReleaseAt[NBR_SERVOS] = { 0 };
static bool servoReleased[NBR_SERVOS] = { true, true, true, true, true, true };

void moveServo(uint8_t servoIndex, uint16_t pos, uint8_t speed)
{
  pos = clampServoPulse(servoIndex, pos);
  maestroSetSpeed(servoIndex, speed);
  maestroSetTarget(servoIndex, pos);

  // Same distance/speed math the Maestro itself uses internally (see the
  // comment above the SPEED #defines for why VarSpeedServo's old units
  // carry over 1:1) -- so this is a real completion-time estimate, not
  // just a guess reused from elsewhere.
  uint16_t from = servoLastPos[servoIndex];
  uint16_t distance = (pos > from) ? (pos - from) : (from - pos);
  uint32_t duration = (speed == 0) ? 0 : (uint32_t)distance * 40UL / speed;
  servoLastPos[servoIndex] = pos;
  servoReleaseAt[servoIndex] = millis() + duration + SERVO_RELEASE_MARGIN_MS;
  servoReleased[servoIndex] = false;
}

void releaseIdleServos()
{
  unsigned long now = millis();
  for (uint8_t i = 0; i < NBR_SERVOS; i++) {
    if (!servoReleased[i] && now >= servoReleaseAt[i]) {
      maestroSetTarget(i, 0);
      servoReleased[i] = true;
    }
  }
}

void maestroHardReset()
{
  digitalWrite(MAESTRO_RST_PIN, LOW);
  delay(10);
  digitalWrite(MAESTRO_RST_PIN, HIGH);
}

// Get Errors (0x A1) — reading the bitmask also clears it on the Maestro,
// so this both fetches and acknowledges whatever tripped MAESTRO_ERR_PIN.
static inline uint16_t maestroGetErrors()
{
  MaestroSerial.write((uint8_t)0xA1);
  unsigned long start = millis();
  while (MaestroSerial.available() < 2) {
    if (millis() - start > 50) return 0; // timeout - nothing to report
  }
  uint8_t lo = MaestroSerial.read();
  uint8_t hi = MaestroSerial.read();
  return ((uint16_t)hi << 8) | lo;
}

void maestroReportErrors()
{
  uint16_t errors = maestroGetErrors();
  if (errors == 0) return;
  DEBUG_PRINT(F("Maestro error 0x"));
  DEBUG_PRINT_LN_DEC(errors, HEX);
  if (errors & 0x0001) DEBUG_PRINT_LN(F("  Serial Signal Error"));
  if (errors & 0x0002) DEBUG_PRINT_LN(F("  Serial Overrun Error"));
  if (errors & 0x0004) DEBUG_PRINT_LN(F("  Serial Buffer Full"));
  if (errors & 0x0008) DEBUG_PRINT_LN(F("  Serial CRC Error"));
  if (errors & 0x0010) DEBUG_PRINT_LN(F("  Serial Protocol Error"));
  if (errors & 0x0020) DEBUG_PRINT_LN(F("  Serial Timeout"));
  if (errors & 0x0040) DEBUG_PRINT_LN(F("  Script Stack Error"));
  if (errors & 0x0080) DEBUG_PRINT_LN(F("  Script Call Stack Error"));
  if (errors & 0x0100) DEBUG_PRINT_LN(F("  Script Program Counter Error"));
}
