#ifndef EVENT_LOGGER_H
#define EVENT_LOGGER_H

#define LOG_LEVEL_ERROR   0
#define LOG_LEVEL_WARN    1
#define LOG_LEVEL_INFO    2
#define LOG_LEVEL_DEBUG   3
#define LOG_LEVEL_TRACE   4

#ifdef LOG_LEVEL

#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#include <time.h>

// Configuration
#define USE_TIMESTAMP 1
#if USE_TIMESTAMP
extern time_t __logger_time;
#define _TS_FMT "[%02d:%02d:%02d] "
#define _TS_ARGS localtime(&__logger_time)->tm_hour, localtime(&__logger_time)->tm_min, localtime(&__logger_time)->tm_sec
#else
#define _TS ""
#define _TS_ARGS
#endif

#define __LOG(fmt, level, ...) do { \
    __logger_time = time(NULL); \
    fprintf(stderr, _TS_FMT level fmt "\n", _TS_ARGS, ##__VA_ARGS__); \
    } while(0)

#if LOG_LEVEL >= LOG_LEVEL_ERROR
#define ERROR_LOG(fmt, ...) __LOG(fmt, "[ERROR] ", ##__VA_ARGS__)
#else
#define ERROR_LOG(fmt, ...) ((void)0)
#endif

#if LOG_LEVEL >= LOG_LEVEL_WARN
#define WARN_LOG(fmt, ...)  __LOG(fmt, "[WARN] ", ##__VA_ARGS__)
#else
#define WARN_LOG(fmt, ...) ((void)0)
#endif

#if LOG_LEVEL >= LOG_LEVEL_INFO
#define INFO_LOG(fmt, ...)  __LOG(fmt, "[INFO] ", ##__VA_ARGS__)
#else
#define INFO_LOG(fmt, ...) ((void)0)
#endif

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
#define DEBUG_LOG(fmt, ...) __LOG(fmt, "[DEBUG] ", ##__VA_ARGS__)
#else
#define DEBUG_LOG(fmt, ...) ((void)0)
#endif

#if LOG_LEVEL >= LOG_LEVEL_TRACE
#define TRACE_LOG(fmt, ...) __LOG(fmt, "[TRACE] ", ##__VA_ARGS__)
#else
#define TRACE_LOG(fmt, ...) ((void)0)
#endif

#ifdef __cplusplus
} // Extern "C"
#endif // __cplusplus

#endif // LOG_LEVEL

#endif // EVENT_LOGGER_H