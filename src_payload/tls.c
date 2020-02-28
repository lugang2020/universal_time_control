#include "tls.h"
#include <Windows.h>

static DWORD tls_index;

void tls_init() {
	tls_index = TlsAlloc();
	tls_attach_thread();
}

void tls_destroy() {
	tls_detach_thread();
	TlsFree(tls_index);
}

void tls_attach_thread() {
	void* data = (void*) LocalAlloc(LPTR, sizeof(tls_values_t));
	memset(data, 0, sizeof(tls_values_t));
	TlsSetValue(tls_index, data);
}

void tls_detach_thread() {
	void* data = TlsGetValue(tls_index);
	if (data) {
		LocalFree(data);
	}
}

tls_values_t* tls_get() {
	void* data = TlsGetValue(tls_index);
	if (!data) {
		tls_attach_thread();
		data = TlsGetValue(tls_index);
	}
	return data;
}
