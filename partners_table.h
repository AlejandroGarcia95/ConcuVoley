#ifndef PARTNERS_TABLE_H
#define PARTNERS_TABLE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>


typedef struct partners_table_ {
	size_t players_amount;
	bool* table;
} partners_table_t;

/* Dinamically allocates a new partners_table based on the 
 * amount of players received. Returns NULL on error.*/
partners_table_t* partners_table_create(size_t players);

/* Destroys the allocated partners_table.*/
void partners_table_destroy(partners_table_t* pt);

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
