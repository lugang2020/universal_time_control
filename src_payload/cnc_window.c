#include <Windows.h>

#include "cnc_window.h"

#include "../shared.h"

#include <string.h>
#include <stdio.h>

#include "speedhack.h"

LRESULT WINAPI cnc_wndproc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

HWND g_hwnd = NULL;

HWND universal_time_control_get_hwnd() {
	return g_hwnd;
}

DWORD WINAPI cnc_threadproc(void* param) {
	(void)param;

	speedhack_init();

	wchar_t* class_name = CNC_WNDCLASS_NAME;
	WNDCLASSEXW wc = { sizeof(WNDCLASSEXW), CS_CLASSDC, cnc_wndproc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, class_name, NULL };

	HINSTANCE hinst = GetModuleHandle(NULL);
	(void)hinst;

	ATOM cls = RegisterClassExW(&wc);
	(void)cls;

	wchar_t window_name[200];
	swprintf(window_name, 200, L"%ls%lu", CNC_WNDNAME_PREFIX, GetCurrentProcessId());

	UINT flags = 0;//WS_OVERLAPPEDWINDOW;
	HWND hwnd = CreateWindowExW(0, class_name, window_name, flags, CW_USEDEFAULT, CW_USEDEFAULT,
		0, // w
		0, // h
	NULL, NULL, wc.hInstance, NULL);


	g_hwnd = hwnd;
	// (void)hwnd;
	// Hide window
	// ShowWindow(hwnd, SW_SHOWDEFAULT);
	// UpdateWindow(hwnd);


	MSG msg;
	while (1) {
		BOOL get_result = GetMessage(&msg, NULL, 0, 0);
		if (get_result == -1) return 1;

		if (msg.message == WM_QUIT) {
			return 0;
		}

		TranslateMessage(&msg);
		DispatchMessage(&msg);
		continue;
	}

	return 1;
}

HINSTANCE g_hinstDLL;

void setup_cnc_window(HINSTANCE hinstDLL) {
	g_hinstDLL = hinstDLL;
	CreateThread(NULL, 0, cnc_threadproc, NULL, 0, NULL);
	Sleep(250);
}

static void _process_cmd(HWND hWnd, timecontrol_ipc_cmd_t* cmd) {
	if (cmd->cmd_type == UTC_IPC_CMD_SET_TIMESCALE) {
		speedhack_set_timescale(cmd->timeScale);
	} else if (cmd->cmd_type == UTC_IPC_CMD_SHOULD_UNLOAD) {
		speedhack_set_timescale(1.0);
		// speedhack_unload();
		// DestroyWindow(hWnd);
		// UnregisterClassW(CNC_WNDCLASS_NAME, GetModuleHandle(NULL));
		// FreeLibraryAndExitThread(g_hinstDLL, 0);
	}
}

LRESULT WINAPI cnc_wndproc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {

		case WM_COPYDATA: {
			COPYDATASTRUCT* cds = (COPYDATASTRUCT*) lParam;
			if (cds->dwData != WM_COPYDATA_ID) return 0;

			timecontrol_ipc_cmd_t* cmd = (timecontrol_ipc_cmd_t*) cds->lpData;
			_process_cmd(hWnd, cmd);
			return TRUE;
		}

		default:
		return DefWindowProcW(hWnd, msg, wParam, lParam);
	}

	printf("end of wndproc\n");
	MessageBoxA(NULL, "end of wndproc", "", MB_OK);
}
