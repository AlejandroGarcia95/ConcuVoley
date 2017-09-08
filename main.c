#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include "log.h"
#include "player.h"
#include "match.h"

#define LOG_ROUTE "ElLog.txt"
#define CONF_ROUTE "conf.txt"


/* struct conf: an aux structure for parsing the
 * configuration file at path CONF_ROUTE. */
struct conf {
	size_t rows;
	size_t cols;
	size_t matches;
};

/* Stores the value of the parameter recieved
 * into the respective field of struct conf sc.*/
void parse_value(struct conf* sc, char* param, size_t p_value){
	if((!sc) || (!param)) return;
	
	if(strcmp(param, "F") == 0) {
		sc->rows = p_value;
		return;
		}
		
	if(strcmp(param, "K") == 0) {
		sc->matches = p_value;
		return;
		}
		
	if(strcmp(param, "C") == 0) {
		sc->cols = p_value;
		return;
		}	
}

/* Reads configuration file and stores its
 * contents at sc. Returns true if the operation
 * was successful, false otherwise. Take note
 * that if any parameter from struct conf is
 * missing at the configuration file, the value
 * returned will be false, even if other fields
 * were successfully read.*/
bool read_conf_file(struct conf* sc){
	FILE *pf = fopen(CONF_ROUTE, "r");
	if(!pf) return false;
	int parsed_amount = 0, r = 8;
	// For every line in pf, parse its value
	while(r > 0){
		char param[15];
		size_t p_value;
		r = fscanf(pf, "%s : %d\n", param, &p_value);
		if(r == 2) {
			parse_value(sc, param, p_value);
			parsed_amount++;
			}
		}
	fclose(pf);	
	// Check all fields were correctly parsed
	int expected_amount = sizeof(struct conf) / sizeof(size_t);
	if(expected_amount == parsed_amount)
		return true;
	// Print error message. May we log_write it?
	printf("Error! Missing parameters or conf file format invalid\n");
	return false;
}


int main(int argc, char **argv){
	srand(time(NULL));
	
	// Test log levels
	log_t* log = log_open(LOG_ROUTE, false);
	log_write(log, "Testing: none", NONE_L);
	log_write(log, "Testing: warning", WARNING_L);
	log_write(log, "Testing: ERROR", ERROR_L);
	log_write(log, "Testing: info", INFO_L);
	log_write(log, "", NONE_L); // empty line
	
	log_write(log, "--------------------------------------", NONE_L);
	
	// Read conf file
	struct conf sc;
	bool parse_ok = read_conf_file(&sc);
	if(parse_ok){ // Write parsed args to log
		char aux_value[10];
		log_write(log, "Field F value:", NONE_L);
		sprintf(aux_value, "%d", sc.rows);
		log_write(log, aux_value, NONE_L);
		log_write(log, "Field C value:", NONE_L);
		sprintf(aux_value, "%d", sc.cols);
		log_write(log, aux_value, NONE_L);
		log_write(log, "Field K value:", NONE_L);
		sprintf(aux_value, "%d", sc.matches);
		log_write(log, aux_value, NONE_L);
		}
		
	log_write(log, "--------------------------------------", NONE_L);	
	
	// Create 10 players, check fields and delete them	
	int i;
	for(i = 0; i < 10; i++){
		char plyr_name[15], plyr_skill[10], plyr_matches[10];
		sprintf(plyr_name, "Player%d", i);
		player_t* player_act = player_create(plyr_name);
		// Check and log_write fields
		sprintf(plyr_skill, "%d", player_get_skill(player_act));
		sprintf(plyr_matches, "%d", player_get_matches(player_act));
		
		log_write(log, "Created new player. Name:", NONE_L);
		log_write(log, player_get_name(player_act), NONE_L);
		log_write(log, "Skill:", NONE_L);
		log_write(log, plyr_skill, NONE_L);
		log_write(log, "Matches played:", NONE_L);
		log_write(log, plyr_matches, NONE_L);
		player_destroy(player_act);
		}	
		
	log_write(log, "--------------------------------------", NONE_L);
	
	// Create 2 players and make them play a match
	player_t* p1 = player_create("Player 1");
	player_t* p2 = player_create("Player 2");
	match_t* match = match_create(p1, p2);
	
	char msg_log[100];
	sprintf(msg_log, "Player 1 skill: %d", player_get_skill(p1));
	log_write(log, msg_log, NONE_L);
	sprintf(msg_log, "Player 2 skill: %d", player_get_skill(p2));
	log_write(log, msg_log, NONE_L);
	log_write(log, "Let the match begin!", NONE_L);
	match_play(match);
	log_write(log, "Match ended. Results are...", NONE_L);
	sprintf(msg_log, "Player 1: %d", match_get_home_sets(match));
	log_write(log, msg_log, NONE_L);
	sprintf(msg_log, "Player 2: %d", match_get_away_sets(match));
	log_write(log, msg_log, NONE_L);
	
	
	player_destroy(p1);
	player_destroy(p2);
	match_destroy(match);
		
		
	log_close(log);
	return 0;
}

