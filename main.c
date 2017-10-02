#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <sys/wait.h>
#include <time.h>
#include <errno.h>
#include "confparser.h"
#include "log.h"
#include "player.h"
#include "court.h"
#include "namegen.h"
#include "partners_table.h"
#include "protocol.h"

#define LOG_ROUTE "ElLog.txt"

/* Launches a new process which will assume a
 * player's role. Each player is given an id, from which
 * the fifo of the player can be obtained. After launching the new 
 * player process, this function must call the
 * player_main function to allow it to play 
 * against other in a court. */
int launch_player(unsigned int player_id, log_t* log) {

	// Note: this static variable seems not to work for some reason...
	// Really? I was sure I could test it working
	// properly before... although it ain't that dramatic
	// static int player_id = 0;

	log_write(log, INFO_L, "Launching player %03d!\n", player_id);

	// Create the fifo for the player.
	char player_fifo_name[MAX_FIFO_NAME_LEN];
	sprintf(player_fifo_name, "fifos/player_%03d.fifo", player_id);
	if(!create_fifo(player_fifo_name)) {
		log_write(log, ERROR_L, "FIFO creation error for player %03d [errno: %d]\n", player_id, errno);
		return -1;
	}

	// New process, new player!
	pid_t pid = fork();

	if (pid < 0) { // Error
		log_write(log, ERROR_L, "Fork failed!\n");
		return -1;
	} else if (pid == 0) { // Son aka player
		player_main(player_id, log);
	} else { // Father aka main process
		player_id++;
	}
	return 0;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * 
 * 		MAIN DOWN HERE
 * 
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


int main(int argc, char **argv){
	struct timeval time_start;
	gettimeofday (&time_start, NULL);
	srand(time(NULL));
	
	log_t* log = log_open(LOG_ROUTE, false, time_start);
	if(!log){
		printf("FATAL: No log could be opened!\n");
		return -1;
	}
	log_write(log, NONE_L, "Let the tournament begin!\n");

	// "Remember who you are and where you come from; 
	// otherwise, you don't know where you are going."
	pid_t main_pid = getpid();

	// Let's launch player processes and make them play, yay!
	
	// We want main process to ignore SIG_SET signal as
	// it's just for players processes
	signal(SIG_SET, SIG_IGN);
	int i, j;
	// Launch players processes
	for(i = 0; i < PLAYERS_PER_MATCH; i++){
		if (getpid() == main_pid)
			launch_player(i, log);
	}

	// The children/player processes reach here
	// once they're done playing the matches.
	// Then, if we're a player, we should die here
	if(getpid() != main_pid){
		log_close(log);
		// IMPORTANT NOTE: the use of 'return 0' is only for
		// properly testing the children with valgrind. Should
		// be changed by an exit() later.
		return 0;
	}

	// Create table of partners
	partners_table_t* pt = partners_table_create(PLAYERS_PER_MATCH);

	// Launch a single court, then end the tournament
	court_t* court = court_create(log, pt);
	
	for (j = 0; j < NUM_MATCHES; j++) 
		court_lobby(court, log);
	
	for (i = 0; i < PLAYERS_PER_MATCH; i++) 
		log_write(log, INFO_L, "Player %03d won a total of %d matches\n", i, court->player_points[i]);

	court_destroy(court);		
	log_close(log);
	return 0;
}

