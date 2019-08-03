#ifndef MALENA_LOG_H
#define MALENA_LOG_H

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

enum {
	LOG_DEBUG = 0,
	LOG_INFO,
	LOG_WARN,
	LOG_ERROR,
	LOG_FATAL,
	LOG_LEVEL_NUM
};

#define LOG_OUTPUT_LEVEL 0

#define LOG(level, format, ...) do { \
	static const char *log_prefix[LOG_LEVEL_NUM] = {"DEBUG", "INFO", "WARN", "ERROR", "FATAL"}; \
	char timestr[32]; \
	time_t now = time(NULL); \
	if (level >= LOG_OUTPUT_LEVEL) { \
		strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", localtime(&now)); \
		printf("[%s] [%s] [%s:%d] ", log_prefix[level], timestr, __FILE__, __LINE__); \
		printf(format, ##__VA_ARGS__); \
		printf("\n");\
	} \
} while(0)

/* 
#define ERROR(format, ...) LOG("ERROR", format, ...)
#define INFO(format, ...) LOG("INFO", format, ...)
#define DEBUG(format, ...) LOG("DEBUG", format, ...)
#define FATAL(format, ...) LOG("FATAL", format, ...)
*/

#endif
