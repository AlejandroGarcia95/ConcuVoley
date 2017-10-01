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
#include "match.h"
#include "namegen.h"

#define LOG_ROUTE "ElLog.txt"

/* Launches a new process which will assume a
 * player's role. Each player is given a sort
 * of bidirectional pipe for comunicating with
 * the main process. After launching the new 
 * player process, this function must call the
 * do_in_player function to allow it to play 
 * against other in a match. Note the main
 * process can comunicate with a bidirectional
 * pipe of his own.*/
int launch_player(int this_player_id, log_t* log){
	//static int this_player_id = 0; // useless by the time being, but helpful in the future
	log_write(log, INFO_L, "Launching player %d!\n", this_player_id);

	// Create and open the fifo for the player.
	char* player_fifo_name = malloc(sizeof(char) * 20);
	sprintf(player_fifo_name, "fifos/player_%d.fifo", this_player_id);

	if (mknod(player_fifo_name, S_IFIFO | 0666, 0) < 0) {
		log_write(log, ERROR_L, "Fifo creation (mknod) failed (payer %d) - %d!\n", this_player_id, errno);
		return -1;
	}
	/*
	int new_fifo_fd = open(player_fifo_name, O_WRONLY);
	if (new_fifo_fd < 0) {
		log_write(log, ERROR_L, "Fifo opening failed (payer %d), %d!\n", this_player_id, errno);
		return -1;
	}
	*/
	free(player_fifo_name);

	// New process, new player!
	pid_t pid = fork();

	if (pid < 0) { // Error
		log_write(log, ERROR_L, "Fork failed!\n");
		return -1;
	} else if(pid == 0) { // Son aka player
		player_main(this_player_id, log);
	} else { // Father aka main process
		this_player_id++;
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
		if(getpid() == main_pid)
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
	
	match_t* match = match_create(true, log);
	
	match_play(match, log);
	
	match_destroy(match);		
	log_close(log);
	return 0;
}

