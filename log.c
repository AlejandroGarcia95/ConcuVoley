#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <sys/time.h>
#include <unistd.h>
#include "lock.h"
#include "log.h"


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
log_t* log_open(char* route, bool append){
	FILE *pf = fopen(route, (append ? "a" : "w"));
	if(!pf)	return NULL;
	
	log_t* log = malloc(sizeof(log_t));
	if(!log) {
		fclose(pf);
		return NULL;
		}
		
	log->log_file = pf;
	
	struct timeval time_start;
	gettimeofday (&time_start, NULL);
	
	log->time_created = time_start;
	log->debug = false;
	
	log->lock = lock_create("log");
	if(!log->lock) {
		fclose(pf);
		free(log);
		return NULL;
	}
	
	return log;
}

/* Retrieves log singleton instance. */
log_t* log_get_instance(){
	static log_t* log = NULL;
	// Check if there's log already
	if(log)
		return log;
	// If not, create log
	log = log_open(LOG_ROUTE, false);
	return log;
}

/* Closes the received log file and destroys the log itself.*/
void log_close(){
	log_t* log = log_get_instance();
	if(log && log->log_file)
		fclose(log->log_file);
		
	if(log) {
		lock_destroy(log->lock);
		free(log);
	}
}

/* Enables or disables "debug mode" for log depending on the
 * value received in set_debug. Only NONE_L log level strings
 * will be written if debug mode is off.*/
void log_set_debug_mode(bool set_debug){
	log_t* log = log_get_instance();
	if(log)
		log->debug = set_debug;
}

/* Write string msg to the received log file, using the log
 * level specified for the writing. If successful, returns
 * the total of characters written. Otherwise, a negative
 * number is returned.*/
int log_write(log_level lvl, char* msg, ... ){
	log_t* log = log_get_instance();	
	if(!(log && log->log_file)) return -1;
	
	if((!log->debug) && (lvl != NONE_L))
		return -1;
		
	lock_acquire(log->lock);
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
		lock_release(log->lock);
		return 0;
	}
		
	char* str_lvl;
	switch(lvl){
		case STAT_L:
			str_lvl = "STATS";
			break;
		case INFO_L: 
			str_lvl = "INFO";
			break;
		case DEBUG_L: 
			str_lvl = "DEBUG";
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
		default:
			break;
		}
			
	fprintf(log->log_file, "\x1b[39m[%s] [%s]\x1b[1;38;5;%dm [%d] ", time_str, str_lvl, ((getpid() % 20) * 2 + 1), getpid());
	vfprintf(log->log_file, msg, args);
	//fprintf(log->log_file, "\x1b[37;1", time_str, str_lvl);
	va_end(args);
	fflush(log->log_file);
	lock_release(log->lock);
	return 0;
}
