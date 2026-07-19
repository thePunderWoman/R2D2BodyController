# Libraries

Most third-party libraries are pulled automatically by PlatformIO via
`lib_deps` in `platformio.ini` (including HumanCyborgRelationsAPI, pinned to
a tagged release of our fork: https://github.com/thePunderWoman/HumanCyborgRelationsAPI).

Any library not listed there needs to be cloned or downloaded into this
`lib/` directory before building.

| Library | Purpose | Source |
|---|---|---|
| LedControl | MAX7219/MAX7221 LED matrix driver | https://github.com/wayoda/LedControl |
| VarSpeedServo | Variable-speed servo control (originally Michael Margolis) | https://github.com/pvanallen/VarSpeedServo |
