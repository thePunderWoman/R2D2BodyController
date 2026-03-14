# Body Controller

Arduino firmware for the R2-D2 body expander. Controls servo-driven doors and utility arms, LED panel animations, audio/vocalizer integration, and battery monitoring — all coordinated over I2C and serial.

## Hardware

- **Microcontroller:** Arduino Pro Mini (16MHz, ATmega328P)
- **Servos (6):** Left door, right door, top utility arm, bottom utility arm, CBI door, data panel door
- **LED Drivers:** 2× MAX7219/MAX7221 chips (SPI) driving the Charge Bay Interface (CBI) and Data Panel displays
- **Audio:** HCR Vocalizer (external Teensy 4.1 + Audio Shield) via serial
- **Power:** 24V system with analog voltage divider on A3 for battery monitoring
- **Switches:** CBI on/off (pin 8), Data Panel on/off (pin 9), Volt Meter on/off (pin 14)
- **Communication:** I2C (address 9) + hardware serial at 9600 baud

## Project Structure

```
src/
  CMB_Body_Expander_1_7.ino   Main firmware
  config.h                     Pin assignments, servo positions/speeds, timing constants
lib/
  VarSpeedServo/               Variable-speed async servo control
  LedControl/                  MAX7219/MAX7221 LED matrix driver
  HumanCyborgRelationsAPI/     HCR Vocalizer audio + emotion API
```

## Building

This project uses [PlatformIO](https://platformio.org/). Target board is `pro16MHzatmega328`.

```bash
pio run          # build
pio run -t upload  # upload to board
```

## Serial Command Interface

Commands are sent over serial in the format `BD:COMMAND\n` at 9600 baud.

| Command | Description |
|---|---|
| `RESET` | Close all doors/arms, reset vocalizer |
| `OPENALL` | Open all doors and utility arms |
| `DOORS` | Toggle all four doors |
| `UARMS` | Toggle both utility arms |
| `TOPARM` / `BOTARM` | Toggle individual utility arms |
| `LDOOR` / `RDOOR` | Toggle left/right front doors |
| `CBIDOOR` / `DATADOOR` | Toggle CBI / Data Panel doors |
| `CBIDATADOOR` | Toggle CBI and Data Panel doors together |
| `LEIA` | Play Leia message with full body sequence |
| `VADER` | Play Imperial March sequence |
| `THEME` | Play Star Wars theme sequence |
| `CANTINA` | Play Cantina Band sequence |
| `SCREAM` | Dramatic scream sequence (servos + lights + audio) |
| `HELLO` | Play hello audio |
| `HAPPY` / `HAPPY2` | Happy emotion (moderate / strong) |
| `SAD` / `SAD2` | Sad emotion |
| `MAD` / `MAD2` | Mad emotion |
| `SCARED` / `SCARED2` | Scared emotion |
| `OVERLOAD` | Overload sequence |
| `VOL` | Cycle volume (High → Low → Mute) |

## Configuration

All tunable parameters are in [src/config.h](src/config.h):

- Servo pin assignments and open/close positions (in microseconds)
- Servo movement speeds (0–255)
- LED animation timing constants
- Battery voltage thresholds (green/yellow/red)
- I2C address and peer device addresses
- HCR Vocalizer volume levels per channel
- Conditional compile flags: `DEBUG`, `TEST`, `LEGACY`, `monitorVCC`, `BLUELEDTRACKGRAPH`
