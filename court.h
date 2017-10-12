#ifndef court_H
#define court_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include "log.h"
#include "score_table.h"
#include "partners_table.h"
#include "tournament.h"
#include "protocol.h"

#define SETS_AMOUNT 5
#define SETS_WINNING 3

#define JOIN_ATTEMPTS_MAX (PLAYERS_PER_MATCH + 3) // 3 "wrong attempts" before kicking everyone

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

	bool flooded;
	int flood_sem;

	court_team_t team_home; // team 0
	court_team_t team_away; // team 1
	uint8_t connected_players;
	
	tournament_t* tm;
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

/* Increases player's score for every player on the received team*/
void court_team_add_score_players(court_team_t team, score_table_t* st, int score);

// --------------- Court section -------------------

/* Returns the current court singleton!*/
court_t* court_get_instance();


/* Kills the received court and sends flowers
 * to his widow.*/
void court_destroy();

/* Plays the court. Communication is done
 * using the players' pipes, and sets are
 * played until one of the two teams wins
 * SETS_WINNING sets, or until SETS_AMOUNT
 * sets are played. If the close_pipes field
 * was set on this court creation, this
 * function also closes the pipes file des-
 * criptors at the end of the court. */
void court_play();
void court_lobby();

/* Finish the current set by signaling
 * the players with SIG_SET.*/
void court_finish_set();

/* Returns a number between 0 and PLAYERS_PER_MATCH -1 which 
 * represents the "player_court_id", a player id that is 
 * "relative" to this court. If the player_id received doesn't
 * belong to a player on this court, returns something above
 * PLAYERS_PER_MATCH.*/
unsigned int court_player_to_court_id(unsigned int player_id);

/* Inverse of the function above. Receives a "player_court_id"
 * relative to this court and returns the player_id. Returns
 * something above players amount in case of error.*/
unsigned int court_court_id_to_player(unsigned int pc_id);
void court_main(unsigned int court_id, tournament_t* tm);

#endif
