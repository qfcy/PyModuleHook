#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include "dllinject.h"

const char *lastErrMsg = nullptr;

// 主注入函数
bool injectDLL(DWORD pid, const char *dllPath) {
    HANDLE hProcess, hThread;HMODULE hKernel32, hModule;
    FARPROC pLoadLibrary;LPVOID pRemoteBuf;

    lastErrMsg = nullptr;
    // 获取目标进程句柄
    hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProcess) {
        lastErrMsg = "Failed to open target process";
        goto ret;
    }

    // 为DLL路径分配远程内存
    pRemoteBuf = VirtualAllocEx(hProcess, nullptr, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!pRemoteBuf) {
        lastErrMsg = "Failed to allocate remote memory";
        goto close_proc;
    }
    // 转换为绝对路径
    char fullPath[MAX_PATH];
    if(!GetFullPathNameA(dllPath, MAX_PATH, fullPath, nullptr)){
        lastErrMsg = "Failed to get absolute path of DLL";
        goto free_remote_mem;
    }
    // 写入DLL路径
    if (!WriteProcessMemory(hProcess, pRemoteBuf, (LPCVOID)fullPath, MAX_PATH, nullptr)) {
        lastErrMsg = "Failed to write process memory";
        goto free_remote_mem;
    }

    // 获取LoadLibraryA地址
    hKernel32 = GetModuleHandleA("Kernel32");
    pLoadLibrary = GetProcAddress(hKernel32, "LoadLibraryA");
    // 创建远程线程，加载DLL
    hThread = CreateRemoteThread(hProcess, nullptr, 0, (LPTHREAD_START_ROUTINE)pLoadLibrary, pRemoteBuf, 0, nullptr);
    if (!hThread) {
        lastErrMsg = "Failed to create remote thread for LoadLibraryA";
        goto free_remote_mem;
    }

    WaitForSingleObject(hThread, INFINITE);
    if (!GetExitCodeThread(hThread, (LPDWORD)&hModule) || !hModule) {
        lastErrMsg = "Failed to get remote module handle, the module may be not loaded";
        goto close_thread;
    }

close_thread:
    CloseHandle(hThread);
free_remote_mem:
    VirtualFreeEx(hProcess, pRemoteBuf, 0, MEM_RELEASE);
close_proc:
    CloseHandle(hProcess);
ret:
    if (lastErrMsg) return false;
    return true;
}
