#pragma once

// #define _WIN32_WINNT 0x0600
#define _WIN32_WINNT _WIN32_WINNT_WIN7


#include <Windows.h>
#include <WtsApi32.h>
#include <TlHelp32.h>


#ifndef PROCESS_QUERY_LIMITED_INFORMATION
#define PROCESS_QUERY_LIMITED_INFORMATION (0x1000)
#endif

#ifdef __TINYC__

WINBASEAPI BOOL WINAPI QueryFullProcessImageNameW(
  HANDLE hProcess,
  DWORD dwFlags,
  LPWSTR lpExeName,
  PDWORD lpdwSize
);

#endif