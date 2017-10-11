#ifndef TIDE_H
#define TIDE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "tournament.h"
#include "confparser.h"

#define TIDE_FILE "tide.txt"


typedef struct tide_ {
	FILE *pf;
} tide_t;

/* Returns the current tide singleton!*/
tide_t* tide_get_instance();

/* Destroys the current tide.*/
void tide_destroy();

/* Executes main for this process. Finishes via exit(0)*/
void tide_main(tournament_t* tm, struct conf sc);

#endif
