#ifndef court_H
#define court_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include "log.h"
#include "partners_table.h"
#include "protocol.h"

#define PLAYERS_PER_MATCH 4
#define PLAYERS_PER_TEAM (PLAYERS_PER_MATCH / 2)

#define SETS_AMOUNT 5
#define SETS_WINNING 3


#define SIG_SET SIGUSR1

/* Structure for modelling each team playing on the court. */
typedef struct court_team_ {
	unsigned int team_players[PLAYERS_PER_TEAM];
	size_t team_size;
	uint8_t sets_won;
} court_team_t;

/* court structure used to model a court of
 * the tournament. It temporally stores the
 * amount of sets won by each team and the
 * pipes' file descriptors used to send and
 * receive messages from the players. The
 * bool close_pipes field determinates if
 * the court is to close them.*/
typedef struct court_ {
	unsigned int court_id;
	int player_fifos[PLAYERS_PER_MATCH];
	int court_fifo;
	char court_fifo_name[MAX_FIFO_NAME_LEN];
	bool close_pipes; 

	court_team_t team_home; // team 0
	court_team_t team_away; // team 1
	uint8_t connected_players;
	partners_table_t* pt;

	// Remove this later
	uint8_t player_points[TOTAL_PLAYERS];
} court_t;

// --------------- Court team section --------------

/* Initializes received team as empty team.*/
void court_team_initialize(court_team_t* team);

/* Returns true if team is full (no other member can
 * join it) or false otherwise.*/
bool court_team_is_full(court_team_t team);

/* Returns true if the received player can join the team.*/
bool court_team_player_can_join_team(court_team_t team, unsigned int player_id, partners_table_t* pt);

/* Returns true if the received player is already on the team*/
bool court_team_player_in_team(court_team_t team, unsigned int player_id);

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
court_t* court_create(unsigned int court_id, partners_table_t* pt);

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
void court_play(court_t* court);
void court_lobby(court_t* court);

/* Finish the current set by signaling
 * the players with SIG_SET.*/
void court_finish_set(court_t* court);

/* Returns a number between 0 and PLAYERS_PER_MATCH -1 which 
 * represents the "player_court_id", a player id that is 
 * "relative" to this court. If the player_id received doesn't
 * belong to a player on this court, returns something above
 * PLAYERS_PER_MATCH.*/
unsigned int court_player_to_court_id(court_t* court, unsigned int player_id);

/* Inverse of the function above. Receives a "player_court_id"
 * relative to this court and returns the player_id. Returns
 * something above TOTAL_PLAYERS in case of error.*/
unsigned int court_court_id_to_player(court_t* court, unsigned int pc_id);

/* Returns the sets won by home and away 
 * playes. If court is NULL, let them both 
 * return 0 for the time being.*/
size_t court_get_home_sets(court_t* court);
size_t court_get_away_sets(court_t* court);

#endif
