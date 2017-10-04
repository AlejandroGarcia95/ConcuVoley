#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <sys/time.h>
#include "lock.h"

/* Creates a lock associated with the file descriptor
 * given. Returns NULL in case of error.*/
lock_t* lock_create(int fd, int etc){
	return NULL;
}

/* Sends lock to lock's heaven.*/
void lock_destroy(lock_t* lock){
	
}

/* Acquires the lock. If it can't be acquired, blocks
 * the current process until it can be obtained.
 * Pre: the process ain't have the lock.
 * Post: the process has the lock, and no other process
 * acquire it until this process releases it.*/
void lock_acquire(lock_t lock){
	
}

/* Releases the lock, allowing other processes to
 * acquire it.
 * Pre: the process HAS the lock, and is the only
 * one with it.
 * Post: the process ain't have the lock.*/
void lock_release(lock_t lock){
	
}
