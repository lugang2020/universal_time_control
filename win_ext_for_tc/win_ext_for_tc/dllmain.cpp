// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

#include <Windows.h>
#include <tchar.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <hidsdi.h>


#define ARRAY_SIZE(x)	(sizeof(x) / sizeof((x)[0]))
#define WC_MAINFRAME	TEXT("MainFrame")
#define MAX_BUTTONS		128
#define CHECK(exp)		{ if(!(exp)){ ret = -1;goto Error;} }
#define SAFE_FREE(p)	{ if(p) { HeapFree(hHeap, 0, p); (p) = NULL; } }

//INT  g_NumberOfButtons = 0;
//BOOL bButtonStates[MAX_BUTTONS];

#include <stdio.h>

extern "C" __declspec(dllexport) int parse_game_controller_key(PRAWINPUT pRawInput, LONG & lAxisX, LONG & lAxisY, LONG & lAxisZ, LONG & lAxisRz, LONG & lHat, int& iNumberOfButtons, bool* pbBtnStates);
int parse_game_controller_key(PRAWINPUT pRawInput,	LONG &lAxisX, LONG &lAxisY, LONG &lAxisZ, LONG &lAxisRz, LONG &lHat,int &iNumberOfButtons, bool* pbBtnStates)
{
	PHIDP_PREPARSED_DATA pPreparsedData;
	HIDP_CAPS            Caps;
	PHIDP_BUTTON_CAPS    pButtonCaps;
	PHIDP_VALUE_CAPS     pValueCaps;
	USHORT               capsLength;
	UINT                 bufferSize;
	HANDLE               hHeap;
	USAGE                usage[MAX_BUTTONS];
	ULONG                i, usageLength, value;

	pPreparsedData = NULL;
	pButtonCaps = NULL;
	pValueCaps = NULL;
	hHeap = GetProcessHeap();

	int ret = 0;

	//
	// Get the preparsed data block
	//

	CHECK(GetRawInputDeviceInfo(pRawInput->header.hDevice, RIDI_PREPARSEDDATA, NULL, &bufferSize) == 0);
CHECK(pPreparsedData = (PHIDP_PREPARSED_DATA)HeapAlloc(hHeap, 0, bufferSize));
CHECK((int)GetRawInputDeviceInfo(pRawInput->header.hDevice, RIDI_PREPARSEDDATA, pPreparsedData, &bufferSize) >= 0);

//
// Get the joystick's capabilities
//

// Button caps
CHECK(HidP_GetCaps(pPreparsedData, &Caps) == HIDP_STATUS_SUCCESS)
CHECK(pButtonCaps = (PHIDP_BUTTON_CAPS)HeapAlloc(hHeap, 0, sizeof(HIDP_BUTTON_CAPS) * Caps.NumberInputButtonCaps));

capsLength = Caps.NumberInputButtonCaps;
CHECK(HidP_GetButtonCaps(HidP_Input, pButtonCaps, &capsLength, pPreparsedData) == HIDP_STATUS_SUCCESS)
iNumberOfButtons = pButtonCaps->Range.UsageMax - pButtonCaps->Range.UsageMin + 1;

// Value caps
CHECK(pValueCaps = (PHIDP_VALUE_CAPS)HeapAlloc(hHeap, 0, sizeof(HIDP_VALUE_CAPS) * Caps.NumberInputValueCaps));
capsLength = Caps.NumberInputValueCaps;
CHECK(HidP_GetValueCaps(HidP_Input, pValueCaps, &capsLength, pPreparsedData) == HIDP_STATUS_SUCCESS)

//
// Get the pressed buttons
//

usageLength = iNumberOfButtons;
CHECK(
	HidP_GetUsages(
		HidP_Input, pButtonCaps->UsagePage, 0, usage, &usageLength, pPreparsedData,
		(PCHAR)pRawInput->data.hid.bRawData, pRawInput->data.hid.dwSizeHid
	) == HIDP_STATUS_SUCCESS);

ZeroMemory(pbBtnStates, sizeof(bool) * 128);
for (i = 0; i < usageLength; i++)
	pbBtnStates[usage[i] - pButtonCaps->Range.UsageMin] = TRUE;

//
// Get the state of discrete-valued-controls
//

for (i = 0; i < Caps.NumberInputValueCaps; i++)
{
	CHECK(
		HidP_GetUsageValue(
			HidP_Input, pValueCaps[i].UsagePage, 0, pValueCaps[i].Range.UsageMin, &value, pPreparsedData,
			(PCHAR)pRawInput->data.hid.bRawData, pRawInput->data.hid.dwSizeHid
		) == HIDP_STATUS_SUCCESS);

	switch (pValueCaps[i].Range.UsageMin)
	{
	case 0x30:	// X-axis
		lAxisX = (LONG)value - 128;
		break;

	case 0x31:	// Y-axis
		lAxisY = (LONG)value - 128;
		break;

	case 0x32: // Z-axis
		lAxisZ = (LONG)value - 128;
		break;

	case 0x35: // Rotate-Z
		lAxisRz = (LONG)value - 128;
		break;

	case 0x39:	// Hat Switch
		lHat = value;
		break;
	}
}

//
// Clean up
//

Error:
SAFE_FREE(pPreparsedData);
SAFE_FREE(pButtonCaps);
SAFE_FREE(pValueCaps);

return ret;
}


void play(char* path);

DWORD CALLBACK play_thread(void* p)
{
	char* path = (char*)p;
	play(path);

	return 0;
}

char mp3_path[255];
HANDLE hPlayThread = 0;

extern "C" __declspec(dllexport) void play_sound(int start);

void play_sound(int speedup)
{
#pragma warning (disable : 4996)
	if (hPlayThread != 0)
	{
		DWORD dwRet = WaitForSingleObject(hPlayThread, 0);
		if (dwRet == WAIT_ABANDONED )
		{
			return;
		}

	}

	char cur_dir[255];
	GetModuleFileNameA(NULL, cur_dir, MAX_PATH);
	char* last_slash = strrchr(cur_dir, '\\');
	*(++last_slash) = 0;

	
	if (speedup)
	{
		sprintf(mp3_path, "%s\\%s", cur_dir, "Entering.mp3");
	}
	else
	{
		sprintf(mp3_path, "%s\\%s", cur_dir, "Exiting.mp3");
	}

	hPlayThread = CreateThread(NULL, 0, play_thread, mp3_path, 0, NULL);

	

}