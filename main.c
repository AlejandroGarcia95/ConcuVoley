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

#define PIPE_READ 0
#define PIPE_WRITE 1

#define SETS_AMOUNT 5
#define SETS_WINNING 3

/* Handler function for a player process.
 * It should only be used with SIG_SET
 * signal. Makes the player stop playing 
 * the set if they were playing.*/
void handler_players_set(int signum) {
	assert(signum == SIG_SET);
	// Check if set is to start or finish
	if(player_is_playing()) 
		player_stop_playing();
}

/* Function that makes the process adopt
 * a player's role. Basically, it creates
 * a player, and make them play a match.
 * Every set of the match starts when the
 * main process sends a "start playing" 
 * message through the pipe, and ends when
 * the player receives a SIG_SET signal 
 * (as a set could end unexpectedly). At
 * the end of each set, the player must
 * send through the pipe their score.*/
void do_in_player(int pipes[2], log_t* log){
	// Re-srand with a changed seed
	srand(time(NULL) ^ (getpid() << 16));
	char p_name[NAME_MAX_LENGTH];
	generate_random_name(p_name);
	
	player_t* player = player_get_instance();
	if(!player){
		close(pipes[PIPE_WRITE]);
		close(pipes[PIPE_READ]);
		return;
		}
	player_set_name(p_name);
	
	log_write(log, INFO_L, "Created new player: %s\n", p_name);
	log_write(log, INFO_L, "%s skill is: %d\n", p_name, player_get_skill());
	
	// Set the hanlder for the SIG_SET signal
	struct sigaction sa;
	sigset_t sigset;	
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = handler_players_set;
	sigaction(SIG_SET, &sa, NULL);
	
	// Here the player is on "waiting for playing"
	// status. Will go on when the main process
	// sends him a message for doing so.
	char msg = 1;
	while(msg == 1){
		int msg_bytes = read(pipes[PIPE_READ], &msg, sizeof(char));
		
		if(msg_bytes < 0){
			log_write(log, ERROR_L, "Player %s: bad read\n", p_name);
			close(pipes[PIPE_WRITE]);
			close(pipes[PIPE_READ]);
			return;
			}

		// msg = 1 for now is the "start playing" message
		if(msg == 1){
			unsigned long int set_score = 0;
			// Play the set until it's done (by SIG_SET)
			log_write(log, INFO_L, "Player %s started playing\n", p_name);
			player_start_playing();
			player_play_set(&set_score);
			// When set is finished, we use the pipe to
			// send main process our set_score
			write(pipes[PIPE_WRITE], &set_score, sizeof(set_score));
			log_write(log, INFO_L, "Player %s stopped playing\n", p_name);
			}
		else
			log_write(log, INFO_L, "Player %s: wont start playing\n", p_name);
		}
	 
	close(pipes[PIPE_WRITE]);
	close(pipes[PIPE_READ]);
	player_destroy(player);
	// Beware! Returns to main!
}


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
	time_t time_start = time(NULL);
	srand(time_start);
	// "Remember who you are and where you come from; 
	// otherwise, you don't know where you are going."
	pid_t main_pid = getpid();


	// Test log levels
	log_t* log = log_open(LOG_ROUTE, false, time_start);
/*	log_write(log, NONE_L, "Testing: none\n");
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
	*/
	
	// Let's launch two player processes and make them play, yay!
	int pipes[2][PLAYERS_FOR_NOW] = {};
	unsigned long int players_scores[PLAYERS_FOR_NOW];
	
	// We want main process to ignore SIG_SET signal as
	// it's just for players processes
	signal(SIG_SET, SIG_IGN);
	
	int i, j;
	// Launch players processes
	for(i = 0; i < PLAYERS_FOR_NOW; i++){
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
	// From now onwards, only main process can access
	size_t sets_won_home = 0, sets_won_away = 0;
	for(j = 0; j < SETS_AMOUNT; j++){
		log_write(log, NONE_L, "Set %d started!\n", j+1);
		// Here we make the two players play a set by  
		// sending them a message through the pipe.  
		// Change later for a better message protocol.
		for(i = 0; i < PLAYERS_FOR_NOW; i++){
			char msg = 1; // 1 is "start playing"
			write(pipes[i][PIPE_WRITE], &msg, sizeof(char));	
			}
		
		// Let the set last 6 seconds for now. After that, the
		// main process will make all players stop
		sleep(6);
		kill(0, SIG_SET);
		
		// Wait for the two player's scores
		for(i = 0; i < PLAYERS_FOR_NOW; i++){
			read(pipes[i][PIPE_READ], &players_scores[i], sizeof(players_scores[i]));
			}
		// Show this set score
		for(i = 0; i < PLAYERS_FOR_NOW; i++)
			log_write(log, NONE_L, "Player %d set score: %ld\n", i+1, players_scores[i]);
		
		// Determinate the winner of the set
		if(players_scores[0] > players_scores[1])
			sets_won_home++;
		else
			sets_won_away++;
		// If any won SETS_WINNING sets than the other, match over
		if(sets_won_home == SETS_WINNING)
			break;
		if(sets_won_away == SETS_WINNING)
			break;	
		}
		
	// Here we make the players stop the match
	for(i = 0; i < PLAYERS_FOR_NOW; i++){
		char msg = 0; // 0 is "stop the match"
		write(pipes[i][PIPE_WRITE], &msg, sizeof(char));	
		}

	// Close pipes and show final results
	if(sets_won_home > sets_won_away)
		log_write(log, NONE_L, "Player 1 won!\n");
	else
		log_write(log, NONE_L, "Player 2 won!\n");
	
	for(i = 0; i < PLAYERS_FOR_NOW; i++){
		close(pipes[i][PIPE_WRITE]);
		close(pipes[i][PIPE_READ]);
		}
			
	log_close(log);
	return 0;
}

