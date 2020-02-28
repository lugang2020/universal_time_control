#pragma once

#include "../misc.h"

SEPPLES_GUARD_START

TYPEDEF_STRUCT(db_owner)
struct sqlite3;

db_owner_t*     db_owner_init();
void            db_owner_destroy(db_owner_t* d);

struct sqlite3* db_owner_lock_and_get_db(db_owner_t* d);
void            db_owner_surrender_db_and_unlock(db_owner_t* d);

SEPPLES_GUARD_END