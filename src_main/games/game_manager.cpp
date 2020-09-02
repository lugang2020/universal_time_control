#include "../windows_include.h"
#include "game_manager.h"


#include "../util.h"

#include "../../third_party/sqlite/sqlite3.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wignored-qualifiers"

#include "../../third_party/vdf/vdf_parser.hpp"

#pragma GCC diagnostic pop

// #include "../../third_party/mingw-std-threads/mingw.thread.h"
// #include "../../third_party/mingw-std-threads/mingw.mutex.h"

static const char* const ignore_exes[] = {
#include "ignore_exes.inc.cpp"
};

static int const bad_appids[] = {
#include "bad_appids.inc.cpp"
};

#include "../../third_party/mingw-std-threads/mingw.thread.h"
#include "../../third_party/mingw-std-threads/mingw.mutex.h"
#include "../../third_party/mingw-std-threads/mingw.condition_variable.h"
#include <atomic>

struct _game_manager {
	// std::thread thread;
	// std::mutex main_mutex;
	// std::mutex update_and_delete_mutex;
	db_owner_t* db_owner;
	std::thread thread;
	std::mutex mutex;
	std::atomic<bool> is_importing;
	std::atomic<bool> should_quit;
	std::atomic<bool> should_import;

	// std::vector<control_t> to_modify;
	// std::vector<control_t> to_delete;
	// bool should_insert;

	// set_of_controls controls;

	// bool should_quit;
};

// 0: normal, 1: importing
int  game_manager_get_state(game_manager_t* s) {
	return s->is_importing ? 1 : 0;
}

void game_manager_set_should_import(game_manager_t* s) {
	s->should_import = true;
}

static const char* GAME_UPSERT_QUERY = R"""(
	insert into game (steam_appid, name, allowed, enabled, profile_id, date_touched)
	values (?,?,?,1,1,?)
	on conflict(steam_appid) do update set name=excluded.name, allowed=excluded.allowed, date_touched=excluded.date_touched;
)""";

// returns primary key id (not appid)
static int upsert_steam_game(sqlite3* db, int steam_appid, const char* name, bool allowed, int64_t date_touched) {

	// upsert
	{
		sqlite3_stmt* upsert_stmt;
		int prepare_result = sqlite3_prepare_v2(db, GAME_UPSERT_QUERY, -1, &upsert_stmt, NULL);
		if (prepare_result != SQLITE_OK) {
			const char* err = sqlite3_errmsg(db);
			printf("sqlite err1: %s\n", err);
			printf("query: %s", GAME_UPSERT_QUERY);
			printf("version: %s", sqlite3_libversion());
			abort();
		}
		sqlite3_bind_int(upsert_stmt, 1, steam_appid);
		sqlite3_bind_text(upsert_stmt, 2, name, -1, NULL);
		sqlite3_bind_int(upsert_stmt, 3, allowed ? 1 : 0);
		sqlite3_bind_int64(upsert_stmt, 4, date_touched);

		if (sqlite3_step(upsert_stmt) != SQLITE_DONE) {
			const char* err = sqlite3_errmsg(db);
			LOGI("sqlite upsert game err: %s,abort", err);
			abort();
		}

		sqlite3_reset(upsert_stmt);
		sqlite3_clear_bindings(upsert_stmt);
		sqlite3_finalize(upsert_stmt);
	}

	// then re-fetch ID (can't use sqlite3_last_insert_rowid() for upserts...)
	{
		sqlite3_stmt* fetch_id_stmt;
		int prepare_result = sqlite3_prepare_v2(db, "select id from game where steam_appid = ?", -1, &fetch_id_stmt, NULL);
		if (prepare_result != SQLITE_OK) {
			const char* err = sqlite3_errmsg(db);
			LOGI("sqlite err2: %s\n", err);
			abort();
		}
		sqlite3_bind_int(fetch_id_stmt, 1, steam_appid);

		if (sqlite3_step(fetch_id_stmt) != SQLITE_ROW) {
			const char* err = sqlite3_errmsg(db);
			LOGI("sqlite upsert game refetch err: %s\n", err);
			abort();
		}

		int result = sqlite3_column_int(fetch_id_stmt, 0);


		sqlite3_reset(fetch_id_stmt);
		sqlite3_clear_bindings(fetch_id_stmt);
		sqlite3_finalize(fetch_id_stmt);

		return result;


	}




}


static void delete_exes_from_db(sqlite3* db, int game_id) {
	sqlite3_stmt* del_stmt;
	int prepare_result = sqlite3_prepare_v2(db, "delete from game_exe where game_id = ?;", -1, &del_stmt, NULL);
	if (prepare_result != SQLITE_OK) {
		const char* err = sqlite3_errmsg(db);
		printf("sqlite err3: %s\n", err);
		abort();
	}
	sqlite3_bind_int(del_stmt, 1, game_id);

	if (sqlite3_step(del_stmt) != SQLITE_DONE) {
		const char* err = sqlite3_errmsg(db);
		printf("sqlite delete game exe err: %s\n", err);
		abort();
	}

	sqlite3_reset(del_stmt);
	sqlite3_clear_bindings(del_stmt);
	sqlite3_finalize(del_stmt);
}

static void delete_old_steam_games_from_db(sqlite3* db, int timestamp) {
	sqlite3_stmt* del_stmt;
	int prepare_result = sqlite3_prepare_v2(db, "delete from game where date_touched < ?;", -1, &del_stmt, NULL);
	if (prepare_result != SQLITE_OK) {
		const char* err = sqlite3_errmsg(db);
		printf("sqlite err4: %s\n", err);
		abort();
	}
	sqlite3_bind_int(del_stmt, 1, timestamp);

	if (sqlite3_step(del_stmt) != SQLITE_DONE) {
		const char* err = sqlite3_errmsg(db);
		printf("sqlite delete old steam games err: %s\n", err);
		abort();
	}

	sqlite3_reset(del_stmt);
	sqlite3_clear_bindings(del_stmt);
	sqlite3_finalize(del_stmt);
}


static void insert_exe_to_db(sqlite3* db, int game_id, const char* path) {
	LOGI("%d,%s",game_id,path);
	sqlite3_stmt* insert_stmt;
	int prepare_result = sqlite3_prepare_v2(db, "insert into game_exe (game_id, path) values (?, ?);", -1, &insert_stmt, NULL);
	if (prepare_result != SQLITE_OK) {
		const char* err = sqlite3_errmsg(db);
		LOGI("sqlite err5: %s\n", err);
		abort();
	}
	sqlite3_bind_int(insert_stmt, 1, game_id);
	sqlite3_bind_text(insert_stmt, 2, path, -1, NULL);

	if (sqlite3_step(insert_stmt) != SQLITE_DONE) {
		const char* err = sqlite3_errmsg(db);
		LOGI("sqlite insert game exe err: %s\n", err);
		abort();
	}

	sqlite3_reset(insert_stmt);
	sqlite3_clear_bindings(insert_stmt);
	sqlite3_finalize(insert_stmt);
}


static void _game_manager_thread(game_manager_t* s) {

	while (1) {
		util_sleep_for_ms(10*1000);
		if (s->should_quit) {
			break;
		}

		if (!s->should_import) continue;

		s->is_importing = true;

		game_manager_import_all(s);

		s->is_importing = false;
		s->should_import = false;
	}

}


game_manager_t*   game_manager_create(db_owner_t* db) {
	game_manager_t* s = new game_manager_t;
	s->db_owner = db;
	s->is_importing = false;
	s->should_quit = false;
	s->should_import = false;
	s->thread = std::thread(_game_manager_thread, s);
	return s;
}
void               game_manager_destroy(game_manager_t* s) {
	s->should_quit = true;
	s->thread.join();
	delete s;
}

typedef tyti::vdf::object vdf_t;

// #include <filesystem>

#define BUFFERSIZE 8192


#include <sstream>

static void _DisplayError(const char* lpszFunction)
// Routine Description:
// Retrieve and output the system error message for the last-error code
{
    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError();

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0,
        NULL );

    lpDisplayBuf =
        (LPVOID)LocalAlloc( LMEM_ZEROINIT,
                            ( lstrlen((LPCTSTR)lpMsgBuf)
                              + lstrlen((LPCTSTR)lpszFunction)
                              + 40) // account for format string
                            * sizeof(TCHAR) );

    if (FAILED( sprintf((LPTSTR)lpDisplayBuf,
                     // LocalSize(lpDisplayBuf) / sizeof(TCHAR),
                     TEXT("%s failed with error code %lu as follows:\n%s"),
                     lpszFunction,
                     dw,
                     (const char*)lpMsgBuf)))
    {
        printf("FATAL ERROR: Unable to output error code.\n");
    }

    printf(TEXT("ERROR: %s\n"), (LPCTSTR)lpDisplayBuf);

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
}


static void _MyReadFile(const char* filename, char* ReadBuffer)
  {

    HANDLE hFile;





    hFile = CreateFileA(filename,
                       GENERIC_READ,          // open for reading
                       FILE_SHARE_READ,       // share for reading
                       NULL,                  // default security
                       OPEN_EXISTING,         // existing file only
                       FILE_ATTRIBUTE_NORMAL, // normal file
                       NULL);                 // no attr. template

    if (hFile == INVALID_HANDLE_VALUE)
    {
    	printf("Invalid file handle: %s\n", filename);
    	_DisplayError("CreateFileA");
        return;
    }

    // printf("Created file handle OK\n");

    DWORD  dwBytesRead = 0;
    if (ReadFile(hFile, ReadBuffer, BUFFERSIZE-1, &dwBytesRead, NULL) == FALSE)
    {
    	_DisplayError("ReadFile");
        CloseHandle(hFile);
        return;
    }

    if (dwBytesRead > 0 && dwBytesRead <= BUFFERSIZE-1)
    {
        ReadBuffer[dwBytesRead]='\0'; // NULL character
    }
    else if (dwBytesRead == 0)
    {
    	printf("Read 0 bytes: %s\n", filename);
    	CloseHandle(hFile);
    	return;
    }

    CloseHandle(hFile);
}

static vdf_t _read_vdf(const std::string& vdf_path) {
	// printf("about to parse: %s\n", vdf_path.c_str());

	// char short_path[]
	// GetShortPathNameA


	char ReadBuffer[BUFFERSIZE] = {0};

	// printf("Will attempt to read path: %s\n", vdf_path.c_str());
	_MyReadFile(vdf_path.c_str(), ReadBuffer);
	// printf("Read data:\n%s\n", ReadBuffer);
	// printf("Read file OK\n");
	// printf("Will try to parse\n");

	auto str = std::string(ReadBuffer);
	auto ss = std::stringstream(str);

	auto obj = tyti::vdf::read(ss);
	// printf("Parsed OK\n");
	return obj;
}

typedef std::vector<std::string> strvec_t;

// #include <Windows.h>


// static bool endsWith(const std::string& str, const std::string& suffix)
// {
//     return str.size() >= suffix.size() && 0 == str.compare(str.size()-suffix.size(), suffix.size(), suffix);
// }

// static bool startsWith(const std::string& str, const std::string& prefix)
// {
//     return str.size() >= prefix.size() && 0 == str.compare(0, prefix.size(), prefix);
// }

static bool endsWith(const std::string& str, const char* suffix, unsigned suffixLen)
{
    return str.size() >= suffixLen && 0 == str.compare(str.size()-suffixLen, suffixLen, suffix, suffixLen);
}

static bool endsWith(const std::string& str, const char* suffix)
{
    return endsWith(str, suffix, std::string::traits_type::length(suffix));
}

static bool startsWith(const std::string& str, const char* prefix, unsigned prefixLen)
{
    return str.size() >= prefixLen && 0 == str.compare(0, prefixLen, prefix, prefixLen);
}

// static bool startsWith(const std::string& str, const char* prefix)
// {
//     return startsWith(str, prefix, std::string::traits_type::length(prefix));
// }

static strvec_t _find_exes(const std::string& base_path) {
	strvec_t exes;

	WIN32_FIND_DATAA data;
	std::string path_suffixed = base_path + "\\*";
	HANDLE find = FindFirstFileA(path_suffixed.c_str(), &data);
	if (find == INVALID_HANDLE_VALUE) {
		return exes;
	}

	do {
        if(strcmp(data.cFileName, ".") != 0 && strcmp(data.cFileName, "..") != 0) {
			if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				std::string dir = base_path + "\\" + data.cFileName;
				// printf("considering dir: %s\n", dir.c_str());
				strvec_t more_exes = _find_exes(dir);
				// exes.insert(exes.end(), more_exes.begin(), more_exes.end());
				for (const auto& _exe : more_exes) {
					exes.push_back(_exe);
				}
			} else {
				std::string name(data.cFileName);
				if (endsWith(name, ".exe", 4)) {
					// printf("%s\n",data.cFileName);
					exes.push_back(base_path + "\\" + name);
				}
			}
        }



// next:;

	} while(FindNextFileA(find, &data) != 0);

	FindClose(find);

	// printf("")

	strvec_t result;
	for (const auto& exe : exes) {
		for (const auto& ignore : ignore_exes) {
			if (endsWith(exe, ignore)) {
				goto next_exe;
			}
		}
		result.push_back(exe);
next_exe:;
	}

	return result;
}

static strvec_t _find_acfs(const std::string& base_path) {
	strvec_t acfs;

	LOGI("searching: %s\n", base_path.c_str());

	WIN32_FIND_DATAA data;
	std::string path_suffixed = base_path + "\\*";
	HANDLE find = FindFirstFileA(path_suffixed.c_str(), &data);
	if (find == INVALID_HANDLE_VALUE) {
		return acfs;
	}

	// printf("looking...\n");

	do {
		// printf("found file: %s\n", data.cFileName);
		if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			std::string name(data.cFileName);

			if (startsWith(name, "appmanifest_", 12) && endsWith(name, ".acf", 4)) {
				LOGI("found file: %s\n", name.c_str());
				acfs.push_back(base_path + "\\" + name);
			}
		}
	} while(FindNextFileA(find, &data) != 0);

	FindClose(find);

	return acfs;
}


static int parse_positive_int_or_minus1(const char* str) {
	int maybe_number = strtol(str, NULL, 10);
	if (errno == ERANGE) {
		errno = 0;
		return -1;
	}
	if (maybe_number == 0) {
		return -1;
	}
	return maybe_number;
}

#include <chrono>
#include <inttypes.h>


static bool _find_steamapps_path(char* out, size_t out_len) {

    HKEY key;
    LONG res = RegOpenKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\Valve\\Steam", 0, KEY_READ | KEY_WOW64_64KEY, &key);
    if (res != ERROR_SUCCESS) {
      printf("ERR1 Failed to find Steam path. Is Steam installed?\n");
      return false;
    }

    memset(out, 0, out_len);
    DWORD len = out_len;


    ULONG read_err = RegQueryValueExA(key, "SteamPath", NULL, NULL, (uint8_t*)out, &len);
    if (read_err != ERROR_SUCCESS) {
        printf("ERR2 Failed to find Steam path. Is Steam installed?\n");
        RegCloseKey(key);
        return false;
    }

    RegCloseKey(key);

    // printf("asdf: %c\n", out[len-2] );
    if ((out[len-2] != '/') && (out[len-2] != '\\')) {
        strcat(out, "/");
    }

    strcat(out, "steamapps");

    // printf("path:\n");

    return true;


}

static void _get_installed_games(game_manager_t* s) {
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));

	if(!_find_steamapps_path(buffer, 1024)) {
		MessageBoxA(NULL, "Failed to find Steam path. Is Steam installed?", "Error", MB_OK);
		abort();
	}

	LOGI("Detected Steam path: %s\n", buffer);

	std::string default_library_dir = buffer;
	std::string libraryfolders_path = default_library_dir + "\\libraryfolders.vdf";

	// std::string default_library_dir = "C:\\Program Files (x86)\\Steam\\steamapps"; // todo
	// std::string libraryfolders_path = default_library_dir + "\\libraryfolders.vdf";
	// std::string libraryfolders_path = "C:\\Code\\universal_time_control\\bad_libfolders.txt";

	std::vector<std::string> libraries;
	libraries.push_back(default_library_dir);

	try {
		auto libraryfolders_vdf = _read_vdf(libraryfolders_path);

		for (auto elem : libraryfolders_vdf.attribs) {
			const char* name = elem.first.c_str();
			int maybe_number = parse_positive_int_or_minus1(name);
			if (maybe_number == -1) continue;

			// printf("Child %s: %s\n", name, elem.second.c_str());
			libraries.push_back(elem.second + "\\steamapps");
		}

		for (const auto& lib : libraries) {
			LOGI("Library: %s\n", lib.c_str());
		}
	} catch (const std::exception& e) {
		printf("Error reading libraryfolders: %s\n", e.what());
	}



	// std::vector<std::string> acfs;

	int64_t timestamp = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	LOGI("time: %lld", timestamp);

	sqlite3* db = db_owner_lock_and_get_db(s->db_owner);

	for (const auto& lib : libraries) {
		// printf("Library: %s\n", lib.c_str());
		strvec_t _acfs = _find_acfs(lib);
		// acfs.insert(acfs.end(), _acfs.begin(), _acfs.end());

		for (const auto& acf : _acfs) {
			LOGI("%s\n", acf.c_str());

			try {
				auto vdf = _read_vdf(acf);

				auto name_it = vdf.attribs.find("name");
				if (name_it == vdf.attribs.end()) {
					continue;
				}
				// printf("game name: %s\n", name_it->second.c_str());

				auto install_it = vdf.attribs.find("installdir");
				if (install_it == vdf.attribs.end()) {
					continue;
				}
				std::string install_dir = lib + "\\common\\" + install_it->second;
				LOGI("game path: %s\n", install_dir.c_str());

				auto appid_it = vdf.attribs.find("appid");
				if (appid_it == vdf.attribs.end()) {
					continue;
				}

				int maybe_number = parse_positive_int_or_minus1(appid_it->second.c_str());
				if (maybe_number == -1) continue;
				if (maybe_number == 228980) continue; // common redist- not just "bad", don't include at all

				bool is_forbidden = false;
				for (auto bad : bad_appids) {
					if (maybe_number == bad) {
						is_forbidden = true;
						break;
					}
				}

				int id = upsert_steam_game(db, maybe_number, name_it->second.c_str(), !is_forbidden, timestamp);
				delete_exes_from_db(db, id);

				if (!is_forbidden) {
					auto exes = _find_exes(install_dir);
					for (const auto& exe : exes) {
						insert_exe_to_db(db, id, exe.c_str());
					}
				}

			} catch (const std::exception& e) {
				LOGI("Error reading acf (or finding EXEs) %s: %s\n", acf.c_str(), e.what());
			}

		}


	}

	delete_old_steam_games_from_db(db, timestamp);
	db_owner_surrender_db_and_unlock(s->db_owner);



	// let apps = [];
	// let all_exes = [];

	// for (let acf of acfs) {
	// 	try {
	// 		let acf_content = await SteamUtils._readVdf(acf);
	// 		// console.log(acf_content);

	// 		let install_path = path.join(path.dirname(acf), "common", acf_content.AppState.installdir);
	// 		// console.log(install_path);

	// 		let exes = await SteamUtils._findExes(install_path);
	// 		// console.log(exes);
	// 		all_exes = all_exes.concat(exes);
	// 	} catch (e) {
	// 		console.log("WARNING: Failed to parce ACF:");
	// 		console.log(e);
	// 	}

	// }

	// console.log(all_exes);

}











void  game_manager_import_all(game_manager_t* s) {
	_get_installed_games(s);
}
