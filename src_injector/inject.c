#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <Windows.h>
#include <shellapi.h>
#include <stdlib.h>
#include <stdint.h>

// static void fatal_error(const char* msg) {
// 	printf("%s\n", msg);
// 	exit(1);
// }

static void get_dll_path(wchar_t* out, size_t len, uint32_t* out_len_bytes) {
	GetModuleFileNameW(NULL, out, len);
	wchar_t* last_slash = wcsrchr(out, L'\\');
	last_slash++;
	wcscpy(last_slash,
		#if defined(_M_X64) || defined(__amd64__)
			L"payload_x64.dll"
		#else
			L"payload_x32.dll"
		#endif
	);
	printf("%ls\n", out);

	uint32_t path_to_dll_bytes = (lstrlenW(out) + 1) * sizeof(wchar_t);
	*out_len_bytes = path_to_dll_bytes;
}

static int inject(DWORD pid)
{
	wchar_t path_to_dll[MAX_PATH];
	uint32_t path_to_dll_bytes;
	get_dll_path(path_to_dll, MAX_PATH, &path_to_dll_bytes);
	
	HANDLE process = OpenProcess(
		PROCESS_QUERY_INFORMATION |
		PROCESS_CREATE_THREAD |
		PROCESS_VM_OPERATION |
		PROCESS_VM_WRITE,
		FALSE,
		pid
	);

	if (process == NULL)
	{
		printf("Could not open process for PID (%lu).\n", pid);
		return 1;
	}

	// Allocate space in the remote process for the pathname
	LPVOID pathname_remote_storage = (PWSTR)VirtualAllocEx(process, NULL, path_to_dll_bytes, MEM_COMMIT, PAGE_READWRITE);
	if (pathname_remote_storage == NULL)
	{
		printf("Could not allocate memory inside PID %lu.\n", pid);
		CloseHandle(process);
		return 1;
	}

	// Copy the DLL's pathname to the remote process address space
	DWORD n = WriteProcessMemory(process, pathname_remote_storage, (PVOID)path_to_dll, path_to_dll_bytes, NULL);
	if (n == 0)
	{
		printf("Could not write any bytes into PID %lu address space.\n", pid);
		VirtualFreeEx(process, pathname_remote_storage, 0, MEM_RELEASE);
		CloseHandle(process);
		return 1;
	}

	// Get the real address of LoadLibraryW in Kernel32.dll
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
	PTHREAD_START_ROUTINE loadlibrary_addr =
		(PTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleW(L"Kernel32"), "LoadLibraryW");
#pragma GCC diagnostic pop
	if (loadlibrary_addr == NULL)
	{
		printf("Could not find LoadLibraryA function inside kernel32.dll library.\n");
		VirtualFreeEx(process, pathname_remote_storage, 0, MEM_RELEASE);
		CloseHandle(process);
		return 1;
	}

	// Create a remote thread that calls LoadLibraryW(DLLPathname)
	HANDLE thread = CreateRemoteThread(process, NULL, 0, loadlibrary_addr, pathname_remote_storage, 0, NULL);
	if (thread == NULL)
	{
		printf("Could not create the Remote Thread.\n");
		VirtualFreeEx(process, pathname_remote_storage, 0, MEM_RELEASE);
		CloseHandle(process);
		return 1;
	}
	else {
		printf("Success: DLL injected via CreateRemoteThread().\n");
	}

	// TODO: Currently leaks
	// Free the remote memory that contained the DLL's pathname and close Handles
	// if (pathname_remote_storage != NULL)
	// 	VirtualFreeEx(process, pathname_remote_storage, 0, MEM_RELEASE);

	CloseHandle(thread);
	CloseHandle(process);

	return 0;
}

int main(int argc, char** argv) {
	if (argc != 2) {
		printf("Incorrect arguments.\n");
		return 1;
	}

	DWORD pid;
	sscanf_s(argv[1], "%lu", &pid);

	int res = inject(pid);
	printf("result: %i\n", res);

	return res;

}