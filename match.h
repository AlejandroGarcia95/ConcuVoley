#ifndef MATCH_H
#define MATCH_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "player.h"

// INTERNAL QUESTION: Is this really a match, a game or what? 

/* Match structure used to model a match of
 * the tournament. For the time being, it
 * can only store two players and how many
 * sets each one won.*/
typedef struct match_ {
	player_t* home;
	player_t* away;
	uint8_t sets_won_home;
	uint8_t sets_won_away;
} match_t;

/* Dynamically creates a match with the two
 * received players. If ANY of them both is
 * NULL, the match should not be created and
 * the NULL value should be returned.*/
 /* Obs: Two players cannot play one against each other
  * if they had previously done so. We may be needing
  * to consider this from "outside", perhaps. */
match_t* match_create(player_t* home, player_t* away);

/* Destroys the match received BUT NOT the
 * players of it.*/
void match_destroy(match_t* match);

/* Simulates the playing of the match. This
 * function must emulate the match duration
 * considering each player's skill. 
 * Pre: The match has valid players, and both
 * can still play on the tournament.
 * Post: The match takes some time and then
 * finishes. The sets_won fields are updated
 * with the result of the simulation. */
void match_play(match_t* match);

// TODO: Create a function to determinate player's scores

#endif
