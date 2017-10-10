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
#define SET_MAX_DURATION 2500000 
#define SET_MIN_DURATION 350000 


// --------------- Court team section --------------

/* Initializes received team as empty team.*/
void court_team_initialize(court_team_t* team){
	team->team_size = 0;
	team->sets_won = 0;
	int i;
	for (i = 0; i < PLAYERS_PER_TEAM; i++)
		team->team_players[i] = TOTAL_PLAYERS + 8;
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
unsigned int court_player_to_court_id(court_t* court, unsigned int player_id){
	int i;
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
 * something above TOTAL_PLAYERS in case of error.*/
unsigned int court_court_id_to_player(court_t* court, unsigned int pc_id){
	if(pc_id > PLAYERS_PER_MATCH) return TOTAL_PLAYERS + 8;
	court_team_t team = court->team_home;
	if(pc_id >=  PLAYERS_PER_TEAM) {
		pc_id -= PLAYERS_PER_TEAM;
		team = court->team_away;
	}
	return team.team_players[pc_id];
}

void manage_players_scores(court_t* court){
	int won_team = (int) (court->team_home.sets_won < court->team_away.sets_won);  
	log_write(NONE_L, "Court %03d: Team %d won!\n", court->court_id, won_team + 1);
	// Set scores properly
	switch(court->team_home.sets_won){
			case 0:
			case 1:
				assert(court->team_away.sets_won == 3);
				court_team_add_score_players(court->team_away, court->st, 3);
				break;
			case 2:
				assert(court->team_away.sets_won == 3);
				court_team_add_score_players(court->team_away, court->st, 2);
				court_team_add_score_players(court->team_home, court->st, 1);
				break;
			case 3:
				if(court->team_away.sets_won == 2){
					court_team_add_score_players(court->team_away, court->st, 1);
					court_team_add_score_players(court->team_home, court->st, 2);
				}
				else
					court_team_add_score_players(court->team_home, court->st, 3);
				break;
	}		
}

void connect_player_in_team(court_t* court, unsigned int p_id, unsigned int team){
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
		exit(-1);	// TODO: don't bail, just revert changes
	}

	court->connected_players++;
}

void reject_player(court_t* court, unsigned int p_id) {
	log_write(CRITICAL_L, "Court %03d: Player %03d couldn't find a team, we should kick him!!\n", court->court_id, p_id);
	message_t msg = {};
	msg.m_player_id = p_id;
	msg.m_type = MSG_MATCH_REJECT;
	if (!send_msg(court->player_fifos[court->connected_players], &msg)) {
		log_write(ERROR_L, "Court %03d: Failed to send reject msg to player %03d [errno: %d]\n", court->court_id, p_id, errno);
		exit(-1);	// TODO: don't bail, just revert changes
	}
	close(court->player_fifos[court->connected_players]);
}

/* Marks each player's partner on the partners_table
 * stored at court.*/
void mark_players_partners(court_t* court){
	if((!court) || (!court->pt)) return;
	// Mark each players' partner (home)
	int i, j;
	for(i = 0; i < PLAYERS_PER_TEAM; i++)
		for(j = i + 1; j < PLAYERS_PER_TEAM; j++) {
			unsigned int p1 = court->team_home.team_players[i];
			unsigned int p2 = court->team_home.team_players[j];
			set_played_together(court->pt, p1, p2);
			}
	// Mark each players' partner (away)
	for(i = 0; i < PLAYERS_PER_TEAM; i++)
		for(j = i + 1; j < PLAYERS_PER_TEAM; j++) {
			unsigned int p1 = court->team_away.team_players[i];
			unsigned int p2 = court->team_away.team_players[j];
			set_played_together(court->pt, p1, p2);
			}
}

/* Auxiliar function that receives a MSG_PLAYER_JOIN_REQ message
 * for court and determinates if that player can join or not. If
 * the player can join the match, this functions accepts them and
 * assign them a team. If the player can't, this function should
 * kick them off!*/
void handle_player_team(court_t* court, message_t msg){
	static int join_attempts = 0;
	switch(court->connected_players){
		case 0: // First player to connect. Instantly accepted on team 0
			connect_player_in_team(court, msg.m_player_id, 0);
			join_attempts = 0; // Question for the reader: why is this line important?
			break;
		case 1: // Second player to connect.
			// If can team up with first player, let them do it
			if(court_team_player_can_join_team(court->team_home ,msg.m_player_id, court->pt))
				connect_player_in_team(court, msg.m_player_id, 0);
			else // If not, go to other team
				connect_player_in_team(court, msg.m_player_id, 1);
			break;
		default:
			if(court->connected_players >= PLAYERS_PER_MATCH){
				// Should not happen
				log_write(CRITICAL_L, "Court %03d: Wrong value for court->connected_players: %d\n", court->court_id, court->connected_players);
				break; 
				}
			
			// Check if can join team0
			if(court_team_player_can_join_team(court->team_home ,msg.m_player_id, court->pt))
				connect_player_in_team(court, msg.m_player_id, 0);
			// Check if can join team1
			else if(court_team_player_can_join_team(court->team_away ,msg.m_player_id, court->pt))
				connect_player_in_team(court, msg.m_player_id, 1);
			else // kick player
				reject_player(court, msg.m_player_id);
			
		}
		
	join_attempts++;
	if(join_attempts == JOIN_ATTEMPTS_MAX) {
		// Kick all players!
		int i;
		for(i = 0; i < court->connected_players; i++) {
			int p_id = court_court_id_to_player(court, i);
			
			log_write(CRITICAL_L, "Court %03d: Player %03d remained too long, let's kick them!\n", court->court_id, p_id);
			message_t msg = {};
			msg.m_player_id = p_id;
			msg.m_type = MSG_MATCH_REJECT;
			if (!send_msg(court->player_fifos[i], &msg)) {
				log_write(ERROR_L, "Court %03d: Failed to send reject msg to player %03d [errno: %d]\n", court->court_id, p_id, errno);
				exit(-1);	// TODO: don't bail, just revert changes
			}
			close(court->player_fifos[i]);
		}
		court->connected_players = 0;
		court_team_initialize(&court->team_home);
		court_team_initialize(&court->team_away);
		close(court->court_fifo);
		court->court_fifo = -1;
		for (i = 0; i < PLAYERS_PER_MATCH; i++) 
			court->player_fifos[i] = -1;
	
		join_attempts = 0;
		}
}


/* Auxiliar function that opens this court's fifo. If fifo was 
 * already opened, silently does nothing.
 * Watch out! open is blocking!*/
void open_court_fifo(court_t* court){
	if (court->court_fifo < 0) {
		log_write(INFO_L, "Court %03d: Court FIFO is closed, need to open one\n", court->court_id, errno);
		int court_fifo = open(court->court_fifo_name, O_RDONLY);
		log_write(INFO_L, "Court %03d: Court FIFO opened!!!\n", court->court_id, errno);
		if (court_fifo < 0) {
			log_write(ERROR_L, "Court %03d: FIFO opening error [errno: %d]\n", court->court_id, errno);
			exit(-1);
		}
		court->court_fifo = court_fifo;
	}
}

// --------------------------------------------------------------

/* Dynamically creates a new court. Returns NULL in failure.
 * Pre 1: The players processes were already created the fifos for communicating with
 * them are the ones received.*/
court_t* court_create(unsigned int court_id, partners_table_t* pt, score_table_t* st, tournament_t* tm) {

	court_t* court = malloc(sizeof(court_t));
	if (!court) return NULL;
	
	court->court_fifo = -1;
	court->court_id = court_id;
	int i;
	for (i = 0; i < PLAYERS_PER_MATCH; i++) {
		court->player_fifos[i] = -1;
	}

	court_team_initialize(&court->team_away);
	court_team_initialize(&court->team_home);
	court->pt = pt;
	court->st = st;
	court->tm = tm;
	court->connected_players = 0;
	return court;
}

/* Kills the received court and sends flowers
 * to his widow.*/
// TODO: Refactor this (add _destroy calls for court->tm, court->pt, etc)
void court_destroy(court_t* court){
	free(court);
	// Sry, no flowers
}

/*
 * Starts the court in its 'lobby' state. For now court is empty and
 * waiting for incomming players. It will open its fifo, and accept any
 * incomming connections. When all players are connected, court starts
 * and calls to court_play.
 * Once court ends, it comes here again.. eager to start another court.
 */
 // TODO: Refactor using new court_team_t
void court_lobby(court_t* court) {
	log_write(INFO_L, "Court %03d: A new match is about to begin... lobby\n", court->court_id, errno);
	court->connected_players = 0;
	court_team_initialize(&court->team_home);
	court_team_initialize(&court->team_away);
	message_t msg = {};

	while (court->connected_players < PLAYERS_PER_MATCH) {
		sem_wait(court->tm->tm_data->tm_courts_sem, court->court_id);

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
				log_write(ERROR_L, "Court %03d: Received a non-request-to-join msg\n", court->court_id);
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
					handle_player_team(court, msg);
				}
			}
		}
	}
	
	court_play(court);

	close(court->court_fifo);
	court->court_fifo = -1;
	int i;
	for (i = 0; i < PLAYERS_PER_MATCH; i++) {
		court->player_fifos[i] = -1;
	}
}


/* Plays the match. Communication is done
 * using the players' fifos, and sets are
 * played until one of the two teams wins
 * SETS_WINNING sets, or until SETS_AMOUNT
 * sets are played. If the close_pipes field
 * was set on this court creation, this
 * function also closes the fifos file des-
 * criptors at the end of the court. */
void court_play(court_t* court){
	int i, j;
	unsigned long int players_scores[PLAYERS_PER_MATCH] = {0};
	message_t msg = {};

	// Play SETS_AMOUNT sets
	for (j = 0; j < SETS_AMOUNT; j++) {
		msg.m_type = MSG_SET_START;

		log_write(NONE_L, "Court %03d: Set %d started!\n", court->court_id, j+1);
		// Here we make the four players play a set by  
		// sending them a message through the pipe.  
		// Change later for a better message protocol.
		for (i = 0; i < PLAYERS_PER_MATCH; i++) {
			send_msg(court->player_fifos[i], &msg);
		}
		
		// Let the set last 6 seconds for now. After that, the
		// main process will make all players stop
		// TODO: Change this for something random!
		unsigned long int t_rand = rand() % (SET_MAX_DURATION - SET_MIN_DURATION);
		usleep(t_rand + SET_MIN_DURATION);
		court_finish_set(court);
		
		// Wait for the four player's scores
		for (i = 0; i < PLAYERS_PER_MATCH; i++){
			if(!receive_msg(court->court_fifo, &msg))
				log_write(ERROR_L, "Court %03d: Error reading score from player %03d [errno: %d]\n", court->court_id, i, errno);
			else {
				log_write(INFO_L, "Court %03d: Received %d from player %03d\n", court->court_id, msg.m_type, msg.m_player_id);
				assert(msg.m_type == MSG_PLAYER_SCORE);
				int pc_id = court_player_to_court_id(court, msg.m_player_id);
				players_scores[pc_id] = msg.m_score;
			}
		}
		// Show this set score
		for(i = 0; i < PLAYERS_PER_MATCH; i++) {
			int p_id = court_court_id_to_player(court, i);
			log_write(INFO_L, "Court %03d: Player %03d set score: %ld\n", court->court_id, p_id, players_scores[i]);
			}
		// Determinate the winner of the set
		unsigned long int score_home = 0, score_away = 0;
		for (i = 0; i < PLAYERS_PER_TEAM; i++) 
			score_home += players_scores[i];
			
		for (i = PLAYERS_PER_TEAM; i < PLAYERS_PER_MATCH; i++) 
			score_away += players_scores[i];
		log_write(INFO_L, "Court %03d: This set ended (team 1, team 2): %d - %d\n", court->court_id, score_home, score_away);

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
		msg.m_type = MSG_MATCH_END;
		send_msg(court->player_fifos[i], &msg);
	}


	// Here we update the tournament info to set the court free
	lock_acquire(court->tm->tm_lock);
	court->tm->tm_data->tm_courts[court->court_id].court_status = TM_C_FREE;
	court->tm->tm_data->tm_courts[court->court_id].court_num_players = 0;
	lock_release(court->tm->tm_lock);
	
	manage_players_scores(court);
	mark_players_partners(court);
}

/* Finish the current set by signaling
 * the players with SIG_SET.*/
void court_finish_set(court_t* court){
	kill(0, SIG_SET);
}

/* Returns the sets won by home and away 
 * playes. If court is NULL, let them both 
 * return 0 for the time being.*/
size_t court_get_home_sets(court_t* court){
	return (court ? court->team_home.sets_won : 0);
}

size_t court_get_away_sets(court_t* court){
	return (court ? court->team_away.sets_won : 0);
}




void court_main(unsigned int court_id, partners_table_t* pt, score_table_t* st, tournament_t* tm) {

	// Launch a single court, then end the tournament
	// TODO: refactor this? Make it singleton to handle signals?
	court_t* court = court_create(court_id, pt, st, tm);
	if(!court)
		exit(-1);
		
	log_write(INFO_L, "Court %03d: Launched using PID: %d\n", court->court_id, getpid());

	get_court_fifo_name(court_id, court->court_fifo_name);
	// Como es un semaphore set.. se pueden crear de un saque todos lo semáforos!
	//int sem = sem_get(court->court_fifo_name, 1);
	//if (sem < 0)
	///	log_write(ERROR_L, "Court %03d: Error creating semaphore [errno: %d]\n", court->court_id, errno);

	//log_write(INFO_L, "Court %03d: Got semaphore %d with name %s!!\n", court->court_id, sem, court->court_fifo_name);
	//	if (sem_put(sem, 0, 4) < 0) {
	//		log_write(ERROR_L, "Court %03d: Error initializing the semaphore [errno: %d]\n", court->court_id, errno);
	//		exit(-1);
	//	}

	int i;
	// TODO: Change for a 'graceful quit'
	for (i = 0; i < COURT_LIFE; i++) {
		// Warning reading on a shared memory without locking!
		court_lobby(court);
	}

	score_table_print(court->st);

	log_write(INFO_L, "Court %03d: Destroying court\n", court->court_id);
	
	lock_acquire(court->tm->tm_lock);
	court->tm->tm_data->tm_courts[court->court_id].court_status = TM_C_DISABLED;
	lock_release(court->tm->tm_lock);

	partners_table_destroy(court->pt);
	tournament_destroy(court->tm);
	score_table_destroy(court->st);
	court_destroy(court);
	//sem_destroy(sem);
	log_close();
	exit(0);

}
