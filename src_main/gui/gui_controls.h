#pragma once

#include "gui_common.h"
#include "../controls/control_manager.h"

TYPEDEF_STRUCT(gui_controls_data)

gui_controls_data_t* gui_controls_init(control_manager_t* cm);
void gui_controls_draw(gui_controls_data_t* data);
void gui_controls_cleanup(gui_controls_data_t* data);
void update_setting(gui_controls_data_t* data);
