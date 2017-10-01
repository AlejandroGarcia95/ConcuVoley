#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h> 
#include <unistd.h>
#include <assert.h>
#include <sys/wait.h>
#include <time.h>
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
int launch_player(int pipes[2], log_t* log){
	static int this_player_id = 0; // useless by the time being, but helpful in the future

	// Create pipes between player and main process
	int fd_main_to_player[2], fd_player_to_main[2];
	if(pipe(fd_main_to_player) < 0){
		log_write(log, ERROR_L, "Pipe creation failed!\n");
		return -1;
	}

	if(pipe(fd_player_to_main) < 0){
		log_write(log, ERROR_L, "Pipe creation failed!\n");
		close(fd_main_to_player[PIPE_WRITE]);
		close(fd_main_to_player[PIPE_READ]);
		return -1;
	}
	// New process, new player!
	pid_t pid = fork();

	if(pid < 0){ // Error
		close(fd_player_to_main[PIPE_WRITE]);
		close(fd_player_to_main[PIPE_READ]);
		close(fd_main_to_player[PIPE_WRITE]);
		close(fd_main_to_player[PIPE_READ]);
		return -1;
	}
	else if(pid == 0){ // Son aka player
		close(fd_main_to_player[PIPE_WRITE]);
		close(fd_player_to_main[PIPE_READ]);
		int player_pipes[2];
		player_pipes[PIPE_WRITE] = fd_player_to_main[PIPE_WRITE];
		player_pipes[PIPE_READ] = fd_main_to_player[PIPE_READ];
		do_in_player(player_pipes, log);
	}
	else { // Father aka main process
		close(fd_main_to_player[PIPE_READ]);
		close(fd_player_to_main[PIPE_WRITE]);
		pipes[PIPE_WRITE] = fd_main_to_player[PIPE_WRITE];
		pipes[PIPE_READ] = fd_player_to_main[PIPE_READ];
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
	int pipes[PLAYERS_PER_MATCH][2] = {};
	
	// We want main process to ignore SIG_SET signal as
	// it's just for players processes
	signal(SIG_SET, SIG_IGN);
	
	int i, j;
	// Launch players processes
	for(i = 0; i < PLAYERS_PER_MATCH; i++){
		if(getpid() == main_pid)
			launch_player(&pipes[i][0], log);	
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
	
	match_t* match = match_create(pipes, true);
	
	match_play(match, log);
	
	match_destroy(match);		
	log_close(log);
	return 0;
}

