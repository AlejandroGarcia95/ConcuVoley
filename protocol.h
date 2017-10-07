#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "log.h"
#include "protocol.h"

// Temporary
#define TOTAL_PLAYERS 4
#define TOTAL_COURTS 1
#define NUM_MATCHES 2


#define MAX_FIFO_NAME_LEN 50
#define FIFO_CREAT_FLAGS (S_IFIFO | 0666)

#define MAIN_FIFO_NAME ("fifos/main.fifo")
/*
 *			FIFO Naming convention
 *
 * If you are a player, your fifo is...
 *		fifos/player_{id}.fifo
 *	where {id} is your player_id, with exactly 3 digits
 *	(i.e. if player_id = 1 then the fifo is fifos/player_001.fifo
 *
 * If you are a court, your fifo is...
 *		fifos/court_{id}.fifo
 *	where {id} is your court_id, with exactly 3 digits
 */

typedef enum _msg_type {
	MSG_PLAYER_JOIN_REQ = 0,
	MSG_PLAYER_SCORE,
	MSG_SET_START,
	MSG_MATCH_ACCEPT,
	MSG_MATCH_REJECT,
	MSG_MATCH_END,
	MSG_FREE_COURT,
	MSG_TOURNAMENT_END
} msg_type;


struct message {
	msg_type m_type;
	unsigned int m_player_id;
	unsigned long int m_score;
	unsigned int m_court_id;
};

typedef struct message message_t;

/* Stores player's fifo filename from their id on dest_buffer.
 * Returns true if it was successful, or false otherwise.*/
bool get_player_fifo_name(unsigned int id, char* dest_buffer);
bool get_court_fifo_name(unsigned int id, char* dest_buffer);
bool get_referee_fifo_name(char* dest_buffer);

bool create_fifo(char* fifo_name);

/* Receives a message from fifo_fd and stores it on msg.
 * On any error, returns false. Notice read is blocking.*/
bool receive_msg(int fifo_fd, message_t* msg);

/* Sends the message msg through fifo_fd. Returns true if
 * successful, or false otherwise.*/
bool send_msg(int fifo_fd, message_t* msg);

// Prop: Create a function "message_create(m_type, m_id, m_score)"
// for shortening code


#endif //PROTOCOL_H
