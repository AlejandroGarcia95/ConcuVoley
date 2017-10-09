#ifndef SCORE_TABLE_H
#define SCORE_TABLE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "lock.h"

// Set this flag if players ids start ar 0; clear it if ids start at 1
#define START_AT_ZERO 1

typedef struct score_table_ {
	size_t players_amount;
	int shmid;
	unsigned int* table;
	lock_t* lock;
} score_table_t;

/* Dinamically allocates a new score_table based on the 
 * amount of players received. Returns NULL on error.*/
score_table_t* score_table_create(size_t players);

/* Destroys the allocated partners_table and detaches share
 * memory. This function SHOULD NOT be called in main
 * process (instead use function below).*/
void score_table_destroy(score_table_t* st);

/* Destroys the partners table and also the shared memory
 * used inside it. This function is to replace the destroy
 * function for main process, and only main process should
 * call it.*/
void score_table_free_table(score_table_t* st);

/* Returns the received player's score. If the received player
 * isn't on the score table, returns -1.*/
unsigned int get_player_score(score_table_t* st, size_t player_id);

/* Increase the received player's score by the amount provided.
 * If such player isn't on the score table, silently does nothing.*/
void increase_player_score(score_table_t* st, size_t player_id, int amount);

/* Print the received score_table on log.*/
void score_table_print(score_table_t* st);

#endif
