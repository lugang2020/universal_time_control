#include "cnc_window.h"

#include <stdio.h>
#include <stdbool.h>
#include <Windows.h>

#include "tls.h"

static bool GLOBAL_init_done = false;

BOOL WINAPI DllMain(
	HINSTANCE hinstDLL,
	DWORD     fdwReason,
	LPVOID    lpvReserved
) {
	(void) hinstDLL;
	(void) lpvReserved;
	if (fdwReason == DLL_PROCESS_ATTACH) {
		tls_init();

		if (!GLOBAL_init_done) {
			GLOBAL_init_done = true;

			setbuf(stdout, NULL);
			setbuf(stderr, NULL);
			printf("attach\n");
			setup_cnc_window(hinstDLL);
		}

	} else if (fdwReason == DLL_PROCESS_DETACH) {
		tls_destroy();
	} else if (fdwReason == DLL_THREAD_ATTACH) {
		tls_attach_thread();
	} else if (fdwReason == DLL_THREAD_DETACH) {
		tls_detach_thread();
	}

	return TRUE;
}

void universal_time_control() {

}