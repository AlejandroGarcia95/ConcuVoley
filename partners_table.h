#ifndef PARTNERS_TABLE_H
#define PARTNERS_TABLE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "lock.h"

// Set this flag if players ids start ar 0; clear it if ids start at 1
#define START_AT_ZERO 1

typedef struct partners_table_ {
	size_t players_amount;
	int shmid;
	bool* table;
	lock_t* lock;
} partners_table_t;

/* Dinamically allocates a new partners_table based on the 
 * amount of players received. Returns NULL on error.*/
partners_table_t* partners_table_create(size_t players);

/* Destroys the allocated partners_table and detaches share
 * memory. This function SHOULD NOT be called in main
 * process (instead use function below).*/
void partners_table_destroy(partners_table_t* pt);

/* Destroys the partners table and also the shared memory
 * used inside it. This function is to replace the destroy
 * function for main process, and only main process should
 * call it.*/
void partners_table_free_table(partners_table_t* pt);

/* Checks if the two players received already played together.
 * Returns true if that was the case, or false otherwise. If
 * any of the players' id is greater than players_amount, it
 * just returns false.*/
bool get_played_together(partners_table_t* pt, size_t p1_id, size_t p2_id);

/* Mark in the partners table the two players received (i.e.
 * they've already played together as partners). If any of
 * the players' id is greater than players_amount, silently
 * does nothing.*/
void set_played_together(partners_table_t* pt, size_t p1_id, size_t p2_id);

#endif
