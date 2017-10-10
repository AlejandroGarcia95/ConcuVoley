#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "lock.h"
#include "tournament.h"
#include "confparser.h"

// For the constans... remove later and allocate dynamically trough parameters
#include "protocol.h"
#include "player.h"

tournament_t* tournament_create(struct conf sc) {
	key_t key = ftok("makefile", 77);
	if (key < 0) return NULL;
	
	tournament_t* tm = malloc(sizeof(tournament_t));
	if (!tm) return NULL;
	
	tm->tm_lock = lock_create("tournament");
	if (!tm->tm_lock) {
		free(tm);
		return NULL;
	}
	
	tm->tm_shmid = shmget(key, sizeof(tournament_data_t), IPC_CREAT | 0644);
	if (tm->tm_shmid < 0) {
		lock_destroy(tm->tm_lock);
		free(tm);
		return NULL;
	}
	
	void *shm = shmat(tm->tm_shmid, NULL, 0);
	if (shm == (void*) -1) {
		lock_destroy(tm->tm_lock);
		free(tm);
		return NULL;
	}
	
	tm->tm_data = (tournament_data_t*) shm;

	tournament_init(tm, sc);
	return tm;
}


void tournament_init(tournament_t* tm, struct conf sc) {
	int i, j;
	for (i = 0; i < sc.players; i++) {
		tm->tm_data->tm_players[i].player_status = TM_P_IDLE;
	}

	for (i = 0; i < MAX_COURTS; i++) {
		court_data_t cd;
		cd.court_num_players = 0;
		cd.court_status = TM_C_FREE;
		for(j = 0; j < PLAYERS_PER_MATCH; j++)
			cd.court_players[j] = INVALID_PLAYER_ID;
		tm->tm_data->tm_courts[i] = cd;
	}

	tm->tm_data->tm_active_players = sc.players;
	tm->tm_data->tm_idle_courts = (sc.rows * sc.cols);

	tm->tm_data->tm_players_sem = -1;
	tm->tm_data->tm_courts_sem = -1;
	tm->tm_data->pt = NULL;
	tm->tm_data->st = NULL;
	
	tm->total_players = sc.players;
	tm->total_courts = (sc.rows * sc.cols);
}

void tournament_destroy(tournament_t* tm) {
	if (!tm) return;
	lock_destroy(tm->tm_lock);

	// Detaches shared memory 
	shmdt((void*) tm->tm_data);
	free(tm);
}

/* Destroys tournament struct and also frees shared memory */
void tournament_free(tournament_t* tm) {
	if (!tm) return;
	if (tm->tm_data->tm_players_sem >= 0)
		sem_destroy(tm->tm_data->tm_players_sem);
	if (tm->tm_data->tm_courts_sem >= 0)
		sem_destroy(tm->tm_data->tm_courts_sem);
		
	int shmid = tm->tm_shmid;
	tournament_destroy(tm);
	shmctl(shmid, IPC_RMID, NULL);
}

/* Should be called after launching players.*/
void tournament_set_tables(tournament_t* tm, partners_table_t* pt, score_table_t* st){
	if(!tm) return;
	tm->tm_data->st = st;
	tm->tm_data->pt = pt;
}
