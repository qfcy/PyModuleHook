// 注入的主DLL的实现
#include <windows.h>
#include <stdio.h>

typedef void* (*PyImportModuleFunc)(const char*);
typedef void (*emptyFunc)();
#define PYMODULE_NAME "__hook__"

DWORD WINAPI hookImpl(LPVOID lpParam) {
    char dllName[16];
    HMODULE hPyDll = nullptr;

    for (int i = 1; i <= 32; i++) { // 逐步从python31.dll查找到python332.dll
        snprintf(dllName, sizeof(dllName), "python3%d.dll", i);

        hPyDll = GetModuleHandleA(dllName);
        if (hPyDll) {
            PyImportModuleFunc fnImportModule =
                (PyImportModuleFunc)GetProcAddress(hPyDll, "PyImport_ImportModule");
            emptyFunc fnGILEnsure = (emptyFunc)GetProcAddress(hPyDll, "PyGILState_Ensure");
            emptyFunc fnGILRelease = (emptyFunc)GetProcAddress(hPyDll, "PyGILState_Release");
            if (!fnImportModule || !fnGILEnsure || !fnGILRelease) {
                MessageBoxA(nullptr, "Cannot retrieve functions from DLL", "Info from DLL", MB_ICONEXCLAMATION);
                return 1;
            } else {
                fnGILEnsure();
                void *pMod = fnImportModule(PYMODULE_NAME);
                fnGILRelease();
                if (pMod) {
                    MessageBoxA(nullptr, "successfully imported " PYMODULE_NAME " module :)", "Info from DLL", MB_OK);
                    return 0;
                } else {
                    MessageBoxA(nullptr, "Failed to import " PYMODULE_NAME " module", "Info from DLL", MB_ICONEXCLAMATION);
                    return 1;
                }
            }
        }
    }
    MessageBoxA(nullptr, "Cannot find loaded python3x.dll, the process is not a Python process", "Info from DLL", MB_ICONEXCLAMATION);
    return 1;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    switch(fdwReason){
        case DLL_PROCESS_ATTACH: {
            HANDLE hThread = CreateThread(
                nullptr,  // 安全属性
                0,        // 堆栈大小
                hookImpl,
                nullptr,  // 传递参数
                0,        // 创建标志
                nullptr   // 返回的线程ID
            );
            if(!hThread){
                MessageBoxA(nullptr, "Error starting hook thread", "Info from DLL", MB_ICONEXCLAMATION);
            }
        }
        default:
            break;
    }
    return TRUE;
}