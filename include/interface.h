#ifndef INTERFACE_H
#define INTERFACE_H

// Stream a PAC-RF command like "--gps" and print lines to stdout.
// Good for CLI runs.
int run_pacrf_cmd(const char *args);

// Stream a PAC-RF command and deliver each output line to a callback.
// Good for GUI runs. The callback is called from the same thread that
// calls run_pacrf_cmd_cb(); if you need to touch GTK widgets, marshal
// to the GTK main thread (see main_gui.c example).
typedef void (*pacrf_line_cb)(const char *line, void *user);
int run_pacrf_cmd_cb(const char *args, pacrf_line_cb on_line, void *user);

#endif
