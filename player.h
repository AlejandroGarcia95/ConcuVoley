#ifndef PLAYER_H
#define PLAYER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define NAME_MAX_LENGTH 50

#define SKILL_MAX 100

/* Player structure used to model a player of
 * the tournament. For the time being, each
 * player has name, a skill field which measures 
 * from 0 to SKILL_MAX how good that player is, 
 * and a matches_played field which counts how 
 * many matches the player has been in.*/
typedef struct player_ {
	char name[NAME_MAX_LENGTH];
	size_t skill;
	size_t matches_played;
} player_t;

/* Dynamically creates a new player with a given 
 * name and properly initialize all other values.
 * Returns NULL if the allocation fails.*/
player_t* player_create(char* name);

/* Destroys the received player.*/
void player_destroy(player_t* player);

/* Returns the skill of the received player.
 * If player is NULL, returns some number
 * above SKILL_MAX as an error code.*/
size_t player_get_skill(player_t* player);

/* Returns the amount of matches played by the 
 * received player. If player is NULL, let it
 * return 0 by the time being.*/
size_t player_get_matches(player_t* player);

/*Increase by 1 the amount of matches played.*/
void player_increase_matches_played(player_t* player);


#endif
