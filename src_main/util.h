#pragma once

#include "misc.h"

SEPPLES_GUARD_START

#include <string.h>
#include <stdbool.h>

void util_sleep_for_ms(unsigned ms);

void util_wide_to_utf8(wchar_t* wide, size_t wide_len, char* utf8, size_t utf8_len);

// bool util_compare_utf8_ci(char* a, char* b);

void util_casefold_utf8(const char* in, char* out, size_t out_len);

SEPPLES_GUARD_END