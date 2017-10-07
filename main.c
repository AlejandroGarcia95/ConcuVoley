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
#include <errno.h>
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
#include "semaphore.h"

#define LOG_ROUTE "ElLog.txt"



/* Launches a new process which will assume a
 * player's role. Each player is given an id, from which
 * the fifo of the player can be obtained. After launching the new 
 * player process, this function must call the
 * player_main function to allow it to play 
 * against other in a court. */
int launch_player(unsigned int player_id, log_t* log) {
	
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
		assert(false);	// Should not return!
	} 
	return 0;
}



// TODO: make so it is not necesary to pass pointer to partners table
int launch_court(unsigned int court_id, log_t* log, partners_table_t* pt) {
	log_write(log, INFO_L, "Launching court %03d!\n", court_id);

	// New process, new court!
	pid_t pid = fork();

	if (pid < 0) { // Error
		log_write(log, ERROR_L, "Fork failed!\n");
		return -1;
	} else if (pid == 0) { // Son aka court
		court_main(court_id + 55, log, pt);
		assert(false); // Should not return!
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
	
	// "Remember who you are and where you come from; 
	// otherwise, you don't know where you are going."
	pid_t main_pid = getpid();
	
	// TODO: refactor this, extract in init() function
	log_t* log = log_open(LOG_ROUTE, false, time_start);
	if(!log){
		printf("FATAL: No log could be opened!\n");
		return -1;
	}
	log_write(log, INFO_L, "Main process (pid: %d) succesfully created log \n", main_pid);
	log_write(log, NONE_L, "Let the tournament begin!\n");


	// Create table of partners
	partners_table_t* pt = partners_table_create(TOTAL_PLAYERS);
	if(!pt) {
		log_write(log, ERROR_L, "Error creating partners table [errno: %d]\n", errno);
	}

	// Let's launch player processes and make them play, yay!
	
	// We want main process to ignore SIG_SET signal as
	// it's just for players processes
	signal(SIG_SET, SIG_IGN);
	int i;
	// Launch court processes
	for (i = 0; i < TOTAL_COURTS; i++) {
		launch_court(i, log, pt);
	}

	// Launch players processes
	for(i = 0; i < TOTAL_PLAYERS; i++){
		launch_player(i, log);
	}

	// No child proccess should end here
	// ALL childs must finish with a exit(status) call.
	// The only cleaning to be done by them is log_close(log)
	if(getpid() != main_pid){
		assert(false);
		//log_write(log, INFO_L, "Proccess pid %d about to finish correctly \n", getpid());
		//court_destroy(court);
		//partners_table_destroy(pt);	
		//log_close(log);
		//return 0;
	}
	

	for (i = 0; i < (TOTAL_PLAYERS + TOTAL_COURTS); i++) {
		int status;
		int pid = wait(&status);
		int ret = WEXITSTATUS(status);
		log_write(log, INFO_L, "Proccess pid %d finished with exit status %d\n", pid, ret);
	}

	partners_table_free_table(pt);		

	log_write(log, INFO_L, "Tournament ended correctly \\o/\n");
	log_close(log);
	return 0;
}

