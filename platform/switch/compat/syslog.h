#pragma once

#include <stdarg.h>

// No syslog daemon on Horizon: log calls compile to nothing (stdout goes through nxlink instead).
#define LOG_EMERG 0
#define LOG_ALERT 1
#define LOG_CRIT 2
#define LOG_ERR 3
#define LOG_WARNING 4
#define LOG_NOTICE 5
#define LOG_INFO 6
#define LOG_DEBUG 7
#define LOG_PID 0x01
#define LOG_CONS 0x02
#define LOG_USER (1 << 3)

static inline void openlog(const char *, int, int) {}
static inline void closelog(void) {}
static inline void syslog(int, const char *, ...) {}
static inline void vsyslog(int, const char *, va_list) {}
