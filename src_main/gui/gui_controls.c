#include "gui_util.h"
#include "gui_controls.h"
#include "gui_common.h"

#include "../../src_ui_impl/gui_impl_c.h"

#include <stdlib.h>
#include <string.h>

#include "../util.h"
#include "../../shared.h"

static const ImVec2 VEC2_ZERO = {0,0};

#define NUM_CONTROLS 3

struct _gui_controls_data
{
	// gui_control_t controls[NUM_CONTROLS];
	control_manager_t* cm;
	int selected_id;
};

gui_controls_data_t* gui_controls_init(control_manager_t* cm)
{
	MALLOC_TYPE(struct _gui_controls_data, data)

	data->cm = cm;
	data->selected_id = 0;

	// for (int i = 0; i < NUM_CONTROLS; i++) {
	// 	gui_control_init(data->controls+i);
	// }

	// data->controls[1].limit_mode = 1;
	// strcpy(data->controls[0].hot_key, "CTRL+SHIFT+A");



	// strcpy(data->controls[1].hot_key, "CTRL+ALT+SHIFT+F12");
	// data->controls[1].limit_mode = 2;


	// data->controls[2].activation_mode = 2;


	return data;
}

#define DPI (gui_common_get_data()->dpi_scale)



void draw_hotkey_block(gui_controls_data_t* data, control_t* c, float block_width)
{

	igBeginGroup();

	igPushItemWidth(block_width);

	const char* options[] = {"Press to use", "Hold to use", "Toggle to use"};

	// igComboStr("###activation_mode", &c->activation_mode, "Press to Use\0Hold to Use\0Toggle to Use\0\0", -1);

	if (c->activation_mode < CTRL_ACTIVATION_MODE_PRESS || c->activation_mode > CTRL_ACTIVATION_MODE_TOGGLE)
	{
		printf("unknown activation mode\n");
		abort();
	}

	//LOGI("c->activation_mode is %d",c->activation_mode);

	if (igBeginCombo("###activation_mode", options[c->activation_mode], 0))
	{

		for (int i = 0; i <= 2; i++)
		{
			bool selected = (i == c->activation_mode);
			int flags = 0; // (i == 0) ? ImGuiSelectableFlags_Disabled : 0;
			bool chosen = igSelectable(options[i], selected, flags, VEC2_ZERO);
			if (chosen)
			{
				c->activation_mode = i;
			}
			if (selected)
			{
				igSetItemDefaultFocus();
			}
		}

		igEndCombo();
	}

	igPopItemWidth();

	igAlignTextToFramePadding();
	ImVec2 btn_size = {block_width, 0};

	bool btn_pressed = false;

	const uint32_t ESC_VKEY = 0x1B;

	bool is_key_mode = gui_impl_is_get_key_mode();

	if (is_key_mode)
	{
		igGetIO()->ConfigFlags |= ImGuiConfigFlags_NoMouse;
	}
	else
	{
		igGetIO()->ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
	}

	if (is_key_mode && (data->selected_id == c->id))
	{
		//20200824： try to fix issue 7
		c->is_toggled_on = false;
		c->press_within_duration = false;
		c->is_in_cooldown = false;


		igButton("Press key. ESC to clear", btn_size);

		// ImVec2 window_size = igGetWindowSize();
		// ImVec2 window_pos = igGetWindowPos();

		ImVec2 btn_min = igGetItemRectMin();
		ImVec2 btn_max = igGetItemRectMax();

		// ImVec2 top_rect_min = {-window_pos.x, -window_pos};
		// ImVec2 top_rect_max = {window_size.x, btn_min.y};

		ImDrawList_AddRect(igGetWindowDrawList(), btn_min, btn_max, IM_COL32(255,128,128,255), 4.0, ImDrawCornerFlags_All, 3.0f);
		// ImDrawList_AddRectFilled(igGetWindowDrawList(), top_rect_min, top_rect_max, IM_COL32(128,128,255,255), 0, 0.0f);

		key_t key;
		bool have_key = gui_impl_get_set_key(&key);
		if (have_key)
		{
			if (key.v_code == ESC_VKEY)
			{
				c->key.desc_utf8[0] = '\0';
				c->key.v_code = 0;
			}
			else
			{
				memcpy(&c->key, &key, sizeof(key_t));
			}
			gui_impl_leave_get_key_mode();
		}
	}
	else if (c->key.desc_utf8[0])     //(c->hot_key[0]) {
	{
		btn_pressed = igButton(c->key.desc_utf8, btn_size);
	}
	else
	{
		igPushStyleColorU32(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
		igPushStyleColorU32(ImGuiCol_Button, IM_COL32(242, 169, 169, 255));
		btn_pressed = igButton("Click to set key", btn_size);
		igPopStyleColor(2);

		// draw_list->AddRect(GetItemRectMin(), GetItemRectMax(), IM_COL32(255,255,0,255), 4.0f);

	}

	if (btn_pressed)
	{
		gui_impl_enter_get_key_mode();
		data->selected_id = c->id;
	}

	igPushStyleColorU32(ImGuiCol_Button, IM_COL32(210, 50, 50, 255));
	igPushStyleColorU32(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
	bool delete_pressed = igButton("Delete Control", btn_size);
	igPopStyleColor(2);

	if (delete_pressed)
	{
		control_manager_que_control_deletion(data->cm, c);
		// data->cm
	}

	igEndGroup();
}

void cap_int(int* n, int min, int max)
{
	if (*n < min) *n = min;
	if (*n > max) *n = max;
}

void draw_fastslow_block(control_t* c, float block_width)
{
	igBeginGroup();

	igPushItemWidth(block_width);
	igComboStr("###slow_or_fast", &c->slow_or_fast, "Slow Time By\0Speedup Time By\0\0", -1);
	igPopItemWidth();

	float select_width = block_width;
	const float limit = 100.0*DPI;
	if (select_width > limit)
	{
		select_width = limit;
	}

	igIndent(block_width - select_width);

	igPushItemWidth(select_width);
	igInputInt("###slowdown_or_speedup_percent", &c->slowdown_or_speedup_percent, 5, 20, 0);
	if (c->slow_or_fast == 1)
	{
		cap_int(&c->slowdown_or_speedup_percent, 5, 200);
	}
	else
	{
		cap_int(&c->slowdown_or_speedup_percent, 5, 95);
	}

	igPopItemWidth();
	gui_util_same_line();

	ImVec2 pos = igGetCursorPos();
	pos.y += 46.6 * DPI;
	pos.x -= 70.0 * DPI;

	ImU32 col = igGetColorU32(ImGuiCol_Text, 1.0f);

	ImDrawList_AddText(igGetWindowDrawList(), pos, col, "%", NULL);

	igNewLine();


	// gui_util_spacing(4);
	igEndGroup();
}

void draw_duration_block(control_t* c)
{
	igBeginGroup();
	// gui_util_spacing(4);

	igAlignTextToFramePadding();
	igText("Effect Duration:");
	float width = igGetItemRectSize().x;

	igPushItemWidth(width);
	// the space is important for strange reasons that aren't worth fixing
	igInputInt(" ###duration", &c->duration, 1, 5, 0);
	cap_int(&c->duration, 5, 30);
	igPopItemWidth();


	gui_util_same_line();

	ImVec2 pos = igGetCursorPos();
	pos.y += 44.2 * DPI;
	pos.x -= 72.0 * DPI;

	ImU32 col = igGetColorU32(ImGuiCol_Text, 1.0f);

	ImDrawList_AddText(igGetWindowDrawList(), pos, col, "s", NULL);

	igNewLine();

	igEndGroup();
}

void draw_limits_block(control_t* c, float block_width)
{
	igBeginGroup();
	// gui_util_spacing(4);

	float start_pos = igGetCursorPosX();
	float max_x = start_pos + block_width;

	igPushItemWidth(block_width);

	const char* options[] = {"No Limit", "Cooldown", "Energy"};

	if (igBeginCombo("###test4", options[c->limit_mode], 0))
	{

		for (int i = 0; i <= 2; i++)
		{
			bool selected = (i == c->activation_mode);
			int flags = 0;//((i == 0)||(i == i)) ? 0 : ImGuiSelectableFlags_Disabled;
			bool chosen = igSelectable(options[i], selected, flags, VEC2_ZERO);
			if (chosen)
			{
				c->limit_mode = i;
			}
			if (selected)
			{
				igSetItemDefaultFocus();
			}
		}

		igEndCombo();
	}


	igPopItemWidth();

	if (c->limit_mode == CTRL_LIMITED_MODE_COOLDOWN)
	{

		igPushItemWidth(block_width - igCalcTextSize(" seconds", NULL, false, -0.0).x - igGetStyle()->ItemInnerSpacing.x);
		igInputInt(" seconds###cooldown", &c->cooldown_secs, 1, 5, 0);
		igPopItemWidth();
		cap_int(&c->cooldown_secs, 1, 999);

		if (c->activation_mode == CTRL_ACTIVATION_MODE_HOLD)
		{
		}
		else if (c->activation_mode == CTRL_ACTIVATION_MODE_TOGGLE)
		{
		 
		}


	}
	else if (c->limit_mode == CTRL_LIMITED_MODE_ENERGY)
	{
		{
			igAlignTextToFramePadding();
			igText("Max energy:");
			gui_util_same_line();


			float text_diff = igCalcTextSize("Recharge per sec:", NULL, false, -0.0).x - igCalcTextSize("Max energy:", NULL, false, -0.0).x;

			igPushItemWidth(max_x - igGetCursorPosX() - text_diff);
			igSetCursorPosX(igGetCursorPosX() + text_diff);
			igInputInt("###max_energy", &c->max_energy, 1, 5, 0);
			cap_int(&c->max_energy, 0, 999);
			igPopItemWidth();
		}

		{
			igAlignTextToFramePadding();
			igText("Cost per use:");
			gui_util_same_line();


			float text_diff = igCalcTextSize("Recharge per sec:", NULL, false, -0.0).x - igCalcTextSize("Cost per use:", NULL, false, -0.0).x;

			igPushItemWidth(max_x - igGetCursorPosX() - text_diff);
			// igIndent(igGetCursorPosX());
			igSetCursorPosX(igGetCursorPosX() + text_diff);
			igInputInt("###cost_per_use", &c->cost_per_use, 1, 5, 0);
			cap_int(&c->cost_per_use, 0, 999);
			igPopItemWidth();
		}

		{
			igAlignTextToFramePadding();
			igText("Recharge per sec:");
			gui_util_same_line();

			igPushItemWidth(max_x - igGetCursorPosX());
			igInputInt("###recharge_rate", &c->recharge_rate, 1, 5, 0);
			cap_int(&c->recharge_rate, 0, 999);
			igPopItemWidth();
		}


		{
			igAlignTextToFramePadding();
			igText("Current:");
			gui_util_same_line();


			float text_diff = igCalcTextSize("Recharge per sec:", NULL, false, -0.0).x - igCalcTextSize("Current:", NULL, false, -0.0).x;

			igPushItemWidth(max_x - igGetCursorPosX() - text_diff);
			// igIndent(igGetCursorPosX());
			igSetCursorPosX(igGetCursorPosX() + text_diff);
			igText("%f", c->current_energy_level);
			// igText
			// igInputInt("###current", &c->cost_per_use, 1, 5, 0);
			// cap_int(&c->cost_per_use, 0, 999);
			igPopItemWidth();
		}


	}

	// gui_util_spacing(4);
	igEndGroup();
}

#include <math.h>

void push_hue_tinted_col(ImGuiCol idx)
{
	const ImVec4* _current_rgb = igGetStyleColorVec4(idx);
	// ImVec4  current_hsv = ImColor ImColor_HSV(ImColor *self, float h, float s, float v, float a)

	float hue, sat, val;
	igColorConvertRGBtoHSV(_current_rgb->x, _current_rgb->y, _current_rgb->z, &hue, &sat, &val);

	hue += 0.8;
	// sat -= 0.2;
	hue = fmod(hue, 1.0);

	// const ImVec4 new_hsv;
	// hsv.x = hue;
	// hsv.y = sat;
	// hsv.z = val;
	// hsv.w = _current_rgb->w;

	float red, green, blue;
	igColorConvertHSVtoRGB(hue, sat, val, &red, &green, &blue);

	ImVec4 new_rgb = {red, green, blue, _current_rgb->w};

	igPushStyleColor(idx, new_rgb);

}


void draw_control(gui_controls_data_t* data, control_t* c)
{

	float overall_width = igGetContentRegionMax().x;// - (igGetStyle()->IndentSpacing*0.0);

	// float width1 = (overall_width/4.0) -

	ImVec2 p0 = igGetCursorScreenPos();

	float block_gap = 16.0 * DPI;

	float width1 = (overall_width - (5.0*block_gap))/4.0;


	// igIndent(block_gap);

	igBeginGroup();
	igIndent(0.0);
	gui_util_spacing(4);

	{
		draw_hotkey_block(data, c, width1);
		gui_util_same_line();
	}


	{
		int count = 0;
		for (int i = ImGuiCol_Border; i < ImGuiCol_COUNT; i++)
		{
			count++;
			push_hue_tinted_col(i);
		}

		draw_fastslow_block(c, width1);
		gui_util_same_line();
		igPopStyleColor(count);
	}


	// igPopStyleColor(count);

	if (c->activation_mode == CTRL_ACTIVATION_MODE_PRESS)
	{
		draw_duration_block(c);
		gui_util_same_line();
	}

	//if (c->activation_mode == CTRL_ACTIVATION_MODE_PRESS)
	{
		int count = 0;
		for (int i = ImGuiCol_Border; i < ImGuiCol_COUNT; i++)
		{
			count++;
			push_hue_tinted_col(i);
		}

		draw_limits_block(c, igGetContentRegionAvail().x - igGetStyle()->ScrollbarSize/2.0 - igGetStyle()->IndentSpacing);
		igPopStyleColor(count);
	}



	gui_util_spacing(4);
	igEndGroup();

	ImVec2 p1 = igGetItemRectMax();
	p1.x = igGetContentRegionMax().x + 0.0f * gui_common_get_data()->dpi_scale;

	int state = control_manager_duringcallback_get_control_state(data->cm, c);

	if (state != CTRL_STATE_NOTHING)
	{
		//high light active control
		ImVec2 _p0 = p0;
		ImVec2 _p1 = p1;

		_p1.x = _p0.x + 10 * gui_common_get_data()->dpi_scale;

		// _p1.y += p1.y;
		// _p1.x += 15;

		ImDrawList_AddRectFilled(igGetWindowDrawList(), _p0, _p1,
		                         // IM_COL32(255,20,20,255),
		                         (state == 2) ? IM_COL32(255,20,20,255) : IM_COL32(20,255,20,255)
		                         ,
		                         4.0, ImDrawCornerFlags_Left);

	}


	ImDrawList_AddRect(igGetWindowDrawList(), p0, p1, IM_COL32(128,128,255,255), 4.0, ImDrawCornerFlags_All, 3.0f);
	// ImDrawList_AddRectFilled(igGetWindowDrawList(), p0, p1, IM_COL32(128,128,255,255), 4.0, ImDrawCornerFlags_All);

	if(0)
	{
		ImVec2 _p0 = VEC2_ZERO;
		ImVec2 _p1 = { igGetWindowContentRegionWidth(), 380.0};
		ImDrawList_AddRectFilled(igGetWindowDrawList(), _p0, _p1, IM_COL32(128,128,255,255), 4.0, ImDrawCornerFlags_All);
	}

	gui_util_spacing(2);

}

static void draw_control_cb(void* _data, const control_t* _control)
{
	gui_controls_data_t* data = (gui_controls_data_t*) _data;

	control_t mutable_control = *_control;
	igPushIDInt(mutable_control.id);
	draw_control(data, &mutable_control);

	if (memcmp(_control, &mutable_control, sizeof(mutable_control)) != 0)
	{
		control_manager_que_control_modification(data->cm, &mutable_control);
	}

	igPopID();
}


void gui_controls_draw(gui_controls_data_t* data)
{
	igBeginChild("ControlsChild", (ImVec2)
	{
		0,-28.0 * gui_common_get_data()->dpi_scale
	}, false, ImGuiWindowFlags_AlwaysVerticalScrollbar);

	// igText("Test");

	// int a = 4;

	igSpacing();



	control_manager_foreach_control(data->cm, draw_control_cb, data);

	// draw_control();

	// igUnindent(0.0);




	igEndChild();

	bool insert_pressed = igButton("Add New Control", VEC2_ZERO);

	if (insert_pressed)
	{
		control_manager_que_control_insertion(data->cm);
	}


}

void gui_controls_cleanup(gui_controls_data_t* data)
{
	free(data);
}

