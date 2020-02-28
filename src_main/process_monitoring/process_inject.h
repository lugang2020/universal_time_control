#pragma once

#include "../windows_include.h"
#include <stdbool.h>
#include "../misc.h"

SEPPLES_GUARD_START

bool inject_process(DWORD pid, HANDLE process, HWND* out_cnc_hwnd);

SEPPLES_GUARD_END