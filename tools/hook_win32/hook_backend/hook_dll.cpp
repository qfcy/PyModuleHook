// 注入的主DLL的实现
#include <windows.h>
#include <stdio.h>

typedef void* (*PyImportModuleFunc)(const char*);
typedef void* (*PyRun_StringFunc)(const char*,int,void*,void*);
typedef void* (*PyUnicode_AsUTF8StringFunc)(void*);
typedef const char* (*PyBytes_AsStringFunc)(void*);
typedef void* (*PyDict_NewFunc)();
typedef int (*PyDict_SetItemStringFunc)(void*,const char*,void*);
typedef void (*voidFunc)();
#define PYMODULE_NAME "__hook__"
#define Py_eval_input 258 // CPython 3.7, 3.13, PyPy

DWORD WINAPI hookImpl(LPVOID lpParam) {
    char dllName[16];
    HMODULE hPyDll = nullptr;

    for (int i = 1; i <= 32; i++) { // 逐步从python31.dll查找到python332.dll
        snprintf(dllName, sizeof(dllName), "python3%d.dll", i);

        hPyDll = GetModuleHandleA(dllName);
        if (hPyDll) {
            PyImportModuleFunc PyImport_ImportModule =
                (PyImportModuleFunc)GetProcAddress(hPyDll, "PyImport_ImportModule");
            PyRun_StringFunc PyRun_String = 
                (PyRun_StringFunc)GetProcAddress(hPyDll, "PyRun_String");
            PyUnicode_AsUTF8StringFunc PyUnicode_AsUTF8String = 
                (PyUnicode_AsUTF8StringFunc)GetProcAddress(hPyDll, "PyUnicode_AsUTF8String");
            PyBytes_AsStringFunc PyBytes_AsString = 
                (PyBytes_AsStringFunc)GetProcAddress(hPyDll, "PyBytes_AsString");
            PyDict_NewFunc PyDict_New = 
                (PyDict_NewFunc)GetProcAddress(hPyDll, "PyDict_New");
            PyDict_SetItemStringFunc PyDict_SetItemString = 
                (PyDict_SetItemStringFunc)GetProcAddress(hPyDll, "PyDict_SetItemString");
            voidFunc PyGILState_Ensure = (voidFunc)GetProcAddress(hPyDll, "PyGILState_Ensure");
            voidFunc PyGILState_Release = (voidFunc)GetProcAddress(hPyDll, "PyGILState_Release");
            voidFunc PyErr_Print = (voidFunc)GetProcAddress(hPyDll, "PyErr_Print");
            if (!PyImport_ImportModule || !PyGILState_Ensure || !PyGILState_Release ||
                !PyUnicode_AsUTF8String || !PyBytes_AsString || !PyDict_New ||
                !PyDict_SetItemString || !PyRun_String || !PyErr_Print) {
                MessageBoxA(nullptr, "Cannot retrieve essential functions from DLL", "Info from DLL", MB_ICONEXCLAMATION);
                return 1;
            }
            PyGILState_Ensure();
            void *pMod = PyImport_ImportModule(PYMODULE_NAME);
            PyGILState_Release();
            if (pMod) {
                MessageBoxA(nullptr, "successfully imported " PYMODULE_NAME " module :)", "Info from DLL", MB_OK);
                return 0;
            } else {
                PyGILState_Ensure();
                void *scope = PyDict_New();
                PyDict_SetItemString(scope,"__builtins__",PyImport_ImportModule("builtins"));
                void *result = PyRun_String(
                    "'[%s]' % ',\\n'.join(repr(path) for path in __import__('sys').path)",
                    Py_eval_input,scope,PyDict_New());
                const char *path_info;
                if(result!=nullptr){
                    void *bytes = PyUnicode_AsUTF8String(result);
                    path_info = bytes!=nullptr?PyBytes_AsString(bytes):"<Failed to get: encode error>";
                } else {
                    PyErr_Print();
                    path_info = "<Failed to get: eval error>";
                }
                PyGILState_Release();
                char msg[4096];
                snprintf(msg,sizeof(msg),"Failed to import " PYMODULE_NAME " module\nsys.path: %s",path_info);
                MessageBoxA(nullptr, msg, "Info from DLL", MB_ICONEXCLAMATION);
                return 1;
            }
        }
    }
    MessageBoxA(nullptr, "Cannot find loaded python3x.dll, the process might not be a Python process",
                "Info from DLL", MB_ICONEXCLAMATION);
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