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
  - UART1 — Pololu Maestro Compact Protocol
  - Maestro RST (hard reset, ESP32 → Maestro) and ERR (error flag, Maestro → ESP32) lines, wired for recovery/diagnostics — see `BD:MRESET` below
  - WiFi (self-hosted AP) — wireless firmware updates, see below

Voltage monitoring and LED-matrix driving that used to live on this board
have moved to RGB-DPL, which has its own onboard voltage monitor.

## Project Structure

```
src/
  BodyController.ino           setup()/loop() only — everything else lives in its own file below
  bus.cpp                      WCB serial trunk (UART0) + sendBusCommand()
  maestro.cpp                  Pololu Maestro servo driver (Compact Protocol)
  vocalizer.cpp                HCR Vocalizer play*()/emote wrappers
  doors.cpp                    Door/utility-arm open-close toggles + their shared state
  sequences.cpp                Performance sequences (audio + panel lights + periscope + servos)
  estop.cpp                    ESTOP + waitTime() (the ESTOP-watching blocking delay)
  serial_commands.cpp          Serial command parsing + BD:COMMAND dispatch table
  web_page.cpp                 Wireless (OTA) firmware update page + WiFi AP/WebServer setup
include/
  config.h                     Pin assignments, servo positions/speeds, firmware identity (no code)
  bus.h, maestro.h, vocalizer.h, doors.h, sequences.h, estop.h,
  serial_commands.h, web_page.h    One header per src/*.cpp above
lib/
  HumanCyborgRelationsAPI/     HCR Vocalizer audio + emotion API
```

Split out of a single ~1300-line `.ino` so each concern fits on a screen and
`doCommand()`'s dispatch table isn't buried at the bottom of a huge file.
PlatformIO compiles every `.cpp` under `src/` as its own translation unit and
adds `include/` to the header search path automatically — unlike the
classic single-`.ino` Arduino sketch, cross-file calls need an explicit
prototype in a header, which is why `config.h` and friends declare things as
`extern`/function prototypes rather than defining objects directly (that
pattern broke once headers were included from more than one `.cpp`).

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
| `MRESET` | Hard-reset just the Maestro's own microcontroller (RST line) — for when it's stopped responding to serial entirely and a normal `RESET` can't reach it |
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
the WCB, forwarded to RGB-DPL (see `sendBusCommand()` in the .ino).

**`:OP<nn>` / `:CL<nn>` / `:SE<nn>`** — Marcduino-style panel open/close/sequence
commands (`nn` = panel number, see `doMarcduinoOpen()`/`doMarcduinoClose()`).

### Panel light on/off

Every door/arm function that opens or closes the CBI or Data Panel doors
sends a `sendBusCommand()` on/off pair right before opening and right after
closing — direct port of what the old `CBI_SWITCH_PIN`/`DP_SWITCH_PIN`
relays did (power the physical matrices on/off), just via RGB-DPL's serial
command instead of a GPIO. Three pairs are available, matching what RGB-DPL
actually powers:

| Pair | Scope |
|---|---|
| `ALLON` / `ALLOFF` | Every panel (CBI + Data Panel together) — the true all-the-way-off; unlike `BRIGHTNESS 0`, which only dims to a minimum, this actually blacks out |
| `CBION` / `CBIOFF` | CBI matrix only |
| `DPON` / `DPOFF` | Data Panel (LDPL, strip E) only |

`resetServos()`, `openEverything()`, `Doors()`, `openCBI_DataDoor()`, the
Marcduino "all panels" case, and the performance sequences that touch both
zones (`Scream()`, `Flutter()`, `Cantina()`, `overload()`) use `ALLON`/
`ALLOFF`. Functions that only touch one zone use the matching pair instead —
`openCBIDoor()`, the CB<val> legacy forwarding, and the Marcduino CBI case
use `CBION`/`CBIOFF`; `openDataDoor()`, DP<val> forwarding, and the Marcduino
Data Panel case use `DPON`/`DPOFF`; `heart()` (CBI-door-only thumps) uses
`CBION`/`CBIOFF`. This gives independent CBI-vs-Data-Panel control, matching
what the old two relays could do. Each performance sequence sends its
on/off pair exactly once, at the very start and very end, never inside a
repeated flap/thump loop, so rapid-fire door movement mid-sequence doesn't
spam the shared serial line.

RGB-DPL now boots dark by default on its own (its default boot profile
starts off), so this board no longer needs to defensively resend `ALLOFF`
a few seconds after power-on — the boot-time race that used to require that
workaround is resolved on the RGB-DPL side.

### ESTOP

`BD:ESTOP` (or `ESTOP` arriving mid-sequence) sets every servo's target to 0,
which tells the Maestro to stop pulsing those channels entirely — servos go
limp/unpowered rather than being held in place — then blocks forever. The
only way out is power-cycling or sending `BD:RESET`, which reboots the board via
`ESP.restart()`.

### Idle servo release

Unlike VarSpeedServo/ReelTwo, the Maestro holds a channel under power
indefinitely once it reaches a commanded position — which causes an audible
hunting/buzz noise on a servo held stationary. `moveServo()` estimates each
move's completion time (same distance/speed math the Maestro uses
internally) and `releaseIdleServos()` — pumped from both `loop()` and
`waitTime()`, since sequences spend most of their time blocked in the
latter — releases (Set Target 0) any channel that's gone quiet since. A
channel still being actively re-commanded (e.g. mid-sequence) never hits
this. Tune the settle padding via `SERVO_RELEASE_MARGIN_MS` in `config.h`.

### Maestro error reporting

The Maestro's ERR line is polled every `loop()` (cheap digital read); when it
goes high, `maestroReportErrors()` fetches and decodes the error bitmask
(Get Errors, `0xA1`) and prints it to the debug console — rate-limited to
once/second so a persistent error can't flood the console or the shared
serial line. See `maestroReportErrors()` in `config.h` for the specific
error bits decoded.

## Configuration

All tunable parameters are in [src/config.h](src/config.h):

- Servo positions (in microseconds) and Maestro Set Speed values (0–255, same scale as the old VarSpeedServo speeds — see the comment above `moveServo()` for why no conversion was needed)
- UART0/UART1 pin assignments and baud rates
- WiFi AP SSID/password for wireless updates, and firmware version/board identity shown on the update page
- I2C address and peer device addresses (kept for reference/parity with sibling boards on the bus)
- `DEBUG` conditional compile flag (debug output goes out the native USB serial console, independent of UART0/UART1)
