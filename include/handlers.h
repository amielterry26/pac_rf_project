#ifndef HANDLERS_H
#define HANDLERS_H

#include <stdio.h>       // For printf (optional, used for TERM: logs)
#include "logger.h"      // For structured logging output

/**
 * Command Handlers
 * ----------------------
 * Each handler corresponds to one command and performs its action.
 *
 * Note:
 *  - `argc` and `argv` are included for future argument support.
 *  - Current handlers are stubbed to simulate functionality.
 */

/** Handle capture command: Simulates a device capture operation. */
void handle_capture(int argc, char **argv);

/** Handle GPS command: Simulates retrieving GPS coordinates. */
void handle_gps(int argc, char **argv);

/** Handle stream start command: Simulates initializing a data stream. */
void handle_stream_start(int argc, char **argv);

/** Handle tone send command: Simulates sending a tone to the device. */
void handle_tone_send(int argc, char **argv);

#endif // HANDLERS_H
