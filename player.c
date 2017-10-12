#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>

#include <errno.h>
#include "player.h"
#include "court.h"

#include "protocol.h"
#include "semaphore.h"
#include "tournament.h"

// In microseconds!
#define MIN_SCORE_TIME 900
#define MAX_SCORE_TIME 3000

#define MAX_ATTEMPTS 10

/* Auxiliar function that generates a random skill field for a 
 * new player. It returns a number "s" for the skill such that 
 * s <= SKILL_MAX and s < (SKILL_AVG + DELTA_SKILL) and
 * (SKILL_AVG - DELTA_SKILL) < s.
 * Pre: srand was already called.*/
size_t generate_random_skill(){
	int interval_width;
	if(SKILL_MAX < (SKILL_AVG + DELTA_SKILL))
		interval_width = SKILL_MAX - SKILL_AVG + DELTA_SKILL;
	else
		interval_width = 2 * DELTA_SKILL;
	int r_numb = rand();
	return (r_numb % interval_width) + SKILL_AVG - DELTA_SKILL;
}

/* Auxiliar function that makes the player sleep some time 
 * accordingly to their skill. A random component is added to the
 * time, so a little luck could be better than skill*/
void emulate_score_time(){
	player_t* player = player_get_instance();
	unsigned long int x = SKILL_MAX - player_get_skill();
	// Now x is in the range (0, SKILL_MAX) and it has low 
	// values for good skilled players. Hence, we can map time 
	// directly with x values (bigger x, bigger score time)
	unsigned long int t = MIN_SCORE_TIME;
	int pend = (MAX_SCORE_TIME - MIN_SCORE_TIME) / SKILL_MAX;
	t += (unsigned long int) (pend * x);
	// Random component of time. 
	unsigned long int t_rand = rand() % MAX_SCORE_TIME;
	usleep(t + t_rand);
}

/* Dynamically creates a new player with a given name and properly 
 * initialize all other values. Returns NULL if the allocation fails.*/
player_t* player_create(){	
	player_t* player = malloc(sizeof(player_t));
	if(!player)	return NULL;
	// Initialize fields
	player->skill = generate_random_skill();
	player->matches_played = 0;
	player->times_kicked = 0;
	player->currently_playing = false;
	player->id = 0;
	player->tm = NULL;
	return player;
}

/* Auxiliar function to replace all exit(-1) calls,
 * offering the possibility of releasing resources.*/
void player_seppuku(bool release_res){
	if(release_res){
		player_t* player =  player_get_instance();
		lock_acquire(player->tm->tm_lock);
		player->tm->tm_data->tm_active_players--;
		lock_release(player->tm->tm_lock);
		
		player_destroy(player);
		log_close();
	}

	exit(-1);
}

// ----------------------------------------------------------------

/* Returns the current player singleton!*/
player_t* player_get_instance(){
	static player_t* player = NULL;
	// Check if there's already a player
	if(player)
		return player;
	// If not, create it!
	player = player_create();
	return player;
}

/* Destroys the received player.*/
void player_destroy(){
	player_t* player = player_get_instance();
	if (player) {
		if (player->tm)
			tournament_destroy(player->tm);
	    free(player);
	}
}

/* Set the name for the current player.
 * If name is NULL, silently does nothing.*/
void player_set_name(char* name){
	if(!name) return;
	player_t* player = player_get_instance();
	if(!player) return;
	strcpy(player->name, name);
}

/* Returns the skill of the current player.*/
size_t player_get_skill(){
	player_t* player = player_get_instance();
	return (player ? player->skill : SKILL_MAX+8);
}

/* Returns the name of the current player.*/
char* player_get_name(){
	player_t* player = player_get_instance();
	return (player ? player->name : NULL);
}

/* Returns true or false if the player is or not playing.*/
bool player_is_playing(){
	player_t* player = player_get_instance();
	return (player ? player->currently_playing : false);
}

/* Makes player stop playing.*/
void player_stop_playing(){
	player_t* player = player_get_instance();
	if(player)
		player->currently_playing = false;
}

/* Makes player start playing.*/
void player_start_playing(){
	player_t* player = player_get_instance();
	if(player)
		player->currently_playing = true;
}

/* Handler function for a player process. It should only be 
 * used with SIG_SET signal. Makes the player stop playing 
 * the set if they were playing.*/
void handler_players_set(int signum) {
	assert(signum == SIG_SET);
	// Check if set is to start or finish
	if(player_is_playing()) 
		player_stop_playing();
}

/* Handler function for SIGTERM signal*/
void player_handler_termination(int signum) {
	assert(signum == SIGTERM);
	player_t* player = player_get_instance();
	log_write(INFO_L, "Player %03d: No more matches can be played. Player decided to leave the tournament on his own!\n", player->id);
	player_seppuku(true);
}

void player_set_termination_handler() {
	// Set the hanlder for the SIGTERM signal
	struct sigaction sa;
	sigset_t sigset;	
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = player_handler_termination;
	sigaction(SIGTERM, &sa, NULL);
}

void player_set_sigset_handler() {
	// Set the hanlder for the SIG_SET signal
	struct sigaction sa;
	sigset_t sigset;	
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = handler_players_set;
	sigaction(SIG_SET, &sa, NULL);
}

void player_unset_sigset_handler() {
	signal(SIG_SET, SIG_IGN);
	return;
}

/* Make this player play the current set storing their score in 
 * the set_score variable. This function should do the following: 
 * make this player sleep an amount of time inversely proportional
 * to their skill, and after that, make it add a point to their 
 * score. Then, repeat all over while player_is_playing*/
void player_play_set(unsigned long int* set_score){
	player_t* player = player_get_instance();
	if(!player) return;

	while(player_is_playing()){
		emulate_score_time();
		(*set_score)++;
	}
}


/* Call this function once player has found a court to join.
 * Makes the player play every set and leave when necessary.*/
void player_at_court(player_t* player, int court_fifo, int player_fifo) {
	message_t msg = {};
	char* p_name = player->name;
	while(msg.m_type != MSG_MATCH_END){
		int miss_count = 0;

		if(!receive_msg(player_fifo, &msg)) {
			log_write(ERROR_L, "Player %03d: Bad read [errno: %d]\n", player->id, errno);
			close(court_fifo);
			close(player_fifo);
			player_seppuku(true);
		}

		if(msg.m_type == MSG_SET_START) {
			unsigned long int set_score = 0;
			msg.m_type = MSG_PLAYER_SCORE;
			msg.m_player_id = player->id;
	
			// Play the set until it's done (by SIG_SET)
			log_write(INFO_L, "Player %03d: Started playing\n", player->id);
			player_start_playing();
			player_play_set(&set_score);
			msg.m_score = set_score;
			// When set is finished, we use the fifo to
			// send main process our set_score
			if(!send_msg(court_fifo, &msg))
				log_write(ERROR_L, "Player %03d: Cannot write in court [errno: %d]\n", player->id, errno);
			log_write(INFO_L, "Player %03d: Finished set (scored %lu)\n", player->id, set_score);
		} else if(msg.m_type == MSG_MATCH_REJECT){
			// If here, player was accepted and was on the court a while
			// but other players that arrived couldn't make a team with them.
			// Hence, court kicked every player there
			log_write(INFO_L, "Player %03d: Kicked from previously accepted court\n", player->id);
			player->times_kicked++;
			return;
		} else {
			log_write(INFO_L, "Player %03d: Wont start playing, received message %d\n", player->id, msg.m_type);
			miss_count++;
			if (miss_count >= 2) {
				log_write(INFO_L, "Player %03d: Kicked from previously accepted court\n", player->id);
				player->times_kicked++;
			}
		}
	}
	
	if(msg.m_type == MSG_MATCH_END) // Not needed for now, but good sanity check
		player->matches_played++;
}

/* Make the player join the court found. This function is the one that
 * establishes a connection between the player and the court using the
 * protocol of messages defined. It ends with either the player being
 * rejected by the court (aka player can play with no partner on the
 * court found), or with the player joining the court, calling the
 * function player_at_court.*/
void player_join_court(player_t* player, unsigned int court_id) {
	log_write(INFO_L, "Player %03d: Found court %03d, attempting to join\n", player->id, court_id);
	char court_fifo_name[MAX_FIFO_NAME_LEN];
	get_court_fifo_name(court_id, court_fifo_name);

	// Get the court key!!
	sem_post(player->tm->tm_data->tm_courts_sem, court_id);

	player_set_sigset_handler();
	log_write(DEBUG_L, "Player %03d: Took semaphore of court %03d\n", player->id, court_id);

	// Joining lobby!!
	char* p_name;
	p_name = player->name;
	// Open court fifo
	int court_fifo = open(court_fifo_name, O_WRONLY);
	if (court_fifo < 0) {
		log_write(ERROR_L, "Player %03d: FIFO opening error for court %03d [errno: %d]\n", player->id, court_id, errno);
		player_seppuku(true);
	}
	log_write(DEBUG_L, "Player %03d: Opened court %03d FIFO\n", player->id, court_id);

	// Send "I want to play" message
	message_t msg = {};
	msg.m_type = MSG_PLAYER_JOIN_REQ;
	msg.m_player_id = player->id;
	if(!send_msg(court_fifo, &msg)){
		log_write(ERROR_L, "Player %03d: Failed to write to court %03d [errno: %d]\n", player->id, court_id, errno);
		close(court_fifo);
		player_seppuku(true);
	}

	// Open self fifo (to rcv joined msg)
	char my_fifo_name[MAX_FIFO_NAME_LEN];
	if(!get_player_fifo_name(player->id, my_fifo_name)) {
		log_write(ERROR_L, "Player %03d: FIFO player opening error [errno: %d]\n", player->id, errno);
		player_seppuku(true);
	}
	int my_fifo = open(my_fifo_name, O_RDONLY);
	if (my_fifo < 0) {
		log_write(ERROR_L, "Player %03d: FIFO opening error for player [errno: %d]\n", player->id, errno);
		player_seppuku(true);
	}
	log_write(DEBUG_L, "Player %03d: Opened self FIFO\n", player->id);

	// If accepted join court
	if(!receive_msg(my_fifo, &msg)) {
		log_write(ERROR_L, "Player %03d: Error reading accepted/rejected msg [errno: %d]\n", player->id, errno);
		player_seppuku(true);
	}
	
	// Received a msg!!
	bool qiq = ((msg.m_type == MSG_MATCH_ACCEPT) || (msg.m_type == MSG_MATCH_REJECT));
	if(!qiq) {
		log_write(CRITICAL_L, "Player %03d: Message was %d\n", player->id, msg.m_type);
		player_seppuku(true);
	}

	if (msg.m_type == MSG_MATCH_ACCEPT) {
		player_at_court(player, court_fifo, my_fifo);
	} else {
		log_write(INFO_L, "Player %03d: Player was rejected from Court %03d\n", player->id, court_id);
		player->times_kicked++;
	}
	player_unset_sigset_handler();

	if (close(court_fifo) < 0)
		log_write(ERROR_L, "Player %03d: Close court_fifo error [errno: %d]\n", player->id, errno);
	if (close(my_fifo) < 0)
		log_write(ERROR_L, "Player %03d: Close self_fifo error [errno: %d]\n", player->id, errno);

	log_write(DEBUG_L, "Player %03d: Released semaphore of court %03d\n", player->id, court_id);

}


/* The player who calls this function is willing to join a court.
 * Returns true if could found a court. */
bool player_looking_for_court(player_t* player) {
	log_write(INFO_L, "Player %03d: Looking for a court\n", player->id);

	// Search for a free court
	int court_id = -1;
	lock_acquire(player->tm->tm_lock);
	
	int i;
	int best_so_far = -1;
	int best_num_players = -1;
	for (i = 0; i < player->tm->total_courts; i++) {
		court_data_t cd = player->tm->tm_data->tm_courts[i];
		
		// Search for a court with most num_players which has room.
		//		   if there is a tie, choose the first one.
		if ((cd.court_status == TM_C_FREE) && (cd.court_num_players > best_num_players)) {
			log_write(INFO_L, "Player %03d: checking for court %03d, and is %d with %d players\n", player->id, i, cd.court_status, cd.court_num_players);
			best_so_far = i;
			best_num_players = cd.court_num_players;
		}
	}
	if ((best_so_far >= 0) && (player->tm->tm_data->tm_active_players >= PLAYERS_PER_MATCH)) {
		court_data_t cd = player->tm->tm_data->tm_courts[best_so_far];
		cd.court_players[cd.court_num_players] = player->id;
		cd.court_num_players++;
		if (cd.court_num_players == PLAYERS_PER_MATCH)
			cd.court_status = TM_C_BUSY;
		player->tm->tm_data->tm_courts[best_so_far] = cd;
		court_id = best_so_far;
	}
	lock_release(player->tm->tm_lock);

	if (court_id < 0)
		return false;
	player_join_court(player, court_id);
	return true;
}



/* Function that makes the process adopt a player's role. Basically, 
 * it creates a player, and make them play the tournament.*/
void player_main(unsigned int id, tournament_t* tm) {
	// Re-srand with a changed seed
	srand(time(NULL) ^ (getpid() << 16));
	char p_name[NAME_MAX_LENGTH];
	generate_random_name(p_name);
	
	player_t* player = player_get_instance();
	if(!player)
		player_seppuku(false);

	player_set_termination_handler();
	player->id = id;
	player_set_name(p_name);
	player->tm = tm;
	
	// Registering player info
	lock_acquire(player->tm->tm_lock);
	player->tm->tm_data->tm_players[id].player_num_matches = 0;
	player->tm->tm_data->tm_players[id].player_pid = getpid();
	strcpy(player->tm->tm_data->tm_players[id].player_name, p_name);
	lock_release(player->tm->tm_lock);

	log_write(INFO_L, "Player %03d: Launched as %s using PID: %d\n", player->id, p_name, getpid());
	log_write(INFO_L, "Player %03d: Player skill is: %d\n", player->id, player_get_skill());
	
	log_write(INFO_L, "Player %03d: decided to enter the tournament\n", player->id);

	lock_acquire(tm->tm_lock);
	int sem_start = tm->tm_data->tm_init_sem;
	lock_release(tm->tm_lock);

	// Post the arrival to the main referee, then wait for the start tournament signal (so exciting!!)
	sem_post(sem_start, 0);
	sem_wait(sem_start, 1);
	lock_acquire(tm->tm_lock);
	tm->tm_data->tm_on_beach_players++;
	lock_release(tm->tm_lock);
	log_write(INFO_L, "Player %03d: Has entered the beach\n", player->id);

	int i, r;
	bool cut_condition = false;
	int attempts = 0;
	while((player->matches_played < tm->num_matches) && (attempts < MAX_ATTEMPTS)) {
		
		lock_acquire(tm->tm_lock);
		int players_alive = tm->tm_data->tm_active_players;
		lock_release(tm->tm_lock);
		
		unsigned long int prob = rand() % 100;
		if (prob < LEAVING_PROB) {
			log_write(INFO_L, "Player %03d: Decided to leave the tournament on his own!\n", player->id);
			sem_post(sem_start, 1);
			lock_acquire(tm->tm_lock);
			tm->tm_data->tm_on_beach_players--;
			lock_release(tm->tm_lock);
			break;
		}

		if (prob < RESTING_PROB) {
			log_write(INFO_L, "Player %03d: Decided to leave the beach!\n", player->id);
			sem_post(sem_start, 1);
			lock_acquire(tm->tm_lock);
			tm->tm_data->tm_on_beach_players--;
			lock_release(tm->tm_lock);
			unsigned long int t_rest = rand() % MAX_TIME_RESTING + 1000;
			usleep(t_rest);
			log_write(INFO_L, "Player %03d: Is back, wanting to enter the beach\n", player->id);
			sem_wait(sem_start, 1);
			lock_acquire(tm->tm_lock);
			tm->tm_data->tm_on_beach_players++;
			lock_release(tm->tm_lock);
			log_write(INFO_L, "Player %03d: Has re-entered the beach successfully\n", player->id);
			continue;
		}

		log_write(INFO_L, "Player %03d: Decided to play!\n", player->id);
		if (!player_looking_for_court(player))
			attempts++;
		else
			attempts = 0;
		
		// Sad ending for player: leaves when nobody loves him 
		if (player->times_kicked == MAX_TIMES_KICKED) {
			log_write(INFO_L, "Player %03d: Kicked too many times. Giving up!\n", player->id);
			break;
		}
		
		// Wait some time before doing anything
		unsigned long int t_rand = rand() % 2000000;
		usleep(t_rand);
	}
	
	sem_post(sem_start, 1);
	lock_acquire(player->tm->tm_lock);
	player->tm->tm_data->tm_active_players--;
	tm->tm_data->tm_on_beach_players--;
	lock_release(player->tm->tm_lock);

	log_write(INFO_L, "Player %03d: Now leaving\n", player->id);
	player_destroy(player);
	log_close();
	exit(0);
	return; 
}


