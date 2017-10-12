#ifndef TOURNAMENT_H
#define TOURNAMENT_H

#include "lock.h"
#include "protocol.h"
#include "semaphore.h"
#include "score_table.h"
#include "partners_table.h"
#include "confparser.h"

#define MAX_NUM_MATCHES 40
#define NAME_MAX_LENGTH 50

/*
 * Tournament distributed information. Everyone should be able
 * to access and modify, previously locking the TDA.
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
	TM_C_FLOODED,
	TM_C_DISABLED
} c_status;


typedef struct _match_data {
	int match_players[PLAYERS_PER_MATCH];
	int match_score[2];
	int match_played_at;
} match_data_t;

typedef struct _player_data {
	int player_pid;
	char player_name[NAME_MAX_LENGTH];
	p_status player_status;

	match_data_t player_matches[MAX_NUM_MATCHES];
	int player_num_matches;
} player_data_t;


typedef struct _court_data {
	unsigned int court_players[PLAYERS_PER_MATCH];
	int court_pid;
	int court_num_players;
	c_status court_status;

	int court_completed_matches;
	int court_suspended_matches;
} court_data_t;

typedef struct tournament_data {
	player_data_t tm_players[MAX_PLAYERS];
	court_data_t tm_courts[MAX_COURTS];
	// General stats.
	unsigned int tm_on_beach_players;
	unsigned int tm_active_players;

	unsigned int tm_idle_courts;
	unsigned int tm_active_courts;

	int tm_players_sem;
	int tm_courts_sem;
	int tm_courts_flood_sem;
	
	int tm_init_sem;

	int tm_tide_lvl;
	
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
