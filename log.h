#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <sys/time.h>
#include "lock.h"

#define LOG_ROUTE "ElLog.txt"

/* Log structure used to represent the log. 
 * Shall we add more fields to it? */
typedef struct log_ {
	FILE* log_file;
	struct timeval time_created;
	lock_t* lock;
} log_t;

/* Log level specifier for writing to the log. */
typedef enum log_level_ {NONE_L, INFO_L, WARNING_L, ERROR_L, CRITICAL_L} log_level;

/* Closes the received log file and destroys the log itself.*/
void log_close();

/* Write string msg to the received log file, using the log
 * level specified for the writing. Works exactly like printf
 * meaning that you can pass msg as a formated string, and then
 * specify list of arguments. If successful, returns
 * the total of characters written. Otherwise, a negative
 * number is returned.*/
int log_write(log_level lvl, char* msg, ... );

#endif
