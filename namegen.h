#ifndef NAMEGEN_H
#define NAMEGEN_H

#include <stdlib.h>
#include <string.h>

#define MAX_LEGTH_HALFNAME 24
#define MAX_LENGTH_NAME 50
#define NAMES_AMMOUNT 15
#define SURNAMES_AMMOUNT 10

/* Random name generator for the players
 * names. Because I really had nothing
 * better to do*/

/* Generates a random name of at most 
 * MAX_LENGTH_NAME chars and writes it 
 * to the buffer. Beware of overflows.*/
void generate_random_name(char* buffer);

#endif
