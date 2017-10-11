#ifndef TOURNAMENT_H
#define TOURNAMENT_H

#include "lock.h"
#include "protocol.h"
#include "semaphore.h"
#include "score_table.h"
#include "partners_table.h"
#include "confparser.h"


/*
 * Referee information will now be distributed. Everyone should be able
 * to access and modify, previously locking the TDA.
 *
 * What specific data is stored here should be defined soon.
 */

typedef enum _player_status {
	TM_P_OUTSIDE,
	TM_P_IDLE,
	TM_P_PLAYING,
	TM_P_LEAVED
} p_status;

typedef enum _court_status {
	TM_C_FREE,
	TM_C_LOBBY,
	TM_C_BUSY,
	TM_C_DISABLED
} c_status;

typedef struct _player_data {
	int player_pid;
	//char player_name[NAME_MAX_LENGTH];
	p_status player_status;
} player_data_t;


typedef struct _court_data {
	unsigned int court_players[PLAYERS_PER_MATCH];
	int court_num_players;
	c_status court_status;
} court_data_t;

typedef struct tournament_data {
	// TODO: Change this for dynamic allocation
	player_data_t tm_players[MAX_PLAYERS];
	court_data_t tm_courts[MAX_COURTS];
	// General stats.
	unsigned int tm_idle_players;
	unsigned int tm_active_players;

	unsigned int tm_idle_courts;
	unsigned int tm_active_courts;

	int tm_players_sem;
	int tm_courts_sem;
	int tm_init_sem;
	
	partners_table_t* pt;
	score_table_t* st;
} tournament_data_t;

typedef struct tournament {
	size_t total_players;
	size_t total_courts;
	size_t num_matches;
	int tm_shmid;
	tournament_data_t *tm_data;
	lock_t *tm_lock;
} tournament_t;


tournament_t* tournament_create(struct conf sc);
void tournament_init(tournament_t* tm, struct conf sc);
void tournament_shmrm(tournament_t* tm);
void tournament_destroy(tournament_t* tm);
void tournament_set_tables(tournament_t* tm, partners_table_t* pt, score_table_t* st);

#endif // TOURNAMENT_H
