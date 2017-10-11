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
#include "score_table.h"
#include "tournament.h"
#include "tide.h"

/* Returns negative in case of error!*/
int main_init(tournament_t* tm, struct conf sc){
	srand(time(NULL));
	// Spawn log
	if(log_write(NONE_L, "Main: Simulation started!\n") < 0){
		printf("FATAL: No log could be opened!\n");
		return -1;
	}
	log_set_debug_mode(sc.debug);
	
	log_write(INFO_L, "Main: Self pid is %d\n", getpid());
	
	// We want processes to ignore SIG_SET and
	// SIG_TIDE signals unless we set it explicitily
	signal(SIG_SET, SIG_IGN);
	signal(SIG_TIDE, SIG_IGN);
	
	// Create every player FIFO
	int i;
	for(i = 0; i < sc.players; i++){
		char player_fifo_name[MAX_FIFO_NAME_LEN];
		get_player_fifo_name(i, player_fifo_name);
		// Create the fifo for the player.
		if(!create_fifo(player_fifo_name)) {
			log_write(ERROR_L, "Main: FIFO creation error for player %03d [errno: %d]\n", i, errno);
			return -1;
		}
	}
	
	// Create every court FIFO
	for(i = 0; i < (sc.rows * sc.cols); i++){
		char court_fifo_name[MAX_FIFO_NAME_LEN];
		get_court_fifo_name(i, court_fifo_name);
		// Creating fifo for court
		if(!create_fifo(court_fifo_name)) {
			log_write(ERROR_L, "Main: FIFO creation error for court %03d [errno: %d]\n", i, errno);
			return -1;
		}
	}
	
	// Court semaphores
	int sem = sem_get("court.c", (sc.rows * sc.cols));
	if (sem < 0) {
		log_write(ERROR_L, "Main: Error creating semaphore [errno: %d]\n", errno);
		return -1;
	}
	log_write(INFO_L, "Main: Got semaphore %d with name %s\n", sem, "court.c");
	if (sem_init_all(sem, (sc.rows * sc.cols), 0) < 0) {
		log_write(ERROR_L, "Main: Error initializing the semaphore [errno: %d]\n", errno);
		return -1;
	}
	tm->tm_data->tm_courts_sem = sem;

	// PLayers semaphores
	sem = sem_get("player.c", sc.players);
	if (sem < 0) {
		log_write(ERROR_L, "Main: Error creating semaphore [errno: %d]\n", errno);
		return -1;
	}
	log_write(INFO_L, "Main: Got semaphore %d with name %s\n", sem, "player.c");
	if (sem_init_all(sem, sc.players, 0) < 0) {
		log_write(ERROR_L, "Main: Error initializing the semaphore [errno: %d]\n", errno);
		return -1;
	}
	tm->tm_data->tm_players_sem = sem;
}

/* Launches a new process which will assume a
 * player's role. Each player is given an id, from which
 * the fifo of the player can be obtained. After launching the new 
 * player process, this function must call the
 * player_main function to allow it to play 
 * against other in a court. */
int launch_player(unsigned int player_id, tournament_t* tm) {
	
	log_write(INFO_L, "Main: Launching player %03d!\n", player_id);

	// New process, new player!
	pid_t pid = fork();

	if (pid < 0) { // Error
		log_write(CRITICAL_L, "Main: Fork failed!\n");
		return -1;
	} else if (pid == 0) { // Son aka player
		player_main(player_id, tm);
		assert(false);	// Should not return!
	} 
	return 0;
}



int launch_court(unsigned int court_id, tournament_t* tm) {
	log_write(INFO_L, "Main: Launching court %03d!\n", court_id);

	// New process, new court!
	pid_t pid = fork();

	if (pid < 0) { // Error
		log_write(CRITICAL_L, "Main: Fork failed!\n");
		return -1;
	} else if (pid == 0) { // Son aka court
		court_main(court_id, tm);
		assert(false); // Should not return!
	}
	return 0;
}

int launch_tide(tournament_t* tm, struct conf sc) {
	log_write(INFO_L, "Main: Launching tide process!\n");

	pid_t pid = fork();

	if (pid < 0) { // Error
		log_write(CRITICAL_L, "Main: Fork failed!\n");
		return -1;
	} else if (pid == 0) { // Son aka tide
		tide_main(tm, sc);
		assert(false); // Should not return!
	}
	return 0;
}


// Debug only!
void print_tournament_status(tournament_t* tm) {
	lock_acquire(tm->tm_lock);
	int i;
	log_write(INFO_L, "Main: %d players remain active!\n", tm->tm_data->tm_active_players);
	for (i = 0; i < tm->total_courts; i++) {
		court_data_t cd = tm->tm_data->tm_courts[i];
		log_write(INFO_L, "Main: Court %03d is in state %d, with %d players inside\n", i, cd.court_status, cd.court_num_players);
		int j;
		for (j = 0; j < cd.court_num_players; j++) {
			log_write(INFO_L, "\t\t\t---> Player %03d is inside\n", cd.court_players[j]);
		}
	}
	lock_release(tm->tm_lock);
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
	
	struct conf sc;
	if(!read_conf_file(&sc)){
		printf("FATAL: Error parsing configuration file [errno: %d]\n", errno);
		return -1;		
	}
	
	tournament_t* tm = tournament_create(sc);
	if (!tm) {
		printf("FATAL: Error creating tournament data [errno: %d]\n", errno);
		return -1;
	}

	if(main_init(tm, sc) < 0) {
		printf("FATAL: Something went really wrong!\n");
		return -1;
	}

	log_write(NONE_L, "Main: Let the tournament begin!\n");

	// Let's launch player processes and make them play, yay!
	int i, j;

	// Launch players processes
	for(i = 0; i < sc.players; i++){
		launch_player(i, tm);
	}
	
	// Launch tide
	launch_tide(tm, sc);
	
	// Create table of partners
	partners_table_t* pt = partners_table_create(sc.players);
	if(!pt) {
		log_write(CRITICAL_L, "Main: Error creating partners table [errno: %d]\n", errno);
	}

	// Create score table
	score_table_t* st = score_table_create(sc.players);
	if(!st) {
		log_write(CRITICAL_L, "Main: Error creating score table [errno: %d]\n", errno);
	}

	tournament_set_tables(tm, pt, st);

	// Launch court processes
	for (i = 0; i < tm->total_courts; i++) {
		launch_court(i, tm);
	}
	

	// No child proccess should end here
	// ALL childs must finish with a exit(status) call.
	// The only cleaning to be done by them is log_close(log)
	if(getpid() != main_pid){
		assert(false);
		// Sanity check
	}

	bool courts_waken = false;

	for (i = 0; i < (sc.players + tm->total_courts + 1); i++) {
		int status;
		int pid = wait(&status);
		int ret = WEXITSTATUS(status);
		log_write(INFO_L, "Main: Proccess pid %d finished with exit status %d\n", pid, ret);
		print_tournament_status(tm);
		
		lock_acquire(tm->tm_lock);
		int players_alive = tm->tm_data->tm_active_players;
		lock_release(tm->tm_lock);
		if((players_alive < 4) && (!courts_waken)) {
			courts_waken = true;
			log_write(CRITICAL_L, "Main: No more matches can be performed. Waking up courts!.\n");
			for(j = 0; j < tm->total_courts; j++)
				sem_post(tm->tm_data->tm_courts_sem, j);
			break;
		}
	}

	print_tournament_status(tm);
	partners_table_free_table(pt);
	tournament_free(tm);
	score_table_free_table(st);		

	log_write(INFO_L, "Main: Tournament ended correctly \\o/\n");
	log_close();
	return 0;
}

