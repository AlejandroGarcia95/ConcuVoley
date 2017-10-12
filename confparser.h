#ifndef CONFPARSER_H
#define CONFPARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define CONF_ROUTE "conf.txt"

/* struct conf: an aux structure for parsing the
 * configuration file at path CONF_ROUTE. */
struct conf {
	size_t rows;
	size_t cols;
	size_t matches;
	size_t capacity;
	size_t players;
	bool debug;
};

/* Stores the value of the parameter received
 * into the respective field of struct conf sc.*/
void parse_value(struct conf* sc, char* param, size_t p_value);

/* Reads configuration file and stores its
 * contents at sc. Returns true if the operation
 * was successful, false otherwise. Take note
 * that if any parameter from struct conf is
 * missing at the configuration file, the value
 * returned will be false, even if other fields
 * were successfully read.*/
bool read_conf_file(struct conf* sc);

#endif
