#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include "player.h"
#include "match.h"

// DEPRECATED !!!!!!!!!!!!!!!!!!!!!!!!!!

/* This auxiliar function simulates the
 * duration of the match with a simple
 * sleep call. */
void simulate_time_elapsed(unsigned int secs){
	sleep(secs);
}

#define MATCH_BIGGEST_TIME 12
#define MATCH_SMALLEST_TIME 2
#define MATCH_RAND_TIME 3


/* Auxiliar function which calculates the
 * duration of the match by using both
 * players skills. Let D be the difference
 * between them. Hence D is in the range
 * (0, 2*DELTA_SKILL). This function maps
 * that interval to a "fixed" time interval
 * (MATCH_BIGGEST_TIME, MATCH_SMALLEST_TIME)
 * and then calculates the time by adding a
 * "random" component from 0 to RAND_TIME.*/
unsigned int get_match_duration(size_t skill_home, size_t skill_away){
	int D = skill_home - skill_away;
	if(D < 0) D *= -1;
	// Fixed-time by mapping interval
	unsigned int fixed_time = MATCH_BIGGEST_TIME;
	unsigned int pend = (MATCH_BIGGEST_TIME - MATCH_SMALLEST_TIME);
	pend = (int) (pend * D / (2 * DELTA_SKILL));
	fixed_time -= pend;
	// Random time
	unsigned int rand_time = (rand() % MATCH_RAND_TIME);
	return fixed_time + rand_time;
}

/* Auxiliar function which sets the proper match
 * results after it finishes. This means, this is the
 * one function who determinates which player won and
 * which player lost, and the sets each one scored.
 * For the time being, it only makes the player with
 * most skill win by 3 to 0.*/
void set_match_result(match_t* match){
	size_t skill_h = player_get_skill(match->home);
	size_t skill_a = player_get_skill(match->away);
	if(skill_h > skill_a)
		match->sets_won_home = 3;
	else
		match->sets_won_away = 3;
}

// ----------------------------------------------------------------

/* Dynamically creates a match with the two
 * received players. If ANY of them both is
 * NULL, the match should not be created and
 * the NULL value should be returned.*/
 /* Obs: Two players cannot play one against 
  * each other if they had previously done so. 
  * We may be needing to consider this from 
  * the "outside", perhaps. */
match_t* match_create(player_t* home, player_t* away){
	if((!home) || (!away)) return NULL;

	match_t* match = malloc(sizeof(match_t));
	if(!match) return NULL;
	
	match->home = home;
	match->away = away;
	match->sets_won_away = 0;
	match->sets_won_home = 0;
	return match;
}

/* Destroys the match received BUT NOT the
 * players of it.*/
void match_destroy(match_t* match){
	free(match);
}

/* Simulates the playing of the match. This
 * function must emulate the match duration
 * considering each player's skill. 
 * Pre: The match has valid players, and both
 * can still play on the tournament.
 * Post: The match takes some time and then
 * finishes. The sets_won fields are updated
 * with the result of the simulation. */
void match_play(match_t* match){
	// Get match duration
	unsigned int match_duration;
	size_t skill_h = player_get_skill(match->home);
	size_t skill_a = player_get_skill(match->away);
	match_duration = get_match_duration(skill_h, skill_a);
	// Simulate that match being played
	simulate_time_elapsed(match_duration);
	// Match finished. Set results
	set_match_result(match);
}

/* Returns the sets won by home and away 
 * playes. If match is NULL, let them both 
 * return 0 for the time being.*/
size_t match_get_home_sets(match_t* match){
	return (match ? match->sets_won_home : 0);
}
size_t match_get_away_sets(match_t* match){
	return (match ? match->sets_won_away : 0);
}
