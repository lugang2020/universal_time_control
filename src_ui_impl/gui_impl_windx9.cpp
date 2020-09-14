
#define GUI_IMPL_API			extern "C" __declspec(dllexport)
#include "../src_main/windows_include.h"
#include "gui_impl_c.h"

// #include <imgui/imgui.h>
#include "../third_party/cimgui/imgui/imgui.h"

// #define IMGUI_IMPL_API
#include "../third_party/imgui_dx9/imgui_impl_dx9.h"
#include "../third_party/imgui_dx9/imgui_impl_win32.h"



// unscaled
// #define DEFAULT_WINDOW_WIDTH 1280 
// #define DEFAULT_WINDOW_HEIGHT 800
#define DEFAULT_WINDOW_WIDTH	800 
#define DEFAULT_WINDOW_HEIGHT	600

#define MIN_WINDOW_WIDTH		800
#define MIN_WINDOW_HEIGHT		600


static bool 	quit;


#include <Windows.h>

#include <d3d9.h>

// #define DIRECTINPUT_VERSION 0x0800
// #include <dinput.h>
#include <tchar.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define ID_TRAY_APP_ICON		1001
#define WM_SYSICON				(WM_USER + 1)

static LPDIRECT3D9 g_pD3D = NULL;
static LPDIRECT3DDEVICE9 g_pd3dDevice = NULL;
static D3DPRESENT_PARAMETERS g_d3dpp =
{
};


static HWND 	hwnd;
static WNDCLASSEX wc;

static NOTIFYICONDATAA * tbe;

// bool show_demo_window = true;
// bool show_another_window = false;
static ImVec4	clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);


// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void ResetDevice();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


// Helper functions
bool CreateDeviceD3D(HWND hWnd)
{
	if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
		return false;

	// Create the D3DDevice
	ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
	g_d3dpp.Windowed	= TRUE;
	g_d3dpp.SwapEffect	= D3DSWAPEFFECT_DISCARD;
	g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
	g_d3dpp.EnableAutoDepthStencil = TRUE;
	g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
	g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE; // Present with vsync

	//g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;   // Present without vsync, maximum unthrottled framerate
	if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) <
		 0)
		return false;

	return true;
}


void CleanupDeviceD3D()
{
	if (g_pd3dDevice)
	{
		g_pd3dDevice->Release();
		g_pd3dDevice		= NULL;
	}

	if (g_pD3D)
	{
		g_pD3D->Release();
		g_pD3D				= NULL;
	}
}


void ResetDevice()
{
	ImGui_ImplDX9_InvalidateDeviceObjects();
	HRESULT 		hr	= g_pd3dDevice->Reset(&g_d3dpp);

	if (hr == D3DERR_INVALIDCALL)
		IM_ASSERT(0);

	ImGui_ImplDX9_CreateDeviceObjects();
}



static void _codeduplicationftw_util_wide_to_utf8(wchar_t * wide, size_t wide_len, char * utf8, size_t utf8_len)
{
	int 			utf8_result_len = WideCharToMultiByte(CP_UTF8, 
		WC_NO_BEST_FIT_CHARS, 
		wide, 
		wide_len, 
		utf8, 
		utf8_len -	1, 
		NULL, 
		NULL);

	utf8[utf8_result_len] = '\0';
}

typedef int (*KeyPaserFunc)(PRAWINPUT, LONG&, LONG&, LONG&, LONG&, LONG&, int&, bool*);

//typedef int(*KeyPaserFunc) (PRAWINPUT);
KeyPaserFunc	key_parser = NULL;


int init_ext_lib()
{


	HINSTANCE		hInstLibrary = LoadLibrary("win_ext_for_tc.dll");

	if (!hInstLibrary)
	{
		printf("win_ext_for_tc.dll not found\n");
	}
	
	key_parser			= (KeyPaserFunc)
	GetProcAddress(hInstLibrary, "parse_game_controller_key");

	if (!key_parser)
	{
		printf("Failed to load game controler key paser lib!!!!\n");
		return - 1;
	}


	//FreeLibrary(hInstLibrary);
	

	return 0;
}





// #include <optional>
#include "gui_impl_cpp.hpp"
#include "../../third_party/mingw-std-threads/mingw.thread.h"
#include "../../third_party/mingw-std-threads/mingw.mutex.h"
#include "../../third_party/mingw-std-threads/mingw.condition_variable.h"

static std::unordered_set <key_t> keys_down;
static std::unordered_set <key_t> keys_pressed;

static bool 	is_key_set_mode = false;
static bool 	key_set_has_value = false;
static key_t	key_set;

static std::mutex keys_lock;

static std::condition_variable keys_cv;

std::condition_variable * gui_impl_get_keys_cv()
{
	return & keys_cv;
}


std::mutex * gui_impl_get_keys_mutex()
{
	return & keys_lock;
}


void gui_impl_enter_get_key_mode()
{
	std::lock_guard <std::mutex> lock(keys_lock);

	// key_set.reset();
	key_set_has_value	= false;
	is_key_set_mode 	= true;

}


void gui_impl_leave_get_key_mode()
{
	std::lock_guard <std::mutex> lock(keys_lock);

	// key_set.reset();
	key_set_has_value	= false;
	is_key_set_mode 	= false;
}


bool gui_impl_is_get_key_mode()
{
	std::lock_guard <std::mutex> lock(keys_lock);
	return is_key_set_mode;
}


bool gui_impl_get_set_key(key_t * out)
{
	std::lock_guard <std::mutex> lock(keys_lock);

	if (!is_key_set_mode || !key_set_has_value)
	{
		return false;
	}

	memcpy(out, &key_set, sizeof(key_t));
	return true;

}






GUI_IMPL_API void gui_impl_get_keys_data(set_of_keys * out_keys_down, set_of_keys * out_keys_just_pressed)
{
	std::lock_guard <std::mutex> lock(keys_lock);

	*out_keys_down		= keys_down;
	*out_keys_just_pressed = keys_pressed;

	keys_pressed.clear();

	// keys_down.clear();
}


static void _handle_raw_input(LPARAM lParam)
{
	// std::lock_guard<std::mutex> lock(keys_lock);
	UINT			size;

	GetRawInputData((HRAWINPUT) lParam, RID_INPUT, NULL, &size, sizeof(RAWINPUTHEADER));
	uint8_t 		data[size];

	if (GetRawInputData((HRAWINPUT) lParam, RID_INPUT, data, &size, sizeof(RAWINPUTHEADER)) != size)
	{
		MessageBoxA(tbe->hWnd, "Raw input data read failed!", "Universal Time Control Error", MB_OK);

		// abort();
	}



	RAWINPUT *		raw = (RAWINPUT *)data;
	//printf("_handle_raw_input:key pressed:%d\n", raw->header.dwType);

	if (raw->header.dwType == RIM_TYPEHID)
	{
		if (key_parser == NULL)
		{
			printf("key parser for game controller is null!!!\n");
			return;
		}


		LONG lAxisX, lAxisY,lAxisZ,lAxisRz, lHat;
		int iNumberOfButtons = 0;
		bool bBtnStates[128];

		//typedef int (*KeyPaserFunc)(PRAWINPUT, LONG&, LONG&, LONG&, LONG&, LONG&, int&, bool*);

		int ret = key_parser(raw,lAxisX, lAxisY,lAxisZ,lAxisRz, lHat,iNumberOfButtons,bBtnStates);
		if (ret != 0)
		{
			printf("key_parser failed:%d\n",ret);
			return;
		}

		printf("lAxisX:%d lAxisY:%d lAxisZ:%d lAxisRz:%d lHat:%d iNumberOfButtons:%d\n",lAxisX, lAxisY,lAxisZ,lAxisRz, lHat,iNumberOfButtons);
		//printf("Pressed:");
		int pressed = 0;
		for (int i = 0; i < iNumberOfButtons; ++i)
		{
			//printf("%d ",bBtnStates[i]);
			pressed = i;
			
		}

		
		//printf("\n");


		/* output of my xbox game controller Y/X/B/A button
		lAxisX:33643 lAxisY:31246 lAxisZ:32640 lAxisRz:1836687558 lHat:0 iNumberOfButtons:10
		Pressed:0 0 0 1 0 0 0 0 0 0
		lAxisX:33643 lAxisY:31246 lAxisZ:32640 lAxisRz:1836687558 lHat:0 iNumberOfButtons:10
		Pressed:0 0 0 0 0 0 0 0 0 0
		
		
		lAxisX:33643 lAxisY:31246 lAxisZ:32640 lAxisRz:1836687558 lHat:0 iNumberOfButtons:10
		Pressed:0 0 1 0 0 0 0 0 0 0
		lAxisX:33643 lAxisY:31246 lAxisZ:32640 lAxisRz:1836687558 lHat:0 iNumberOfButtons:10
		Pressed:0 0 0 0 0 0 0 0 0 0
		
		
		lAxisX:33643 lAxisY:31246 lAxisZ:32640 lAxisRz:1836687558 lHat:0 iNumberOfButtons:10
		Pressed:0 1 0 0 0 0 0 0 0 0
		lAxisX:33643 lAxisY:31246 lAxisZ:32640 lAxisRz:1836687558 lHat:0 iNumberOfButtons:10
		Pressed:0 0 0 0 0 0 0 0 0 0
		*/

		

		
		return;
	}

	if (raw->header.dwType == RIM_TYPEKEYBOARD)
	{

		RAWKEYBOARD 	kb	= raw->data.keyboard;


		static unsigned char dummyKeyState[256] =
		{
			0
		};
		static unsigned char keyState[256] =
		{
			0
		};

		{ // update our keystate

			// if you don't care about getting the unicode translated thing later you might choose to store it differnetly.
			bool			e0	= kb.Flags & RI_KEY_E0;

			// bool e1 = kb.Flags & RI_KEY_E1;
			// these are unasigned but not reserved as of now.
			// this is bad but, you know, we'll fix it if it ever breaks.
#define 			VK_LRETURN				0x9E
#define 			VK_RRETURN				0x9F

#define 			UPDATE_KEY_STATE(key)	do{keyState[key] = (kb.Flags & 1) ? 0 : 0xff;}while(0)

			// note: we set all bits in the byte if the key is down. 
			// This is becasue windows expects it to be in the high_order_bit (when using it for converting to unicode for example)
			// and I like it to be in the low_order_bit,  
			if (kb.VKey == VK_CONTROL)
			{
				if (e0)
					UPDATE_KEY_STATE(VK_RCONTROL);
				else 
					UPDATE_KEY_STATE(VK_LCONTROL);

				keyState[VK_CONTROL] = keyState[VK_RCONTROL] | keyState[VK_LCONTROL];
			}
			else if (kb.VKey == VK_SHIFT)
			{
				// because why should any api be consistant lol
				// (because we get different scancodes for l/r-shift but not for l/r ctrl etc... but still)
				UPDATE_KEY_STATE(MapVirtualKey(kb.MakeCode, MAPVK_VSC_TO_VK_EX));
				keyState[VK_SHIFT]	= keyState[VK_LSHIFT] | keyState[VK_RSHIFT];

				// do{dummyKeyState[key] = (kb.Flags & 1) ? 0 : 0xff;}while(0)
				// dummyKeyState[VK_SHIFT] = dummyKeyState[VK_LSHIFT] | dummyKeyState[VK_RSHIFT];
			}
			else if (kb.VKey == VK_MENU)
			{
				if (e0)
					UPDATE_KEY_STATE(VK_LMENU);
				else 
					UPDATE_KEY_STATE(VK_RMENU);

				keyState[VK_MENU]	= keyState[VK_RMENU] | keyState[VK_LMENU];
			}
			else if (kb.VKey == VK_RETURN)
			{
				if (e0)
					UPDATE_KEY_STATE(VK_RRETURN);
				else 
					UPDATE_KEY_STATE(VK_LRETURN);

				keyState[VK_RETURN] = keyState[VK_RRETURN] | keyState[VK_LRETURN];
			}
			else 
			{
				UPDATE_KEY_STATE(kb.VKey);
			}

#undef UPDATE_KEY_STATE
		}


		// char buffer[4096];
		// sprintf(buffer, "Kbd: make=%04x Flags:%04x Reserved:%04x ExtraInformation:%08x, msg=%04x VK=%04x \n",
		//		 (unsigned int)raw->data.keyboard.MakeCode, 
		//		 (unsigned int)raw->data.keyboard.Flags, 
		//		 (unsigned int)raw->data.keyboard.Reserved, 
		//		 (unsigned int)raw->data.keyboard.ExtraInformation, 
		//		 (unsigned int)raw->data.keyboard.Message, 
		//		 (unsigned int)raw->data.keyboard.VKey);
		// printf("%s\n", buffer);
		bool			is_down = ! (kb.Flags & 0x1);


		//if(1)
		{
			// char *buff=0;
			// int utf8_len = 0;
			// get unicode.
			wchar_t 		utf16_buffer[32];

			//simulating altgr, assumes all leftalts is algr
			// which seem to work since ctrl is ignored on US versions of ToUnicode. Bad way of doing it, Not sure how to detect if left alt is altgr though.
			unsigned char	ctrl = keyState[VK_CONTROL];

			keyState[VK_CONTROL] |= keyState[VK_RMENU];

			const wchar_t * desc = NULL;

			switch (kb.VKey)
			{
#include "gui_impl_windx9_vkeynames.inc.cpp"
			}

			if (desc)
			{
				wcscpy(utf16_buffer, desc);
			}
			else 
			{
				int 			bytes_written = ToUnicode(kb.VKey, kb.MakeCode, dummyKeyState, utf16_buffer,
					 sizeof(utf16_buffer) / sizeof(utf16_buffer[0]), 0);

				if (bytes_written < 1)
				{
					wcscpy(utf16_buffer, L"<??" L"?>"); // avoid trigraph
				}
			}


			keyState[VK_CONTROL] = ctrl;

			bool			shift = keyState[VK_LSHIFT] || keyState[VK_RSHIFT];
			bool			any_ctrl = keyState[VK_LCONTROL] || keyState[VK_RCONTROL];
			bool			alt = keyState[VK_LMENU] || keyState[VK_RMENU];

			// not interested if only modifiers...
			if ((kb.VKey == VK_CONTROL) || (kb.VKey == VK_SHIFT) || (kb.VKey == VK_MENU))
			{
				// except that on release we do want to clear any other keys that are marked as down.
				if (!is_down)
				{
					for (auto it = keys_down.begin(); it != keys_down.end(); )
					{

						bool			should_delete = ((kb.VKey == VK_CONTROL) && (it->ctrl)) ||
							 ((kb.VKey == VK_SHIFT) && (it->shift)) || ((kb.VKey == VK_MENU) && (it->alt));

						if (should_delete)
						{
							keys_down.erase(it++);
						}
						else 
						{
							++it;
						}
					}
				}

				return;
			}
			else 
			{
				// todo this makes some of the handling below obsolete...
				if (!is_down)
				{
					for (auto it = keys_down.begin(); it != keys_down.end(); )
					{

						bool			should_delete = (it->v_code == kb.VKey) && (it->scan_code == kb.MakeCode);

						if (should_delete)
						{
							keys_down.erase(it++);
						}
						else 
						{
							++it;
						}
					}
				}
			}

			wchar_t 		key_fmt_buffer[64];

			key_fmt_buffer[0]	= 0;

			if (any_ctrl)
				wcscat(key_fmt_buffer, L"CTRL+");

			if (alt)
				wcscat(key_fmt_buffer, L"ALT+");

			if (shift)
				wcscat(key_fmt_buffer, L"SHIFT+");

			wcscat(key_fmt_buffer, utf16_buffer);
			CharUpperW(key_fmt_buffer);

			// printf("%ls\n", key_fmt_buffer);
			char			utf8_buffer[128];

			_codeduplicationftw_util_wide_to_utf8(key_fmt_buffer, wcslen(key_fmt_buffer), utf8_buffer,
				 sizeof(utf8_buffer));

			// printf("%s: %s\n", (is_down ? "D" : "U"), utf8_buffer);
			// typedef struct {
			//	uint32_t v_code    PACKED;
			//	uint32_t scan_code PACKED;
			//	str31	 desc_utf8 PACKED;
			//	bool	 ctrl	   PACKED;
			//	bool	 alt	   PACKED;
			//	bool	 shift	   PACKED;
			// } PACKED key_t;
			key_t			key;

			memset(&key, 0, sizeof(key));
			key.v_code			= kb.VKey;
			key.scan_code		= kb.MakeCode;
			strncpy(key.desc_utf8, utf8_buffer, 31);
			key.ctrl			= any_ctrl;
			key.alt 			= alt;
			key.shift			= shift;

			{
				std::lock_guard <std::mutex> lock(keys_lock);

				auto			does_set_contain_key =[] (const set_of_keys & set, const key_t & key)->bool
				{
					return set.find(key) != set.end();
				};

				if (is_key_set_mode)
				{
					if (!key_set_has_value)
					{
						key_set_has_value	= true;
						memcpy(&key_set, &key, sizeof(key_t));

						// is_key_set_mode = false;
					}


				}
				else 
				{
					if (is_down && !does_set_contain_key(keys_down, key))
					{
						keys_pressed.insert(key);
					}
					else 
					{
						// keys_pressed.erase(key);
					}


					if (is_down)
					{
						// printf("inserting: %s\n", buffer);
						keys_down.insert(key);
					}
					else 
					{
						keys_down.erase(key);
					}
				}
			}



			keys_cv.notify_all();

			// next:;
		}


	}
	/*else 
	{
		// printf("type: %u\n", raw->header.dwType);
		// printf("unknown\n");
	}*/



}


// Win32 message handler
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
		case WM_GETMINMAXINFO:
			{
				LPMINMAXINFO	lpMMI = (LPMINMAXINFO)

				lParam;
				lpMMI->ptMinTrackSize.x = MIN_WINDOW_WIDTH * gui_impl_get_scale();
				lpMMI->ptMinTrackSize.y = MIN_WINDOW_HEIGHT * gui_impl_get_scale();
				return 0;
			}

		case WM_SIZE:
			if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
			{
				g_d3dpp.BackBufferWidth = LOWORD(lParam);
				g_d3dpp.BackBufferHeight = HIWORD(lParam);
				ResetDevice();
			}

			return 0;

		case WM_SYSCOMMAND:
			if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
				return 0;

			if (((wParam & 0xfff0) == SC_MINIMIZE))
			{
				// if (((wParam & 0xfff0) == SC_MINIMIZE) || ((wParam & 0xfff0) == SC_CLOSE)) {
				ShowWindow(hwnd, SW_HIDE);
				Shell_NotifyIconA(NIM_ADD, tbe);
				return 0;
			}

			break;

		case WM_DESTROY:
			printf("destroy\n");
			::PostQuitMessage(0);
			return 0;

		case WM_SYSICON:
			if (lParam == WM_LBUTTONUP || lParam == WM_RBUTTONUP)
			{
				ShowWindow(hwnd, SW_SHOW);
				Shell_NotifyIconA(NIM_DELETE, tbe);
				return 0;
			}

			break;

		case WM_INPUT:
			_handle_raw_input(lParam);
			return 0;
	}

	return::DefWindowProc(hWnd, msg, wParam, lParam);
}




bool gui_impl_init(const char * title)
{

	// SetProcessDPIAware();
	// Create application window
	wc					=
	{
		sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, title, NULL
	};

	HINSTANCE		hinst = GetModuleHandle(NULL);
	HICON			icon = LoadIconA(hinst, "MAINICON");

	if (icon)
	{
		wc.hIcon			= icon;
	}

	::RegisterClassEx(&wc);
	UINT			flags = WS_OVERLAPPEDWINDOW;	// WS_OVERLAPPEDWINDOW

	hwnd				=::CreateWindowA(wc.lpszClassName, title, flags, CW_USEDEFAULT, CW_USEDEFAULT, 
		DEFAULT_WINDOW_WIDTH * gui_impl_get_scale(), 
		DEFAULT_WINDOW_HEIGHT * gui_impl_get_scale(), 
		NULL, NULL, wc.hInstance, NULL);

	// SetWindowLong(hwnd, GWL_STYLE, WS_SIZEBOX);
	// Initialize Direct3D
	if (!CreateDeviceD3D(hwnd))
	{
		CleanupDeviceD3D();
		::UnregisterClass(wc.lpszClassName, wc.hInstance);
		return 1;
	}

	// Show the window
	::ShowWindow(hwnd, SW_SHOWDEFAULT);
	::UpdateWindow(hwnd);

	tbe 				= (NOTIFYICONDATAA *)
	malloc(sizeof(NOTIFYICONDATAA));
	ZeroMemory(tbe, sizeof(NOTIFYICONDATAA));
	tbe->cbSize 		= sizeof(NOTIFYICONDATAA);
	tbe->hWnd			= hwnd;
	tbe->uID			= ID_TRAY_APP_ICON;
	tbe->uFlags 		= /*NIF_INFO |*/ NIF_MESSAGE | NIF_ICON | NIF_TIP;
	tbe->uCallbackMessage = WM_SYSICON;

	if (icon)
	{
		tbe->hIcon			= icon;
	}

	strcpy_s(tbe->szTip, 128, "Universal Time Control");

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX9_Init(g_pd3dDevice);


#if 0
	RAWINPUTDEVICE	keyboard_device;

	keyboard_device.usUsagePage = 0x01;
	keyboard_device.usUsage = 0x06;
	keyboard_device.dwFlags = RIDEV_INPUTSINK;		//RIDEV_NOLEGACY;
	keyboard_device.hwndTarget = hwnd;

	if (RegisterRawInputDevices(&keyboard_device, 1, sizeof(keyboard_device)) == FALSE)
	{
		MessageBoxA(tbe->hWnd, "raw input reg failed!", "Error", MB_OK);
		abort();
	}

#else

	RAWINPUTDEVICE	rid[2];

	rid[0].usUsagePage	= 0x01;
	rid[0].usUsage		= 0x05;
	rid[0].dwFlags		= RIDEV_INPUTSINK;
	rid[0].hwndTarget	= hwnd;

	rid[1].usUsagePage	= 0x01;
	rid[1].usUsage		= 0x04;
	rid[1].dwFlags		= RIDEV_INPUTSINK;
	rid[1].hwndTarget	= hwnd;

	if (!RegisterRawInputDevices(rid, 2, sizeof(rid[0])))
	{
		MessageBoxA(tbe->hWnd, "raw input reg failed!", "Error", MB_OK);
		abort();
	}

	init_ext_lib();


#endif

	quit				= false;
	return true;
}


void gui_impl_cleanup()
{

	quit				= true;

	//printf("impl_exit1\n");
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();


	//printf("impl_exit2\n");
	// Sleep(500);
	CleanupDeviceD3D();

	// Sleep(500);
	//printf("impl_exit2\n");
	// Sleep(500);
	::DestroyWindow(hwnd);

	// Sleep(500);
	//printf("impl_exit3\n");
	::UnregisterClass(wc.lpszClassName, wc.hInstance);

	//printf("impl_exit4\n");
}


bool gui_impl_begin_frame_and_return_should_quit()
{
	if (quit)
		return true;

	MSG 			msg;

	ZeroMemory(&msg, sizeof(msg));

	while (1)
	{
		if (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				quit				= true;
				return true;
			}

			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			continue;
		}
		else 
		{
			break;
		}
	}

	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();


	return false;
}


void gui_impl_end_frame()
{

	ImGui::EndFrame();
	g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, false);
	g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
	g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, false);
	D3DCOLOR		clear_col_dx = D3DCOLOR_RGBA((int) (clear_color.x * 255.0f), (int) (clear_color.y * 255.0f),
		 (int) (clear_color.z * 255.0f), (int) (clear_color.w * 255.0f));

	g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0);

	if (g_pd3dDevice->BeginScene() >= 0)
	{
		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
		g_pd3dDevice->EndScene();
	}

	HRESULT 		result = g_pd3dDevice->Present(NULL, NULL, NULL, NULL);

	// Handle loss of D3D9 device
	if (result == D3DERR_DEVICELOST && g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
		ResetDevice();


	std::lock_guard <std::mutex> lock(keys_lock);

	if (keys_down.size() > 0)
	{
		// printf("Down: ");
		// for (const auto& key_down : keys_down) {
		//	printf("%s, ", key_down.desc_utf8);
		// }
		// printf("\n");
	}


	// keys_down.clear();
}


float gui_impl_get_scale()
{
	HDC 			screen = GetDC(0);
	int 			dpiX = GetDeviceCaps(screen, LOGPIXELSX);

	float			dpiscale = ((float) dpiX) / 96.0f;

	return dpiscale * 1.0f;
}


void gui_impl_set_clear_colour(int r, int g, int b)
{
	// rem
	clear_color.x		= ((float) r) / 255.0f;
	clear_color.y		= ((float) g) / 255.0f;
	clear_color.z		= ((float) b) / 255.0f;

}












