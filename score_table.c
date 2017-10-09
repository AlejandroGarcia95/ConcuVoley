#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "lock.h"
#include "log.h"
#include "player.h"
#include "score_table.h"

/* Dinamically allocates a new score_table based on the 
 * amount of players received. Returns NULL on error.*/
score_table_t* score_table_create(size_t players){
	key_t key_s = ftok("makefile", 8);
	if(key_s < 0) return NULL;
	int i;
	key_t key_n = ftok("makefile", 11);
	if(key_n < 0) return NULL;
	
	score_table_t* st = malloc(sizeof(score_table_t));
	if(!st) return NULL;
	
	st->players_amount = players;
	
	st->lock = lock_create("score_table");
	if(!st->lock){
		free(st);
		return NULL;
	}
	
	// Create score table as shared memory
	st->shmid = shmget(key_s, sizeof(unsigned int) * players, IPC_CREAT | 0644);
	if(st->shmid < 0){
		lock_destroy(st->lock);
		free(st);
		return NULL;
		}
	
	void* shm = shmat(st->shmid, NULL, 0);
	if(shm == (void*) -1) {
		lock_destroy(st->lock);
		free(st);
		return NULL;
		}
	
	st->table = (unsigned int*) shm;
	
	// Initialize table
	for(i = 0; i < players; i++)
		st->table[i] = 0;
	
	return st;
}

/* Destroys the allocated partners_table and detaches share
 * memory. This function SHOULD NOT be called in main
 * process (instead use function below).*/
void score_table_destroy(score_table_t* st){
	if(!st) return;
	lock_destroy(st->lock);
	// Detaches shared memory 
	shmdt((void*) st->table);
	free(st);
}

/* Destroys the partners table and also the shared memory
 * used inside it. This function is to replace the destroy
 * function for main process, and only main process should
 * call it.*/
void score_table_free_table(score_table_t* st){
	if(!st) return;

	int idshm = st->shmid;
	score_table_destroy(st);
	// Au revoir shared memory:
	shmctl(idshm, IPC_RMID, NULL);
}

/* Returns the received player's score. If the received player
 * isn't on the score table, returns -1.*/
unsigned int get_player_score(score_table_t* st, size_t player_id){
	if(!st) return -1;
	if(!START_AT_ZERO)	player_id--;
	
	if(player_id >= st->players_amount)
		return -1;

	lock_acquire(st->lock);
	unsigned int res = st->table[player_id];
	lock_release(st->lock);
	return res;
}

/* Increase the received player's score by the amount provided.
 * If such player isn't on the score table, silently does nothing.*/
void increase_player_score(score_table_t* st, size_t player_id, int amount){
	if(!st) return;
	if(!START_AT_ZERO)	player_id--;
	
	if(player_id >= st->players_amount)
		return;

	lock_acquire(st->lock);
	st->table[player_id] += amount;
	lock_release(st->lock);
}

#define TABLE_LINE_LENGTH 50

/* Print the received score_table on log.*/
void score_table_print(score_table_t* st){
	int i = 0, j;
	if(!START_AT_ZERO) i++;
	char* mega_string = malloc(sizeof(char) * TABLE_LINE_LENGTH * st->players_amount + 1);
	if(!mega_string) return;
	
	sprintf(mega_string, "\n--- Player %03d has a total score of %d ---", i, get_player_score(st, i));
	for(j = 1; j < st->players_amount; j++){
		i++;
		char aux_buff[TABLE_LINE_LENGTH];
		sprintf(aux_buff, "\n--- Player %03d has a total score of %d ---", i, get_player_score(st, i));
		strcat(mega_string, aux_buff);
	}
	
	strcat(mega_string, "\n");
	log_write(NONE_L, mega_string);
	free(mega_string);
}
