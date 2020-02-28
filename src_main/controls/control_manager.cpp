#include "../windows_include.h"

#include "control_manager.h"


#include "../util.h"

#include <functional>
#include "../../third_party/sqlite/sqlite3.h"

#include "../../src_ui_impl/gui_impl_cpp.hpp"

#include "../../third_party/mingw-std-threads/mingw.thread.h"
#include "../../third_party/mingw-std-threads/mingw.mutex.h"
#include "../../third_party/mingw-std-threads/mingw.condition_variable.h"

#include <set>
#include <vector>

namespace std {
  template <> struct less<control_t>
  {
    bool operator()(const control_t& a, const control_t& b) const
    {
    	return a.id < b.id;
    }
  };
}

typedef std::set<control_t> set_of_controls;

struct _control_manager {
	std::thread thread;
	std::mutex main_mutex;
	std::mutex update_and_delete_mutex;
	db_owner_t* db_owner;

	std::vector<control_t> to_modify;
	std::vector<control_t> to_delete;
	bool should_insert;

	set_of_controls controls;

	bool should_quit;


	std::mutex controls_changed_mutex;
	std::condition_variable controls_changed;
};

static std::vector<control_t*> find_controls_matching_key(control_manager_t* ct, const key_t& key, int activation_mode) {
	std::vector<control_t*> result;
	for (const control_t& ctrl : ct->controls) {



		if (
			   (key.v_code != 0)
			&& (key.v_code    == ctrl.key.v_code)
			&& (key.scan_code == ctrl.key.scan_code)
			&& (key.ctrl      == ctrl.key.ctrl)
			&& (key.alt       == ctrl.key.alt)
			&& (key.shift     == ctrl.key.shift)
			&& (ctrl.activation_mode == activation_mode)
		) {
			// printf("match!\n");
			// naughty
			// don't mutate the id of any of these controls...
			result.push_back((control_t*)&ctrl);
		}


	}
	return result;
}

#include <chrono>
static int64_t get_seconds_epoch() {
	// todo use ms?
	std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

void control_manager_thread(control_manager_t* ct) {
	// printf("%s\n", "control_manager_thread");

	std::condition_variable* keys_cv = gui_impl_get_keys_cv();
	std::mutex* keys_mutex = gui_impl_get_keys_mutex();

	while(1)
	{
		// printf("%s\n", "control_manager_thread");
		// locked
		bool have_changed = false;
		{
			std::lock_guard<std::mutex> lock(ct->main_mutex);
			if (ct->should_quit) return;


			set_of_keys keys_down;
			set_of_keys keys_pressed;

			gui_impl_get_keys_data(&keys_down, &keys_pressed);

			if (keys_pressed.size() > 0) {
				// printf("Pressed: ");
				for (const auto& key_pressed : keys_pressed) {
					// printf("%s, ", key_pressed.desc_utf8);
					auto matching_toggle_controls = find_controls_matching_key(ct, key_pressed, 2);

					for (control_t* control : matching_toggle_controls) {
						control->is_toggled_on = !control->is_toggled_on;
						have_changed = true;
					}

					auto matching_press_controls = find_controls_matching_key(ct, key_pressed, 0);

					for (control_t* control : matching_press_controls) {
						// todo: should allow?
						if (control->press_within_duration) continue;
						int64_t now = get_seconds_epoch();
						control->press_within_duration = true;
						control->press_began = now;
						have_changed = true;

						// control->is_toggled_on = !control->is_toggled_on;
					}

				}
				// printf("\n");
			}

			for (const auto& control : ct->controls) {
				if ((control.activation_mode == 0) && (control.press_within_duration)) {
					int64_t now = get_seconds_epoch();
					if ((now - control.press_began) >= control.duration) {
						((control_t*)&control)->press_within_duration = false;
						have_changed = true;
					}
					
				} else if (control.activation_mode == 1) {
					((control_t*)&control)->is_held = false;
					have_changed = true;
					// control.is_held = false;
				}
			}

			for (const auto& key_down : keys_down) {
				auto matching_hold_controls = find_controls_matching_key(ct, key_down, 1);

				for (control_t* control : matching_hold_controls) {
					control->is_held = true;
					have_changed = true;
				}
			}

		}

		if (have_changed) {
			{
				// std::unique_lock lock2(ct->controls_changed_mutex);
			}
			ct->controls_changed.notify_all();
		}
		

		{

			// std::condition_variable test;



			std::unique_lock<std::mutex> keys_lock(*keys_mutex);


			keys_cv->wait_for(
				keys_lock,
				std::chrono::milliseconds(100)
			);

		}



		// util_sleep_for_ms(125);

	}

	
}

void control_manager_sleep(control_manager_t* c) {
	std::unique_lock<std::mutex> lock(c->controls_changed_mutex);


	c->controls_changed.wait_for(
		lock,
		std::chrono::milliseconds(500)
	);
}

static bool _is_control_active(control_manager_t* c, const control_t* cont) {
	if (cont->activation_mode == 0) { // press
		return cont->press_within_duration;
	} else if (cont->activation_mode == 1) { // hold
		return cont->is_held;
	} else if (cont->activation_mode == 2) { // toggle
		return cont->is_toggled_on;
	} else {
		return false;
	}
}

// assume the control is active- what's its timescale?
static float _get_timescale_for_control(control_manager_t* c, const control_t* cont) {
	// float mult = (cont->slow_or_fast == 1) ? 1.0 : -1.0;
	float val = ((float)cont->slowdown_or_speedup_percent) / 100.0f;
	if (cont->slow_or_fast == 1) {
		return 1.0 + val;
	} else {
		return 1.0 - val;
	}
}

float control_manager_calculate_timescale(control_manager_t* ct) {
	std::lock_guard<std::mutex> lock(ct->main_mutex);
	float timescale = 1.0;

	for (const control_t& control : ct->controls) {
		if (!_is_control_active(ct, &control)) continue;
		timescale = _get_timescale_for_control(ct, &control);
	}

	printf("timescale: %f\n", timescale);

	return timescale;
}

// int from_db_row_cb(void* _c, int num_cols) {

// }

static const char* GET_CONTROLS_QUERY = R"""(
	select * from control;
)""";

static const char* UPDATE_CONTROLS_QUERY = R"""(
	update control set
		 activation_mode = ?
		,key__v_code = ?
		,key__scan_code = ?
		,key__desc_utf8 = ?
		,key__ctrl = ?
		,key__alt = ?
		,key__shift = ?
		,slow_or_fast = ?
		,slowdown_or_speedup_percent = ?
		,duration = ?
		,limit_mode = ?
		,cooldown_secs = ?
		,max_energy = ?
		,cost_per_use = ?
		,recharge_rate = ?
	where
		id = ?
	;
)""";

#include <string.h>

static void load_control_from_db(sqlite3_stmt* get_from_db_stmt, control_t& control, bool include_id=true) {
	if (include_id) {
		control.id                                 = sqlite3_column_int(get_from_db_stmt,  0);
	}
	control.activation_mode                    = sqlite3_column_int(get_from_db_stmt,  1);
	control.key.v_code                         = sqlite3_column_int(get_from_db_stmt,  2);
	control.key.scan_code                      = sqlite3_column_int(get_from_db_stmt,  3);
	 strncpy(control.key.desc_utf8, (const char*)sqlite3_column_text(get_from_db_stmt, 4), 31);
	              control.key.desc_utf8[31] = '\0';
	control.key.ctrl                           = sqlite3_column_int(get_from_db_stmt,  5) == 1; 
	control.key.alt                            = sqlite3_column_int(get_from_db_stmt,  6) == 1; 
	control.key.shift                          = sqlite3_column_int(get_from_db_stmt,  7) == 1; 
	control.slow_or_fast                       = sqlite3_column_int(get_from_db_stmt,  8); 
	control.slowdown_or_speedup_percent        = sqlite3_column_int(get_from_db_stmt,  9); 
	control.duration                           = sqlite3_column_int(get_from_db_stmt,  10); 
	control.limit_mode                         = sqlite3_column_int(get_from_db_stmt,  11); 
	control.cooldown_secs                      = sqlite3_column_int(get_from_db_stmt,  12); 
	control.max_energy                         = sqlite3_column_int(get_from_db_stmt,  13); 
	control.cost_per_use                       = sqlite3_column_int(get_from_db_stmt,  14); 
	control.recharge_rate                      = sqlite3_column_int(get_from_db_stmt,  15); 



	control.is_toggled_on = false;
	control.is_held = false;
	control.press_within_duration = false;
	control.press_began = 0;
}

// reads control.id
// writes control.<everything else>
static void load_specific_control_from_db(sqlite3* db, control_t& control) {
	sqlite3_stmt* get_from_db_stmt;
	int prepare_result = sqlite3_prepare_v2(db, "select * from control where id = ?", -1, &get_from_db_stmt, NULL);
	sqlite3_bind_int(get_from_db_stmt, 1, control.id);
	if (prepare_result != SQLITE_OK) {
		const char* err = sqlite3_errmsg(db);
		printf("sqlite err: %s\n", err);
		abort();
	}

	if (sqlite3_step(get_from_db_stmt) != SQLITE_ROW) {
		const char* err = sqlite3_errmsg(db);
		printf("sqlite load control err: %s\n", err);
		abort();
	}

	load_control_from_db(get_from_db_stmt, control, false);

	sqlite3_clear_bindings(get_from_db_stmt);
	sqlite3_finalize(get_from_db_stmt);
}


static int insert_new_control(sqlite3* db) {
	sqlite3_stmt* insert_stmt;
	int prepare_result = sqlite3_prepare_v2(db, "insert into control default values;", -1, &insert_stmt, NULL);
	if (prepare_result != SQLITE_OK) {
		const char* err = sqlite3_errmsg(db);
		printf("sqlite err: %s\n", err);
		abort();
	}

	if (sqlite3_step(insert_stmt) != SQLITE_DONE) {
		const char* err = sqlite3_errmsg(db);
		printf("sqlite insert control err: %s\n", err);
		abort();
	}

	sqlite3_reset(insert_stmt);
	sqlite3_finalize(insert_stmt);
	return (int)sqlite3_last_insert_rowid(db);
}

static void delete_control_from_db(sqlite3* db, int id) {
	sqlite3_stmt* del_stmt;
	int prepare_result = sqlite3_prepare_v2(db, "delete from control where id = ?;", -1, &del_stmt, NULL);
	if (prepare_result != SQLITE_OK) {
		const char* err = sqlite3_errmsg(db);
		printf("sqlite err: %s\n", err);
		abort();
	}
	sqlite3_bind_int(del_stmt, 1, id);

	if (sqlite3_step(del_stmt) != SQLITE_DONE) {
		const char* err = sqlite3_errmsg(db);
		printf("sqlite delete control err: %s\n", err);
		abort();
	}

	sqlite3_reset(del_stmt);
	sqlite3_clear_bindings(del_stmt);
	sqlite3_finalize(del_stmt);
}

static void update_control_in_db(sqlite3* db, const control_t& control) {
	sqlite3_stmt* update_stmt;
	int prepare_result = sqlite3_prepare_v2(db, UPDATE_CONTROLS_QUERY, -1, &update_stmt, NULL);
	if (prepare_result != SQLITE_OK) {
		const char* err = sqlite3_errmsg(db);
		printf("sqlite err: %s\n", err);
		abort();
	}
	// sqlite3_bind_int(update_stmt, 1, id);

	sqlite3_bind_int (update_stmt, 1,  control.activation_mode);
	sqlite3_bind_int (update_stmt, 2,  control.key.v_code);
	sqlite3_bind_int (update_stmt, 3,  control.key.scan_code);
	sqlite3_bind_text(update_stmt, 4,  control.key.desc_utf8, -1, NULL);
	sqlite3_bind_int (update_stmt, 5,  control.key.ctrl ? 1 : 0);
	sqlite3_bind_int (update_stmt, 6,  control.key.alt ? 1 : 0);
	sqlite3_bind_int (update_stmt, 7,  control.key.shift ? 1 : 0);
	sqlite3_bind_int (update_stmt, 8,  control.slow_or_fast);
	sqlite3_bind_int (update_stmt, 9,  control.slowdown_or_speedup_percent);
	sqlite3_bind_int (update_stmt, 10, control.duration);
	sqlite3_bind_int (update_stmt, 11, control.limit_mode);
	sqlite3_bind_int (update_stmt, 12, control.cooldown_secs);
	sqlite3_bind_int (update_stmt, 13, control.max_energy);
	sqlite3_bind_int (update_stmt, 14, control.cost_per_use);
	sqlite3_bind_int (update_stmt, 15, control.recharge_rate);
	sqlite3_bind_int (update_stmt, 16, control.id);


	if (sqlite3_step(update_stmt) != SQLITE_DONE) {
		const char* err = sqlite3_errmsg(db);
		printf("sqlite update control err: %s\n", err);
		abort();
	}

	sqlite3_reset(update_stmt);
	sqlite3_clear_bindings(update_stmt);
	sqlite3_finalize(update_stmt);
}



control_manager_t*   control_manager_create(db_owner_t* _db) {
	control_manager_t* c = new control_manager_t;
	c->db_owner = _db;
	c->should_quit = false;
	c->should_insert = false;


	sqlite3* db = db_owner_lock_and_get_db(_db);

	sqlite3_stmt* get_from_db_stmt;
	int prepare_result = sqlite3_prepare_v2(db, GET_CONTROLS_QUERY, -1, &get_from_db_stmt, NULL);
	if (prepare_result != SQLITE_OK) {
		const char* err = sqlite3_errmsg(db);
		printf("sqlite err: %s\n", err);
		abort();
	}

	// initial load from db
	while(1) {
		if (sqlite3_step(get_from_db_stmt) != SQLITE_ROW) break;

		printf("processing control from DB\n");
		control_t control;
		// control.id = sqlite3_column_int(get_from_db_stmt, 0);

		load_control_from_db(get_from_db_stmt, control);

		c->controls.insert(control);


		// const char* path = (const char*)sqlite3_column_text(p->get_enabled_paths_statement, 0);
	}

	sqlite3_reset(get_from_db_stmt);
	sqlite3_finalize(get_from_db_stmt);

	db_owner_surrender_db_and_unlock(_db);


	c->thread = std::thread(control_manager_thread, c);

	return c;
}

void control_manager_destroy(control_manager_t* c) {

	{
		std::lock_guard<std::mutex> guard(c->main_mutex);
		c->should_quit = true;
	}

	c->thread.join();

	delete c;
}

void control_manager_que_control_modification(control_manager_t* c, control_t* cont) {
	std::lock_guard<std::mutex> guard(c->update_and_delete_mutex);

	control_t _cont = *cont;
	c->to_modify.push_back(std::move(_cont));

}

void control_manager_que_control_insertion(control_manager_t* c) {
	std::lock_guard<std::mutex> guard(c->update_and_delete_mutex);
	c->should_insert = true;
}



void control_manager_que_control_deletion(control_manager_t* c, control_t* cont) {
	std::lock_guard<std::mutex> guard(c->update_and_delete_mutex);

	control_t _cont = *cont;
	c->to_delete.push_back(std::move(_cont));
}

void control_manager_foreach_control(control_manager_t* c, void(*cb)(void* context, const control_t* control), void* context) {

	std::lock_guard<std::mutex> guard(c->main_mutex);
	// c->should_quit = true;

	// printf("controls count: %i\n", c->controls.size());

	for (const auto& control : c->controls) {
		cb(context, &control);
	}

	std::lock_guard<std::mutex> mutation_guard(c->update_and_delete_mutex);
	sqlite3* db = db_owner_lock_and_get_db(c->db_owner);

	for (const auto& to_modify : c->to_modify) {
		c->controls.erase(to_modify);
		c->controls.insert(to_modify);
		// write to db
		update_control_in_db(db, to_modify);
	}
	c->to_modify.clear();

	for (const auto& to_delete : c->to_delete) {
		int id = to_delete.id;
		c->controls.erase(to_delete);
		delete_control_from_db(db, id);
	}
	c->to_delete.clear();


	if (c->should_insert) {

		int id = insert_new_control(db);

		control_t control;
		control.id = id;
		load_specific_control_from_db(db, control);
		c->controls.insert(control);

		c->should_insert = false;
	}

	db_owner_surrender_db_and_unlock(c->db_owner);




}


bool control_manager_duringcallback_is_control_active(control_manager_t* c, control_t* cont) {
	// std::lock_guard<std::mutex> guard(c->main_mutex);
	return _is_control_active(c, cont);

}