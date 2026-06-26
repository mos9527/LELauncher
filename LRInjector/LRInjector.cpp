#include "LRInjector.h"
#include <string>
#include <detours.h>

extern "C" __declspec(dllexport) UINT WINAPI LeCreateProcess(
    LEB* leb,
    LPCWSTR applicationName,
    LPCWSTR commandLine,
    LPCWSTR currentDirectory,
    UINT creationFlags,
    LPSTARTUPINFOW startupInfo,
    LPPROCESS_INFORMATION processInformation,
    LPSECURITY_ATTRIBUTES processAttributes,
    LPSECURITY_ATTRIBUTES threadAttributes,
    LPVOID environment,
    HANDLE token
) {
    if (!leb) return ERROR_INVALID_PARAMETER;

    // Set Locale_Remulator environment variables exactly as it expects
    // Note: Locale_Remulator casts pointers to values as LPCWSTR and sets them.
    SetEnvironmentVariableW(L"LRCodePage", (LPCWSTR)&leb->AnsiCodePage);
    SetEnvironmentVariableW(L"LRLCID", (LPCWSTR)&leb->LocaleID);
    SetEnvironmentVariableW(L"LRBIAS", (LPCWSTR)&leb->Timezone.Bias);
    
    int hookIme = 0;
    int hookLcid = 0;
    SetEnvironmentVariableW(L"LRHookIME", (LPCWSTR)&hookIme);
    SetEnvironmentVariableW(L"LRHookLCID", (LPCWSTR)&hookLcid);

    // Prepare Detours injection
    WCHAR dllPath[MAX_PATH] = { 0 };
    HMODULE hMod = NULL;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       (LPCWSTR)&LeCreateProcess, &hMod);
    GetModuleFileNameW(hMod, dllPath, MAX_PATH);
    
    std::wstring basePath = dllPath;
    size_t lastSlash = basePath.find_last_of(L"\\/");
    if (lastSlash != std::wstring::npos) {
        basePath = basePath.substr(0, lastSlash + 1);
    }

    std::wstring hookDll32 = basePath + L"LRHookx32.dll";
    std::wstring hookDll64 = basePath + L"LRHookx64.dll";

    // Use DetourCreateProcessWithDllsW to automatically inject 32 or 64 bit DLL
    LPCSTR dlls[2] = { NULL, NULL };
    char hookDll32A[MAX_PATH];
    char hookDll64A[MAX_PATH];
    WideCharToMultiByte(CP_ACP, 0, hookDll32.c_str(), -1, hookDll32A, MAX_PATH, NULL, NULL);
    WideCharToMultiByte(CP_ACP, 0, hookDll64.c_str(), -1, hookDll64A, MAX_PATH, NULL, NULL);

    dlls[0] = hookDll32A;
    dlls[1] = hookDll64A;

    BOOL ret = DetourCreateProcessWithDllsW(
        applicationName,
        (LPWSTR)commandLine,
        processAttributes,
        threadAttributes,
        TRUE, // bInheritHandles must be TRUE for Detours injection
        creationFlags,
        environment,
        currentDirectory,
        startupInfo,
        processInformation,
        2,
        dlls,
        NULL
    );

    return ret ? 0 : GetLastError();
}
