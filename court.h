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

// Deprecated, not in use now
//#define PIPE_READ 0
//#define PIPE_WRITE 1

#define SIG_SET SIGUSR1

/* NEW: structure for modelling each team playing on the court. */
typedef struct court_team_ {
	unsigned int team_players[PLAYERS_PER_TEAM];
	size_t team_size;
} court_team_t;

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
	bool close_pipes; 
	// Prop: send this scores to the court_team_t structure
	uint8_t sets_won_home;
	uint8_t sets_won_away;

	// Temporary!!
	// TODO: Replace it by the new court_team_t
	// court_team_t team_home; // team 0
	// court_team_t team_away; // team 1
	uint8_t connected_players;
	partners_table_t* pt;
	uint8_t player_points[PLAYERS_PER_MATCH];
	unsigned int player_connected[PLAYERS_PER_MATCH];
	unsigned int player_team[PLAYERS_PER_MATCH];
} court_t;

// --------------- Court team section --------------

/* Initializes received team as empty team.*/
void court_team_initialize(court_team_t* team);

/* Returns true if team is full (no other member can
 * join it) or false otherwise.*/
bool court_team_is_full(court_team_t team);

/* Returns true if the received player can join the team.*/
bool court_team_player_can_join_team(court_team_t team, unsigned int player_id, partners_table_t* pt);

/* Joins the received player to the team.*/
void court_team_join_player(court_team_t* team, unsigned int player_id);

/* Removes every player of the team from it.*/
void court_team_kick_players(court_team_t* team);

// --------------- Court section -------------------

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
