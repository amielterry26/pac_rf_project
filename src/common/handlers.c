// src/common/handlers.c
//
// NOTE: This file preserves your original style/signatures and adds:
// - Real GPS handler (UART on /dev/ttyPS1, NMEA parse, TERM/LOG output)
// - Small UART helper (file-local)
// - Minimal stubs for stream/spectrum so the command table links cleanly
//
// Contracts kept:
// - Handlers use: void handle_xxx(int argc, char **argv)
// - GUI prefixes: TERM: (human summary), LOG: (raw/debug)

#include "handlers.h"
#include "logger.h"   // log_info(...)
#include "nmea.h"     // NMEA parsing (header in include/)
#include <stdio.h>    // printf
#include <string.h>   // strncpy, strerror
#include <errno.h>    // errno
#include <fcntl.h>    // open
#include <unistd.h>   // read, close, usleep
#include <termios.h>  // UART config
#include <time.h>     // time

/* ============================================================================
 *  Utility: set UART into raw mode at a given baud
 * ==========================================================================*/
static int hs_set_uart_raw(int fd, int baud) {
    struct termios tio;
    if (tcgetattr(fd, &tio) != 0) return -1;

    cfmakeraw(&tio);

    speed_t spd = B9600;
    switch (baud) {
        case 9600:   spd = B9600;   break;
        case 19200:  spd = B19200;  break;
        case 38400:  spd = B38400;  break;
        case 57600:  spd = B57600;  break;
        case 115200: spd = B115200; break;
        default:     spd = B9600;   break;
    }

    cfsetispeed(&tio, spd);
    cfsetospeed(&tio, spd);
    tio.c_cflag |= (CLOCAL | CREAD);

    // Non-blocking-ish: short timeout even if no bytes available
    tio.c_cc[VMIN]  = 0;
    tio.c_cc[VTIME] = 2; // deciseconds (0.2s)

    return tcsetattr(fd, TCSANOW, &tio);
}

/* ============================================================================
 *  GPS (REAL): UART on /dev/ttyPS1 → NMEA → TERM/LOG
 *  - Tries 9600 then 115200 baud
 *  - Reads ~2s non-blocking
 *  - Parses minimal GGA/RMC for summary
 * ==========================================================================*/
void handle_gps(int argc, char **argv) {
    (void)argc; (void)argv;

    const char *dev = "/dev/ttyPS1";
    const int   baud_try[2] = {9600, 115200};
    const long  READ_MS = 2000; // total read window

    log_info("GPS command received (device=%s).", dev);

    // Open UART
    int fd = open(dev, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        log_info("GPS open failed: dev=%s errno=%d (%s)", dev, errno, strerror(errno));
        printf("TERM: GPS ERROR — open failed (%s)\n", strerror(errno));
        return;
    }

    // Configure UART (9600 then 115200)
    int configured = 0;
    int use_baud = baud_try[0];
    for (int i = 0; i < 2; i++) {
        if (hs_set_uart_raw(fd, baud_try[i]) == 0) {
            use_baud = baud_try[i];
            configured = 1;
            break;
        }
    }
    if (!configured) {
        log_info("GPS UART config failed for %s", dev);
        printf("TERM: GPS ERROR — UART config failed\n");
        close(fd);
        return;
    }

    // Read loop (~2s), parse NMEA, keep tail for LOG
    char rbuf[512];
    char line[256]; size_t llen = 0;

    nmea_info_t info;                  // zero-initialize
    memset(&info, 0, sizeof(info));

    char last[5][128];                 // ring buffer of recent lines
    int last_idx = 0, last_cnt = 0;

    time_t t0 = time(NULL);
    while ((time(NULL) - t0) * 1000 < READ_MS) {
        ssize_t n = read(fd, rbuf, sizeof(rbuf));
        if (n <= 0) { usleep(50 * 1000); continue; } // 50ms

        for (ssize_t i = 0; i < n; i++) {
            char c = rbuf[i];
            if (c == '\r') continue;

            if (c == '\n') {
                if (llen > 0) {
                    line[llen] = '\0';

                    // Store into ring buffer for LOG
                    strncpy(last[last_idx], line, sizeof(last[0]) - 1);
                    last[last_idx][sizeof(last[0]) - 1] = '\0';
                    last_idx = (last_idx + 1) % 5;
                    if (last_cnt < 5) last_cnt++;

                    // Parse one sentence into info (best-effort, checksum-guarded)
                    nmea_parse_line(line, &info);

                    llen = 0;
                }
            } else if (llen + 1 < sizeof(line)) {
                line[llen++] = c;
            } else {
                // Overflow guard: reset line to keep parser stable
                llen = 0;
            }
        }
    }

    close(fd);

    // TERM: concise human summary for GUI
    if (info.has_fix || info.fix_quality > 0) {
        printf("TERM: GPS ok baud=%d fix=%s quality=%d sats=%d time=%s lat=%.6f lon=%.6f\n",
               use_baud, "VALID", info.fix_quality, info.sats,
               info.time_utc[0] ? info.time_utc : "unknown",
               info.lat_deg, info.lon_deg);
    } else {
        printf("TERM: GPS no-fix baud=%d quality=%d sats=%d time=%s (likely indoors)\n",
               use_baud, info.fix_quality, info.sats,
               info.time_utc[0] ? info.time_utc : "unknown");
    }

    // LOG: tail of raw NMEA for debugging/traceability
    for (int i = 0; i < last_cnt; i++) {
        int idx = (last_idx + i) % 5;
        printf("LOG: %s\n", last[idx]);
    }
}

/* ============================================================================
 *  Capture (stub for now) — keeps GUI contract
 * ==========================================================================*/
void handle_capture(int argc, char **argv) {
    (void)argc; (void)argv;
    log_info("Capture command received (stub).");
    printf("LOG: Capture request received (stub)\n");
    printf("TERM: Simulated capture complete. (stub)\n");
}

/* ============================================================================
 *  Tone send (stub for now) — keeps GUI contract
 * ==========================================================================*/
void handle_tone_send(int argc, char **argv) {
    (void)argc; (void)argv;
    log_info("Tone send command received (stub).");
    printf("LOG: Tone send request received (stub)\n");
    printf("TERM: Simulated tone transmitted. (stub)\n");
}

/* ============================================================================
 *  Stream / Spectrum stubs — satisfy command table & GUI today
 *  (Replace with real implementations in Step 2.3)
 * ==========================================================================*/
void handle_stream_start(int argc, char **argv) {
    (void)argc; (void)argv;
    log_info("Stream start (stub).");
    printf("LOG: Stream start requested (stub)\n");
    printf("TERM: Stream started (stub)\n");
}

void handle_stream_stop(int argc, char **argv) {
    (void)argc; (void)argv;
    log_info("Stream stop (stub).");
    printf("LOG: Stream stop requested (stub)\n");
    printf("TERM: Stream stopped (stub)\n");
}

void handle_spectrum_start(int argc, char **argv) {
    (void)argc; (void)argv;
    log_info("Spectrum start (stub).");
    printf("LOG: Spectrum start requested (stub)\n");
    printf("TERM: Spectrum started (stub)\n");
}

void handle_spectrum_stop(int argc, char **argv) {
    (void)argc; (void)argv;
    log_info("Spectrum stop (stub).");
    printf("LOG: Spectrum stop requested (stub)\n");
    printf("TERM: Spectrum stopped (stub)\n");
}
