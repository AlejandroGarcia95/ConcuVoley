#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <sys/time.h>

/* Log structure used to represent the log. 
 * Shall we add more fields to it? */
 // TODO: Add a lock for writing in the lock
typedef struct log_ {
	FILE* log_file;
	struct timeval time_created;
} log_t;

/* Log level specifier for writing to the log. */
typedef enum log_level_ {NONE_L, INFO_L, WARNING_L, ERROR_L, CRITICAL_L} log_level;

/* Opens file at route and dynamically creates a log with it. 
 * If append is false, then the file is overwritten. Returns 
 * NULL if creating the log failed. The new field app_started
 * must be a time_t indicating when did the program started, 
 * and is used for timestamps in the log. */
log_t* log_open(char* route, bool append, struct timeval time_app_started);

/* Closes the received log file and destroys the log itself.*/
void log_close(log_t* log);

/* Write string msg to the received log file, using the log
 * level specified for the writing. Works exactly like printf
 * meaning that you can pass msg as a formated string, and then
 * specify list of arguments. If successful, returns
 * the total of characters written. Otherwise, a negative
 * number is returned.*/
int log_write(log_t* log, log_level lvl, char* msg, ... );

#endif
