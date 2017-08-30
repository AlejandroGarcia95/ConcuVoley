#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/* Log structure used to represent the log. 
 * Shall we add more fields to it? */
typedef struct log_ {
	FILE* log_file;
} log_t;

/* Log level specifier for writing to the log. */
typedef enum log_level_ {NONE_L, INFO_L, WARNING_L, ERROR_L} log_level;

/* Opens file at route and dynamically creates a log with it. 
 * If append is false, then the file is overwritten. Returns 
 * NULL if creating the log failed.*/
log_t* log_open(char* route, bool append);

/* Closes the received log file and destroys the log itself.*/
void log_close(log_t* log);

/* Write string msg to the received log file, using the log
 * level specified for the writing. If successful, returns
 * the total of characters written. Otherwise, a negative
 * number is returned.*/
int log_write(log_t* log, char* msg, log_level lvl);

#endif
