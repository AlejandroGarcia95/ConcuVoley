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

// #include "longstrider.h"

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
	signal(SIGTERM, SIG_IGN);
	
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

	// Court flood semaphores
	sem = sem_get("tide.c", (sc.rows * sc.cols));
	if (sem < 0) {
		log_write(ERROR_L, "Main: Error creating semaphore [errno: %d]\n", errno);
		return -1;
	}
	log_write(INFO_L, "Main: Got semaphore %d with name %s\n", sem, "court.c");
	if (sem_init_all(sem, (sc.rows * sc.cols), 0) < 0) {
		log_write(ERROR_L, "Main: Error initializing the semaphore [errno: %d]\n", errno);
		return -1;
	}
	tm->tm_data->tm_courts_flood_sem = sem;

	// Players semaphores
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
	
	// Tournament init semaphores
	sem = sem_get("tournament.c", 2);
	if (sem < 0) {
		log_write(ERROR_L, "Main: Error creating semaphore [errno: %d]\n", errno);
		return -1;
	}
	log_write(INFO_L, "Main: Got semaphore %d with name %s\n", sem, "tournament.c");
	if (sem_init(sem, 0, 0) < 0) {
		log_write(ERROR_L, "Main: Error initializing the semaphore [errno: %d]\n", errno);
		return -1;
	}
	if (sem_init(sem, 1, 0) < 0) {
		log_write(ERROR_L, "Main: Error initializing the semaphore [errno: %d]\n", errno);
		return -1;
	}
	tm->tm_data->tm_init_sem = sem;
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


#define PRINTCOLOR
void print_tournament_results(tournament_t* tm) {
	lock_acquire(tm->tm_lock);
	log_write(STAT_L, "Useful information about the tournament overall\n");

	int i, j;
	int color;
	
	int matches_completed = 0;
	int max_score = 0;
	log_write(STAT_L, "Player information!\n");
	for (i = 0; i < tm->total_players; i++) {
		player_data_t pd = tm->tm_data->tm_players[i];
		color = (pd.player_pid % 20) * 2 + 1;
		log_write(STAT_L, "\t\x1b[1;38;5;%dm - Player %03d, %s (had pid %d)\n", color, i, pd.player_name, pd.player_pid);
		log_write(STAT_L, "\t\t\x1b[1;38;5;%dm %d matches finished:\n", color, pd.player_num_matches);
		// Statistics
		matches_completed += pd.player_num_matches;
		for (j = 0; j < pd.player_num_matches; j++) {
			match_data_t md = pd.player_matches[j];
			log_write(STAT_L, "\t\t\x1b[1;38;5;%dm %03d & %03d (%d) VS (%d) %03d & %03d at court %03d\n", color,
					md.match_players[0], md.match_players[1], md.match_score[0],
					md.match_score[1], md.match_players[2], md.match_players[3], 
					md.match_played_at);
		}
	}

	log_write(STAT_L, "Matches completed: %d\n", matches_completed/PLAYERS_PER_MATCH);
	// Get max score
	for(i = 0; i < tm->total_players; i++) {
		int p_score = get_player_score(tm->tm_data->st, i);
		if(p_score > max_score) max_score = p_score;
	}

	// Get all players with that score
	for(i = 0; i < tm->total_players; i++) {
		int p_score = get_player_score(tm->tm_data->st, i);
		char* p_name = tm->tm_data->tm_players[i].player_name;
		if(p_score == max_score)
			log_write(STAT_L, "\x1b[5m CONGRATULATIONS PLAYER %03d, %s, FOR WINNING (score: %d)\n", i, p_name, p_score);
	}
	
	lock_release(tm->tm_lock);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * 
 * 		MAIN DOWN HERE
 * 
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


int main(int argc, char **argv){
	
	// longstrider_write("Main", "start");
	
	// "Remember who you are and where you come from; 
	// otherwise, you don't know where you are going."
	pid_t main_pid = getpid();
	
	struct conf sc = {};
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
	if(getpid() != main_pid){
		assert(false);
		// Sanity check
	}

	lock_acquire(tm->tm_lock);
	int sem_start = tm->tm_data->tm_init_sem;
	lock_release(tm->tm_lock);

	// Lock until at least MIN_PLAYERS_TO_START have posted on the semaphore
	// (so the tournament starts when that many players have joined)
	sem_take(sem_start, 0, MIN_PLAYERS_TO_START);
	sem_put(sem_start, 1, sc.capacity);

	bool courts_waken = false;
	bool cut_condition = false;

	for (i = 0; i < (sc.players + tm->total_courts + 1); i++) {
		int status;
		int pid = wait(&status);
		int ret = WEXITSTATUS(status);
		log_write(INFO_L, "Main: Proccess pid %d finished with exit status %d\n", pid, ret);
		
		lock_acquire(tm->tm_lock);
		int players_alive = tm->tm_data->tm_active_players;
		lock_release(tm->tm_lock);

		// Different cut condition based on player amount
		if(sc.players > 20)
			cut_condition = (players_alive <= (sc.players * 0.2));
		else
			cut_condition = (players_alive < 4);

		if(cut_condition && (!courts_waken)) {
			courts_waken = true;
			log_write(INFO_L, "Main: Enough matches performed. Terminating processes!\n");
			kill(0, SIGTERM);
		}
	}

	log_write(STAT_L, "Main: Tournament ended correctly \\o/\n");
	print_tournament_results(tm);

	partners_table_free_table(pt);
	tournament_free(tm);
	score_table_free_table(st);		

	log_close();
	// longstrider_write("Main", "finish");
	return 0;
}
