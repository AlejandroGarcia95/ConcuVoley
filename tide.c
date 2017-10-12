#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include "tide.h"
#include "tournament.h"
#include "lock.h"
#include "confparser.h"
#include "log.h"

void print_tournament_status(tournament_t* tm);

/* Dynamically creates new tide.*/
tide_t* tide_create(){	
	tide_t* tide = malloc(sizeof(tide_t));
	if(!tide)	return NULL;
	
	tide->pf = fopen(TIDE_FILE, "r");
	
	if(!tide->pf) {
		free(tide);
		return NULL;
		} 
	
	return tide;
}

/* Handles the flowing of the tide*/
void tide_flow(tournament_t* tm, struct conf sc){

	// Disable (if possible) sc.rows courts and signal them
	lock_acquire(tm->tm_lock);
	int actual_tide = tm->tm_data->tm_tide_lvl;
	log_write(INFO_L, "Tide: Flowing! (tide level: %d -> %d)\n", actual_tide, ((actual_tide+1) > (sc.rows) ? sc.rows : actual_tide));

	tm->tm_data->tm_tide_lvl++;
	if (tm->tm_data->tm_tide_lvl > sc.rows)
		tm->tm_data->tm_tide_lvl = sc.rows;

	int i;
	for (i = 0; i < tm->total_courts; i++) {
		int prev_state = tm->tm_data->tm_courts[i].court_status;
		if ((i % sc.rows) == tm->tm_data->tm_tide_lvl) {
			tm->tm_data->tm_courts[i].court_status = TM_C_FLOODED;
			kill(tm->tm_data->tm_courts[i].court_pid, SIG_TIDE);
		}
		log_write(STAT_L, "Court %03d is in state %d (previously %d)\n", i, tm->tm_data->tm_courts[i].court_status, prev_state);
	}
	print_tournament_status(tm);
	lock_release(tm->tm_lock);
	// with SIG_TIDE signal to make them kick players
}

/* Handles the ebbing of the tide*/
void tide_ebb(tournament_t* tm, struct conf sc){
	// Enable (if possible) sc.rows courts
	lock_acquire(tm->tm_lock);
	int actual_tide = tm->tm_data->tm_tide_lvl;
	log_write(INFO_L, "Tide: Ebbing! (tide level: %d -> %d)\n", actual_tide, ((actual_tide-1) < 0 ? -1 : actual_tide));

	int i;
	for (i = 0; i < tm->total_courts; i++) {
		int prev_state = tm->tm_data->tm_courts[i].court_status;
		if ((i % sc.rows) == tm->tm_data->tm_tide_lvl) {
			tm->tm_data->tm_courts[i].court_status = TM_C_FREE;
			kill(tm->tm_data->tm_courts[i].court_pid, SIG_TIDE);
		}
		log_write(STAT_L, "Court %03d is in state %d\n", i, tm->tm_data->tm_courts[i].court_status, prev_state);
	}
	print_tournament_status(tm);

	tm->tm_data->tm_tide_lvl--;
	if (tm->tm_data->tm_tide_lvl < 0)
		tm->tm_data->tm_tide_lvl = -1;

	lock_release(tm->tm_lock);
}

/* Parses current read line from tide file. If the line
 * specifies tide to flow or ebb (via F or E letters),
 * this function sleeps t_value microseconds and then
 * executes the proper function.*/
void tide_parse_command(struct conf sc, char* param, unsigned long int t_value, tournament_t* tm){
	if(!param) return;
	
	if(strcmp(param, "F") == 0) {
		usleep(t_value);
		tide_flow(tm, sc);
		return;
		}
		
	if(strcmp(param, "E") == 0) {
		usleep(t_value);
		tide_ebb(tm, sc);
		return;
		}
		
}

// ------------------------------------------------------------

/* Returns the current tide singleton!*/
tide_t* tide_get_instance(){
	static tide_t* tide = NULL;
	// Check if there's already a tide
	if(tide)
		return tide;
	// If not, create it!
	tide = tide_create();
	return tide;
}

/* Destroys the current tide.*/
void tide_destroy(){
	tide_t* tide = tide_get_instance();
	if(tide && tide->pf)
		fclose(tide->pf);
	if(tide)
		free(tide);
}


/* Executes main for this process. Finishes via exit(0)*/
void tide_main(tournament_t* tm, struct conf sc){
	tide_t* tide = tide_get_instance();
	if(!tide) {
		log_write(ERROR_L, "Tide: Couldn't create tide\n");
		exit(-1);
	}
	
	int r = 8;
	// For every line in pf, parse its value
	while(r > 0){
		char param[15];
		unsigned long int t_value = 0;
		r = fscanf(tide->pf, "%s : %lu\n", param, &t_value);
		if(r == 2) {
			tide_parse_command(sc, param, t_value, tm);
			}
		}
	

	tide_destroy();
	tournament_destroy(tm);
	log_write(INFO_L, "Tide: Finished process!\n");
	log_close();
	exit(0);
}
