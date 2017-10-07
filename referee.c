
#include "semaphore.h"
#include "partners_table.h"
#include "log.h" 
#include "protocol.h"



// Abbreviate maybe?
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
referee_t ref = {};
log_t* myLog;
partners_table_t* pt;



void do_something() {
	log_write(myLog, CRITICAL_L, "Magic %d\n", ref.ref_tnm.tnm_idle_players);
}



void referee_main(log_t* _log, partners_table_t* _pt) {
	myLog = _log;
	pt = _pt;
	ref.ref_tnm.tnm_idle_players = 10;
	do_something();

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



	log_write(myLog, CRITICAL_L, "Does this work??\n");
	log_close(myLog);
	exit(0);
}
