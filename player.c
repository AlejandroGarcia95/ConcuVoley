#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "player.h"


#define SKILL_AVG 80
#define DELTA_SKILL 15

/* Auxiliar function that generates a random
 * skill field for a new player. It returns a
 * number "s" for the skill such that s <= 100
 * and s < (SKILL_AVG + DELTA_SKILL) and
 * (SKILL_AVG - DELTA_SKILL) < s.*/
size_t generate_random_skill(){
	return 0;
}

// ----------------------------------------------------------------

/* Dynamically creates a new player with a given 
 * name and properly initialize all other values.
 * Returns NULL if the allocation fails.*/
player_t* player_create(char* name){
	return NULL;
}

/* Destroys the received player.*/
void player_destroy(player_t* player){
	
}

/* Returns the skill of the received player.
 * If player is NULL, returns some number
 * above SKILL_MAX as an error code.*/
size_t player_get_skill(player_t* player){
	return 0;
}

/* Returns the amount of matches played by the 
 * received player. If player is NULL, let it
 * return 0 by the time being.*/
size_t player_get_matches(player_t* player){
	return 0;
}

/*Increase by 1 the amount of matches played.*/
void player_increase_matches_played(player_t* player){
	player->matches_played++;
}
