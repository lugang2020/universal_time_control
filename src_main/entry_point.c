#include "windows_include.h"

#include <stdio.h>
#include "db/db_owner.h"
#include "gui/gui_entry_point.h"

#include "process_monitoring/process_monitor.h"
#include "controls/control_manager.h"
#include "games/game_manager.h"

#include "../src_ui_impl/gui_impl_c.h"



int main() {
	setbuf(stdout, NULL);
	setbuf(stderr, NULL);

    HANDLE hMutexHandle = CreateMutexW(NULL, TRUE, L"universaltimecontrol.is_running");
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
    	//MessageBoxA(NULL, "UTC is already running. (Check your system tray!)", "Error", MB_OK);
        //return 1;
    }

	db_owner_t* db = db_owner_init();
	control_manager_t* cm = control_manager_create(db);
	process_monitor_t* process_monitor = process_monitor_create(db, cm);
	game_manager_t* games = game_manager_create(db);

	// game_manager_import_all(games);

		gui_entry_point_start_and_run_loop(db, cm, games);

	game_manager_destroy(games);
	process_monitor_destroy(process_monitor);
	control_manager_destroy(cm);
	db_owner_destroy(db);

	ReleaseMutex(hMutexHandle);
	CloseHandle(hMutexHandle);
}
