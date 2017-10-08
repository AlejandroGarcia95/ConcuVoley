#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "lock.h"

#include "partners_table.h"

// Set this flag if players ids start ar 0; clear it if ids start at 1
#define START_AT_ZERO 1

// TODO: Add a lock to this (just uncomment the locks calls)

/* Dinamically allocates a new partners_table based on the 
 * amount of players received. Returns NULL on error.*/
partners_table_t* partners_table_create(size_t players){
	key_t key = ftok("makefile", 6);
	if(key < 0) return NULL;
	
	partners_table_t* pt = malloc(sizeof(partners_table_t));
	if(!pt) return NULL;
	
	pt->players_amount = players;
	
	pt->lock = lock_create("partners_table");
	if(!pt->lock){
		free(pt);
		return NULL;
	}
	
	// Create table as shared memory
	pt->shmid = shmget(key, sizeof(bool) * players * players, IPC_CREAT | 0644);
	if(pt->shmid < 0){
		lock_destroy(pt->lock);
		free(pt);
		return NULL;
		}
	
	void* shm = shmat(pt->shmid, NULL, 0);
	if(shm == (void*) -1) {
		lock_destroy(pt->lock);
		free(pt);
		return NULL;
		}
	
	pt->table = (bool*) shm;
	
	// Initialize table
	int i, j;
	for(i = 0; i < players; i++)
		for(j = 0; j < players; j++)
			pt->table[i * players + j] = false;
	
	return pt;
}

/* Destroys the allocated partners_table and detaches share
 * memory. This function SHOULD NOT be called in main
 * process (instead use function below).*/
void partners_table_destroy(partners_table_t* pt){
	if(!pt) return;
	lock_destroy(pt->lock);
	// Detaches shared memory 
	shmdt((void*) pt->table);
	free(pt);
}

/* Destroys the partners table and also the shared memory
 * used inside it. This function is to replace the destroy
 * function for main process, and only main process should
 * call it.*/
void partners_table_free_table(partners_table_t* pt){
	if(!pt) return;

	int idshm = pt->shmid;
	partners_table_destroy(pt);
	// Au revoir shared memory:
	shmctl(idshm, IPC_RMID, NULL);
}

/* Checks if the two players received already played together.
 * Returns true if that was the case, or false otherwise. If
 * any of the players' id is greater than players_amount, it
 * just returns false.*/
bool get_played_together(partners_table_t* pt, size_t p1_id, size_t p2_id){
	if(!pt) return false;
	if(!START_AT_ZERO) {
		p1_id--;
		p2_id--;
	}
	if((p1_id >= pt->players_amount) || (p2_id >= pt->players_amount))
		return false;

	lock_acquire(pt->lock);
	bool res = pt->table[p1_id * pt->players_amount + p2_id];
	lock_release(pt->lock);
	return res;
}

/* Mark in the partners table the two players received (i.e.
 * they've already played together as partners). If any of
 * the players' id is greater than players_amount, silently
 * does nothing.*/
void set_played_together(partners_table_t* pt, size_t p1_id, size_t p2_id){
	if(!pt) return;
	if(!START_AT_ZERO) {
		p1_id--;
		p2_id--;
	}
	if((p1_id >= pt->players_amount) || (p2_id >= pt->players_amount))
		return;

	lock_acquire(pt->lock);
	pt->table[p1_id * pt->players_amount + p2_id] = true;
	pt->table[p2_id * pt->players_amount + p1_id] = true;
	lock_release(pt->lock);
}
