#include "../windows_include.h"

#include "process_monitor.h"

#include <stdlib.h>
#include <stdio.h>

// #ifdef __STDC_NO_THREADS__
#include "../../third_party/tinycthread/tinycthread.h"
// #endif

#include "../util.h"

#include "../../third_party/vec/vec.h"

#include "../../third_party/sqlite/sqlite3.h"

#include "process_inject.h"

#include "../../shared.h"

#include <vector>

typedef struct {
	char path[MAX_PATH];
} process_path_t;

typedef vec_t(process_path_t) vec_process_path_t;


typedef struct {
	DWORD pid;
	char  path[MAX_PATH];
	bool  is_represented_by_managed;
} process_path_and_pid_t;

typedef vec_t(process_path_and_pid_t) vec_process_path_and_pid_t;


typedef struct {
	DWORD pid;
	HANDLE handle;
	HWND   cnc_window;
	char  path[MAX_PATH];
} process_path_pid_and_handle_t;

// typedef vec_t(process_path_pid_and_handle_t) vec_process_path_pid_and_handle_t;


struct _process_monitor {
	thrd_t             thread;
	thrd_t             thread_timescale_update;
	mtx_t              mutex;
	bool               should_quit;
	db_owner_t*        db_owner;
	control_manager_t* cm;
	sqlite3_stmt*      get_enabled_paths_statement;

	std::vector<process_path_pid_and_handle_t> managed_processes;
};

static int _process_monitor_thread(void* _p);
static int _timescale_update_thread(void* _p);

static void  _process_monitor_init(process_monitor_t* p, db_owner_t* db_owner, control_manager_t* cm) {
	mtx_init(&p->mutex, mtx_plain);
	p->should_quit = false;
	p->db_owner = db_owner;
	p->cm = cm;

	sqlite3* db = db_owner_lock_and_get_db(db_owner);

	const char* get_processes_query = "select game_exe.path from game_exe join game on game_exe.game_id = game.id where game.allowed = 1 and game.enabled = 1;";

	int prepare_result = sqlite3_prepare_v2(db, get_processes_query, -1, &p->get_enabled_paths_statement, NULL);
	if (prepare_result != SQLITE_OK) {
		const char* err = sqlite3_errmsg(db);
		printf("sqlite err: %s\n", err);
		abort();
	}

	db_owner_surrender_db_and_unlock(db_owner);

	thrd_create(&p->thread, &_process_monitor_thread, (void*)p);
	thrd_create(&p->thread_timescale_update, &_timescale_update_thread, (void*)p);
}

static void _close_all_managed_handles_for_deinit(process_monitor_t* p) {
	for (const auto& managed_exe: p->managed_processes) {
		DWORD exit_code;
		GetExitCodeProcess(managed_exe.handle, &exit_code);
		if (exit_code == STILL_ACTIVE) {
			printf("sending unload cmd\n");
			timecontrol_ipc_cmd_t command;
			command.cmd_type = UTC_IPC_CMD_SHOULD_UNLOAD;
			command.timeScale = 1.0;

			COPYDATASTRUCT cds;
			cds.dwData = WM_COPYDATA_ID;
			cds.cbData = sizeof(command);
			cds.lpData = &command;

			SendMessage(managed_exe.cnc_window, WM_COPYDATA, (WPARAM)(HWND)NULL, (LPARAM)(LPVOID)&cds);

		}

		CloseHandle(managed_exe.handle);
	}
}

static void _process_monitor_deinit(process_monitor_t* p) {
	printf("pm_deinit_1\n");
	mtx_lock(&p->mutex);
	p->should_quit = true;
	mtx_unlock(&p->mutex);
	printf("pm_deinit_2\n");

	// util_sleep_for_ms(5000);
	thrd_join(p->thread, NULL);
	thrd_join(p->thread_timescale_update, NULL);

	printf("pm_deinit_3\n");

	(void)db_owner_lock_and_get_db(p->db_owner);
	sqlite3_finalize(p->get_enabled_paths_statement);
	db_owner_surrender_db_and_unlock(p->db_owner);

	_close_all_managed_handles_for_deinit(p);

	mtx_destroy(&p->mutex);
}

process_monitor_t* process_monitor_create(db_owner_t* db, control_manager_t* cm) {
	auto p = new process_monitor_t;
	if (!p) return NULL;
	_process_monitor_init(p, db, cm);
	return p;
}

void process_monitor_destroy(process_monitor_t* p) {
	_process_monitor_deinit(p);
	delete p;
}




#define CAST_PTR(TYPE, NAME) TYPE* NAME = (TYPE*)_##NAME ;





static void _get_running_processes(vec_process_path_and_pid_t* out_vec) {
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snapshot == INVALID_HANDLE_VALUE) return;

	PROCESSENTRY32W entry;
	entry.dwSize = sizeof(PROCESSENTRY32W);

	

	if (!Process32FirstW(snapshot, &entry)) {
		DWORD last_err = GetLastError();
		// if (last_err == ERROR_NO_MORE_FILES){
		// 	printf("no more\n");
		// }
		printf("Process32FirstW err: %lu\n", last_err);
		CloseHandle(snapshot);
		return;
	}

	while (1) {
		// etc

		//LOGI("pid: %lu\n", entry.th32ProcessID);
		//LOGI("\nPROCESS NAME:  %ls", entry.szExeFile );
		HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, entry.th32ProcessID);
		if (!process) {
			// printf("failed to open: %ls\n", entry.szExeFile);
			// goto next_process;
			if (!Process32NextW(snapshot, &entry)) break; else continue;
			// printf("err: %lu\n", GetLastError());
			// goto error;
		}

		// printf("1\n");

		wchar_t path_name_wide[MAX_PATH];
		DWORD path_len = MAX_PATH;
		BOOL path_query_ok = QueryFullProcessImageNameW(process, 0, path_name_wide, &path_len);
		if (!path_query_ok) {
			//printf("failed to query: %ls\n", entry.szExeFile);
			CloseHandle(process);
			// goto next_process;
			if (!Process32NextW(snapshot, &entry)) break; else continue;
		}

		char process_name_utf8[MAX_PATH];
		memset(process_name_utf8, 0, sizeof(process_name_utf8));
		util_wide_to_utf8(path_name_wide, path_len, process_name_utf8, MAX_PATH);

		char process_name_utf8_lower[MAX_PATH];
		memset(process_name_utf8_lower, 0, sizeof(process_name_utf8_lower));
		util_casefold_utf8(process_name_utf8, process_name_utf8_lower, MAX_PATH-1);

		for (int i = 0; i < MAX_PATH; i++) {
			if (process_name_utf8_lower[i] == '/') {
				process_name_utf8_lower[i] = '\\';
			}
		}

		if (strstr(process_name_utf8_lower, "mindustry") != 0) {
			printf("RN: %s\n", process_name_utf8_lower);
		}

		process_path_and_pid_t info;
		info.pid = entry.th32ProcessID;
		info.is_represented_by_managed = false;
		strcpy(info.path, process_name_utf8_lower);

		vec_push(out_vec, info);

		// printf("hmm0: %ls\n", entry.szExeFile);
		// printf("hmm1: %ls\n", path_name_wide);
		// printf("hmm2: %s\n", process_name_utf8);
		// printf("hmm3: %s\n", process_name_utf8_lower);



		CloseHandle(process);
// next_process:
		if (!Process32NextW(snapshot, &entry)) break;
	}


	CloseHandle(snapshot);
	return;



}

static void _load_enabled_from_db(process_monitor_t* p, vec_process_path_t* out_enabled) {

	(void)db_owner_lock_and_get_db(p->db_owner);


	while(1) {
		if (sqlite3_step(p->get_enabled_paths_statement) != SQLITE_ROW) break;

		const char* path = (const char*)sqlite3_column_text(p->get_enabled_paths_statement, 0);
		
		char path_utf8_lower[MAX_PATH];
		util_casefold_utf8(path, path_utf8_lower, MAX_PATH-1);

		for (int i = 0; i < MAX_PATH; i++) {
			if (path_utf8_lower[i] == '/') {
				path_utf8_lower[i] = '\\';
			}
		}


		/*if (strstr(path_utf8_lower, "mindustry") != 0) {
			printf("DB: %s\n", path_utf8_lower);
		}*/

		

		process_path_t _enabled;
		strcpy(_enabled.path, (const char*)path_utf8_lower);
		vec_push(out_enabled, _enabled);
	}


	sqlite3_reset(p->get_enabled_paths_statement);
	db_owner_surrender_db_and_unlock(p->db_owner);
}

static void _join_enabled_with_running(
	const vec_process_path_and_pid_t* running,
	const vec_process_path_t*         enabled,
	vec_process_path_and_pid_t*       out_enabled_and_running
) {

	int enabled_index; process_path_t* enabled_exe;
	vec_foreach_ptr(enabled, enabled_exe, enabled_index) {
		int running_index; process_path_and_pid_t* running_exe;
		vec_foreach_ptr(running, running_exe, running_index) {
			if (strcmp(running_exe->path, enabled_exe->path) == 0) {
				vec_push(out_enabled_and_running, *running_exe);
				// printf("enabled and running: %s\n", running_exe->path);
				// don't break- might be multiple instances
			}
		}
	}

}

#include <iterator>
#include <algorithm>

static void _delete_ended_processes(process_monitor_t* p) {
	// size_t managed_len = p->managed_processes.length;
	// bool to_delete[managed_len];
	// memset(to_delete, 0, managed_len * sizeof(bool));

	// int managed_index; process_path_pid_and_handle_t* managed_exe;
	// vec_foreach_ptr(&p->managed_processes, managed_exe, managed_index) {
	// 	DWORD exit_code;
	// 	GetExitCodeProcess(managed_exe->handle, &exit_code);
	// 	if (exit_code != STILL_ACTIVE) {
	// 		CloseHandle(managed_exe->handle);
	// 		to_delete[managed_index] = true;
	// 	}
	// }

	// for (int i = managed_len-1; i >= 0; i--) {
	// 	if (to_delete[i]) {
	// 		vec_splice(&p->managed_processes, i, 1);
	// 	}
	// }

	p->managed_processes.erase(std::remove_if(std::begin(p->managed_processes), std::end(p->managed_processes), [](auto& managed_exe){
		DWORD exit_code;
		GetExitCodeProcess(managed_exe.handle, &exit_code);
		if (exit_code != STILL_ACTIVE) {
			CloseHandle(managed_exe.handle);
			return true;
		}
		return false;
	}), std::end(p->managed_processes));


}

// Also mutates running to set is_represented_by_managed
static bool _do_any_enabledandrunning_match_managed(
	process_path_pid_and_handle_t* managed,
	vec_process_path_and_pid_t* enabled_and_running
) {
	int enabled_and_running_index; process_path_and_pid_t* enabled_and_running_exe;
	vec_foreach_ptr(enabled_and_running, enabled_and_running_exe, enabled_and_running_index) {
		if (enabled_and_running_exe->pid == managed->pid) {
			enabled_and_running_exe->is_represented_by_managed = true;
			return true;
		}
	}
	return false;
}

static void _unmanage_dead_processes_after_unloading_if_needed(
	process_monitor_t* p,
	vec_process_path_and_pid_t* enabled_and_running
) {
	// todo: not n^2!
	p->managed_processes.erase(std::remove_if(std::begin(p->managed_processes), std::end(p->managed_processes), [enabled_and_running](auto managed_exe){

		bool found = _do_any_enabledandrunning_match_managed(&managed_exe, enabled_and_running);

		if (!found) {

			DWORD exit_code;
			GetExitCodeProcess(managed_exe.handle, &exit_code);
			if (exit_code == STILL_ACTIVE) {
				// todo: don't code dupe
				timecontrol_ipc_cmd_t command;
				command.cmd_type = UTC_IPC_CMD_SHOULD_UNLOAD;
				command.timeScale = 1.0;

				COPYDATASTRUCT cds;
				cds.dwData = WM_COPYDATA_ID;
				cds.cbData = sizeof(command);
				cds.lpData = &command;

				SendMessage(managed_exe.cnc_window, WM_COPYDATA, (WPARAM)(HWND)NULL, (LPARAM)(LPVOID)&cds);
			}

			CloseHandle(managed_exe.handle);
			return true;
		}
		return false;
	}), std::end(p->managed_processes));


}

static bool _get_should_quit(process_monitor_t* p) {
	mtx_lock(&p->mutex);
	bool should_quit = p->should_quit;
	mtx_unlock(&p->mutex);
	return should_quit;
}

// relies on is_represented_by_managed having been set correctly by
// _do_any_enabledandrunning_match_managed (via call to _unmanage_dead_processes_after_unloading_if_needed)
static void _start_managing_new_processes(
	process_monitor_t* p,
	vec_process_path_and_pid_t* enabled_and_running
) {
	int enabled_and_running_index; process_path_and_pid_t* enabled_and_running_exe;
	vec_foreach_ptr(enabled_and_running, enabled_and_running_exe, enabled_and_running_index) {
		if (!enabled_and_running_exe->is_represented_by_managed) {
			process_path_pid_and_handle_t to_add;
			to_add.pid = enabled_and_running_exe->pid;
			strcpy(to_add.path, enabled_and_running_exe->path);

			// todo: use fewer permissions if possible?
			HANDLE temp = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, to_add.pid);
			if (!temp) continue;

			HWND cnc_window;
			bool inject_ok = inject_process(to_add.pid, temp, &cnc_window);

			// TODO
			if (inject_ok) {
				to_add.handle = temp;
				to_add.cnc_window = cnc_window;
				p->managed_processes.push_back(to_add);
			}


		}
	}
}

static void _update_injected_timescales(process_monitor_t* p) {
	for (auto& managed_exe : p->managed_processes) {
		LOGI("Managed: (PID %lu) %s\n", managed_exe.pid, managed_exe.path);

		timecontrol_ipc_cmd_t command;
		command.cmd_type = UTC_IPC_CMD_SET_TIMESCALE;
		command.timeScale = control_manager_calculate_timescale(p->cm);

		COPYDATASTRUCT cds;
		cds.dwData = WM_COPYDATA_ID;
		cds.cbData = sizeof(command);
		cds.lpData = &command;

		SendMessage(managed_exe.cnc_window, WM_COPYDATA, (WPARAM)(HWND)NULL, (LPARAM)(LPVOID)&cds);
	}
}

void process_monitor_unmanage_all_and_stop(process_monitor_t* p) {
	mtx_lock(&p->mutex);
	for (auto& managed_exe : p->managed_processes) {
		timecontrol_ipc_cmd_t command;
		command.cmd_type = UTC_IPC_CMD_SHOULD_UNLOAD;
		command.timeScale = 1.0;

		COPYDATASTRUCT cds;
		cds.dwData = WM_COPYDATA_ID;
		cds.cbData = sizeof(command);
		cds.lpData = &command;

		SendMessage(managed_exe.cnc_window, WM_COPYDATA, (WPARAM)(HWND)NULL, (LPARAM)(LPVOID)&cds);
	}

	p->managed_processes.clear();

	p->should_quit = true;
	mtx_unlock(&p->mutex);
	// util_sleep_for_ms(1000);
}

static int _timescale_update_thread(void* _p) {
	CAST_PTR(process_monitor_t, p)

	while (1) {
		if (_get_should_quit(p)) {
			LOGI("timescale update thread should break");
			break;
		}

		mtx_lock(&p->mutex);
		_update_injected_timescales(p);
		mtx_unlock(&p->mutex);

		// TODO: Average latency 125ms, worst case 250
		// Improve by exposing a condvar in control_mangager
		// to indicate when timescale has changed
		// util_sleep_for_ms(250);

		control_manager_sleep(p->cm);
	}

	return 0;
}

static int _process_monitor_thread(void* _p) {
	CAST_PTR(process_monitor_t, p)

	while (1) {
		// printf("process monitor thread loop\n");
		if (_get_should_quit(p)) {
			printf("pm thread should break\n");
			break;
		}

		mtx_lock(&p->mutex);

		// load running (from win32)
		vec_process_path_and_pid_t running;
		vec_init(&running);
		_get_running_processes(&running);

		// load enabled (from sqlite)
		vec_process_path_t enabled;
		vec_init(&enabled);
		_load_enabled_from_db(p, &enabled);

		// join the above on case-insensitive exe path
		vec_process_path_and_pid_t enabled_and_running;
		vec_init(&enabled_and_running);
		_join_enabled_with_running(&running, &enabled, &enabled_and_running);

		_delete_ended_processes(p);
		_unmanage_dead_processes_after_unloading_if_needed(p, &enabled_and_running);

		_start_managing_new_processes(p, &enabled_and_running);

		
		vec_deinit(&enabled_and_running);
		vec_deinit(&enabled);
		vec_deinit(&running);

		// _update_injected_timescales(p);


		mtx_unlock(&p->mutex);

		util_sleep_for_ms(1000);
	}

	printf("pm thread will return\n");

	return 0;
}