#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include <assert.h>
#include "court.h"
#include "log.h"
#include "score_table.h"
#include "partners_table.h"
#include "tournament.h"

#include "protocol.h"

// In microseconds!
#define SET_MAX_DURATION 200000
#define SET_MIN_DURATION 70000 


// --------------- Court team section --------------

/* Initializes received team as empty team.*/
void court_team_initialize(court_team_t* team){
	team->team_size = 0;
	team->sets_won = 0;
	int i;
	for (i = 0; i < PLAYERS_PER_TEAM; i++)
		team->team_players[i] = INVALID_PLAYER_ID;
}

/* Returns true if team is full (no other member can
 * join it) or false otherwise.*/
bool court_team_is_full(court_team_t team){
	return (team.team_size == PLAYERS_PER_TEAM);
}

/* Returns true if the received player can join the team.*/
bool court_team_player_can_join_team(court_team_t team, unsigned int player_id, partners_table_t* pt){
	if(court_team_is_full(team)) return false;
	// If playerd_id has already played with any team member, returns false
	int i;
	for (i = 0; i < team.team_size; i++)
		if(get_played_together(pt, team.team_players[i], player_id))
			return false;
	return true;
}

/* Returns true if the received player is already on the team*/
bool court_team_player_in_team(court_team_t team, unsigned int player_id){
	int i;
	for (i = 0; i < team.team_size; i++)
		if(player_id == team.team_players[i])
			return true;
	return false;
}

/* Joins the received player to the team.*/
void court_team_join_player(court_team_t* team, unsigned int player_id){
	team->team_players[team->team_size] = player_id;
	team->team_size++;
}

/* Increases player's score for every player on the received team*/
void court_team_add_score_players(court_team_t team, score_table_t* st, int score){
	if(!st) return;
	int i;
	for (i = 0; i < team.team_size; i++)
		increase_player_score(st, team.team_players[i], score);
}

// --------------- Auxiliar functions ---------------

/* Returns a number between 0 and PLAYERS_PER_MATCH -1 which 
 * represents the "player_court_id", a player id that is 
 * "relative" to this court. If the player_id received doesn't
 * belong to a player on this court, returns something above
 * PLAYERS_PER_MATCH.*/
unsigned int court_player_to_court_id(unsigned int player_id){
	int i;
	court_t* court = court_get_instance();
	court_team_t team0 = court->team_home;
	for(i = 0; i < PLAYERS_PER_TEAM; i++)
		if(team0.team_players[i] == player_id)
			return i;
	court_team_t team1 = court->team_away;
	for(i = 0; i < PLAYERS_PER_TEAM; i++)
		if(team1.team_players[i] == player_id)
			return i + PLAYERS_PER_TEAM;
			
	return PLAYERS_PER_MATCH + 8;
}

/* Inverse of the function above. Receives a "player_court_id"
 * relative to this court and returns the player_id. Returns
 * something above players amount in case of error.*/
unsigned int court_court_id_to_player(unsigned int pc_id){
	court_t* court = court_get_instance();
	if(pc_id > PLAYERS_PER_MATCH) return INVALID_PLAYER_ID;
	court_team_t team = court->team_home;
	if(pc_id >=  PLAYERS_PER_TEAM) {
		pc_id -= PLAYERS_PER_TEAM;
		team = court->team_away;
	}
	return team.team_players[pc_id];
}

/* Transforms sets won by team into team scores.*/
void manage_players_scores(){
	court_t* court = court_get_instance();
	int won_team = (int) (court->team_home.sets_won < court->team_away.sets_won);  
	log_write(INFO_L, "Court %03d: Team %d won!\n", court->court_id, won_team + 1);
	// Set scores properly
	switch(court->team_home.sets_won){
		case 0:
		case 1:
			assert(court->team_away.sets_won == 3);
			court_team_add_score_players(court->team_away, court->tm->tm_data->st, 3);
			break;
		case 2:
			assert(court->team_away.sets_won == 3);
			court_team_add_score_players(court->team_away, court->tm->tm_data->st, 2);
			court_team_add_score_players(court->team_home, court->tm->tm_data->st, 1);
			break;
		case 3:
			if(court->team_away.sets_won == 2){
				court_team_add_score_players(court->team_away, court->tm->tm_data->st, 1);
				court_team_add_score_players(court->team_home, court->tm->tm_data->st, 2);
			}
			else
				court_team_add_score_players(court->team_home, court->tm->tm_data->st, 3);
			break;
	}
}

// Merely statistics purpose
void update_player_match_data() {
	court_t* court = court_get_instance();
	match_data_t md = {};
	court_team_t team_home = court->team_home;
	court_team_t team_away = court->team_away;

	md.match_players[0] = team_home.team_players[0];
	md.match_players[1] = team_home.team_players[1];
	md.match_players[2] = team_away.team_players[0];
	md.match_players[3] = team_away.team_players[1];
	md.match_score[0] = team_home.sets_won;
	md.match_score[1] = team_away.sets_won;
	md.match_played_at = court->court_id;

	lock_acquire(court->tm->tm_lock);
	int i;
	for (i = 0; i < PLAYERS_PER_MATCH; i++) {
		int aux = court->tm->tm_data->tm_players[md.match_players[i]].player_num_matches;
		court->tm->tm_data->tm_players[md.match_players[i]].player_matches[aux] = md;
		court->tm->tm_data->tm_players[md.match_players[i]].player_num_matches++;
	}
	court->tm->tm_data->tm_courts[court->court_id].court_completed_matches++;
	lock_release(court->tm->tm_lock);
}

void connect_player_in_team(unsigned int p_id, unsigned int team){
	court_t* court = court_get_instance();
	if(team == 0)
		court_team_join_player(&court->team_home, p_id);
	if(team == 1)
		court_team_join_player(&court->team_away, p_id);
	
	log_write(INFO_L, "Court %03d: Player %03d is connected at this court for team %d\n", court->court_id, p_id, team + 1);

	message_t msg = {};
	msg.m_player_id = p_id;
	msg.m_type = MSG_MATCH_ACCEPT;

	if (!send_msg(court->player_fifos[court->connected_players], &msg)) {
		log_write(ERROR_L, "Court %03d: Failed to send accept msg to player %03d [errno: %d]\n", court->court_id, p_id, errno);
		exit(-1);
	}

	court->connected_players++;
}

/* Kicks every player from the court. This function should be used when
 * the players on the court have been alread accepted on the court (i.e.
 * they were accepted and remained too long, hence should be kicked).
 * For kicking the player without accepting, use reject_player. If
 * court_available is true, then the court is marked as free. If not,
 * it's marked as disabled.*/
void kick_all_players(bool court_available){
	court_t* court = court_get_instance();
	lock_acquire(court->tm->tm_lock);
	// Kick all players!
	int i;
	for(i = 0; i < court->connected_players; i++) {
		int p_id = court_court_id_to_player(i);
		
		if (court->flooded)
			log_write(INFO_L, "Court %03d: Player %03d is kicked due to court flooding!\n", court->court_id, p_id);
		else
			log_write(INFO_L, "Court %03d: Player %03d remained too long, let's kick them!\n", court->court_id, p_id);

		message_t msg = {};
		msg.m_player_id = p_id;
		msg.m_type = MSG_MATCH_REJECT;
		if (!send_msg(court->player_fifos[i], &msg)) {
			log_write(ERROR_L, "Court %03d: Failed to send reject msg to player %03d [errno: %d]\n", court->court_id, p_id, errno);
			exit(-1);
		}
		if(court->player_fifos[i] > 0)
			close(court->player_fifos[i]);
	}
	court->connected_players = 0;
	court_team_initialize(&court->team_home);
	court_team_initialize(&court->team_away);
	if(court->court_fifo > 0)
		close(court->court_fifo);
	court->court_fifo = -1;

	for (i = 0; i < PLAYERS_PER_MATCH; i++) 
		court->player_fifos[i] = -1;
			
	if (!court->flooded)
		court->tm->tm_data->tm_courts[court->court_id].court_status = (court_available ? TM_C_FREE : TM_C_DISABLED);

	court->tm->tm_data->tm_courts[court->court_id].court_num_players = 0;
	for(i = 0; i < PLAYERS_PER_MATCH; i++)
		court->tm->tm_data->tm_courts[court->court_id].court_players[i] = INVALID_PLAYER_ID;
	lock_release(court->tm->tm_lock);
}

/* Sends a MSG_MATCH_REJECT to the received player through their
 * FIFO. It also readjust the amount of players on this court.*/
void reject_player(unsigned int p_id) {
	court_t* court = court_get_instance();
	lock_acquire(court->tm->tm_lock);
	log_write(INFO_L, "Court %03d: Player %03d couldn't find a team, we should kick him!!\n", court->court_id, p_id);
	message_t msg = {};
	msg.m_player_id = p_id;
	msg.m_type = MSG_MATCH_REJECT;
	if (!send_msg(court->player_fifos[court->connected_players], &msg)) {
		log_write(ERROR_L, "Court %03d: Failed to send reject msg to player %03d [errno: %d]\n", court->court_id, p_id, errno);
		exit(-1);
	}
	close(court->player_fifos[court->connected_players]);
	
	court_data_t cd = court->tm->tm_data->tm_courts[court->court_id];

	cd.court_num_players--;
	cd.court_players[cd.court_num_players] = INVALID_PLAYER_ID;
	cd.court_status = TM_C_FREE;
	court->tm->tm_data->tm_courts[court->court_id] = cd;

	lock_release(court->tm->tm_lock);
}

/* Pretty self-descripting function.*/
void court_self_destruct(){
	court_t* court = court_get_instance();
	lock_acquire(court->tm->tm_lock);
	court->tm->tm_data->tm_courts[court->court_id].court_status = TM_C_DISABLED;
	lock_release(court->tm->tm_lock);
	// If there were players inside, let'em go
	court_finish_set();
	kick_all_players(false);
	
	lock_acquire(court->tm->tm_lock);
	score_table_print(court->tm->tm_data->st);
	log_write(DEBUG_L, "Court %03d: Destroying court\n", court->court_id);
	lock_release(court->tm->tm_lock);
	court_destroy(court);
	log_close();
	
	exit(0);
}
		
/* Handler function for SIG_TIDE signal*/
void court_handler_tide(int signum) {
	assert(signum == SIG_TIDE);
	court_t* court = court_get_instance();
	tournament_t* tm = court->tm;
	if (tm->tm_data->tm_courts[court->court_id].court_status == TM_C_FLOODED) {
		court->flooded = true;
	} else {
		court->flooded = false;
		sem_post(court->flood_sem, court->court_id);
	}
}

void court_set_tide_handler() {
	// Set the hanlder for the SIG_TIDE signal
	struct sigaction sa;
	sigset_t sigset;	
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = court_handler_tide;
	sigaction(SIG_TIDE, &sa, NULL);
}

/* Handler function for SIGTERM signal*/
void court_handler_termination(int signum) {
	assert(signum == SIGTERM);
	court_t* court = court_get_instance();
	log_write(INFO_L, "Court %03d: No more matches can be played. Self-destruct protocol started.\n", court->court_id);
	court_self_destruct();
}

void court_set_termination_handler() {
	// Set the hanlder for the SIGTERM signal
	struct sigaction sa;
	sigset_t sigset;	
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = court_handler_termination;
	sigaction(SIGTERM, &sa, NULL);
}

/* Marks each player's partner on the partners_table stored at court.*/
void mark_players_partners(){
	court_t* court = court_get_instance();
	if((!court) || (!court->tm->tm_data->pt)) return;
	// Mark each players' partner (home)
	int i, j;
	for(i = 0; i < PLAYERS_PER_TEAM; i++)
		for(j = i + 1; j < PLAYERS_PER_TEAM; j++) {
			unsigned int p1 = court->team_home.team_players[i];
			unsigned int p2 = court->team_home.team_players[j];
			set_played_together(court->tm->tm_data->pt, p1, p2);
			}
	// Mark each players' partner (away)
	for(i = 0; i < PLAYERS_PER_TEAM; i++)
		for(j = i + 1; j < PLAYERS_PER_TEAM; j++) {
			unsigned int p1 = court->team_away.team_players[i];
			unsigned int p2 = court->team_away.team_players[j];
			set_played_together(court->tm->tm_data->pt, p1, p2);
			}
}

/* Auxiliar function that receives a MSG_PLAYER_JOIN_REQ message
 * for court and determinates if that player can join or not. If
 * the player can join the match, this functions accepts them and
 * assign them a team. If the player can't, this function should
 * kick them off!*/
void handle_player_team(message_t msg){
	court_t* court = court_get_instance();
	static int join_attempts = 0;
	switch(court->connected_players){
		case 0: // First player to connect. Instantly accepted on team 0
			connect_player_in_team(msg.m_player_id, 0);
			join_attempts = 0; // Question for the reader: why is this line important?
			break;
		case 1: // Second player to connect.
			// If can team up with first player, let them do it
			if(court_team_player_can_join_team(court->team_home ,msg.m_player_id, court->tm->tm_data->pt))
				connect_player_in_team(msg.m_player_id, 0);
			else // If not, go to other team
				connect_player_in_team(msg.m_player_id, 1);
			break;
		default:
			if(court->connected_players >= PLAYERS_PER_MATCH){
				// Should not happen
				log_write(CRITICAL_L, "Court %03d: Wrong value for court->connected_players: %d\n", court->court_id, court->connected_players);
				break; 
				}
			
			// Check if can join team0
			if(court_team_player_can_join_team(court->team_home ,msg.m_player_id, court->tm->tm_data->pt))
				connect_player_in_team(msg.m_player_id, 0);
			// Check if can join team1
			else if(court_team_player_can_join_team(court->team_away ,msg.m_player_id, court->tm->tm_data->pt))
				connect_player_in_team(msg.m_player_id, 1);
			else // kick player
				reject_player(msg.m_player_id);
		}
		
	join_attempts++;
	if(join_attempts == JOIN_ATTEMPTS_MAX) {
		kick_all_players(true);
		join_attempts = 0;
		}
}


/* Auxiliar function that opens this court's fifo. If fifo was 
 * already opened, silently does nothing.
 * Watch out! open is blocking!*/
void open_court_fifo(){
	court_t* court = court_get_instance();
	if (court->court_fifo < 0) {
		log_write(DEBUG_L, "Court %03d: Court FIFO is closed, need to open one\n", court->court_id, errno);
		int court_fifo = open(court->court_fifo_name, O_RDONLY);
		log_write(DEBUG_L, "Court %03d: Court FIFO opened!!!\n", court->court_id, errno);
		if (court_fifo < 0) {
			log_write(ERROR_L, "Court %03d: FIFO opening error [errno: %d]\n", court->court_id, errno);
			exit(-1);
		}
		court->court_fifo = court_fifo;
	}
}

// --------------------------------------------------------------

/* Dynamically creates a new court. Returns NULL in failure.
 * Should only be called by court_get_instance. */
court_t* court_create() {
	court_t* court = malloc(sizeof(court_t));
	if (!court) return NULL;
	
	court->court_fifo = -1;

	int i;
	for (i = 0; i < PLAYERS_PER_MATCH; i++) {
		court->player_fifos[i] = -1;
	}

	court_team_initialize(&court->team_away);
	court_team_initialize(&court->team_home);
	court->connected_players = 0;
	court->flooded = false;
	return court;
}

/* Kills the received court and sends flowers to his widow.*/
void court_destroy(){
	court_t* court = court_get_instance();
	partners_table_destroy(court->tm->tm_data->pt);
	score_table_destroy(court->tm->tm_data->st);
	tournament_destroy(court->tm);
	free(court);
	// Sry, no flowers
}

/* Returns the current court singleton!*/
court_t* court_get_instance(){
	static court_t* court = NULL;
	// Check if there's already a court
	if(court)
		return court;
	// If not, create it!
	court = court_create();
	return court;
}

/*
 * Starts the court in its 'lobby' state. For now court is empty and
 * waiting for incomming players. It will open its fifo, and accept any
 * incomming connections. When all players are connected, court starts
 * and calls to court_play.
 * Once court ends, it comes here again.. eager to start another court.
 */
void court_lobby() {
	court_t* court = court_get_instance();
	log_write(INFO_L, "Court %03d: A new match is about to begin... (starting lobby)\n", court->court_id, errno);
	court->connected_players = 0;
	court_team_initialize(&court->team_home);
	court_team_initialize(&court->team_away);
	message_t msg = {};
	bool cut_condition = false;

	while (court->connected_players < PLAYERS_PER_MATCH) {
		if (court->flooded) {
			log_write(ERROR_L, "Court %03d: flooded on kicking everybody before semaphore\n", court->court_id);
			kick_all_players(false);
			return;
		}
		
		sem_wait(court->tm->tm_data->tm_courts_sem, court->court_id);

		if (court->flooded) {
			log_write(ERROR_L, "Court %03d: flooded on kicking everybody\n", court->court_id);
			kick_all_players(false);
			return;
		}
		
		lock_acquire(court->tm->tm_lock);
		int players_alive = court->tm->tm_data->tm_active_players;
		lock_release(court->tm->tm_lock);
		// Now court is in the "empty" state, waiting for new connections
		open_court_fifo(court);
		log_write(INFO_L, "Court %03d: Court awaiting connections\n", court->court_id, errno);
		
		// A connection has been made! Waiting for initial message!
		if(!receive_msg(court->court_fifo, &msg)) {
			log_write(ERROR_L, "Court %03d: Bad read new connection message [errno: %d]\n", court->court_id, errno);
			close(court->court_fifo);
			court->court_fifo = -1;
		} else {
			if (msg.m_type != MSG_PLAYER_JOIN_REQ) {
				log_write(ERROR_L, "Court %03d: Received a non-request-to-join msg from Player %03d\n", court->court_id, msg.m_player_id);
				close(court->court_fifo); // Why aborting instead of just ignoring the msg?
				court->court_fifo = -1;
			} else {
				// Is a request, let's open other player's fifo!
				char player_fifo_name[MAX_FIFO_NAME_LEN];
				if(!get_player_fifo_name(msg.m_player_id, player_fifo_name)) {
					log_write(ERROR_L, "Court %03d: FIFO opening error for player %d fifo [errno: %d]\n", court->court_id, msg.m_player_id, errno);
					return;
				}
				court->player_fifos[court->connected_players] = open(player_fifo_name, O_WRONLY);
				if (court->player_fifos[court->connected_players] < 0) {
					log_write(ERROR_L, "Court %03d: FIFO opening error for player %d fifo [errno: %d]\n", court->court_id, msg.m_player_id, errno);
				} else {
					log_write(INFO_L, "Court %03d: Court will handle player %d\n", court->court_id, msg.m_player_id);
					handle_player_team(msg);
				}
			}
		}
	}
	
	if (!court->flooded) {
		court_play(court);
	} else {
		log_write(ERROR_L, "Court %03d: Flooded before starting match\n", court->court_id);
		kick_all_players(court);
	}

	if (court->court_fifo >= 0)
		close(court->court_fifo);
	court->court_fifo = -1;
	int i;
	for (i = 0; i < PLAYERS_PER_MATCH; i++) {
		court->player_fifos[i] = -1;
	}
}

/* Plays the match. Communication is done using the players' fifos, 
 * and sets are played until one of the two teams wins SETS_WINNING 
 * sets, or until SETS_AMOUNT sets are played. If the close_pipes field
 * was set on this court creation, this function also closes the fifos 
 * file descriptors. */
void court_play(){
	court_t* court = court_get_instance();
	int i, j;
	unsigned long int players_scores[PLAYERS_PER_MATCH] = {0};
	message_t msg = {};

	// Play SETS_AMOUNT sets
	for (j = 0; j < SETS_AMOUNT; j++) {

		if (court->flooded) {
			court_finish_set();
			kick_all_players(false);
			return;
		}

		msg.m_type = MSG_SET_START;

		log_write(INFO_L, "Court %03d: Set %d started!\n", court->court_id, j+1);
		// Here we make the four players play a set by  
		// sending them a message through the pipe.  
		// Change later for a better message protocol.
		for (i = 0; i < PLAYERS_PER_MATCH; i++) {
			send_msg(court->player_fifos[i], &msg);
		}
		
		// Let the set last 6 seconds for now. After that, the
		// main process will make all players stop
		unsigned long int t_rand = rand() % (SET_MAX_DURATION - SET_MIN_DURATION);
		usleep(t_rand + SET_MIN_DURATION);

		if (court->flooded) {
			court_finish_set();
			kick_all_players(false);
			return;
		}
		
		court_finish_set(court);
		
		// Wait for the four player's scores
		for (i = 0; i < PLAYERS_PER_MATCH; i++){
			if(!receive_msg(court->court_fifo, &msg)) {
				log_write(ERROR_L, "Court %03d: Error reading score from player %03d [errno: %d]\n", court->court_id, i, errno);
			} else {
				log_write(DEBUG_L, "Court %03d: Received %d from player %03d\n", court->court_id, msg.m_type, msg.m_player_id);
				assert(msg.m_type == MSG_PLAYER_SCORE);
				int pc_id = court_player_to_court_id(msg.m_player_id);
				players_scores[pc_id] = msg.m_score;
			}
		}
		// Show this set score
		for(i = 0; i < PLAYERS_PER_MATCH; i++) {
			int p_id = court_court_id_to_player(i);
			log_write(DEBUG_L, "Court %03d: Player %03d set score: %ld\n", court->court_id, p_id, players_scores[i]);
		}

		// Determinate the winner of the set
		unsigned long int score_home = 0, score_away = 0;
		for (i = 0; i < PLAYERS_PER_TEAM; i++) 
			score_home += players_scores[i];
			
		for (i = PLAYERS_PER_TEAM; i < PLAYERS_PER_MATCH; i++) 
			score_away += players_scores[i];

		log_write(INFO_L, "Court %03d: Set %d ended (team 1, team 2): %d - %d\n", court->court_id, j, score_home, score_away);

		if (score_home > score_away)
			court->team_home.sets_won++;
		else
			court->team_away.sets_won++;

		// If any won SETS_WINNING sets than the other, court over
		if(court->team_home.sets_won == SETS_WINNING)
			break;
		if(court->team_away.sets_won == SETS_WINNING)
			break;	
	}

	// Here we make the players stop the match
	for (i = 0; i < PLAYERS_PER_MATCH; i++) {
		if (court->flooded)
			msg.m_type = MSG_MATCH_END;
		else
			msg.m_type = MSG_MATCH_END;
		send_msg(court->player_fifos[i], &msg);
	}


	// Here we update the tournament info to set the court free
	lock_acquire(court->tm->tm_lock);
	if (!court->flooded)
		court->tm->tm_data->tm_courts[court->court_id].court_status = TM_C_FREE;
	court->tm->tm_data->tm_courts[court->court_id].court_num_players = 0;
	for(i = 0; i < PLAYERS_PER_MATCH; i++)
		court->tm->tm_data->tm_courts[court->court_id].court_players[i] = INVALID_PLAYER_ID;
	lock_release(court->tm->tm_lock);
	
	if (court->flooded) return;

	update_player_match_data();
	manage_players_scores();
	mark_players_partners();
}

/* Finish the current set by signaling
 * the players with SIG_SET.*/
void court_finish_set(){
	court_t* court = court_get_instance();
	int i;
	for(i = 0; i < PLAYERS_PER_MATCH; i++){
		int player_id = court_court_id_to_player(i);
		if(player_id == INVALID_PLAYER_ID) break;
		player_data_t pd = court->tm->tm_data->tm_players[player_id];
		kill(pd.player_pid, SIG_SET);
	}
}

/* Executes main for this process. Finishes via exit(0)*/
void court_main(unsigned int court_id, tournament_t* tm) {
	court_t* court = court_get_instance();
	court_set_termination_handler();
	court_set_tide_handler();

	if(!court)
		exit(-1);
	
	court->court_id	= court_id;
	court->tm = tm;

	lock_acquire(tm->tm_lock);
	tm->tm_data->tm_courts[court_id].court_pid = getpid();
	court->flood_sem = tm->tm_data->tm_courts_flood_sem;
	lock_release(tm->tm_lock);
		
	log_write(DEBUG_L, "Court %03d: Launched using PID: %d\n", court->court_id, getpid());
	get_court_fifo_name(court_id, court->court_fifo_name);

	while(1){ 
		if (court->flooded) {
			log_write(DEBUG_L, "Court %03d: It's flooded!! Waiting till water goes down\n", court_id);
			sem_wait(court->flood_sem, court_id);
			log_write(DEBUG_L, "Court %03d: Water went down\n", court_id);
		}

		court_lobby(court);
	}

}
