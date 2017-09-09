#include <stdlib.h>
#include <string.h>
#include "namegen.h"

/* Generates a random name of at most 
 * MAX_LENGTH_NAME chars and writes it 
 * to the buffer. Beware of overflows.*/
void generate_random_name(char* buffer){
	// Hardcoded names and surnames
	char names[NAMES_AMMOUNT][MAX_LEGTH_HALFNAME] = {
	"Eddard", "Robert", "Stannis", "Tyrion", "Jamie", 
	"Cersei", "Jon", "Brandom", "Daennerys", "Ben",
	"Jorah", "Arya", "Catelyn", "Gregor", "Sansa"
	};

	char surnames[SURNAMES_AMMOUNT][MAX_LEGTH_HALFNAME] = {
	"Stark", "Baratheon", "Lannister", "Targaryen", 
	"Mormont", "Tyrell", "Snow", "Cleganne", 
	"Tarly", "Grayjoy"
	};
	// Random actual name writing
	unsigned int n = rand() % NAMES_AMMOUNT;
	unsigned int s = rand() % SURNAMES_AMMOUNT;
	char *name, *surname;
	name = &names[n][0];
	surname = &surnames[s][0];
	strcpy(buffer, name);
	strcat(buffer, " ");
	strcat(buffer, surname);
}
