#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <time.h>
#include "log.h"

/* Opens file at route and dynamically creates a log with it. 
 * If append is false, then the file is overwritten. Returns 
 * NULL if creating the log failed.*/
log_t* log_open(char* route, bool append, time_t time_app_started){
	FILE *pf = fopen(route, (append ? "a" : "w"));
	if(!pf)	return NULL;
	
	log_t* log = malloc(sizeof(log_t));
	if(!log) {
		fclose(pf);
		return NULL;
		}
		
	log->log_file = pf;
	
	log->time_created = time_app_started;
	return log;
}

/* Closes the received log file and destroys the log itself.*/
void log_close(log_t* log){
	if(log && log->log_file)
		fclose(log->log_file);
	if(log)
		free(log);
}

/* Write string msg to the received log file, using the log
 * level specified for the writing. If successful, returns
 * the total of characters written. Otherwise, a negative
 * number is returned.*/
int log_write(log_t* log, log_level lvl, char* msg, ... ){
	if(!(log && log->log_file)) return -1;
	fflush(log->log_file);
	
	time_t time_now = time(NULL);
	double timestamp = difftime(time_now, log->time_created);
	
	va_list args;
	va_start(args, msg);

	if(lvl == NONE_L) {
		fprintf(log->log_file, "[%.2f] ", timestamp);
		vfprintf(log->log_file, msg, args);
		va_end(args);
		fflush(log->log_file);
		return 0;
	}
		
	char* str_lvl;
	switch(lvl){
		case INFO_L: 
			str_lvl = "INFO";
			break;
		case WARNING_L: 
			str_lvl = "WARN";
			break;
		case ERROR_L: 
			str_lvl = "ERROR";
			break;
		}
			

	fprintf(log->log_file, "[%.2f] ", timestamp);
	fprintf(log->log_file, "[%s] ", str_lvl);
	vfprintf(log->log_file, msg, args);
	va_end(args);
	fflush(log->log_file);
	return 0;
}
