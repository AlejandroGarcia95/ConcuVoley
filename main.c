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
#include "referee.h"

/* Returns negative in case of error!*/
int main_init(){
	srand(time(NULL));
	// Spawn log
	if(log_write(NONE_L, "Main: Simulation started!\n") < 0){
		printf("FATAL: No log could be opened!\n");
		return -1;
	}
	log_write(INFO_L, "Main: Self pid is %d\n", getpid());
	
	// We want processes to ignore SIG_SET signal unless
	// we set it explicitily
	signal(SIG_SET, SIG_IGN);
	
	// Create every player FIFO
	int i;
	for(i = 0; i < TOTAL_PLAYERS; i++){
		char player_fifo_name[MAX_FIFO_NAME_LEN];
		get_player_fifo_name(i, player_fifo_name);
		// Create the fifo for the player.
		if(!create_fifo(player_fifo_name)) {
			log_write(ERROR_L, "Main: FIFO creation error for player %03d [errno: %d]\n", i, errno);
			return -1;
		}
	}
	
	// Create every court FIFO
	for(i = 0; i < TOTAL_COURTS; i++){
		char court_fifo_name[MAX_FIFO_NAME_LEN];
		get_court_fifo_name(i, court_fifo_name);
		// Creating fifo for court
		if(!create_fifo(court_fifo_name)) {
			log_write(ERROR_L, "Main: FIFO creation error for court %03d [errno: %d]\n", i, errno);
			return -1;
	}
	}
	
	// Start semaphores for courts. Every court is responsible for destroying own semaphores
	unsigned int cid = 0;
	// for every court cid in (1, court_max) do:
		char court_fifo_name[MAX_FIFO_NAME_LEN];
		get_court_fifo_name(cid, court_fifo_name);
		int sem = sem_get(court_fifo_name, 1);
		if (sem < 0) {
			log_write(ERROR_L, "Main: Error creating semaphore [errno: %d]\n", errno);
			return -1;
		}

		log_write(INFO_L, "Main: Got semaphore %d with name %s\n", sem, court_fifo_name);
		if (sem_init(sem, 0, 0) < 0) {
			log_write(ERROR_L, "Main: Error initializing the semaphore [errno: %d]\n", errno);
			return -1;
		}
}

/* Launches a new process which will assume a
 * player's role. Each player is given an id, from which
 * the fifo of the player can be obtained. After launching the new 
 * player process, this function must call the
 * player_main function to allow it to play 
 * against other in a court. */
int launch_player(unsigned int player_id) {
	
	log_write(INFO_L, "Main: Launching player %03d!\n", player_id);

	// New process, new player!
	pid_t pid = fork();

	if (pid < 0) { // Error
		log_write(ERROR_L, "Main: Fork failed!\n");
		return -1;
	} else if (pid == 0) { // Son aka player
		player_main(player_id);
		assert(false);	// Should not return!
	} 
	return 0;
}



// TODO: make so it is not necesary to pass pointer to partners table
int launch_court(unsigned int court_id, partners_table_t* pt) {
	log_write(INFO_L, "Main: Launching court %03d!\n", court_id);

	// New process, new court!
	pid_t pid = fork();

	if (pid < 0) { // Error
		log_write(ERROR_L, "Main: Fork failed!\n");
		return -1;
	} else if (pid == 0) { // Son aka court
		court_main(court_id, pt);
		assert(false); // Should not return!
	}
	return 0;

}




int launch_referee(partners_table_t* pt) {
	log_write(INFO_L, "Main: Launching referee!\n");

	pid_t pid = fork();

	if (pid < 0) {
		log_write(ERROR_L, "Main: Fork failed!\n");
		return -1;
	} else if (pid == 0) {
		referee_main(pt);
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
	// "Remember who you are and where you come from; 
	// otherwise, you don't know where you are going."
	pid_t main_pid = getpid();
	
	if(main_init() < 0) {
		printf("FATAL: Something went really wrong!\n");
		return -1;
	}
		

	log_write(NONE_L, "Main: Let the tournament begin!\n");

	// Let's launch player processes and make them play, yay!
	int i;

	// Launch players processes
	for(i = 0; i < TOTAL_PLAYERS; i++){
		launch_player(i);
	}
	
	// Create table of partners
	partners_table_t* pt = partners_table_create(TOTAL_PLAYERS);
	if(!pt) {
		log_write(ERROR_L, "Main: Error creating partners table [errno: %d]\n", errno);
	}

	// Launch court processes
	for (i = 0; i < TOTAL_COURTS; i++) {
		launch_court(i, pt);
	}
	
	// We create a referee to administrate the tournament!
	// launch_referee(pt);

	// No child proccess should end here
	// ALL childs must finish with a exit(status) call.
	// The only cleaning to be done by them is log_close(log)
	if(getpid() != main_pid){
		assert(false);
		//log_write(INFO_L, "Proccess pid %d about to finish correctly \n", getpid());
		//court_destroy(court);
		//partners_table_destroy(pt);	
		//log_close(log);
		//return 0;
	}


	// Important note: Here main process is not waiting for referee!
	for (i = 0; i < (TOTAL_PLAYERS + TOTAL_COURTS); i++) {
		int status;
		int pid = wait(&status);
		int ret = WEXITSTATUS(status);
		log_write(INFO_L, "Main: Proccess pid %d finished with exit status %d\n", pid, ret);
	}

	partners_table_free_table(pt);		

	log_write(INFO_L, "Main: Tournament ended correctly \\o/\n");
	log_close();
	return 0;
}

