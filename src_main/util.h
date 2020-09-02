#pragma once

#include "misc.h"

SEPPLES_GUARD_START

#include <string.h>
#include <stdbool.h>

void util_sleep_for_ms(unsigned ms);

void util_wide_to_utf8(wchar_t* wide, size_t wide_len, char* utf8, size_t utf8_len);

// bool util_compare_utf8_ci(char* a, char* b);

void util_casefold_utf8(const char* in, char* out, size_t out_len);

#define  LOGI(...)  do {printf("%s:%d:%s  ",__FILE__,__LINE__,__FUNCTION__);printf(__VA_ARGS__);printf("\n");}while(0)

SEPPLES_GUARD_END

//0=nothing, 1=active, 2=unavailable
#define CTRL_STATE_NOTHING        0
#define CTRL_STATE_ACTIVE         1
#define CTRL_STATE_UNAVAILABLE    2

#define CTRL_ACTIVATION_MODE_PRESS      0
#define CTRL_ACTIVATION_MODE_HOLD       1
#define CTRL_ACTIVATION_MODE_TOGGLE     2

//0 = no limit, 1 = cooldown, 2 = energy
#define CTRL_LIMITED_MODE_NO_LIMIT      0
#define CTRL_LIMITED_MODE_COOLDOWN      1
#define CTRL_LIMITED_MODE_ENERGY        2
