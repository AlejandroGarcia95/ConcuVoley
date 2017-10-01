#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdio.h>
#include <stdlib.h>

// Temporary
#define NUM_MATCHES 3


#define MAX_FIFO_NAME_LEN 50
#define FIFO_CREAT_FLAGS (S_IFIFO | 0666)

/*
 *			FIFO Naming convention
 *
 * If you are a player, your fifo is...
 *		fifos/player_{id}.fifo
 *	where {id} is your player_id, with exactly 3 digits
 *	(i.e. if player_id = 1 then the fifo is fifos/player_001.fifo
 *
 * If you are a court, your fifo is...
 *		fifos/player_{id}.fif
 *	where {id} is your court_id, with exactly 3 digits
 */

typedef enum _msg_type {
	MSG_PLAYER_JOIN_REQ = 0,
	MSG_PLAYER_SCORE,
	MSG_SET_START,
	MSG_MATCH_END
} msg_type;


struct message {
	msg_type m_type;
	unsigned int m_player_id;
	unsigned long int m_score;
};

typedef struct message message_t;

#endif //PROTOCOL_H
