#ifndef TOURNAMENT_H
#define TOURNAMENT_H

#include "lock.h"
#include "protocol.h"
#include "player.h"

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
	char player_name[NAME_MAX_LENGTH];
	p_status player_status;
} player_data_t;


typedef struct _court_data {
	unsigned int court_players[4];
	unsigned int court_num_players;
	c_status court_status;
} court_data_t;

typedef struct tournament_data {
	player_data_t tm_players[TOTAL_PLAYERS];
	court_data_t tm_courts[TOTAL_COURTS];

	// General stats.
	unsigned int tm_idle_players;
	unsigned int tm_active_players;

	unsigned int tm_idle_courts;
	unsigned int tm_active_courts;

} tournament_data_t;

typedef struct tournament {
	int tm_shmid;
	tournament_data_t *tm_data;
	lock_t *tm_lock;
} tournament_t;


tournament_t* tournament_create();
void tournament_init(tournament_t* tm);
void tournament_shmrm(tournament_t* tm);
void tournament_destroy(tournament_t* tm);

#endif // TOURNAMENT_H
