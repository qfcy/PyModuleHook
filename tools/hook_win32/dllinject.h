#pragma once
#include <windows.h>
#include <tlhelp32.h>

extern const char *lastErrMsg;
bool injectDLL(DWORD pid, const char* dllPath);
