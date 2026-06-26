#pragma once
#include <windows.h>

struct TIME_FIELDS {
    USHORT Year;
    USHORT Month;
    USHORT Day;
    USHORT Hour;
    USHORT Minute;
    USHORT Second;
    USHORT Milliseconds;
    USHORT Weekday;
};

struct RTL_TIME_ZONE_INFORMATION {
    LONG Bias;
    WCHAR StandardName[32];
    TIME_FIELDS StandardDate;
    LONG StandardBias;
    WCHAR DaylightName[32];
    TIME_FIELDS DaylightDate;
    LONG DaylightBias;
};

struct LEB {
    UINT AnsiCodePage;
    UINT OemCodePage;
    UINT LocaleID;
    UINT DefaultCharset;
    UINT HookUILanguageAPI;
    BYTE DefaultFaceName[64];
    RTL_TIME_ZONE_INFORMATION Timezone;
};

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
);
