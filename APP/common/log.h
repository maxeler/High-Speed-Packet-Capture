/*
 * log.h
 *
 */

#ifndef LOG_H_
#define LOG_H_

static const char* g_log_ip;
static int g_log_level;

#define logf(level, format, ...) if( g_log_level >= level ) printf("%s: "format, g_log_ip, __VA_ARGS__)
#define log(level, format) if( g_log_level >= level ) printf("%s: "format, g_log_ip)
#define log_info(format) log(1, format)
#define logf_info(format, ...) logf(1, format, __VA_ARGS__)
#define log_debug(format) log(2, format)
#define logf_debug(format, ...) logf(2, format, __VA_ARGS__)
#define log_trace(format) log(3, format)
#define logf_trace(format, ...) logf(3, format, __VA_ARGS__)

#endif /* LOG_H_ */
