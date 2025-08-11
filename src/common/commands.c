#include <stdio.h>
#include <string.h>       // For strcmp
#include "commands.h"
#include "handlers.h"
#include "logger.h"

/**
 * Global command table
 * ----------------------
 * Map all available commands to their corresponding handlers.
 * ✅ Adding a new command only requires adding an entry here.
 * ✅ Each command has a description for the dynamic help menu.
 */
Command commands[] = {
    { "--capture",      handle_capture,      "Simulate or trigger a capture sequence" },
    { "--gps",          handle_gps,          "Retrieve GPS coordinates" },
    { "--stream-start", handle_stream_start, "Start simulated streaming" },
    { "--tone-send",    handle_tone_send,    "Send a test tone" },
    { "--help",         NULL,                "Show this help menu" }  // ✅ Built-in help command
};

/**
 * Global command count (auto-calculated)
 */
int num_commands = sizeof(commands) / sizeof(Command);

/**
 * dispatch_command
 * ----------------------
 * Finds and executes a command from the table.
 * - Logs the dispatch activity
 * - Calls the command handler (if any)
 * - Handles the built-in --help command
 * - Shows usage if the command is unknown
 */
void dispatch_command(const char *cmd, int argc, char **argv) {
    if (!cmd || strlen(cmd) == 0) {
        log_warning("No command provided.");
        print_usage();
        return;
    }

    for (int i = 0; i < num_commands; i++) {
        if (strcmp(cmd, commands[i].name) == 0) {
            // ✅ Handle the built-in --help command
            if (strcmp(cmd, "--help") == 0) {
                print_usage();
                return;
            }

            log_info("Dispatching command: %s", cmd);
            commands[i].execute(argc, argv);
            return;
        }
    }

    // ❌ Unknown command
    log_warning("Unknown command received: %s", cmd);
    print_usage();
}

/**
 * print_usage
 * ----------------------
 * Prints a dynamic help menu using the command table.
 * Automatically lists all available commands and their descriptions.
 */
void print_usage(void) {
    printf("\nPAC-RF Application Usage:\n");
    printf("  ./pac_rf_exec <command> [options]\n\n");
    printf("Available commands:\n");

    for (int i = 0; i < num_commands; i++) {
        printf("  %-15s - %s\n", commands[i].name, commands[i].description);
    }

    printf("\nExamples:\n");
    printf("  ./pac_rf_exec --gps\n");
    printf("  ./pac_rf_exec --capture --bitwidth 8\n\n");
}
