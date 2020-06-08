#include "process_inject.h"

#include <stdio.h>
#include <string.h>

#include "../../shared.h"

static bool _is_32bit(HANDLE process)
{
    SYSTEM_INFO systemInfo;
    GetNativeSystemInfo(&systemInfo);

    // host is 32bit, so process must be
    if (systemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL) {
        return true;
    }

    BOOL wow64;
    IsWow64Process(process, &wow64);
    return wow64;
}

bool inject_process(DWORD pid, HANDLE process, HWND* out_cnc_hwnd) {

	{
		wchar_t window_name[200];
		swprintf(window_name, 200, L"%ls%lu", CNC_WNDNAME_PREFIX, pid);

		printf("looking for class=%ls, wname=%ls\n", CNC_WNDCLASS_NAME, window_name);

		HWND window = FindWindowW(CNC_WNDCLASS_NAME, window_name);
		if (window) {
			*out_cnc_hwnd = window;
			return true;
		}
	}


	wchar_t exe_path[MAX_PATH];
	GetModuleFileNameW(NULL, exe_path, MAX_PATH);
	wchar_t* last_slash = wcsrchr(exe_path, L'\\');
	last_slash++;
	if (_is_32bit(process)) {
		wcscpy(last_slash, L"data\\inject_x32.exe");
	} else {
		wcscpy(last_slash, L"data\\inject_x64.exe");
	}
	
	printf("%ls\n", exe_path);


	STARTUPINFOW startup;
	memset(&startup, 0, sizeof(startup));
	startup.cb = sizeof(STARTUPINFOW);

	PROCESS_INFORMATION processinfo;
	memset(&processinfo, 0, sizeof(processinfo));

	wchar_t cmd_line[MAX_PATH];
	swprintf(cmd_line, MAX_PATH, L"%ls %lu", exe_path, pid);

	printf("cmd line: %ls\n", cmd_line);

	BOOL res = CreateProcessW(
		NULL,
		cmd_line, //cmd
		NULL, //processattr
		NULL, //threadattr
		FALSE,//handles
		0, // |CREATE_SUSPENDED
		NULL,//env
		NULL,//currdir
		&startup,
		&processinfo
	);

	CloseHandle(processinfo.hProcess);
	CloseHandle(processinfo.hThread);

	if (!res) {
		printf("CreateProcessW failed\n");
		return false;
	}

	Sleep(1500);

	{
		wchar_t window_name[200];
		swprintf(window_name, 200, L"%ls%lu", CNC_WNDNAME_PREFIX, pid);

		printf("looking for class=%ls, wname=%ls\n", CNC_WNDCLASS_NAME, window_name);

		HWND window = FindWindowW(CNC_WNDCLASS_NAME, window_name);
		if (!window) {
			MessageBoxA(NULL, "Failed to take control of game. Please try again.", "Error", MB_OK);
			// abort();
			return false;
		}
		*out_cnc_hwnd = window;


		return true;
	}



}