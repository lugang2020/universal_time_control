#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct {
	int64_t qpc_prev;
	int64_t qpc_fake;
} tls_values_t;

void tls_init();
void tls_destroy();

void tls_attach_thread();
void tls_detach_thread();

tls_values_t* tls_get();
