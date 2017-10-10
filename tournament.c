#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "lock.h"
#include "tournament.h"

// For the constans... remove later and allocate dynamically trough parameters
#include "protocol.h"
#include "player.h"

tournament_t* tournament_create() {
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

	tournament_init(tm);
	return tm;
}


void tournament_init(tournament_t* tm) {
	int i;
	for (i = 0; i < TOTAL_PLAYERS; i++) {
		tm->tm_data->tm_players[i].player_status = TM_P_IDLE;
	}

	for (i = 0; i < TOTAL_COURTS; i++) {
		court_data_t cd;
		cd.court_num_players = 0;
		cd.court_status = TM_C_FREE;
		tm->tm_data->tm_courts[i] = cd;
	}

	tm->tm_data->tm_active_players = TOTAL_PLAYERS;
	tm->tm_data->tm_idle_courts = TOTAL_COURTS;

	tm->tm_data->tm_players_sem = -1;
	tm->tm_data->tm_courts_sem = -1;
}

void tournament_destroy(tournament_t* tm) {
	if (!tm) return;
	lock_destroy(tm->tm_lock);

	if (tm->tm_data->tm_players_sem >= 0)
		sem_destroy(tm->tm_data->tm_players_sem);
	if (tm->tm_data->tm_courts_sem >= 0)
		sem_destroy(tm->tm_data->tm_courts_sem);
	// Detaches shared memory 
	shmdt((void*) tm->tm_data);
	free(tm);
}

/* Destroys tournament struct and also frees shared memory */
void tournament_shmrm(tournament_t* tm) {
	if (!tm) return;
	int shmid = tm->tm_shmid;
	tournament_destroy(tm);
	shmctl(shmid, IPC_RMID, NULL);
}

