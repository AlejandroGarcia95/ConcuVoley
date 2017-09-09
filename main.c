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
	log_write(log, NONE_L, "Testing: none\n");
	log_write(log, WARNING_L, "Testing: warning\n");
	log_write(log, ERROR_L, "Testing: ERROR\n");
	log_write(log, INFO_L, "Testing: info\n");
	log_write(log, NONE_L, "\n"); // empty line
	
	log_write(log, NONE_L, "--------------------------------------\n");
	
	// Read conf file
	struct conf sc;
	bool parse_ok = read_conf_file(&sc);
	if(parse_ok){ // Write parsed args to log
		log_write(log, NONE_L, "Field F value: %d\n", sc.rows);
		log_write(log, NONE_L, "Field C value: %d\n", sc.cols);
		log_write(log, NONE_L, "Field K value: %d\n", sc.matches);
	}
		
	log_write(log, NONE_L, "--------------------------------------\n");	
	
	// Create 10 players, check fields and delete them	
	int i;
	for(i = 0; i < 10; i++){
		char plyr_name[15], plyr_skill[10], plyr_matches[10];
		sprintf(plyr_name, "Player%d", i);
		player_t* player_act = player_create(plyr_name);
		// Check and log_write fields
		sprintf(plyr_skill, "%d", player_get_skill(player_act));
		sprintf(plyr_matches, "%d", player_get_matches(player_act));
		
		log_write(log, NONE_L, "Created new player. Name: %s\n", player_get_name(player_act));
		log_write(log, NONE_L, "Skill: %s\n", plyr_skill);
		log_write(log, NONE_L, "Matches played: %s\n", plyr_matches);
		player_destroy(player_act);
	}	
		
	log_write(log, NONE_L, "--------------------------------------\n");	
	
	// Create 2 players and make them play a match
	player_t* p1 = player_create("Player 1");
	player_t* p2 = player_create("Player 2");
	match_t* match = match_create(p1, p2);
	
	char msg_log[100];
	log_write(log, NONE_L, "Player 1 skill: %d\n", player_get_skill(p1));
	log_write(log, NONE_L, "Player 2 skill: %d\n", player_get_skill(p2));
	log_write(log, NONE_L, "Let the match begin!\n");
	match_play(match);
	log_write(log, NONE_L, "Match ended. Results are...\n");
	log_write(log, NONE_L, "Player 1: %d\n", match_get_home_sets(match));
	log_write(log, NONE_L, "Player 2: %d\n", match_get_away_sets(match));
	
	
	player_destroy(p1);
	player_destroy(p2);
	match_destroy(match);
		

	log_close(log);
	return 0;
}

