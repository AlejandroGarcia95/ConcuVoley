#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "partners_table.h"

// Set this flag if players ids start ar 0; clear it if ids start at 1
#define START_AT_ZERO 1

// TODO: Change this so the table uses shared memory and a giant lock

/* Dinamically allocates a new partners_table based on the 
 * amount of players received. Returns NULL on error.*/
partners_table_t* partners_table_create(size_t players){
	partners_table_t* pt = malloc(sizeof(partners_table_t));
	if(!pt) return NULL;
	
	pt->players_amount = players;
	pt->table = malloc(sizeof(bool) * players * players);
	if(!pt->table){
		free(pt);
		return NULL;
	}
	
	// Initialize table
	int i, j;
	for(i = 0; i < players; i++)
		for(j = 0; j < players; j++)
			pt->table[i * players + j] = false;
	
	return pt;
}

/* Destroys the allocated partners_table.*/
void partners_table_destroy(partners_table_t* pt){
	free(pt->table);
	free(pt);
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

	return pt->table[p1_id * pt->players_amount + p2_id];
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

	
	pt->table[p1_id * pt->players_amount + p2_id] = true;
	pt->table[p2_id * pt->players_amount + p1_id] = true;
}
