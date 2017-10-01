#ifndef MATCH_H
#define MATCH_H

// IT IS BACK, AND WITH PIPES!!
// Prop: Rename as "court"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include "log.h"

#define PLAYERS_PER_MATCH 4
#define PLAYERS_PER_TEAM (PLAYERS_PER_MATCH / 2)

#define SETS_AMOUNT 5
#define SETS_WINNING 3
#define SET_DURATION 6

// Deprecated, not in use now
//#define PIPE_READ 0
//#define PIPE_WRITE 1

#define SIG_SET SIGUSR1

/* Match structure used to model a match of
 * the tournament. It temporally stores the
 * amount of sets won by each team and the
 * pipes' file descriptors used to send and
 * receive messages from the players. The
 * bool close_pipes field determinates if
 * the match is to close them.*/
typedef struct match_ {
	int player_fifos[PLAYERS_PER_MATCH];
	int court_fifo;
	char* court_fifo_name;
	bool close_pipes; // close_fifos
	uint8_t sets_won_home;
	uint8_t sets_won_away;

	// Temporary!!
	uint8_t player_points[PLAYERS_PER_MATCH];
	unsigned int player_connected[PLAYERS_PER_MATCH];
	unsigned int player_team[PLAYERS_PER_MATCH];
} match_t;

/* Dynamically creates a new match. Returns
 * NULL in failure. The pipes argument are
 * the pipes file descriptors used for commu-
 * nication with the players. Note that the
 * pipes[][0] to pipes[PLAYERS_PER_TEAM-1]
 * file descriptors represent players of the
 * same team. The boolean argument tells the
 * match to close the file descriptors. 
 * Pre 1: The players processes were already
 * launched and the pipes for communicating
 * with them are the ones received.
 * Pre 2: The players processes ARE CHILDREN
 * PROCESSES of the process using this match.*/
match_t* match_create(log_t* log);

/* Kills the received match and sends flowers
 * to his widow.*/
void match_destroy(match_t* match);

/* Plays the match. Communication is done
 * using the players' pipes, and sets are
 * played until one of the two teams wins
 * SETS_WINNING sets, or until SETS_AMOUNT
 * sets are played. If the close_pipes field
 * was set on this match creation, this
 * function also closes the pipes file des-
 * criptors at the end of the match. */
void match_play(match_t* match, log_t* log);
void match_lobby(match_t* match, log_t* log);

/* Finish the current set by signaling
 * the players with SIG_SET.*/
void match_finish_set(match_t* match);

/* Returns the sets won by home and away 
 * playes. If match is NULL, let them both 
 * return 0 for the time being.*/
size_t match_get_home_sets(match_t* match);
size_t match_get_away_sets(match_t* match);

#endif
