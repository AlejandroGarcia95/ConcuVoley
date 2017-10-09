#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "lock.h"

/* Creates a lock associated with lock_name
 * given. Returns NULL in case of error.*/
lock_t* lock_create(char* lock_name){
	if(!lock_name) return NULL;
	
	lock_t* lock = malloc(sizeof(lock_t));
	if(!lock) return NULL;
	
	lock->fl.l_type = F_WRLCK;
	lock->fl.l_whence = SEEK_SET;
	lock->fl.l_start = 0;
	lock->fl.l_len = 0;
	
	sprintf(lock->name, "locks/");
	strcat(lock->name, lock_name);
	strcat(lock->name, ".lock");
	
	lock->fd = open(lock->name, LOCK_CREAT_FLAGS, LOCK_CREAT_PERMS);
	if(lock->fd < 0) {
		free(lock);
		return NULL;
	}
	
	return lock;
}

/* Sends lock to lock's heaven.*/
void lock_destroy(lock_t* lock){
	if(!lock) return;
	close(lock->fd);
	free(lock);
}

/* Acquires the lock. If it can't be acquired, blocks
 * the current process until it can be obtained.
 * Pre: the process ain't have the lock.
 * Post: the process has the lock, and no other process
 * acquire it until this process releases it.*/
int lock_acquire(lock_t* lock){
	lock->fl.l_type = F_WRLCK;
	return fcntl(lock->fd, F_SETLKW, &(lock->fl));
}

/* Releases the lock, allowing other processes to
 * acquire it.
 * Pre: the process HAS the lock, and is the only
 * one with it.
 * Post: the process ain't have the lock.*/
int lock_release(lock_t* lock){
	lock->fl.l_type = F_UNLCK;
	return fcntl(lock->fd, F_SETLK, &(lock->fl));
}
