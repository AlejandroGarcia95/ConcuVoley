#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <assert.h>
#include <errno.h>
#include "player.h"
#include "court.h"

#include "protocol.h"
#include <sys/sem.h>

// In microseconds!
#define MIN_SCORE_TIME 15000
#define MAX_SCORE_TIME 40000

/* Auxiliar function that generates a random
 * skill field for a new player. It returns a
 * number "s" for the skill such that s <= 100
 * and s < (SKILL_AVG + DELTA_SKILL) and
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

/* Auxiliar function that makes the player
 * sleep some time accordingly to their skill.
 * A random component is added to the time, so
 * a little luck could be better than skill*/
void emulate_score_time(){
	player_t* player = player_get_instance();
	unsigned long int x = SKILL_MAX - player_get_skill();
	// Now x is in the range (0, SKILL_MAX)
	// and it has low values for good skilled
	// players. Hence, we can map time directly
	// with x values (bigger x, bigger score time)
	unsigned long int t = MIN_SCORE_TIME;
	int pend = (MAX_SCORE_TIME - MIN_SCORE_TIME) / SKILL_MAX;
	t += (unsigned long int) (pend * x);
	// Random component of time. 
	unsigned long int t_rand = rand() % MAX_SCORE_TIME;
	usleep(t + t_rand);
}

/* Dynamically creates a new player with a given 
 * name and properly initialize all other values.
 * Returns NULL if the allocation fails.*/
player_t* player_create(){	
	player_t* player = malloc(sizeof(player_t));
	if(!player)	return NULL;
	
	// Initialize fields
	player->skill = generate_random_skill();
	player->courtes_played = 0;
	player->currently_playing = false;
	player->id = 0;
	return player;
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
	if(player) free(player);
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

/* Returns the amount of courtes played by the 
 * current player. */
size_t player_get_courtes(){
	player_t* player = player_get_instance();
	return (player ? player->courtes_played : 0);
}

/* Returns the name of the current player.*/
char* player_get_name(){
	player_t* player = player_get_instance();
	return (player ? player->name : NULL);
}

/*Increase by 1 the amount of courtes played.*/
void player_increase_courtes_played(){
	player_t* player = player_get_instance();
	if(player)
		player->courtes_played++;
}

/* Returns true or false if the player
 * is or not playing.*/
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
/*	struct sigaction sa;
	sigset_t sigset;	
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = NULL;
	sigaction(SIG_SET, &sa, NULL);*/
}


/* Make this player play the current set
 * storing their score in the set_score
 * variable. This function should do the
 * following: make this player sleep an
 * amount of time inversely proportional
 * to their skill, and after that, make
 * it add a point to their score. Then,
 * repeat all over while player_is_playing*/
void player_play_set(unsigned long int* set_score){
	player_t* player = player_get_instance();
	if(!player) return;

	while(player_is_playing()){
		emulate_score_time();
		(*set_score)++;
	}
}



// TODO: documentation
void player_at_court(player_t* player, log_t* log, int court_fifo, int player_fifo) {
	message_t msg = {};
	char* p_name = player->name;
	while(msg.m_type != MSG_MATCH_END){
		// REMOVE ME
		int miss_count = 0;

		if(!receive_msg(player_fifo, &msg)) {
			log_write(log, ERROR_L, "Player %s: bad read [errno: %d]\n", p_name, errno);
			close(court_fifo);
			close(player_fifo);
			exit(-1);
		}

		if(msg.m_type == MSG_SET_START) {
			unsigned long int set_score = 0;
			msg.m_type = MSG_PLAYER_SCORE;
			msg.m_player_id = player->id;
	
			// Play the set until it's done (by SIG_SET)
			log_write(log, INFO_L, "Player %s started playing\n", p_name);
			player_start_playing();
			player_play_set(&set_score);
			msg.m_score = set_score;
			// When set is finished, we use the fifo to
			// send main process our set_score
			if(!send_msg(court_fifo, &msg))
				log_write(log, ERROR_L, "Player %03d cannot write in court 000 [errno: %d]\n", player->id, errno);
			log_write(log, INFO_L, "Player %s finished set (scored %lu)\n", p_name, set_score);
		} else {
			log_write(log, INFO_L, "Player %s: wont start playing\n", p_name);
			miss_count++;
			if (miss_count >= 100) {
				// Have to really praise you for this
				log_write(log, CRITICAL_L, "Have to shut down player %s [%03d]: wont start playing too many times (maybe deadlock)\n", p_name, player->id);
				exit(-1);
			}
		}
	}
}

// TODO: documentation xd
void player_join_court(player_t* player, log_t* log) {
	// Joining lobby!!
	char* p_name;
	p_name = player->name;
	// Open court fifo
	char* court_fifo_name = "fifos/court.fifo";
	int court_fifo = open(court_fifo_name, O_WRONLY);
	if (court_fifo < 0) {
		log_write(log, ERROR_L, "FIFO opening error for court 000 at player %03d [errno: %d]\n", player->id, errno);
		exit(-1);
	}
	log_write(log, INFO_L, "Player %03d opened court 000 FIFO\n", player->id);

	// Send "I want to play" message
	message_t msg = {};
	msg.m_type = MSG_PLAYER_JOIN_REQ;
	msg.m_player_id = player->id;
	if(!send_msg(court_fifo, &msg)){
		log_write(log, ERROR_L, "Player %03d failed to write to court 000 [errno: %d]\n", player->id, errno);
		close(court_fifo);
		exit(-1);
	}

	// Open self fifo (to rcv joined msg)
	char my_fifo_name[MAX_FIFO_NAME_LEN];
	if(!get_player_fifo_name(player->id, my_fifo_name)) {
		log_write(log, ERROR_L, "FIFO opening error for player %03d at player %03d [errno: %d]\n", player->id, player->id, errno);
		exit(-1);
	}
	int my_fifo = open(my_fifo_name, O_RDONLY);
	if (my_fifo < 0) {
		log_write(log, ERROR_L, "FIFO opening error for player %03d at player %03d [errno: %d]\n", player->id, player->id, errno);
		exit(-1);
	}
	log_write(log, INFO_L, "Player %03d opened self FIFO\n", player->id);

	// If accepted join court
	if(!receive_msg(my_fifo, &msg)) {
		log_write(log, ERROR_L, "Player %03d error reading accepted/rejected msg [errno: %d]\n", player->id, errno);
		exit(-1);
	}
	// Received a msg!!
	assert((msg.m_type == MSG_MATCH_ACCEPT) || (msg.m_type == MSG_MATCH_REJECT));
	if (msg.m_type == MSG_MATCH_ACCEPT) {
		player_at_court(player, log, court_fifo, my_fifo);
	} else {
		log_write(log, ERROR_L, "Player %03d was rejected\n", player->id);
	}
	player_unset_sigset_handler();

	if (close(court_fifo) < 0)
		log_write(log, ERROR_L, "Player %03d close court_fifo error [errno: %d]\n", player->id, errno);

	if (close(my_fifo) < 0)
		log_write(log, ERROR_L, "Player %03d close self_fifo error [errno: %d]\n", player->id, errno);
}





/* Function that makes the process adopt
 * a player's role. Basically, it creates
 * a player, and make them play a court.
 * Every set of the court starts when the
 * main process sends a "start playing" 
 * message through the fifo, and ends when
 * the player receives a SIG_SET signal 
 * (as a set could end unexpectedly). At
 * the end of each set, the player must
 * send through the fifo their score.*/
void player_main(unsigned int id, log_t* log) {
	// Re-srand with a changed seed
	srand(time(NULL) ^ (getpid() << 16));
	char p_name[NAME_MAX_LENGTH];
	generate_random_name(p_name);
	
	player_t* player = player_get_instance();
	if(!player)
		exit(-1);

	player->id = id;
	player_set_name(p_name);
	
	log_write(log, INFO_L, "Created new player [%03d]: %s\n", player->id, p_name);
	log_write(log, INFO_L, "%s skill is: %d\n", p_name, player_get_skill());
	

	// Get the court key!!
	// TODO: change it so it grabs the key to the court it wanna join
	key_t key = ftok("fifos/court.fifo", 6);
	int sem = semget(key, 1, 0666);
	if (sem < 0)
		log_write(log, ERROR_L, "Error opening the semaphore at player %03d [errno: %d]\n", player->id, errno);

	struct sembuf s = {};
	int i, r;
	for (i = 0; i < 2; i++) {
		s.sem_op = -1;
		if (semop(sem, &s, 1) < 0)
			log_write(log, ERROR_L, "Player %03d error taken semaphore [errno: %d]\n", player->id, errno);

		player_set_sigset_handler();
		log_write(log, INFO_L, "Player %03d took semaphore\n", player->id);
		player_join_court(player, log);


		s.sem_op = 1;
		if (semop(sem, &s, 1) < 0)
			log_write(log, ERROR_L, "Player %03d error taken semaphore [errno: %d]\n", player->id, errno);
		log_write(log, INFO_L, "Player %03d released semaphore\n", player->id);
		
		// Wait some tame before joining again
		// TODO: Change for wait/broadcast or similar
		unsigned long int t_rand = rand() % 2000000;
		usleep(t_rand);
	}

	log_write(log, INFO_L, "Player %03d is leaving\n", player->id);
	player_destroy(player);

	usleep(rand() % 20000);
	return; // Have to return to main to destroy things
	// Beware! Returns to main!
}


