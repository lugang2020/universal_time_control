#include <Windows.h>

#include "../third_party/minhook/MinHook.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "cnc_window.h"

#include <stdatomic.h>


static float G_timescale = 1.0f;
CRITICAL_SECTION timescale_lock_cs;


int64_t QueryPerformanceCounter_value_at_start;

static void _init_baseline_readings() {
	QueryPerformanceCounter((LARGE_INTEGER*)&QueryPerformanceCounter_value_at_start);
}

void speedhack_set_timescale(float scale) {
	EnterCriticalSection(&timescale_lock_cs);
	if (G_timescale != scale) {
		G_timescale = scale;
	}
	LeaveCriticalSection(&timescale_lock_cs);
}

#define REAL_AND_FAKE(ret_type, attrs, funcname, ...) \
	static ret_type (attrs * Real_##funcname)(__VA_ARGS__); \
	ret_type attrs Fake_##funcname(__VA_ARGS__)

#include "tls.h"

REAL_AND_FAKE(BOOL, WINAPI, QueryPerformanceCounter, LARGE_INTEGER* lpPerformanceCount) {
	float ts;
	EnterCriticalSection(&timescale_lock_cs);
	ts = G_timescale;
	LeaveCriticalSection(&timescale_lock_cs);

	tls_values_t* tls = tls_get();

	int64_t qpc_current;
	Real_QueryPerformanceCounter((LARGE_INTEGER*)&qpc_current);

	if ((tls->qpc_prev == 0) || (tls->qpc_fake == 0)) {
		tls->qpc_prev = qpc_current;
		tls->qpc_fake = qpc_current;
		*((int64_t*)lpPerformanceCount) = qpc_current;
		return TRUE;
	}

	int64_t real_delta = qpc_current - tls->qpc_prev;
	int64_t scaled_delta = (int64_t)(((double) real_delta) * ((double) ts));

	if (scaled_delta <= 0) {
		// Silence this, it happens too often in most games
		// printf("Warning: Zero or negative delta! Enforcing positive\n");
		scaled_delta = 1;
	} else {
		// printf("delta ok\n");
	}

	tls->qpc_fake += scaled_delta;
	*((int64_t*)lpPerformanceCount) = tls->qpc_fake;

	tls->qpc_prev = qpc_current;
	return TRUE;
}



#define VPPF(thing) ((void**)(&(thing)))
#define API_HOOK(dll, func) MH_CreateHookApi(L##dll, #func, Fake_##func, VPPF(Real_##func));


// Query­Performance­Counter
// Query­Performance­Frequency
// Get­Tick­Count
// Get­Tick­Count64
// Get­Message­Time
// Get­System­Time
// Get­Local­Time
// Get­System­Time­As­File­Time
// Query­Interrupt­Time
// Query­Unbiased­Interrupt­Time
// Get­System­Time­Precise­As­File­Time
// Query­Interrupt­Time­Precise
// Query­Unbiased­Interrupt­Time­Precise


// todo volatile?
static bool init_done = false;
static bool unload_done = false;



HMODULE winmm_dll;

void speedhack_init() {
	if (init_done) return;
	init_done = true;

	MH_Initialize();

	// winmm_dll = LoadLibraryW(L"winmm.dll");
	// API_HOOK("winmm.dll", timeGetTime)

	_init_baseline_readings();
	InitializeCriticalSection(&timescale_lock_cs);


	API_HOOK("kernel32.dll", QueryPerformanceCounter);
	MH_STATUS res = MH_EnableHook(MH_ALL_HOOKS);

	if (res != MH_OK) {
		printf("err\n");
	}

	printf("hooked\n");
}

void speedhack_unload() {
	if (!init_done) return;
	if (unload_done) return;
	unload_done = true;

	MH_DisableHook(MH_ALL_HOOKS);
	MH_Uninitialize();
	DeleteCriticalSection(&timescale_lock_cs);
}
