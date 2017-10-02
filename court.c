#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <assert.h>
#include "court.h"
#include "log.h"
#include "partners_table.h"

#include "protocol.h"

void connect_player_in_team(court_t* court, log_t* log, unsigned int p_id, unsigned int team){
	court->player_connected[court->connected_players] = p_id;
	court->player_team[p_id] = team;
	log_write(log, INFO_L, "Player %03d is connected at court 000 for team %d\n", p_id, team + 1);
	court->connected_players++;
}

/* Auxiliar function that receives a MSG_PLAYER_JOIN_REQ message
 * for court and determinates if that player can join or not. If
 * the player can join the match, this functions accepts them and
 * assign them a team. If the player can't, this function should
 * kick them off!*/
 // TODO: Find out a more beautiful way of thinking this
 // Prop: Store in court_t two arrays of 2 elements... each one
 // is to store the players' ids of each team
void handle_player_team(court_t* court, log_t* log, message_t msg){
	static int team_members[2] = {0};
	int p_act;
	switch(court->connected_players){
		case 0: // First player to connect. Instantly accepted
			connect_player_in_team(court, log, msg.m_player_id, 0);
			team_members[0]++;
			break;
		case 1: // Second player to connect.
			// If can team up with first player, let them do it
			if(!get_played_together(court->pt, court->player_connected[0], msg.m_player_id)){
				connect_player_in_team(court, log, msg.m_player_id, 0);
				team_members[0]++;
			}
			else { // If not, go to other team
				connect_player_in_team(court, log, msg.m_player_id, 1);
				team_members[1]++;
				}
			break;
		case 2: 
		case 3: // TODO: Find out another way of doing this, considering kicking off the player
			for(p_act = 0; p_act < court->connected_players; p_act++){
				unsigned int p_act_id = court->player_connected[p_act];
				unsigned int p_act_team = court->player_team[p_act_id];
				if(!get_played_together(court->pt, msg.m_player_id, p_act_id)){
					if(team_members[p_act_team] < 2) {
						connect_player_in_team(court, log, msg.m_player_id, p_act_team);
						team_members[p_act_team]++;
						break;
					}
					else if(team_members[!p_act_team] < 2) {
						connect_player_in_team(court, log, msg.m_player_id, !p_act_team);
						team_members[!p_act_team]++;
						break;
					}
					else {
						// Should not happen
						return;
						}
				}
					
			}
				
			break;
			
		default:
			break; // Should not happen
	}
	
				
	if(court->connected_players == PLAYERS_PER_MATCH) {
		team_members[0] = 0;
		team_members[1] = 0;
		}

}

// ---------------------------------------------------------------------

/* Opens this court's fifo. If fifo was already
 * opened, silently does nothing.
 * Watch out! open is blocking!*/
void open_court_fifo(court_t* court, log_t* log){
	if (court->court_fifo < 0) {
		log_write(log, INFO_L, "Court FIFO is closed, need to open one\n", errno);
		int court_fifo = open(court->court_fifo_name, O_RDONLY);
		if (court_fifo < 0) {
			log_write(log, ERROR_L, "FIFO opening error for court 000 [errno: %d]\n", errno);
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
court_t* court_create(log_t* log,  partners_table_t* pt) {

	court_t* court = malloc(sizeof(court_t));
	if (!court) return NULL;
	
	//memcpy(&court->close_pipes, &close_pipes, sizeof(int) * 2 * PLAYERS_PER_MATCH);
	
	// Creating fifo for court
	// Ideally move this to main as the courts are build
	// Why should it be moved to main? It could stay here by receiving a court
	// id number as an argument (i.e. "court_create(log_t* log, int court_id)" )
	char* court_fifo_name = "fifos/court.fifo";
	if(!create_fifo(court_fifo_name)) {
		log_write(log, ERROR_L, "FIFO creation error for court 000 [errno: %d]\n", errno);
		exit(-1);
	}
	court->court_fifo_name = court_fifo_name;
	court->court_fifo = -1;
	int i;
	for (i = 0; i < PLAYERS_PER_MATCH; i++) {
		court->player_fifos[i] = -1;
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
void court_lobby(court_t* court, log_t* log) {

	log_write(log, INFO_L, "A new match is about to begin... lobby\n", errno);
	court->connected_players = 0;
	message_t msg = {};

	while (court->connected_players < PLAYERS_PER_MATCH) {
		// Now court is in the "empty" state, waiting for new connections
		open_court_fifo(court, log);
		log_write(log, INFO_L, "Court awaiting connections\n", errno);
		
		// A connection has been made! Waiting for initial message!
		if(!receive_msg(court->court_fifo, &msg)) {
			log_write(log, ERROR_L, "Court 000 bad read new connection message [errno: %d]\n", errno);
			close(court->court_fifo);
			court->court_fifo = -1;
		} else {
			if (msg.m_type != MSG_PLAYER_JOIN_REQ) {
				log_write(log, ERROR_L, "Court 000 received a non-request-to-join msg\n");
				close(court->court_fifo); // Why aborting instead of just ignoring the msg?
				court->court_fifo = -1;
			} else {
				// Is a request, let's open other player's fifo!
				char player_fifo_name[MAX_FIFO_NAME_LEN];
				if(!get_player_fifo_name(msg.m_player_id, player_fifo_name)) {
					log_write(log, ERROR_L, "FIFO opening error for player %d fifo at court 000 [errno: %d]\n", msg.m_player_id, errno);
					return;
				}
				court->player_fifos[court->connected_players] = open(player_fifo_name, O_WRONLY);
				if (court->player_fifos[court->connected_players] < 0) {
					log_write(log, ERROR_L, "FIFO opening error for player %d fifo at court 000 [errno: %d]\n", msg.m_player_id, errno);
				} else {
					handle_player_team(court, log, msg);
				}
			}
		}
	}
	
	court_play(court, log);

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
void court_play(court_t* court, log_t* log){
	int i, j;
	unsigned long int players_scores[PLAYERS_PER_MATCH] = {};
	message_t msg = {};

	// Play SETS_AMOUNT sets
	for (j = 0; j < SETS_AMOUNT; j++) {
		msg.m_type = MSG_SET_START;

		log_write(log, NONE_L, "Set %d started!\n", j+1);
		// Here we make the four players play a set by  
		// sending them a message through the pipe.  
		// Change later for a better message protocol.
		for (i = 0; i < PLAYERS_PER_MATCH; i++) {
			send_msg(court->player_fifos[i], &msg);
		}
		
		// Let the set last 6 seconds for now. After that, the
		// main process will make all players stop
		// TODO: Change this for something random!
		sleep(SET_DURATION);
		court_finish_set(court);
		
		// Wait for the four player's scores
		for (i = 0; i < PLAYERS_PER_MATCH; i++){
			if(!receive_msg(court->court_fifo, &msg))
				log_write(log, ERROR_L, "Error reading score from player %03d at court 000 [errno: %d]\n", i, errno);
			else {
				assert(msg.m_type == MSG_PLAYER_SCORE);
				players_scores[msg.m_player_id] = msg.m_score;
			}
		}
		// Show this set score
		for(i = 0; i < PLAYERS_PER_MATCH; i++)
			log_write(log, NONE_L, "Player %03d set score: %ld\n", i, players_scores[i]);
		
		// Determinate the winner of the set
		// I know this is my own code, but it's ugly. Prop: Send it to another function
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
	// TODO: watch out for this homebrew method for mantain global score, it's rubbish!
	// More that courtes won, we should store players' points
/*	if(court->sets_won_home > court->sets_won_away) {
		log_write(log, NONE_L, "Team 1 won!\n");
		for (i = 0; i < PLAYERS_PER_MATCH; i++)
			if (court->player_team[i] == 0)
				court->player_points[i]++;
	} else {
		log_write(log, NONE_L, "Team 2 won!\n");	
		for (i = 0; i < PLAYERS_PER_MATCH; i++)
			if (court->player_team[i] == 1)
				court->player_points[i]++;
	}*/
	
	int won_team = (int) (court->sets_won_home < court->sets_won_away);  
	log_write(log, NONE_L, "Team %d won!\n", won_team + 1);
	for (i = 0; i < PLAYERS_PER_MATCH; i++)
		if (court->player_team[i] == won_team)
			court->player_points[i]++;
			
	// Mark each players' partner
	// Diu
	for(i = 0; i < PLAYERS_PER_MATCH; i++)
		for(j = i + 1; j < PLAYERS_PER_MATCH; j++) {
			unsigned int p1 = court->player_connected[i];
			unsigned int p2 = court->player_connected[j];
			if(court->player_team[p1] == court->player_team[p2])
				set_played_together(court->pt, p1, p2);
			}
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
