/*
 * interface.c — Remote execution interface for PAC-RF commands
 *
 * Purpose:
 *   - Build a robust SSH command (using your ~/.ssh/config alias "pacrf" by default)
 *   - Stream BOTH stdout and stderr back to the GUI (critical fix: `2>&1`)
 *   - Support either:
 *        a) direct passthrough to stdout (CLI usage), or
 *        b) per-line callback into the GUI (GUI usage)
 *
 * Key changes in this update:
 *   1) STDERR CAPTURED: We now append `2>&1` so SSH/remote errors appear in the GUI Logs pane.
 *   2) Resilient SSH options: BatchMode, ConnectTimeout, ServerAlive* to avoid silent hangs.
 *   3) Safe line-by-line streaming with explicit flushing so GUI updates are snappy.
 *
 * Environment overrides (optional for onboarding):
 *   - PACRF_REMOTE_HOST  (default: "pacrf")
 *   - PACRF_REMOTE_USER  (default: "root")
 *   - PACRF_REMOTE_PATH  (default: "/root/pac_rf_project/bin/pac_rf_exec")
 *   - PACRF_SSH_KEY      (optional: explicit identity file; otherwise rely on SSH config)
 *
 * Expected behavior:
 *   - Backend handlers emit lines prefixed with "TERM:", "LOG:", or "IMG:"
 *   - The GUI router places each line into the correct pane; unknown lines fall back to Logs.
 */

#include "interface.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* ----------------------------------------------------------------------------
 * Internal helpers
 * --------------------------------------------------------------------------*/

/**
 * build_command
 * Compose the SSH command that will run the remote PAC-RF binary with args.
 * We ALWAYS append "2>&1" so stderr is merged into stdout (and visible to GUI).
 */
static void build_command(char *out, size_t outsz, const char *args) {
    /* Pull overrides from environment if set; otherwise use sane defaults. */
    const char *host = getenv("PACRF_REMOTE_HOST");  /* ~/.ssh/config alias is preferred */
    const char *user = getenv("PACRF_REMOTE_USER");  /* default "root" */
    const char *path = getenv("PACRF_REMOTE_PATH");  /* remote binary path */
    const char *key  = getenv("PACRF_SSH_KEY");      /* optional explicit key */

    if (!host || !*host) host = "pacrf"; /* You already configured this alias via ProxyJump dadvm */
    if (!user || !*user) user = "root";
    if (!path || !*path) path = "/root/pac_rf_project/bin/pac_rf_exec";

    /* Compose remote program invocation. */
    char remote_cmd[1024];
    if (args && *args) {
        /* NOTE: args should already be safe flags like "--gps" or "--capture ..." */
        snprintf(remote_cmd, sizeof(remote_cmd), "%s %s", path, args);
    } else {
        snprintf(remote_cmd, sizeof(remote_cmd), "%s", path);
    }

    /* SSH options for resilience on flaky networks. */
    const char *ssh_common_opts =
        "-o BatchMode=yes "
        "-o ConnectTimeout=10 "
        "-o ServerAliveInterval=5 "
        "-o ServerAliveCountMax=2 "
        "-o StrictHostKeyChecking=accept-new";

    if (key && *key) {
        /* Explicit identity file path */
        snprintf(
            out, outsz,
            "ssh %s -i '%s' %s@%s '%s' 2>&1",
            ssh_common_opts, key, user, host, remote_cmd
        );
    } else {
        /* Rely on ~/.ssh/config (recommended) */
        snprintf(
            out, outsz,
            "ssh %s %s@%s '%s' 2>&1",
            ssh_common_opts, user, host, remote_cmd
        );
    }
}

/**
 * run_cmd_internal
 * Execute the composed SSH command and stream output.
 *
 * If passthrough_stdout == 1:
 *   - Print every line to stdout (used by CLI mode)
 *
 * If on_line != NULL:
 *   - Invoke callback for every line (used by GUI mode)
 *
 * Returns process exit status (0 on success).
 */
static int run_cmd_internal(const char *args,
                            pacrf_line_cb on_line,
                            void *user,
                            int passthrough_stdout) {
    char cmd[2048];
    build_command(cmd, sizeof(cmd), args);

    /* Optional: surface the command itself for debugging. */
    if (passthrough_stdout) {
        /* Printed with LOG: so it lands in the GUI Logs pane when CLI is piped through GUI. */
        printf("LOG: Executing remote: %s\n", cmd);
        fflush(stdout);
    }

    FILE *fp = popen(cmd, "r");
    if (!fp) {
        const char *msg = strerror(errno);
        if (passthrough_stdout) {
            printf("TERM: ERROR — failed to start subprocess (%s)\n", msg);
            fflush(stdout);
        }
        if (on_line) {
            char buf[256];
            snprintf(buf, sizeof(buf), "LOG: ERROR — failed to start subprocess (%s)", msg);
            on_line(buf, user);
        }
        /* -1 indicates popen() failed; keep consistent with non-zero error semantics. */
        return -1;
    }

    /* Read stdout (merged with stderr) line by line and forward it. */
    char line[4096];
    while (fgets(line, sizeof(line), fp)) {
        if (passthrough_stdout) {
            /* Forward exactly as produced by the backend (already prefixed). */
            fputs(line, stdout);
            fflush(stdout);
        }
        if (on_line) {
            /* Send to GUI router; it will place by prefix or fall back to Logs. */
            on_line(line, user);
        }
    }

    int status = pclose(fp);
    if (status == -1) {
        const char *msg = strerror(errno);
        if (passthrough_stdout) {
            printf("TERM: ERROR — command close failed (%s)\n", msg);
            fflush(stdout);
        }
        if (on_line) {
            char buf[256];
            snprintf(buf, sizeof(buf), "LOG: ERROR — command close failed (%s)", msg);
            on_line(buf, user);
        }
        return -1;
    }

    /* Non-zero exit code still returns any emitted lines (now visible due to 2>&1). */
    if (status != 0) {
        if (passthrough_stdout) {
            printf("LOG: Subprocess exited with status %d\n", status);
            fflush(stdout);
        }
        if (on_line) {
            char buf[128];
            snprintf(buf, sizeof(buf), "LOG: Subprocess exited with status %d", status);
            on_line(buf, user);
        }
    }

    return status;
}

/* ----------------------------------------------------------------------------
 * Public API
 * --------------------------------------------------------------------------*/

int run_pacrf_cmd(const char *args) {
    /* CLI mode: pass lines straight to stdout (already prefixed) */
    return run_cmd_internal(args, NULL, NULL, /*passthrough_stdout=*/1);
}

int run_pacrf_cmd_cb(const char *args, pacrf_line_cb on_line, void *user) {
    /* GUI mode: route each line back via callback */
    return run_cmd_internal(args, on_line, user, /*passthrough_stdout=*/0);
}
