#include "gui_util.h"

void gui_util_text_with_font(const char* text, ImFont* font) {
    igPushFont(font);
    igText(text);
    igPopFont();
}

void gui_util_same_line() {
	igSameLine(0.0f, -1.0f);
}

void gui_util_same_line_text(const char* text) {
	gui_util_same_line();
	igText(text);
}

void gui_util_same_line_text_with_font(const char* text, ImFont* font) {
	gui_util_same_line();
    igPushFont(font);
    igText(text);
    igPopFont();
}

float gui_util_get_x_pos_endof_last_item() {
    gui_util_same_line();
    float result = igGetCursorPos().x - igGetStyle()->ItemSpacing.x;
    igNewLine();
    return result;
}

void gui_util_spacing(unsigned num) {
    // eww
    for (unsigned i = 0; i < num; i++) {
        igSpacing();
    }
}
