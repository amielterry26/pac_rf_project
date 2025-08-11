#include <stdio.h>
#include <string.h>
#include <stdlib.h>        // For exit codes
#include "commands.h"      // Command table and dispatch
#include "handlers.h"      // Command handler implementations
#include "logger.h"        // Logging functionality
#include "queue_manager.h" // Queue management
#include "bit_parser.h"    // Bit parsing utilities
#include "interface.h"     // Unified command interface

// ============================================================
//  Main Application Entry Point
// ------------------------------------------------------------
//  1. Initializes logging and queue system
//  2. Processes command-line arguments
//  3. Dispatches commands via the interface
//  4. Cleans up and exits
// ============================================================

// Global queue for demonstration (optional; can expand later)
static Queue main_queue;

int main(int argc, char *argv[]) {
    // -------------------------------
    // 1. Initialize Logger
    // -------------------------------
    log_info("PAC-RF Application Starting...");

    // -------------------------------
    // 2. Initialize Queue System
    // -------------------------------
    if (!queue_init(&main_queue, 10)) {
        log_error("Failed to initialize queue. Exiting.");
        return EXIT_FAILURE;
    }

    // -------------------------------
    // 3. Handle Command-Line Arguments
    // -------------------------------
    if (argc < 2) {
        log_warning("No command provided.");
        print_usage();  // ✅ Global dynamic usage from commands.c
        queue_destroy(&main_queue);
        return EXIT_FAILURE;
    }

    const char *cmd = argv[1];

    // -------------------------------
    // 4. Dispatch Command
    // -------------------------------
    log_info("Dispatching command...");
    dispatch_command(cmd, argc - 1, &argv[1]);  // Pass remaining args if needed

    // -------------------------------
    // 5. Optional Queue Operations (Stub for Demonstration)
    // -------------------------------
    QueueItem item = { "SampleData", 10 };
    if (queue_enqueue(&main_queue, &item)) {
        queue_log_status(&main_queue);
    }

    QueueItem out_item;
    if (queue_dequeue(&main_queue, &out_item)) {
        log_info("Dequeued item successfully.");
        queue_log_status(&main_queue);
    }

    // -------------------------------
    // 6. Cleanup Before Exit
    // -------------------------------
    queue_destroy(&main_queue);
    log_info("PAC-RF Application Exiting Cleanly.");
    return EXIT_SUCCESS;
}
