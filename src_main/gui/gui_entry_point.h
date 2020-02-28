#pragma once

#include "../db/db_owner.h"
#include "../controls/control_manager.h"
#include "../games/game_manager.h"

void gui_entry_point_start_and_run_loop(db_owner_t* db, control_manager_t* cm, game_manager_t* games);