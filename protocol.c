#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "log.h"
#include "protocol.h"

/* Stores player's fifo filename from their id on dest_buffer.
 * Returns true if it was successful, or false otherwise.*/
bool get_player_fifo_name(unsigned int id, char* dest_buffer) {
	if(sprintf(dest_buffer, "fifos/player_%03d.fifo", id) < 0)
		return false;
	return true;
}

bool create_fifo(char* fifo_name){
	if(mknod(fifo_name, FIFO_CREAT_FLAGS, 0) < 0)
		return false;
	return true;
}

/* Receives a message from fifo_fd and stores it on msg.
 * On any error, returns false. Notice read is blocking.*/
// TODO: reemplazar con un while para evitar lecturas parciales?
bool receive_msg(int fifo_fd, message_t* msg){
	if (read(fifo_fd, msg, sizeof(message_t)) < sizeof(message_t))
		return false;
	return true;
}

/* Sends the message msg through fifo_fd. Returns true if
 * successful, or false otherwise.*/
bool send_msg(int fifo_fd, message_t* msg){
	if (write(fifo_fd, msg, sizeof(message_t)) < sizeof(message_t))
		return false;
	return true;
}
