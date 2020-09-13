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
	int min_energy_to_activate;

	// not persisted
	bool is_toggled_on;
	bool is_held;
	bool press_within_duration;//20200827: will reuse for toggle/hold as well
	int64_t press_began;//20200827: will reuse for toggle/hold as well
	//int64_t last_key_press;//for hold

	bool is_in_cooldown;
	int64_t cooldown_began;

	// bool have_enough_energy;
	float current_energy_level;

} control_t;




control_manager_t*   control_manager_create(db_owner_t* db);
void                 control_manager_destroy(control_manager_t* c);
void                 control_manager_foreach_control(control_manager_t* c, void(*cb)(void* context, const control_t* control), void* context);
// void                 control_manager_delete_control(cont)

// Call these funcs ONLY during a control_manager_foreach_control cb
void                 control_manager_que_control_modification(control_manager_t* c, control_t* cont);
void                 control_manager_que_control_deletion(control_manager_t* c, control_t* cont);

// 0=nothing, 1=active, 2=unavailable
int                 control_manager_duringcallback_get_control_state(control_manager_t* c, control_t* cont);

void                 control_manager_que_control_insertion(control_manager_t* c);




float                 control_manager_calculate_timescale(control_manager_t* c);
void                 control_manager_sleep(control_manager_t* c);

 void update_setting_in_db( control_manager_t *cm);


SEPPLES_GUARD_END