

#include <sys/types.h>
#include <unistd.h>
#include "semaphore.h"
#include "partners_table.h"
#include "log.h" 
#include "protocol.h"



// Abbreviate maybe?
// Neh... though I ain't like having this definitions
// here instead of having'em in some .h
typedef enum _player_status {
	TNM_P_OUTSIDE,
	TNM_P_IDLE,
	TNM_P_PLAYING,
	TNM_P_LEAVED
} p_status;


typedef enum _court_status {
	TNM_C_FREE,
	TNM_C_BUSY,
	TNM_C_DISABLED
} c_status;


typedef struct tournament {
	unsigned int tnm_idle_players;
	unsigned int tnm_active_players;

	unsigned int tnm_idle_courts;
	unsigned int tnm_active_courts;

	p_status tnm_players[TOTAL_PLAYERS];
	c_status tnm_courts[TOTAL_COURTS];
} tournament_t;

typedef struct referee {
	char ref_fifo_name[MAX_FIFO_NAME_LEN];
	int ref_fifo_fd;
	int ref_sem;

	// Tournament stats & tallys
	tournament_t ref_tnm;
} referee_t;

// Referee variable local for this module.
// Is this legit??
// Legit or not, it's an ugly workaround on my opinion
// I'd rather singletonize these, y'know
referee_t ref = {};
log_t* myLog;
partners_table_t* pt;



void do_something() {
	log_write(CRITICAL_L, "Magic %d\n", ref.ref_tnm.tnm_idle_players);
}



void referee_main(partners_table_t* _pt) {
	myLog = log_get_instance();
	pt = _pt;
	ref.ref_tnm.tnm_idle_players = 10;
	do_something();

	log_write(INFO_L, "Launched referee using PID: %d\n", getpid());

	/*
	 * while( !tournament_ends() ) {
	 *	sem_wait(ref.ref_sem, 0);	// Esperar a que haya algún mensaje en la cola
	 *
	 *	read_msg();
	 *	if (msg.type == MSG_PLAYER_IDLE) {
	 *		idle_players++;
	 *		players[msg.id] = TNM_P_IDLE;
	 *		if (idle_players >= 4 & idle_courts > 0)
	 *			create_match();
	 *	} else if (msg.type = MSG_COURT_FREE) {
	 *		idle_courts++;
	 *		courts[msg.id] = TNM_C_FREE;
	 *		if (idle_players >= 4 & idle_courts > 0)
	 *			create_match();
	 *	} else if (msg.type = MSG_PLAYER_LEAVES) {
	 *		idle_players--;
	 *		players[msg.id] = TNM_P_OUTSIDE;
	 *	} else if (msg.type = MSG_PLAYER_EXITS) {
	 *		check_if_tournament_should_end();
	 *	}
	 * }
	 *
	 */



	log_write(CRITICAL_L, "Does this work??\n");
	log_close();
	partners_table_destroy(pt);
	exit(0);
}
