// serial_commands.h — WCB serial command parsing and dispatch.
#pragma once

// Reads and dispatches all three inbound command styles: BD:COMMAND (see
// doCommand()), CB<val>/DP<val> (legacy panel light forwarding), and
// :OP<nn>/:CL<nn>/:SE<nn> (Marcduino-style).
void readSerial();

// BD:COMMAND dispatch — WIFI is handled directly (variable suffix); every
// other command is a name->function lookup in a static table (see
// serial_commands.cpp).
void doCommand(const char* cmd);
