#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "player.h"


/* Auxiliar function that generates a random
 * skill field for a new player. It returns a
 * number "s" for the skill such that s <= 100
 * and s < (SKILL_AVG + DELTA_SKILL) and
 * (SKILL_AVG - DELTA_SKILL) < s.
 * Pre: srand was already called.*/
size_t generate_random_skill(){
	int interval_width;
	if(SKILL_MAX < (SKILL_AVG + DELTA_SKILL))
		interval_width = SKILL_MAX - SKILL_AVG + DELTA_SKILL;
	else
		interval_width = 2 * DELTA_SKILL;
	int r_numb = rand();
	return (r_numb % interval_width) + SKILL_AVG - DELTA_SKILL;
}

// ----------------------------------------------------------------

/* Dynamically creates a new player with a given 
 * name and properly initialize all other values.
 * Returns NULL if the allocation fails.*/
player_t* player_create(char* name){
	if(!name) return NULL;
	
	player_t* player = malloc(sizeof(player_t));
	if(!player)	return NULL;
	
	// Initialize name
	strcpy(player->name, name);
	
	// Initialize all other fields
	player->skill = generate_random_skill();
	player->matches_played = 0;
	return player;
}

/* Destroys the received player.*/
void player_destroy(player_t* player){
	free(player);
}

/* Returns the skill of the received player.
 * If player is NULL, returns some number
 * above SKILL_MAX as an error code.*/
size_t player_get_skill(player_t* player){
	return (player ? player->skill : SKILL_MAX+10);
}

/* Returns the amount of matches played by the 
 * received player. If player is NULL, let it
 * return 0 by the time being.*/
size_t player_get_matches(player_t* player){
	return (player ? player->matches_played : 0);
}

/* Returns the skill of the received player.
 * If player is NULL, returns NULL.*/
char* player_get_name(player_t* player){
	return (player ? player->name : NULL);
}

/*Increase by 1 the amount of matches played.*/
void player_increase_matches_played(player_t* player){
	player->matches_played++;
}
