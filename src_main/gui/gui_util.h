#pragma once

#include "gui_common.h"

void  gui_util_text_with_font(const char* text, ImFont* font);
void  gui_util_same_line();
void  gui_util_same_line_text(const char* text);
void  gui_util_same_line_text_with_font(const char* text, ImFont* font);
float gui_util_get_x_pos_endof_last_item();
void  gui_util_spacing(unsigned num);
