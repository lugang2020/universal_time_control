#include "gui_mainloop.h"

#include "gui_games_list.h"
#include "gui_controls.h"

#include <stdlib.h>

static const ImVec2 VEC2_ZERO = {0,0};

struct _gui_mainloop_data
{
	gui_games_list_data_t* games_list_data;
	gui_controls_data_t*     controls_data;
	bool show_imgui_demo;
	// db_owner_t* db_owner;
};

gui_mainloop_data_t* gui_mainloop_init(db_owner_t* db, control_manager_t* cm, game_manager_t* games)
{

	gui_mainloop_data_t* data = malloc(sizeof(gui_mainloop_data_t));
	gui_common_data_t* cd = gui_common_get_data();

	// data->db_owner = db;

	igGetIO()->IniFilename = NULL;
	// igStyleColorsDark(igGetStyle());
	igStyleColorsLight(igGetStyle());

	igGetStyle()->WindowRounding = 0.0;

	igGetStyle()->FrameRounding = 3.5;// * cd->dpi_scale;

	// igGetStyle()->ChildBorderSize = 0.0;



	cd->font_main = ImFontAtlas_AddFontFromFileTTF(igGetIO()->Fonts, "data/DroidSans.ttf", 17.0f * cd->dpi_scale, NULL, NULL);
	cd->font_bold = ImFontAtlas_AddFontFromFileTTF(igGetIO()->Fonts, "data/DroidSans-Bold.ttf", 17.0f * cd->dpi_scale, NULL, NULL);
	cd->font_bold_big = ImFontAtlas_AddFontFromFileTTF(igGetIO()->Fonts, "data/DroidSans-Bold.ttf", 21.0f * cd->dpi_scale, NULL, NULL);






	// ImVec4* colors = igGetStyle()->Colors;
	// colors[ImGuiCol_Text]                   = (ImVec4){1.00f, 1.00f, 1.00f, 1.00f};
	// colors[ImGuiCol_TextDisabled]           = (ImVec4){0.50f, 0.50f, 0.50f, 1.00f};
	// colors[ImGuiCol_WindowBg]               = (ImVec4){0.06f, 0.06f, 0.06f, 0.94f};
	// colors[ImGuiCol_ChildBg]                = (ImVec4){1.00f, 1.00f, 1.00f, 0.00f};
	// colors[ImGuiCol_PopupBg]                = (ImVec4){0.08f, 0.08f, 0.08f, 0.94f};
	// colors[ImGuiCol_Border]                 = (ImVec4){0.43f, 0.43f, 0.50f, 0.50f};
	// colors[ImGuiCol_BorderShadow]           = (ImVec4){0.00f, 0.00f, 0.00f, 0.00f};
	// colors[ImGuiCol_FrameBg]                = (ImVec4){0.20f, 0.21f, 0.22f, 0.54f};
	// colors[ImGuiCol_FrameBgHovered]         = (ImVec4){0.40f, 0.40f, 0.40f, 0.40f};
	// colors[ImGuiCol_FrameBgActive]          = (ImVec4){0.18f, 0.18f, 0.18f, 0.67f};
	// colors[ImGuiCol_TitleBg]                = (ImVec4){0.04f, 0.04f, 0.04f, 1.00f};
	// colors[ImGuiCol_TitleBgActive]          = (ImVec4){0.29f, 0.29f, 0.29f, 1.00f};
	// colors[ImGuiCol_TitleBgCollapsed]       = (ImVec4){0.00f, 0.00f, 0.00f, 0.51f};
	// colors[ImGuiCol_MenuBarBg]              = (ImVec4){0.14f, 0.14f, 0.14f, 1.00f};
	// colors[ImGuiCol_ScrollbarBg]            = (ImVec4){0.02f, 0.02f, 0.02f, 0.53f};
	// colors[ImGuiCol_ScrollbarGrab]          = (ImVec4){0.31f, 0.31f, 0.31f, 1.00f};
	// colors[ImGuiCol_ScrollbarGrabHovered]   = (ImVec4){0.41f, 0.41f, 0.41f, 1.00f};
	// colors[ImGuiCol_ScrollbarGrabActive]    = (ImVec4){0.51f, 0.51f, 0.51f, 1.00f};
	// colors[ImGuiCol_CheckMark]              = (ImVec4){0.94f, 0.94f, 0.94f, 1.00f};
	// colors[ImGuiCol_SliderGrab]             = (ImVec4){0.51f, 0.51f, 0.51f, 1.00f};
	// colors[ImGuiCol_SliderGrabActive]       = (ImVec4){0.86f, 0.86f, 0.86f, 1.00f};
	// colors[ImGuiCol_Button]                 = (ImVec4){0.44f, 0.44f, 0.44f, 0.40f};
	// colors[ImGuiCol_ButtonHovered]          = (ImVec4){0.46f, 0.47f, 0.48f, 1.00f};
	// colors[ImGuiCol_ButtonActive]           = (ImVec4){0.42f, 0.42f, 0.42f, 1.00f};
	// colors[ImGuiCol_Header]                 = (ImVec4){0.70f, 0.70f, 0.70f, 0.31f};
	// colors[ImGuiCol_HeaderHovered]          = (ImVec4){0.70f, 0.70f, 0.70f, 0.80f};
	// colors[ImGuiCol_HeaderActive]           = (ImVec4){0.48f, 0.50f, 0.52f, 1.00f};
	// colors[ImGuiCol_Separator]              = (ImVec4){0.43f, 0.43f, 0.50f, 0.50f};
	// colors[ImGuiCol_SeparatorHovered]       = (ImVec4){0.72f, 0.72f, 0.72f, 0.78f};
	// colors[ImGuiCol_SeparatorActive]        = (ImVec4){0.51f, 0.51f, 0.51f, 1.00f};
	// colors[ImGuiCol_ResizeGrip]             = (ImVec4){0.91f, 0.91f, 0.91f, 0.25f};
	// colors[ImGuiCol_ResizeGripHovered]      = (ImVec4){0.81f, 0.81f, 0.81f, 0.67f};
	// colors[ImGuiCol_ResizeGripActive]       = (ImVec4){0.46f, 0.46f, 0.46f, 0.95f};
	// colors[ImGuiCol_PlotLines]              = (ImVec4){0.61f, 0.61f, 0.61f, 1.00f};
	// colors[ImGuiCol_PlotLinesHovered]       = (ImVec4){1.00f, 0.43f, 0.35f, 1.00f};
	// colors[ImGuiCol_PlotHistogram]          = (ImVec4){0.73f, 0.60f, 0.15f, 1.00f};
	// colors[ImGuiCol_PlotHistogramHovered]   = (ImVec4){1.00f, 0.60f, 0.00f, 1.00f};
	// colors[ImGuiCol_TextSelectedBg]         = (ImVec4){0.87f, 0.87f, 0.87f, 0.35f};
	// // colors[ImGuiCol_ModalWindowDarkening]   = (ImVec4){0.80f, 0.80f, 0.80f, 0.35f};
	// colors[ImGuiCol_DragDropTarget]         = (ImVec4){1.00f, 1.00f, 0.00f, 0.90f};
	// colors[ImGuiCol_NavHighlight]           = (ImVec4){0.60f, 0.60f, 0.60f, 1.00f};
	// colors[ImGuiCol_NavWindowingHighlight]  = (ImVec4){1.00f, 1.00f, 1.00f, 0.70f};


	// igGetStyle()->scalea

	ImGuiStyle_ScaleAllSizes(igGetStyle(), cd->dpi_scale);


	data->games_list_data = gui_games_list_init(db, games);
	data->controls_data = gui_controls_init(cm);
	data->show_imgui_demo = false;

	// gui_impl_scale_styles_for_dpi();

	// bool dummy = true;

	printf("dpi: %f\n", cd->dpi_scale);

	return data;
}


void gui_mainloop_draw(gui_mainloop_data_t* data)
{
	// float a = igGetIO()->DisplaySize.x;
	// printf("win width: %f\n", a);


	// data->show_imgui_demo

	// bool dummy = true;

	// return;

	// gui_impl_end_frame(); continue;

	// ImVec4 col = {1.0, 0,0,1.0};

	// igGetStyle()->Colors[ImGuiCol_WindowBg] = col;

	//ImGuiWindowFlags_NoTitleBar

	int width  = igGetIO()->DisplaySize.x;
	int height = igGetIO()->DisplaySize.y;

	// ImVec2 pos = {10,10};
	// ImVec2 size = {(width-20)/2-5, height-20};

	// igSetNextWindowPos(pos, 0, VEC2_ZERO);
	// igSetNextWindowSize(size, 0);


	// igBegin("Games", NULL, 0 | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

	// igEnd();

	// pos.x += (size.x + 10);
	// igSetNextWindowPos(pos, 0, VEC2_ZERO);
	// igSetNextWindowSize(size, 0);

	// igBegin("Hello 2", NULL, 0 | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

	// igEnd();

	{
		// ImVec2 next_pos = {10,10};
		// ImVec2 next_size = {(width-20), height-20};
		// igSetNextWindowPos(next_pos, 0, VEC2_ZERO);
		// igSetNextWindowSize(next_size, 0);


		const float win_margin = 0;
		ImVec2 next_pos = {win_margin,win_margin};
		ImVec2 next_size = {(width-(win_margin*2)), height-(win_margin*2)};
		igSetNextWindowPos(next_pos, 0, VEC2_ZERO);
		igSetNextWindowSize(next_size, 0);

	}


	igBegin("OuterWrapperWithTabs", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);


	ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
	if (igBeginTabBar("MainTabBar", tab_bar_flags))
	{
		igSpacing();
		igSpacing();

		if (igBeginTabItem("My Games", NULL, 0))
		{
			// GLOBAL_selected_tab = TAB_GAMES;

			igPushStyleColorU32(ImGuiCol_ScrollbarBg, IM_COL32(0, 0, 0, 0));
			gui_games_list_draw(data->games_list_data);
			igPopStyleColor(1);

			igEndTabItem();
		}

		ImGuiTabItemFlags f = 0; // ImGuiTabItemFlags_SetSelected
		if (igBeginTabItem("Configure Time Controls", NULL, f))
		{
			// GLOBAL_selected_tab = TAB_CONTROLS;

			// igBeginChild("ControlsChild", VEC2_ZERO, false, 0);
			igPushStyleColorU32(ImGuiCol_ScrollbarBg, IM_COL32(0, 0, 0, 0));
			gui_controls_draw(data->controls_data);
			igPopStyleColor(1);
			// igEndChild();

			igEndTabItem();
		}

		if (igBeginTabItem("Help", NULL, 0))
		{
			igBeginChild("HelpChild", VEC2_ZERO, false, 0);
			igText("TODO: Help messages here");
			igEndChild();
			igEndTabItem();
		}

		if (igBeginTabItem("Settings", NULL, 0))
		{
			igBeginChild("SettingsChild", VEC2_ZERO, false, 0);

			igSpacing();
			igSpacing();
			igIndent(0);

			igButton("Reset All", VEC2_ZERO);
			igSameLine(0.0, 20.0 * gui_common_get_data()->dpi_scale);
			igAlignTextToFramePadding();
			igText("Remove all configured games and controls");

			igEndChild();
			igEndTabItem();
		}

		if (igBeginTabItem("About", NULL, 0))
		{
			igBeginChild("AboutChild", VEC2_ZERO, false, 0);
			igText("Universal Time Control");
			const char copyright_symbol_utf8[3] = {0xC2, 0xA9, 0x00};
			igText(copyright_symbol_utf8);
			igSameLine(0.0, 2.0 * gui_common_get_data()->dpi_scale);
			igText("Asdf");
			igEndChild();
			igEndTabItem();
		}



		if (igBeginTabItem("(DEBUG) UI test", NULL, 0))
		{
			data->show_imgui_demo = true;
			igEndTabItem();
		}
		else
		{
			data->show_imgui_demo = false;
		}





		igEndTabBar();
	}

	igEnd();

	if (data->show_imgui_demo)
	{

		igSetNextWindowFocus();
		igShowDemoWindow(&data->show_imgui_demo);
	}


	// ImVec4 next_col = {1.0, 0,0,1.0};
	// igPushStyleColor(ImGuiCol_WindowBg, next_col);




	// {
	// 	ImVec2 next_pos = {10,10};
	// 	ImVec2 next_size = {(width-20), height-20};
	// 	next_pos.y += 36;
	// 	next_size.y -= 36;
	// 	igSetNextWindowPos(next_pos, 0, VEC2_ZERO);
	// 	igSetNextWindowSize(next_size, 0);
	// }

	//ImGuiWindowFlags_NoTitleBar
	// igBegin("GamesW", NULL, 0 | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

	//         igText("This is the Av\n");


	// igEnd();

	// igPopStyleColor(1);


	// igText("This is the Avocado tab!\nblah blah blah blah blah\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");





}

void gui_mainloop_cleanup(gui_mainloop_data_t* data)
{
	gui_games_list_cleanup(data->games_list_data);
	gui_controls_cleanup(data->controls_data);

	free(data);
}
