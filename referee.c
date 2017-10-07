
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

	log_write(myLog, CRITICAL_L, "Does this work??\n");
	log_close(myLog);
	exit(0);
}
