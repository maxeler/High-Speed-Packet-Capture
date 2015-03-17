/*
 * log.h
 *
 */

#ifndef LOG_H_
#define LOG_H_

static const char* g_log_prepend = "";
static int g_log_level = 0;

static const int LOG_LEVEL_TRACE = 3;
static const int LOG_LEVEL_DEBUG = 2;
static const int LOG_LEVEL_INFO = 1;

#define logf(level, format, ...) if( g_log_level >= (level) ) printf("%s: "format, g_log_prepend, __VA_ARGS__)
#define log(level, format) if( g_log_level >= (level) ) printf("%s: "format, g_log_prepend)
#define log_info(format) log(LOG_LEVEL_INFO, format)
#define logf_info(format, ...) logf(LOG_LEVEL_INFO, format, __VA_ARGS__)
#define log_debug(format) log(LOG_LEVEL_DEBUG, format)
#define logf_debug(format, ...) logf(LOG_LEVEL_DEBUG, format, __VA_ARGS__)
#define log_trace(format) log(LOG_LEVEL_TRACE, format)
#define logf_trace(format, ...) logf(LOG_LEVEL_TRACE, format, __VA_ARGS__)

#endif /* LOG_H_ */
