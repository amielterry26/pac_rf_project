#ifndef NMEA_H
#define NMEA_H

#include <stdbool.h>

typedef struct {
    bool   has_fix;      // true if fix valid (RMC:A or GGA quality>0)
    int    fix_quality;  // 0=no fix, 1=GPS, 2=DGPS, etc. (from GGA)
    int    sats;         // satellites in use (from GGA)
    double lat_deg;      // decimal degrees (+N/-S)
    double lon_deg;      // decimal degrees (+E/-W)
    char   time_utc[16]; // "HHMMSS.sss" if present
} nmea_info_t;

/**
 * Parse a single NMEA sentence (e.g., "$GPGGA,...*CS").
 * Returns true if checksum is OK; fills fields opportunistically.
 */
bool nmea_parse_line(const char *line, nmea_info_t *out);

#endif
