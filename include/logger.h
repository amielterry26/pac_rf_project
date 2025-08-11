#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <stdarg.h>

// ============================================================================
// Logger Module
// ----------------------------------------------------------------------------
// Provides simple, formatted logging functions for informational messages
// and warnings. All functions support printf-style formatting using
// variable argument lists.
// ============================================================================

// Logs an informational message (printf-style)
void log_info(const char *fmt, ...);

// Logs a warning message (printf-style)
void log_warning(const char *fmt, ...);

// Logs an error message (printf-style)
void log_error(const char *fmt, ...);

// Utility to print output directly to a "terminal" (simulated console output)
void term_output(const char *message);

#endif // LOGGER_H
