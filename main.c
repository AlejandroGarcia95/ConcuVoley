#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h> 
#include <sys/shm.h> 
#include <unistd.h>
#include <assert.h>
#include <sys/wait.h>
#include <time.h>
#include "confparser.h"
#include "log.h"
#include "player.h"
#include "namegen.h"

#define LOG_ROUTE "ElLog.txt"

#define PLAYERS_FOR_NOW 2

#define SIG_SET SIGUSR1


/* Handler function for a player process.
 * It should only be used with SIG_SET
 * signal. Makes the player start or stop
 * playing the set accordingly.*/
void handler_players_set(int signum) {
	assert(signum == SIG_SET);
	// Check if set is to start or finish
	if(player_is_playing()) 
		player_stop_playing();
	else
		player_start_playing();
}

/* Function that makes the process adopt
 * a player's role. Basically, it creates
 * a player, and make them play a set.
 * The score of that player on the set is
 * stored in *set_score, assumed to be a
 * shared memory address with main process.*/
void do_in_player(unsigned long int* set_score){
	// Re-srand with a changed seed
	srand(time(NULL) ^ (getpid() << 16));
	char p_name[NAME_MAX_LENGTH];
	generate_random_name(p_name);
	
	player_t* player = player_get_instance();
	if(!player){
		shmdt(set_score);
		return;
		}
	player_set_name(p_name);
	
	// Set the hanlder for the SIG_SET signal
	struct sigaction sa;
	sigset_t sigset;	
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = handler_players_set;
	sigaction(SIG_SET, &sa, NULL);
	
	// Here the player is on "waiting for playing"
	// status... Change this later for a proper
	// thing, i.e. condition variable
	while(!player_is_playing())
		sleep(1);
	
	player_play_set(set_score);
	// When set is finished, we kill the player
	shmdt(set_score);
	player_destroy(player);
	// Beware! Returns to main!
}

/* Launches a new process which will assume a
 * player's role. Each player is given a set_score
 * memory to be shared with the main process.
 * In order to identify each player's set_score
 * the main process must keep a player_id variable
 * on this function's scope. After launching the
 * new player process, this function must call
 * the do_in_player function to allow it to
 * play against other in a match. Note the main
 * process stores the player set_score and the
 * shmid on its side.*/
int launch_player(unsigned long int** set_score, int* shmid){
	static int this_player_id = 0;
	// Get the key. Unique per player
	key_t key = ftok("makefile", this_player_id);
	// Create this player's set_score as shared memory
	if ((*shmid = shmget(key, sizeof(unsigned long int), 
						IPC_CREAT | 0644)) < 0)
        return -1;
    // Attach this player's set_score   
    *set_score = shmat(*shmid, NULL, 0);
    if(*set_score == (unsigned long int*) -1)
		return -1;
	// New process, new player!
	pid_t pid = fork();
	
	if(pid < 0){ // Error
		shmdt(*set_score);
		shmctl(*shmid, IPC_RMID, NULL);
		}
	else if(pid == 0) // Son aka player
		do_in_player(*set_score);
	else { // Father aka main process
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
	time_t time_start = time(NULL);
	srand(time_start);
	// "Remember who you are and where you come from; 
	// otherwise, you don't know where you are going."
	pid_t main_pid = getpid();


	// Test log levels
	log_t* log = log_open(LOG_ROUTE, false, time_start);
	log_write(log, NONE_L, "Testing: none\n");
	log_write(log, WARNING_L, "Testing: warning\n");
	log_write(log, ERROR_L, "Testing: ERROR\n");
	log_write(log, INFO_L, "Testing: info\n");
	log_write(log, NONE_L, "\n"); // empty line
	
	log_write(log, NONE_L, "--------------------------------------\n");
	
	// Read conf file
	struct conf sc;
	bool parse_ok = read_conf_file(&sc);
	if(parse_ok){ // Write parsed args to log
		log_write(log, NONE_L, "Field F value: %d\n", sc.rows);
		log_write(log, NONE_L, "Field C value: %d\n", sc.cols);
		log_write(log, NONE_L, "Field K value: %d\n", sc.matches);
		log_write(log, NONE_L, "Field M value: %d\n", sc.capacity);
	}
		
	log_write(log, NONE_L, "--------------------------------------\n");	
	
	
	// Let's launch two player processes and make them play, yay!
	unsigned long int* players_scores[PLAYERS_FOR_NOW] = {};
	int scores_shmid[PLAYERS_FOR_NOW] = {};
	// We want main process to ignore SIG_SET signal
	signal(SIG_SET, SIG_IGN);
	
	int i;
	// Launch players processes
	for(i = 0; i < PLAYERS_FOR_NOW; i++){
		if(getpid() == main_pid)
			launch_player(&players_scores[i], &scores_shmid[i]);	
		}
			
	// If we're a child/player, we should stop here
	if(getpid() != main_pid){
		log_close(log);
		return 0;
		}
	// From now onwards, only main process can access

	// Here we make the two players play by signaling them.
	// Parent must sleep a while to allow the players to be
	// ready. Change later for a condition variable!
	sleep(2);
	kill(0, SIG_SET);
	// Let the set last 6 seconds for now. After that, the
	// main process will make all players stop
	sleep(6);
	kill(0, SIG_SET);
	
	// Wait for the two player's deaths
	wait(NULL);
	wait(NULL);
	// Destroy players shared memory
	for(i = 0; i < PLAYERS_FOR_NOW; i++){
		log_write(log, NONE_L, "Player %d set score: %ld\n", i+1, *players_scores[i]);
		shmdt(players_scores[i]);
		shmctl(scores_shmid[i], IPC_RMID, NULL);
		}
			
	log_close(log);
	return 0;
}

