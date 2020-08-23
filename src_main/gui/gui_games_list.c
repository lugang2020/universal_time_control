#include "gui_util.h"
#include "gui_games_list.h"
#include "gui_common.h"

#include <stdlib.h>
#include <string.h>

#include "../util.h"

#include "../../third_party/sqlite/sqlite3.h"

static const ImVec2 VEC2_ZERO = {0,0};

#define QUOTE(...) #__VA_ARGS__

const char* get_games_statement_sql = QUOTE(
        select
        game.id,
        game.steam_appid,
        // case when game.steam_appid=-1 then 0 else 1 end as is_steam,
        game.name,
        game.allowed,
        game.enabled,
        game.profile_id
        from game order by game.name;
                                      );

const char* update_game_statement_sql = QUOTE(
        update game set
        steam_appid=?,
        name=?,
        allowed=?,
        enabled=?,
        profile_id=?
                   where
                   id=?
                                        );

typedef struct
{
	int id;
	int steam_appid;
	bool is_steam;
	char name[256];
	bool allowed;
	bool enabled;
	int  profile_id;
} game_t;

#ifdef LIVE_RELOAD
errno_t strncpy_s(char *restrict dest, size_t destsz,
                  const char *restrict src, size_t count);
#endif

static void deserialize_game(sqlite3_stmt* stmt, game_t* out_game)
{
	out_game->id          = sqlite3_column_int(stmt, 0);
	out_game->steam_appid = sqlite3_column_int(stmt, 1);
	out_game->is_steam    = out_game->steam_appid != -1;
	const char* name_temp = (const char*)sqlite3_column_text(stmt, 2);
	strncpy_s(out_game->name, 256, name_temp, 255);

	out_game->allowed = sqlite3_column_int(stmt, 3) != 0;
	out_game->enabled = sqlite3_column_int(stmt, 4) != 0;
	out_game->profile_id = sqlite3_column_int(stmt, 5);
}

static void reserialize_game(sqlite3* db, sqlite3_stmt* stmt, const game_t* game)
{

	printf("setting enabled=%i\n", game->enabled);

	printf("appid: %i\n", game->steam_appid);
	sqlite3_bind_int(stmt, 1, game->steam_appid);

	if (sqlite3_bind_text(stmt, 2, game->name, -1, NULL) != SQLITE_OK)
	{
		printf("bind problem 0\n");
	}

	sqlite3_bind_int(stmt, 3, game->allowed ? 1 : 0);
	sqlite3_bind_int(stmt, 4, game->enabled ? 1 : 0);

	sqlite3_bind_int(stmt, 5, game->profile_id);

	sqlite3_bind_int(stmt, 6, game->id);

	if (sqlite3_step(stmt) != SQLITE_DONE)
	{
		const char* err = sqlite3_errmsg(db);
		printf("sqlite err: %s\n", err);
	}
	printf("reserialize ok\n");


	sqlite3_clear_bindings(stmt);
	sqlite3_reset(stmt);
}

struct _gui_games_list_data
{
	bool testvar_a, testvar_b;

	int item_current;

	db_owner_t* db_owner;
	game_manager_t* games;

	sqlite3_stmt* get_games_statement;
	sqlite3_stmt* update_game_statement;


};

gui_games_list_data_t* gui_games_list_init(db_owner_t* db_owner, game_manager_t* games)
{
	MALLOC_TYPE(gui_games_list_data_t, data)

	data->testvar_a = true;
	data->testvar_b = true;
	data->item_current = 0;

	data->db_owner = db_owner;
	data->games = games;

	sqlite3* db = db_owner_lock_and_get_db(db_owner);
	int prepare_result = sqlite3_prepare_v2(db, get_games_statement_sql, -1, &data->get_games_statement, NULL);
	if (prepare_result != SQLITE_OK)
	{
		const char* err = sqlite3_errmsg(db);
		printf("sqlite err: %s\n", err);
		abort();
	}

	prepare_result = sqlite3_prepare_v2(db, update_game_statement_sql, -1, &data->update_game_statement, NULL);
	if (prepare_result != SQLITE_OK)
	{
		const char* err = sqlite3_errmsg(db);
		printf("sqlite err: %s\n", err);
		abort();
	}


	db_owner_surrender_db_and_unlock(db_owner);

	return data;
}






static void draw_game(gui_games_list_data_t* data, game_t* game)
{
	(void)data;

	gui_common_data_t* cd = gui_common_get_data();

	// if (!is_first) {
	// 	// igSeparator();
	// }

	igPushIDInt(game->id);


	ImVec2 p0 = igGetCursorScreenPos();

	igBeginGroup();



	igSpacing();
	igSpacing();
	igIndent(0.0);

	igPushTextWrapPos(igGetWindowContentRegionWidth() - igGetStyle()->IndentSpacing);

	igPushFont(cd->font_bold_big);
	igText(game->name);
	igPopFont();
	igPopTextWrapPos();

	igSpacing();

	if (!game->allowed)
	{
		igPushStyleColorU32(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
		gui_util_text_with_font("Universal Time Control blacklists some popular multiplayer games.", cd->font_bold);
		igPopStyleColor(1);
		igSpacing();
		igText("This is necessary due to the potential for misuse. We apologise for any inconvenience.");
		igSpacing();
		// igButton("Hide blacklisted games", VEC2_ZERO);

		goto end_of_game_widgets;
	}

	igCheckbox("Control time for this game", &game->enabled);
	igSpacing();


	// gui_util_text_with_font("Source:", cd->font_bold);



	// gui_util_same_line_text(game->is_steam ? "Steam" : "Manual");

	// igText("Steam appid %i", game->steam_appid);

	if (!game->is_steam)
	{
		gui_util_text_with_font("Manually added", cd->font_bold);
	}




end_of_game_widgets:



	igSpacing();
	igSpacing();
	igSpacing();

	igEndGroup();

	ImVec2 p1 = igGetItemRectMax();



	p1.x += igGetStyle()->IndentSpacing;

	p1.x = igGetWindowContentRegionWidth() + 0.0f * gui_common_get_data()->dpi_scale;

	ImDrawList_AddRect(igGetWindowDrawList(), p0, p1, IM_COL32(128,128,255,255), 4.0, ImDrawCornerFlags_All, 3.0f);

	igSpacing();
	igSpacing();

	igPopID();
}

static void draw_games(gui_games_list_data_t* data)
{
	sqlite3* db = db_owner_lock_and_get_db(data->db_owner);



	while(1)
	{
		if (sqlite3_step(data->get_games_statement) != SQLITE_ROW) break;

		game_t game;
		deserialize_game(data->get_games_statement, &game);
		game_t game_backup = game;
		draw_game(data, &game);

		if (memcmp(&game, &game_backup, sizeof(game_backup)) != 0)
		{
			printf("will reserialize...\n");
			printf("was=%i, now=%i\n", game_backup.enabled, game.enabled);
			reserialize_game(db, data->update_game_statement, &game);
		}



		// const char* path = (const char*)sqlite3_column_text(data->get_games_statement, 0);

		// char path_utf8_lower[MAX_PATH];
		// util_casefold_utf8(path, path_utf8_lower, MAX_PATH-1);

		// process_path_t _enabled;
		// strcpy(_enabled.path, (const char*)path_utf8_lower);
		// vec_push(out_enabled, _enabled);
	}


	sqlite3_reset(data->get_games_statement);





	db_owner_surrender_db_and_unlock(data->db_owner);
}

void gui_games_list_draw(gui_games_list_data_t* data)
{
	igBeginChild("GamesChild", (ImVec2)
	{
		0,-28.0 * gui_common_get_data()->dpi_scale
	}, false, ImGuiWindowFlags_AlwaysVerticalScrollbar);



	igSpacing();

	if (game_manager_get_state(data->games) == 1)
	{
		igText("Importing, please wait...");
	}
	else
	{
		draw_games(data);
	}



	igSpacing();
	igSpacing();

	igEndChild();


	igButton("Add non-Steam game...", VEC2_ZERO);
	gui_util_same_line();
	if (igButton("Re-import Steam games", VEC2_ZERO))
	{
		// data->is_importing
		// gui_util_same_line();
		// igText("Please wait, this may take a moment...");
		// game_manager_import_all(data->games);
		game_manager_set_should_import(data->games);
	}
}

void gui_games_list_cleanup(gui_games_list_data_t* data)
{

	(void)db_owner_lock_and_get_db(data->db_owner);

	sqlite3_finalize(data->get_games_statement);
	sqlite3_finalize(data->update_game_statement);

	db_owner_surrender_db_and_unlock(data->db_owner);


	free(data);
}