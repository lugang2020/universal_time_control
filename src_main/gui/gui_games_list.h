#pragma once

#include "gui_common.h"
#include "../db/db_owner.h"
#include "../games/game_manager.h"

TYPEDEF_STRUCT(gui_games_list_data)

gui_games_list_data_t* gui_games_list_init(db_owner_t* db_owner, game_manager_t* games);
void gui_games_list_draw(gui_games_list_data_t* data);
void gui_games_list_cleanup(gui_games_list_data_t* data);