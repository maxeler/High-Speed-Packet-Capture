/*
 * log.h
 */

#ifndef LOG_H_
#define LOG_H_

extern const char* g_log_prepend;
extern int g_log_level;

#define LOG_LEVEL_TRACE  3
#define LOG_LEVEL_DEBUG  2
#define LOG_LEVEL_INFO   1
#define LOG_LEVEL_NONE   0
#define LOG_LEVEL_MAX    LOG_LEVEL_TRACE
#define LOG_LEVEL_MIN    LOG_LEVEL_NONE

#define logf(level, format, ...) if( g_log_level >= (level) ) printf("%s: "format, g_log_prepend, __VA_ARGS__)
#define log(level, format) if( g_log_level >= (level) ) printf("%s: "format, g_log_prepend)
#define log_info(format) log(LOG_LEVEL_INFO, format)
#define logf_info(format, ...) logf(LOG_LEVEL_INFO, format, __VA_ARGS__)
#define log_debug(format) log(LOG_LEVEL_DEBUG, format)
#define logf_debug(format, ...) logf(LOG_LEVEL_DEBUG, format, __VA_ARGS__)
#define log_trace(format) log(LOG_LEVEL_TRACE, format)
#define logf_trace(format, ...) logf(LOG_LEVEL_TRACE, format, __VA_ARGS__)

#define log_level_active(level) ((level) == g_log_level)
#define log_level_set(level) g_log_level = (level)
#define log_prepend_set(message) g_log_prepend = (message)
#define log_level_valid(level) ((level) >= LOG_LEVEL_MIN && (level) <= LOG_LEVEL_MAX)

int log_level_from_str( const char* str );

#endif /* LOG_H_ */
