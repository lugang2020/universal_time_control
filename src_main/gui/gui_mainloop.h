#pragma once

#include "gui_common.h"
#include "../db/db_owner.h"
#include "../controls/control_manager.h"
#include "../games/game_manager.h"

TYPEDEF_STRUCT(gui_mainloop_data)

gui_mainloop_data_t* gui_mainloop_init(db_owner_t* db, control_manager_t* cm, game_manager_t* games);
void                 gui_mainloop_draw(gui_mainloop_data_t* data);
void                 gui_mainloop_cleanup(gui_mainloop_data_t* data);