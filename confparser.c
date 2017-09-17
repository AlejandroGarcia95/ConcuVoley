#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "confparser.h"

/* Stores the value of the parameter recieved
 * into the respective field of struct conf sc.*/
void parse_value(struct conf* sc, char* param, size_t p_value){
	if((!sc) || (!param)) return;
	
	if(strcmp(param, "F") == 0) {
		sc->rows = p_value;
		return;
		}
		
	if(strcmp(param, "K") == 0) {
		sc->matches = p_value;
		return;
		}
		
	if(strcmp(param, "C") == 0) {
		sc->cols = p_value;
		return;
		}			
		
	if(strcmp(param, "M") == 0) {
		sc->capacity = p_value;
		return;
		}
}

/* Reads configuration file and stores its
 * contents at sc. Returns true if the operation
 * was successful, false otherwise. Take note
 * that if any parameter from struct conf is
 * missing at the configuration file, the value
 * returned will be false, even if other fields
 * were successfully read.*/
bool read_conf_file(struct conf* sc){
	FILE *pf = fopen(CONF_ROUTE, "r");
	if(!pf) return false;
	int parsed_amount = 0, r = 8;
	// For every line in pf, parse its value
	while(r > 0){
		char param[15];
		size_t p_value;
		r = fscanf(pf, "%s : %d\n", param, &p_value);
		if(r == 2) {
			parse_value(sc, param, p_value);
			parsed_amount++;
			}
		}
	fclose(pf);	
	// Check all fields were correctly parsed
	int expected_amount = sizeof(struct conf) / sizeof(size_t);
	if(expected_amount == parsed_amount)
		return true;
	printf("Error! Missing parameters or conf file format invalid\n");
	return false;
}
