#pragma once

#include "../misc.h"

SEPPLES_GUARD_START

TYPEDEF_STRUCT(game_manager)


#include "../../shared.h"
#include "../db/db_owner.h"




game_manager_t*    game_manager_create(db_owner_t* db);
void               game_manager_destroy(game_manager_t* s);

// 0: normal, 1: importing
int                game_manager_get_state(game_manager_t* s);
void               game_manager_import_all(game_manager_t* s);   

void game_manager_set_should_import(game_manager_t* s);

SEPPLES_GUARD_END