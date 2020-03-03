#include "cimgui_include.h"
#include "../../src_ui_impl/gui_impl_c.h"

#include "gui_common.h"
#include "gui_mainloop.h"
#include "gui_entry_point.h"



#include <stdio.h>

#include "../../libtcc.h"

#include <stdlib.h>
#include <string.h>

#include "../../third_party/sqlite/sqlite3.h"
#include "../db/db_owner.h"
#include "../controls/control_manager.h"
#include "../games/game_manager.h"

#ifdef LIVE_RELOAD

	// typedef int errno_t;


	static const ImVec2 VEC2_ZERO = {0,0};

	static char* my_strncat(char* destination, const char* source, size_t num)
	{
		// make ptr point to the end of destination string
		char* ptr = destination + strlen(destination);

		// Appends characters of source to the destination string
		while (*source != '\0' && num--)
			*ptr++ = *source++;

		// null terminate destination string
		*ptr = '\0';

		// destination string is returned by standard strncat()
		return destination;
	}


	#define EXPOSE_SYMBOL(sym) tcc_add_symbol(tcc_state, #sym, sym);

	static gui_mainloop_data_t* (*gui_mainloop_init__from_tcc)   (db_owner_t* db, control_manager_t* cm, game_manager_t* games);
	static void                 (*gui_mainloop_draw__from_tcc)   (gui_mainloop_data_t* data);
	static void                 (*gui_mainloop_cleanup__from_tcc)(gui_mainloop_data_t* data);

	TCCState* last_tcc_state = NULL;
	bool last_compile_failed = false;

	int compile_err_chars_remaining = 4095;
	char last_compile_error[4096];


	void tcc_err(void *opaque, const char *msg) {
		// printf("tcc err:\n");
		printf("%s\n", msg);
		int len = strlen(msg);
		char* start_at = last_compile_error + (4095-compile_err_chars_remaining);
		if (len < compile_err_chars_remaining) {
			my_strncat(start_at, msg, compile_err_chars_remaining);
		}
		compile_err_chars_remaining -= len;
		
		if (compile_err_chars_remaining > 1) {
			my_strncat(start_at+len, "\n", compile_err_chars_remaining);
			compile_err_chars_remaining--;
		}
	}


	void compile() {

		last_compile_failed = true;
		last_compile_error[0] = '\0';
		compile_err_chars_remaining = 4095;

		TCCState* tcc_state = tcc_new();
		tcc_set_error_func(tcc_state, NULL, &tcc_err);
		// tcc_set_lib_path(tcc_state, ".");
		tcc_set_lib_path(tcc_state, "tcc");
		tcc_add_library_path(tcc_state, ".");
		tcc_add_include_path(tcc_state, "tcc/include");
		tcc_add_include_path(tcc_state, "tcc/include/winapi");
		tcc_set_output_type(tcc_state, TCC_OUTPUT_MEMORY);
		// tcc_set_options(tcc_state, "-vvv");
		tcc_set_options(tcc_state, "-v -DLIVE_RELOAD");
		tcc_add_library(tcc_state, "user32");
		tcc_add_library(tcc_state, "kernel32");
		tcc_add_library(tcc_state, "ui_impl");

		// tcc_add_symbol(tcc_state, "gui_mainloop_init", const void *val)

		// tcc_add_symbol(tcc_state, "gui_common_get_data", gui_common_get_data);
		EXPOSE_SYMBOL(gui_common_get_data)

		EXPOSE_SYMBOL(db_owner_lock_and_get_db)
		EXPOSE_SYMBOL(db_owner_surrender_db_and_unlock)
		EXPOSE_SYMBOL(sqlite3_prepare_v2)
		EXPOSE_SYMBOL(sqlite3_finalize)
		EXPOSE_SYMBOL(sqlite3_errmsg)
		EXPOSE_SYMBOL(sqlite3_column_int)
		EXPOSE_SYMBOL(sqlite3_column_text)
		EXPOSE_SYMBOL(sqlite3_step)
		EXPOSE_SYMBOL(sqlite3_reset)
		EXPOSE_SYMBOL(sqlite3_bind_int)
		EXPOSE_SYMBOL(sqlite3_bind_text)
		EXPOSE_SYMBOL(sqlite3_clear_bindings)
		EXPOSE_SYMBOL(control_manager_foreach_control)
		EXPOSE_SYMBOL(control_manager_que_control_modification)
		EXPOSE_SYMBOL(control_manager_que_control_deletion)
		EXPOSE_SYMBOL(control_manager_que_control_insertion)
		EXPOSE_SYMBOL(control_manager_duringcallback_is_control_active)



		EXPOSE_SYMBOL(gui_impl_enter_get_key_mode)
		EXPOSE_SYMBOL(gui_impl_leave_get_key_mode)
		EXPOSE_SYMBOL(gui_impl_is_get_key_mode)
		EXPOSE_SYMBOL(gui_impl_get_set_key)

		EXPOSE_SYMBOL(game_manager_import_all)


		tcc_add_file(tcc_state, "src_main/gui/gui_util.c");
		tcc_add_file(tcc_state, "src_main/gui/gui_mainloop.c");
		tcc_add_file(tcc_state, "src_main/gui/gui_games_list.c");
		tcc_add_file(tcc_state, "src_main/gui/gui_controls.c");

		// abort();

		int compile_result = tcc_compile_string(tcc_state, "");
		if (compile_result == -1) {
			printf("failed to compile, using old\n");
			last_compile_failed = true;
			return;
			// abort();
		}

		int relocate_result = tcc_relocate(tcc_state, TCC_RELOCATE_AUTO);
		if (relocate_result == -1) {
			printf("failed to relocate, using old\n");
			last_compile_failed = true;
			return;
			// abort();
		}

		void* temp1 = tcc_get_symbol(tcc_state, "gui_mainloop_init");
		void* temp2 = tcc_get_symbol(tcc_state, "gui_mainloop_draw");
		void* temp3 = tcc_get_symbol(tcc_state, "gui_mainloop_cleanup");

		if (!temp1) {
			printf("NULL mainloop init, using old\n");
			return;
		}

		if (!temp2) {
			printf("NULL mainloop draw, using old\n");
			return;
		}

		if (!temp3) {
			printf("NULL mainloop cleanup, using old\n");
			return;
		}

		gui_mainloop_init__from_tcc = temp1;
		gui_mainloop_draw__from_tcc    = temp2;
		gui_mainloop_cleanup__from_tcc = temp3;

		if (last_tcc_state) {
			tcc_delete(last_tcc_state);
		}
		
		last_tcc_state = tcc_state;
		last_compile_failed = false;
	}

#endif

void gui_entry_point_start_and_run_loop(db_owner_t* db, control_manager_t* cm, game_manager_t* games) {

	gui_impl_init("Universal Time Control | BETA build");
	gui_impl_set_clear_colour(36, 40, 47);

	gui_common_get_data()->dpi_scale = gui_impl_get_scale();

#ifdef LIVE_RELOAD
	last_compile_error[0] = '\0';
	compile();
	if (!last_tcc_state) {
		printf("couldn't compile\n");
		abort();
	}
	gui_mainloop_data_t* mainloop_data = gui_mainloop_init__from_tcc(db, cm, games);
#else
	gui_mainloop_data_t* mainloop_data = gui_mainloop_init(db, cm, games);
#endif

	

	while (1 && !gui_impl_begin_frame_and_return_should_quit()) {

#ifdef LIVE_RELOAD

		compile();

		if (last_compile_failed) {

			int width  = igGetIO()->DisplaySize.x;
			int height = igGetIO()->DisplaySize.y;
			ImVec2 next_pos = {10,10};
			ImVec2 next_size = {(width-20), height-20};
			igSetNextWindowPos(next_pos, 0, VEC2_ZERO);
			igSetNextWindowSize(next_size, 0);

			igBegin("CompileFailedWindow", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
			

		    igPushFont(gui_common_get_data()->font_bold_big);
		    igText("GUI failed to compile");
		    igPopFont();
		    igSpacing();

			igText("%s", last_compile_error);
			igEnd();
		} else {
			gui_mainloop_draw__from_tcc(mainloop_data);
		}

#else
		gui_mainloop_draw(mainloop_data);
#endif

		gui_impl_end_frame();
	}

#ifdef LIVE_RELOAD
	gui_mainloop_cleanup__from_tcc(mainloop_data);
	if (last_tcc_state) {
		tcc_delete(last_tcc_state);
	}
#else
	printf("exit4\n");
	gui_mainloop_cleanup(mainloop_data);
	printf("exit5\n");
#endif

	gui_impl_cleanup();
	printf("exit6\n");
}


