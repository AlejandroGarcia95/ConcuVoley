#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <sys/time.h>
#include "lock.h"
#include "log.h"

/* Important comment about the log: La consigna dice que debería
 * tener un "modo debug" y que los mensajes de debug sólo se deberían
 * imprimir en ese modo... Es decir, habría que agregar un flag en
 * el log_open y que en el log_write sólo imprima los mensajes de E,W,I
 * si el flag está en 1
 * */

// TODO: Add a lock to this (just uncomment the locks calls)

/* Auxiliar function that creates a pretty-printeable
 * time string. The final string is stored at str.*/
void get_time_string(log_t* log, char* str){
	struct timeval time_now; 
	gettimeofday (&time_now, NULL);
	
	unsigned long int timestamp = (time_now.tv_sec - log->time_created.tv_sec)*1000000L;
	timestamp += (time_now.tv_usec - log->time_created.tv_usec);
	 // Check microseconds
	if(timestamp < 1000L)
		sprintf(str, "%lu Us", timestamp);
	else if(timestamp < 1000000L) // Check miliseconds
		sprintf(str, "%.3f ms", ((double)timestamp)/1000);
	else
		sprintf(str, "%.3f s", ((double)timestamp)/1000000L);
 }

/* Opens file at route and dynamically creates a log with it. 
 * If append is false, then the file is overwritten. Returns 
 * NULL if creating the log failed.*/
log_t* log_open(char* route, bool append, struct timeval time_app_started){
	FILE *pf = fopen(route, (append ? "a" : "w"));
	if(!pf)	return NULL;
	
	log_t* log = malloc(sizeof(log_t));
	if(!log) {
		fclose(pf);
		return NULL;
		}
		
	log->log_file = pf;
	
	log->time_created = time_app_started;
	// log->lock = lock_create(...);
	return log;
}

/* Closes the received log file and destroys the log itself.*/
void log_close(log_t* log){
	if(log && log->log_file)
		fclose(log->log_file);
	// lock_destroy(log->lock);	
	if(log)
		free(log);
}

/* Write string msg to the received log file, using the log
 * level specified for the writing. If successful, returns
 * the total of characters written. Otherwise, a negative
 * number is returned.*/
int log_write(log_t* log, log_level lvl, char* msg, ... ){
	if(!(log && log->log_file)) return -1;
	// lock_acquire(log->lock);
	fflush(log->log_file);
	
	char time_str[10];
	get_time_string(log, time_str);
	
	va_list args;
	va_start(args, msg);

	if(lvl == NONE_L) {
		fprintf(log->log_file, "[%s] ", time_str);
		vfprintf(log->log_file, msg, args);
		va_end(args);
		fflush(log->log_file);
		return 0;
	}
		
	// TODO: Add colors maybe? \x1b[32m \x1b[37;1
	// Strongly agree to it
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
		case CRITICAL_L:
			str_lvl = "CRITICAL";
			break;
		}
			
	fprintf(log->log_file, "[%s] [%s] ", time_str, str_lvl);
	vfprintf(log->log_file, msg, args);
	va_end(args);
	fflush(log->log_file);
	// lock_release(log->lock);
	return 0;
}
