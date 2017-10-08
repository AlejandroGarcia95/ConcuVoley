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
#include "partners_table.h"

#include "protocol.h"

// In microseconds!
#define SET_MAX_DURATION 4000000 
#define SET_MIN_DURATION 1000000 

#define OTHER_TEAM(x) ((x + 1) % 2)

// --------------- Court team section --------------

/* Initializes received team as empty team.*/
void court_team_initialize(court_team_t* team){
	team->team_size = 0;
	int i;
	for (i = 0; i < PLAYERS_PER_TEAM; i++)
		team->team_players[i] = 0;
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
		if(get_played_together(pt, i, player_id))
			return false;
	return true;
}

/* Joins the received player to the team.*/
void court_team_join_player(court_team_t* team, unsigned int player_id){
	team->team_players[team->team_size] = player_id;
	team->team_size++;
}

/* Removes every player of the team from it.*/
void court_team_kick_players(court_team_t* team){
	court_team_initialize(team);
}

// --------------- Auxiliar functions ---------------

void connect_player_in_team(court_t* court, unsigned int p_id, unsigned int team){
	unsigned int match_id = court->connected_players;
	court->player_connected[match_id] = p_id;
	court->player_team[match_id] = team;
	court->player_number[p_id] = match_id;
	log_write(INFO_L, "Player %03d is connected at court %03d for team %d\n", p_id, court->court_id, team + 1);

	message_t msg = {};
	msg.m_player_id = p_id;
	msg.m_type = MSG_MATCH_ACCEPT;
	if (!send_msg(court->player_fifos[match_id], &msg)) {
		log_write(ERROR_L, "Court failed to send accept msg to player %03d [errno: %d]\n", p_id, errno);
		exit(-1);	// TODO: don't bail, just revert changes
	}

	court->connected_players++;
}

void reject_player(court_t* court, unsigned int p_id) {
	message_t msg = {};
	msg.m_player_id = p_id;
	msg.m_type = MSG_MATCH_REJECT;
	if (!send_msg(court->player_fifos[court->connected_players], &msg)) {
		log_write(ERROR_L, "Court failed to send accept msg to player %03d [errno: %d]\n", p_id, errno);
		exit(-1);	// TODO: don't bail, just revert changes
	}

}

/* Marks each player's partner on the partners_table
 * stored at court.*/
// TODO: Rewrite this using the court_team_t
void mark_players_partners(court_t* court){
	if((!court) || (!court->pt)) return;
	// Mark each players' partner
	int i, j;
	for(i = 0; i < PLAYERS_PER_MATCH; i++)
		for(j = i + 1; j < PLAYERS_PER_MATCH; j++) {
			unsigned int p1 = MATCH_TO_PLAYER(i);
			unsigned int p2 = MATCH_TO_PLAYER(j);
			if(court->player_team[i] == court->player_team[j])
				set_played_together(court->pt, p1, p2);
			}
}

/* Auxiliar function that receives a MSG_PLAYER_JOIN_REQ message
 * for court and determinates if that player can join or not. If
 * the player can join the match, this functions accepts them and
 * assign them a team. If the player can't, this function should
 * kick them off!*/
 // TODO: Rewrite this using the court_team_t
void handle_player_team(court_t* court, message_t msg){
	static int team_members[2] = {0};
	int p_act;
	bool joined;
	switch(court->connected_players){
		case 0: // First player to connect. Instantly accepted
			connect_player_in_team(court, msg.m_player_id, 0);
			team_members[0]++;
			break;
		case 1: // Second player to connect.
			// If can team up with first player, let them do it
			if(!get_played_together(court->pt, court->player_connected[0], msg.m_player_id)){
				connect_player_in_team(court, msg.m_player_id, 0);
				team_members[0]++;
			} else { // If not, go to other team
				connect_player_in_team(court, msg.m_player_id, 1);
				team_members[1]++;
			}
			break;
		case 2: 
		case 3: // TODO: Find out another way of doing this, considering kicking off the player

			joined = false; // Debug only
			for(p_act = 0; p_act < court->connected_players; p_act++){
				unsigned int p_act_id = MATCH_TO_PLAYER(p_act);
				unsigned int p_act_team = court->player_team[p_act];

				if(!get_played_together(court->pt, msg.m_player_id, p_act_id)){
					if(team_members[p_act_team] < 2) {
						connect_player_in_team(court, msg.m_player_id, p_act_team);
						team_members[p_act_team]++;
						joined = true;
						break;
					}
					else if(team_members[OTHER_TEAM(p_act_team)] < 2) {
						connect_player_in_team(court, msg.m_player_id, OTHER_TEAM(p_act_team));
						team_members[OTHER_TEAM(p_act_team)]++;
						joined = true;
						break;
					}
					else {
						// Should not happen
						log_write(CRITICAL_L, "Oops!! Player %03d tried to join a full match.!\n", msg.m_player_id);
						return;
					}
				}
			}
			if (!joined) {
				log_write(CRITICAL_L, "Player %03d couldn't find a team, we should kick him!!\n", msg.m_player_id);
				reject_player(court, msg.m_player_id);
				close(court->player_fifos[court->connected_players]);
			}
			break;
			
		default:
			log_write(CRITICAL_L, "Wrong value for court->connected_players: %d\n", court->connected_players);
			break; // Should not happen
	}
	
				
	if(court->connected_players == PLAYERS_PER_MATCH) {
		team_members[0] = 0;
		team_members[1] = 0;
	}

}


/* Auxiliar function that opens this court's fifo. If fifo was 
 * already opened, silently does nothing.
 * Watch out! open is blocking!*/
void open_court_fifo(court_t* court){
	if (court->court_fifo < 0) {
		log_write(INFO_L, "Court FIFO is closed, need to open one\n", errno);
		int court_fifo = open(court->court_fifo_name, O_RDONLY);
		log_write(INFO_L, "Court FIFO opened!!!\n", errno);
		if (court_fifo < 0) {
			log_write(ERROR_L, "FIFO opening error for court %03d [errno: %d]\n", court->court_id, errno);
			exit(-1);
		}
		court->court_fifo = court_fifo;
	}
}

// --------------------------------------------------------------

/* Dynamically creates a new court. Returns NULL in failure.
 * Pre 1: The players processes were already created the fifos for communicating with
 * them are the ones received.
 * Pre 2: The players processes ARE CHILDREN PROCESSES of the process using this court.*/
court_t* court_create(unsigned int court_id, partners_table_t* pt) {

	court_t* court = malloc(sizeof(court_t));
	if (!court) return NULL;
	
	court->court_fifo = -1;
	court->court_id = court_id;
	int i;
	for (i = 0; i < PLAYERS_PER_MATCH; i++) {
		court->player_fifos[i] = -1;
	}

	for (i = 0; i < TOTAL_PLAYERS; i++) {
		court->player_points[i] = 0;
	}

	court->sets_won_away = 0;
	court->sets_won_home = 0;
	court->pt = pt;
	court->connected_players = 0;
	return court;
}

/* Kills the received court and sends flowers
 * to his widow.*/
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

	log_write(INFO_L, "A new match is about to begin... lobby\n", errno);
	court->connected_players = 0;
	message_t msg = {};

	while (court->connected_players < PLAYERS_PER_MATCH) {
		// Now court is in the "empty" state, waiting for new connections
		open_court_fifo(court);
		log_write(INFO_L, "Court awaiting connections\n", errno);
		
		// A connection has been made! Waiting for initial message!
		if(!receive_msg(court->court_fifo, &msg)) {
			log_write(ERROR_L, "Court %03d bad read new connection message [errno: %d]\n", court->court_id, errno);
			close(court->court_fifo);
			court->court_fifo = -1;
		} else {
			if (msg.m_type != MSG_PLAYER_JOIN_REQ) {
				log_write(ERROR_L, "Court %03d received a non-request-to-join msg\n", court->court_id);
				close(court->court_fifo); // Why aborting instead of just ignoring the msg?
				court->court_fifo = -1;
			} else {
				// Is a request, let's open other player's fifo!
				char player_fifo_name[MAX_FIFO_NAME_LEN];
				if(!get_player_fifo_name(msg.m_player_id, player_fifo_name)) {
					log_write(ERROR_L, "FIFO opening error for player %d fifo at court %03d [errno: %d]\n", msg.m_player_id, court->court_id, errno);
					return;
				}
				court->player_fifos[court->connected_players] = open(player_fifo_name, O_WRONLY);
				if (court->player_fifos[court->connected_players] < 0) {
					log_write(ERROR_L, "FIFO opening error for player %d fifo at court %03d [errno: %d]\n", msg.m_player_id, court->court_id, errno);
				} else {
					log_write(INFO_L, "Court will handle player %d\n", msg.m_player_id);
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


/* Plays the court. Communication is done
 * using the players' fifos, and sets are
 * played until one of the two teams wins
 * SETS_WINNING sets, or until SETS_AMOUNT
 * sets are played. If the close_pipes field
 * was set on this court creation, this
 * function also closes the fifos file des-
 * criptors at the end of the court. */
void court_play(court_t* court){
	int i, j;
	unsigned long int players_scores[PLAYERS_PER_MATCH] = {};
	message_t msg = {};

	// Play SETS_AMOUNT sets
	for (j = 0; j < SETS_AMOUNT; j++) {
		msg.m_type = MSG_SET_START;

		log_write(NONE_L, "Set %d started!\n", j+1);
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
				log_write(ERROR_L, "Error reading score from player %03d at court %03d [errno: %d]\n", i, court->court_id, errno);
			else {
				log_write(INFO_L, "Received %d from player %03d at court %03d\n", msg.m_type, msg.m_player_id, court->court_id);
				assert(msg.m_type == MSG_PLAYER_SCORE);
				players_scores[PLAYER_TO_MATCH(msg.m_player_id)] = msg.m_score;
			}
		}
		// Show this set score
		for(i = 0; i < PLAYERS_PER_MATCH; i++)
			log_write(NONE_L, "Player %03d set score: %ld\n", MATCH_TO_PLAYER(i), players_scores[i]);
		
		// Determinate the winner of the set
		// I know this is my own code, but it's ugly.
		// Prop: Rewrite this using court_team_t with scores
		unsigned long int score_home = 0, score_away = 0;
		for (i = 0; i < PLAYERS_PER_MATCH; i++) {
			if (court->player_team[i] == 0)
				score_home += players_scores[i];
			else
				score_away += players_scores[i];
		}

		if (score_home > score_away)
			court->sets_won_home++;
		else
			court->sets_won_away++;

		// If any won SETS_WINNING sets than the other, court over
		if(court->sets_won_home == SETS_WINNING)
			break;
		if(court->sets_won_away == SETS_WINNING)
			break;	
	}
		
	// Here we make the players stop the match
	for (i = 0; i < PLAYERS_PER_MATCH; i++) {
		msg.m_type = MSG_MATCH_END;
		send_msg(court->player_fifos[i], &msg);
	}

	// Close pipes and show final results
	// TODO: Refactor with new court_team_t
/*	if(court->sets_won_home > court->sets_won_away) {
		log_write(NONE_L, "Team 1 won!\n");
		for (i = 0; i < PLAYERS_PER_MATCH; i++)
			if (court->player_team[i] == 0)
				court->player_points[i]++;
	} else {
		log_write(NONE_L, "Team 2 won!\n");	
		for (i = 0; i < PLAYERS_PER_MATCH; i++)
			if (court->player_team[i] == 1)
				court->player_points[i]++;
	}*/
	
	int won_team = (int) (court->sets_won_home < court->sets_won_away);  
	log_write(NONE_L, "Team %d won!\n", won_team + 1);
	for (i = 0; i < PLAYERS_PER_MATCH; i++)
		if (court->player_team[i] == won_team)
			court->player_points[MATCH_TO_PLAYER(i)]++;
			
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
	return (court ? court->sets_won_home : 0);
}

size_t court_get_away_sets(court_t* court){
	return (court ? court->sets_won_away : 0);
}




void court_main(unsigned int court_id, partners_table_t* pt) {

	// Launch a single court, then end the tournament
	// TODO: refactor this? Make it singleton to handle signals?
	court_t* court = court_create(court_id, pt);
	if(!court)
		exit(-1);
		
	log_write(INFO_L, "Launched new court [%03d]: using PID: %d\n", court->court_id, getpid());

	// Creating fifo for court
	get_court_fifo_name(court_id, court->court_fifo_name);
	if(!create_fifo(court->court_fifo_name)) {
		log_write(ERROR_L, "FIFO creation error for court %03d [errno: %d]\n", errno, court->court_id);
		exit(-1);
	}

	// Como es un semaphore set.. se pueden crear de un saque todos lo semáforos!
	int sem = sem_get(court->court_fifo_name, 1);
	if (sem < 0)
		log_write(ERROR_L, "Error creating semaphore [errno: %d]\n", errno);

	log_write(ERROR_L, "Got semaphore %d at main\n", sem);
	if (sem_init(sem, 0, 4) < 0)
		log_write(ERROR_L, "Error initializing the semaphore [errno: %d]\n", errno);

	int i;
	for (i = 0; i < NUM_MATCHES; i++) 
		court_lobby(court);

	for (i = 0; i < TOTAL_PLAYERS; i++) 
		log_write(INFO_L, "Player %03d won a total of %d matches\n", i, court->player_points[i]);

	log_write(INFO_L, "Destroying court\n", i, court->player_points[i]);
	
	partners_table_destroy(court->pt);
	court_destroy(court);
	sem_destroy(sem);
	log_close();
	exit(0);

}
