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
#include "match.h"
#include "log.h"

#include "protocol.h"


/* Dynamically creates a new match. Returns NULL in failure.
 * Pre 1: The players processes were already created the fifos for communicating with
 * them are the ones received.
 * Pre 2: The players processes ARE CHILDREN PROCESSES of the process using this match.*/
match_t* match_create(log_t* log) {

	match_t* match = malloc(sizeof(match_t));
	if (!match) return NULL;
	
	//memcpy(&match->close_pipes, &close_pipes, sizeof(int) * 2 * PLAYERS_PER_MATCH);
	
	// Creating fifo fir court
	// Ideally move this to main as the courts are build
	char* court_fifo_name = "fifos/court.fifo";
	if (mknod(court_fifo_name, FIFO_CREAT_FLAGS, 0) < 0) {
		log_write(log, ERROR_L, "FIFO creation error for court 000 [errno: %d]\n", errno);
		exit(-1);
	}
	match->court_fifo_name = court_fifo_name;
	match->court_fifo = -1;

	for (int i = 0; i < PLAYERS_PER_MATCH; i++) 
		match->player_fifos[i] = -1;

	match->sets_won_away = 0;
	match->sets_won_home = 0;
	return match;
}

/* Kills the received match and sends flowers
 * to his widow.*/
void match_destroy(match_t* match){
	free(match);
	// Sry, no flowers
}



/*
 * Starts the match in its 'lobby' state. For now match is empty and
 * waiting for incomming players. It will open its fifo, and accept any
 * incomming connections. When all players are connected, match starts
 * and calls to match_play.
 * Once match ends, it comes here again.. eager to start another match.
 */
void match_lobby(match_t* match, log_t* log) {

	unsigned int connected_players = 0;
	message_t msg = {};

	while (connected_players < PLAYERS_PER_MATCH) {
		// Now match is in the "empty" state, waiting for new connections
		// Watch out! open is blocking!
		if (match->court_fifo < 0) {
			int court_fifo = open(match->court_fifo_name, O_RDONLY);
			if (court_fifo < 0) {
				log_write(log, ERROR_L, "FIFO opening error for court 000 [errno: %d]\n", errno);
				exit(-1);
			}
			match->court_fifo = court_fifo;
		}

		// A connection has been made! Waiting for initial message!
		if (read(match->court_fifo, &msg, sizeof(message_t)) < 0) {
			log_write(log, ERROR_L, "Court 000 bad read new connection message [errno: %d]\n", errno);
			close(match->court_fifo);
			match->court_fifo = -1;
		} else {
			if (msg.m_type != MSG_PLAYER_JOIN_REQ) {
				log_write(log, ERROR_L, "Court 000 received a non-request-to-join msg\n");
				close(match->court_fifo);
				match->court_fifo = -1;
			} else {
				// Is a request, let's open other player's fifo!
				char* player_fifo_name = get_player_fifo_name(msg.m_player_id);
				match->player_fifos[connected_players] = open(player_fifo_name, O_WRONLY);
				if (match->player_fifos[connected_players] < 0)
					log_write(log, ERROR_L, "FIFO opening error for player %d fifo at court 000 [errno: %d]\n", msg.m_player_id, errno);
				else
					connected_players++;
				free(player_fifo_name);
			}
		}
	}
	
	match_play(match, log);

	close(match->court_fifo);
	match->court_fifo = -1;
	for (int i = 0; i < PLAYERS_PER_MATCH; i++) {
		match->player_fifos[i] = -1;
	}
}


/* Plays the match. Communication is done
 * using the players' fifos, and sets are
 * played until one of the two teams wins
 * SETS_WINNING sets, or until SETS_AMOUNT
 * sets are played. If the close_pipes field
 * was set on this match creation, this
 * function also closes the fifos file des-
 * criptors at the end of the match. */
void match_play(match_t* match, log_t* log){
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
			write(match->player_fifos[i], &msg, sizeof(message_t));	
		}
		
		// Let the set last 6 seconds for now. After that, the
		// main process will make all players stop
		sleep(SET_DURATION);
		match_finish_set(match);
		
		// Wait for the four player's scores
		for (i = 0; i < PLAYERS_PER_MATCH; i++){
			if (read(match->court_fifo, &msg, sizeof(msg)) < 0) {
				log_write(log, ERROR_L, "Error reading score from player %03d at court 000 [errno: %d]\n", i, errno);
			} else {
				assert(msg.m_type == MSG_PLAYER_SCORE);
				players_scores[msg.m_player_id] = msg.m_score;
			}
		}
		// Show this set score
		for(i = 0; i < PLAYERS_PER_MATCH; i++)
			log_write(log, NONE_L, "Player %03d set score: %ld\n", i, players_scores[i]);
		
		// Determinate the winner of the set
		unsigned long int score_home = 0, score_away = 0;
		for (i = 0; i < PLAYERS_PER_MATCH; i++) {
			if (i < PLAYERS_PER_TEAM)
				score_home += players_scores[i];
			else
				score_away += players_scores[i];
		}

		if(score_home > score_away)
			match->sets_won_home++;
		else
			match->sets_won_away++;
		// If any won SETS_WINNING sets than the other, match over
		if(match->sets_won_home == SETS_WINNING)
			break;
		if(match->sets_won_away == SETS_WINNING)
			break;	
		}
		
	// Here we make the players stop the match
	for (i = 0; i < PLAYERS_PER_MATCH; i++) {
		msg.m_type = MSG_MATCH_END;
		write(match->player_fifos[i], &msg, sizeof(message_t));	
	}

	// Close pipes and show final results
	if(match->sets_won_home > match->sets_won_away)
		log_write(log, NONE_L, "Team 1 won!\n");
	else
		log_write(log, NONE_L, "Team 2 won!\n");	
}

/* Finish the current set by signaling
 * the players with SIG_SET.*/
void match_finish_set(match_t* match){
	kill(0, SIG_SET);
}

/* Returns the sets won by home and away 
 * playes. If match is NULL, let them both 
 * return 0 for the time being.*/
size_t match_get_home_sets(match_t* match){
	return (match ? match->sets_won_home : 0);
}

size_t match_get_away_sets(match_t* match){
	return (match ? match->sets_won_away : 0);
}
