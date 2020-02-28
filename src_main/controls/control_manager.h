#pragma once



#include "../misc.h"

SEPPLES_GUARD_START

TYPEDEF_STRUCT(control_manager)



#include "../../shared.h"
#include "../db/db_owner.h"

typedef struct {
	int id;
	int activation_mode; // 0=press, 1=hold, 2=toggle
	key_t key;

	int slow_or_fast; // 0 = slow, 1 = fast
	int slowdown_or_speedup_percent;
	int duration; // seconds

	int limit_mode; // 0 = no limit, 1 = cooldown, 2 = energy

	int cooldown_secs;

	int max_energy;
	int cost_per_use;
	int recharge_rate;

	// not persisted
	bool is_toggled_on;
	bool is_held;
	bool press_within_duration;
	int64_t press_began;
} control_t;




control_manager_t*   control_manager_create(db_owner_t* db);
void                 control_manager_destroy(control_manager_t* c);
void                 control_manager_foreach_control(control_manager_t* c, void(*cb)(void* context, const control_t* control), void* context);
// void                 control_manager_delete_control(cont)

// Call these funcs ONLY during a control_manager_foreach_control cb
void                 control_manager_que_control_modification(control_manager_t* c, control_t* cont);
void                 control_manager_que_control_deletion(control_manager_t* c, control_t* cont);
bool                 control_manager_duringcallback_is_control_active(control_manager_t* c, control_t* cont);

void                 control_manager_que_control_insertion(control_manager_t* c);




float                 control_manager_calculate_timescale(control_manager_t* c);
void                 control_manager_sleep(control_manager_t* c);


SEPPLES_GUARD_END