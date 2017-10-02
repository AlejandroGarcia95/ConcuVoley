#ifndef court_H
#define court_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include "log.h"
#include "partners_table.h"

#define PLAYERS_PER_MATCH 4
#define PLAYERS_PER_TEAM (PLAYERS_PER_MATCH / 2)

#define SETS_AMOUNT 5
#define SETS_WINNING 3
#define SET_DURATION 4

// Deprecated, not in use now
//#define PIPE_READ 0
//#define PIPE_WRITE 1

#define SIG_SET SIGUSR1

/* court structure used to model a court of
 * the tournament. It temporally stores the
 * amount of sets won by each team and the
 * pipes' file descriptors used to send and
 * receive messages from the players. The
 * bool close_pipes field determinates if
 * the court is to close them.*/
typedef struct court_ {
	int player_fifos[PLAYERS_PER_MATCH];
	int court_fifo;
	char* court_fifo_name;
	bool close_pipes; // close_fifos
	uint8_t sets_won_home;
	uint8_t sets_won_away;

	// Temporary!!
	uint8_t connected_players;
	partners_table_t* pt;
	uint8_t player_points[PLAYERS_PER_MATCH];
	unsigned int player_connected[PLAYERS_PER_MATCH];
	unsigned int player_team[PLAYERS_PER_MATCH];
} court_t;

/* Dynamically creates a new court. Returns
 * NULL in failure. The pipes argument are
 * the pipes file descriptors used for commu-
 * nication with the players. Note that the
 * pipes[][0] to pipes[PLAYERS_PER_TEAM-1]
 * file descriptors represent players of the
 * same team. The boolean argument tells the
 * court to close the file descriptors. 
 * Pre 1: The players processes were already
 * launched and the pipes for communicating
 * with them are the ones received.
 * Pre 2: The players processes ARE CHILDREN
 * PROCESSES of the process using this court.*/
court_t* court_create(log_t* log, partners_table_t* pt);

/* Kills the received court and sends flowers
 * to his widow.*/
void court_destroy(court_t* court);

/* Plays the court. Communication is done
 * using the players' pipes, and sets are
 * played until one of the two teams wins
 * SETS_WINNING sets, or until SETS_AMOUNT
 * sets are played. If the close_pipes field
 * was set on this court creation, this
 * function also closes the pipes file des-
 * criptors at the end of the court. */
void court_play(court_t* court, log_t* log);
void court_lobby(court_t* court, log_t* log);

/* Finish the current set by signaling
 * the players with SIG_SET.*/
void court_finish_set(court_t* court);

/* Returns the sets won by home and away 
 * playes. If court is NULL, let them both 
 * return 0 for the time being.*/
size_t court_get_home_sets(court_t* court);
size_t court_get_away_sets(court_t* court);

#endif
