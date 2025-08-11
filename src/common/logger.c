#include "logger.h"

// ============================================================================
// Internal Helper: Variadic Logger
// ----------------------------------------------------------------------------
// Handles printf-style formatting for all logger functions. Takes a format
// string and a variable argument list, prefixes the message with a tag,
// and prints it to stdout.
// ============================================================================

static void log_message(const char *prefix, const char *fmt, va_list args) {
    printf("%s", prefix);      // Print the log level prefix
    vprintf(fmt, args);        // Print the formatted message
    printf("\n");              // Add a newline for cleanliness
}

// ============================================================================
// Info Logger
// ----------------------------------------------------------------------------
// Used for normal operation messages and general logs.
// Example: log_info("Processed %d packets", packet_count);
// ============================================================================
void log_info(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_message("[INFO] ", fmt, args);
    va_end(args);
}

// ============================================================================
// Warning Logger
// ----------------------------------------------------------------------------
// Used for potential issues or abnormal conditions that are not fatal.
// Example: log_warning("Buffer is %d%% full", buffer_usage);
// ============================================================================
void log_warning(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_message("[WARNING] ", fmt, args);
    va_end(args);
}

// ============================================================================
// Error Logger
// ----------------------------------------------------------------------------
// Used for serious problems or errors that may halt execution.
// Example: log_error("Failed to open file: %s", filename);
// ============================================================================
void log_error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_message("[ERROR] ", fmt, args);
    va_end(args);
}

// ============================================================================
// Terminal Output Utility
// ----------------------------------------------------------------------------
// Prints output that would be considered the "main program output" rather
// than a log message. Used for showing command responses or simulation
// outputs to the user.
// ============================================================================
void term_output(const char *message) {
    printf("%s\n", message);
}
