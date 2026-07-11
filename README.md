# Body Controller

ESP32 firmware for the R2-D2 body controller. Runs the servo-driven doors and
utility arm sequences and the audio/vocalizer integration, and acts as a thin
serial router between the WCB, a Pololu Mini Maestro 12 (servos), and the
RGB-DPL panel controller (lights) — both of which are separate boards this
firmware talks to over serial rather than drives directly.

## Hardware

- **Microcontroller:** ESP32-C3 SuperMini (`nologo_esp32c3_super_mini`)
- **Servos (6):** Left door, right door, top utility arm, bottom utility arm, CBI door, data panel door — driven by a Pololu Mini Maestro 12 over UART1 (Compact Protocol), not by this board's own GPIOs
- **Lights:** [RGB-DPL](https://github.com/thePunderWoman/RGB-DPL-Firmware), a separate ESP32-S3 board driving WS2811/WS2812 panels — controlled via plain-text serial commands forwarded over UART0
- **Audio:** HCR Vocalizer (external Teensy 4.1 + Audio Shield) via serial, sharing UART0
- **Communication:**
  - UART0 — WCB commands in, HCR vocalizer commands out, RGB-DPL panel commands out (one shared trunk)
  - UART1 — Pololu Maestro only
  - WiFi (self-hosted AP) — wireless firmware updates, see below

Voltage monitoring and LED-matrix driving that used to live on this board
have moved to RGB-DPL, which has its own onboard voltage monitor.

## Project Structure

```
src/
  BodyController.ino           Main firmware — setup/loop, sequences, serial command dispatch
  config.h                     Pin assignments, servo positions/speeds, Maestro driver, firmware identity
  web_page.h                   Wireless (OTA) firmware update page + WiFi AP/WebServer setup
lib/
  HumanCyborgRelationsAPI/     HCR Vocalizer audio + emotion API
```

## Building

This project uses [PlatformIO](https://platformio.org/). Target environment is `esp32-c3-supermini`.

```bash
pio run              # build
pio run -t upload    # upload over USB
```

The first flash after any partition-table change (e.g. pulling this repo
fresh) must be over USB. After that, firmware can be updated wirelessly:
connect to the board's WiFi AP (SSID/password in `config.h`) and browse to
`http://bodycontroller.local/`.

## Serial Command Interface

Three command styles arrive on UART0, all originating from the WCB:

**`BD:COMMAND\n`** — the main command set:

| Command | Description |
|---|---|
| `ESTOP` | Release every servo (goes limp/unpowered) and block forever (see below) |
| `WIFI` / `WIFI0` / `WIFI1` | Toggle / disable / enable the WiFi AP + OTA update server — starts on at boot, turn off to cut RF noise during a show. Matches the `#APWIFI[0\|1]` / `#PWIFI[0\|1]` convention on AstroPixelsPlus and the Periscope |
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
| `ROCKMARCH` | Play rock march audio |
| `THEME` | Play Star Wars theme sequence |
| `BATTLEALARM` | Play battle alarm audio |
| `CLONES` | Play Clones audio |
| `DUEL` | Play duel audio |
| `LUKEJABBA` | Play Luke/Jabba audio |
| `THRONE` | Play throne room audio |
| `CANTINA` | Play Cantina Band sequence |
| `SCREAM` | Dramatic scream sequence (servos + panel lights + audio) |
| `HELLO` | Play hello audio |
| `ALARM` | Play Imperial alarm audio |
| `HEART` | Heartbeat sequence (CBI door thumps + panel lights + audio) |
| `DISCO` | Play disco audio |
| `FLUTTER` | Door flutter wave sequence |
| `OVERLOAD` | Overload/glitch sequence |

**`CB<val>` / `DP<val>`** — legacy Marcduino-style panel light commands from
the WCB, forwarded to RGB-DPL (see `sendPanelCommand()` in the .ino).

**`:OP<nn>` / `:CL<nn>` / `:SE<nn>`** — Marcduino-style panel open/close/sequence
commands (`nn` = panel number, see `doMarcduinoOpen()`/`doMarcduinoClose()`).

### ESTOP

`BD:ESTOP` (or `ESTOP` arriving mid-sequence) sets every servo's target to 0,
which tells the Maestro to stop pulsing those channels entirely — servos go
limp/unpowered rather than being held in place — then blocks forever. The
only way out is power-cycling or sending `BD:RESET`, which reboots the board via
`ESP.restart()`.

## Configuration

All tunable parameters are in [src/config.h](src/config.h):

- Servo positions (in microseconds) and Maestro Set Speed values (0–255, same scale as the old VarSpeedServo speeds — see the comment above `moveServo()` for why no conversion was needed)
- UART0/UART1 pin assignments and baud rates
- WiFi AP SSID/password for wireless updates, and firmware version/board identity shown on the update page
- I2C address and peer device addresses (kept for reference/parity with sibling boards on the bus)
- `DEBUG` conditional compile flag (debug output goes out the native USB serial console, independent of UART0/UART1)
