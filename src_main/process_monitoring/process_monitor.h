#pragma once

#include "../misc.h"
#include "../task_que/task_que.h"
#include "../db/db_owner.h"
#include "../controls/control_manager.h"

SEPPLES_GUARD_START

TYPEDEF_STRUCT(process_monitor)


process_monitor_t*   process_monitor_create(db_owner_t* db,control_manager_t* cm);
void                 process_monitor_destroy(process_monitor_t* p);

void                 process_monitor_unmanage_all_and_stop(process_monitor_t* p);

SEPPLES_GUARD_END