#include "nmea.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static unsigned char hex2uc(char c){
    if(c>='0'&&c<='9')return (unsigned char)(c-'0');
    if(c>='A'&&c<='F')return (unsigned char)(c-'A'+10);
    if(c>='a'&&c<='f')return (unsigned char)(c-'a'+10);
    return 0xFF;
}

static int checksum_ok(const char *line){
    if(!line||*line!='$')return 0;
    unsigned char cs=0; const char *p=line+1;
    while(*p&&*p!='*'){ cs^=(unsigned char)(*p); p++; }
    if(*p!='*') return 0;
    unsigned char hi=hex2uc(*(p+1)), lo=hex2uc(*(p+2));
    if(hi==0xFF||lo==0xFF) return 0;
    return cs==((hi<<4)|lo);
}

static double nmea_deg_to_decimal(const char *ddmm,const char *hem){
    if(!ddmm||!*ddmm) return 0.0;
    const char *dot=strchr(ddmm,'.');
    int deg_len=(int)((dot?dot:ddmm+strlen(ddmm))-ddmm)-2;
    if(deg_len<=0||deg_len>3) return 0.0;
    char d[4]={0}; strncpy(d,ddmm,(size_t)deg_len);
    double deg=atof(d), min=atof(ddmm+deg_len);
    double val=deg+(min/60.0);
    if(hem&&(*hem=='S'||*hem=='W')) val=-val;
    return val;
}

bool nmea_parse_line(const char *line, nmea_info_t *out){
    if(!line||!out) return false;
    if(!checksum_ok(line)) return false;

    char buf[256]; strncpy(buf,line,sizeof(buf)-1); buf[sizeof(buf)-1]='\0';
    char *p=buf; if(*p=='$') p++; char *star=strchr(p,'*'); if(star)*star='\0';

    if(!strncmp(p,"GPGGA",5)||!strncmp(p,"GNGGA",5)){
        char *f[16]={0}; int n=0;
        for(char *t=strtok(p,","); t&&n<16; t=strtok(NULL,",")) f[n++]=t;
        if(n>1 && f[1] && *f[1]){ strncpy(out->time_utc,f[1],sizeof(out->time_utc)-1); out->time_utc[15]='\0'; }
        if(n>6 && f[6] && *f[6]){ out->fix_quality=atoi(f[6]); out->has_fix=(out->fix_quality>0); }
        if(n>7 && f[7] && *f[7]) out->sats=atoi(f[7]);
        if(n>5 && f[2]&&f[3]&&f[4]&&f[5]){
            out->lat_deg=nmea_deg_to_decimal(f[2],f[3]);
            out->lon_deg=nmea_deg_to_decimal(f[4],f[5]);
        }
        return true;
    }
    if(!strncmp(p,"GPRMC",5)||!strncmp(p,"GNRMC",5)){
        char *f[16]={0}; int n=0;
        for(char *t=strtok(p,","); t&&n<16; t=strtok(NULL,",")) f[n++]=t;
        if(n>1 && f[1] && *f[1]){ strncpy(out->time_utc,f[1],sizeof(out->time_utc)-1); out->time_utc[15]='\0'; }
        if(n>2 && f[2] && *f[2]){ if(*f[2]=='A') out->has_fix=true; } // A=valid, V=void
        if(n>6 && f[3]&&f[4]&&f[5]&&f[6]){
            out->lat_deg=nmea_deg_to_decimal(f[3],f[4]);
            out->lon_deg=nmea_deg_to_decimal(f[5],f[6]);
        }
        return true;
    }
    // GSA/GSV are ignored for summary; still return true on checksum OK
    return true;
}
