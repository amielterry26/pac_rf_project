#ifndef COMMANDS_H
#define COMMANDS_H

#include <stddef.h>      // For size_t
#include "handlers.h"    // Command handler prototypes
#include "logger.h"      // For logging

/**
 * Command
 * ----------------------
 * Represents a single command entry in the dispatch table.
 *
 * @param name        The string that triggers the command (e.g., "--capture")
 * @param execute     Pointer to the handler function for this command
 * @param description A human-readable description for help/usage output
 */
typedef struct {
    const char *name;
    void (*execute)(int argc, char **argv);
    const char *description;
} Command;

/**
 * commands[]
 * ----------------------
 * Global command table mapping CLI/GUI commands to handler functions.
 */
extern Command commands[];

/**
 * num_commands
 * ----------------------
 * Number of available commands in the table.
 */
extern int num_commands;

/**
 * dispatch_command
 * ----------------------
 * Looks up and executes a command based on its string.
 *
 * @param cmd   The command string (e.g., "--gps")
 * @param argc  Argument count (for future flexibility)
 * @param argv  Argument vector (for future flexibility)
 */
void dispatch_command(const char *cmd, int argc, char **argv);

/**
 * print_usage
 * ----------------------
 * Prints a dynamically generated usage/help menu based on commands[].
 */
void print_usage(void);

#endif // COMMANDS_H
