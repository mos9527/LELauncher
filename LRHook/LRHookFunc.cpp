#include<Windows.h>
#include<detours.h>

#include"LROriginalFunc.h"
#include"LRHookFunc.h"
#include"User32Hook.h"
#include"KernelbaseHook.h"

ORIGINAL Original = { NULL };
SendMessageAFn OriginalSendMessageA_Global = NULL;

//OriginalNtUserCreateWindowEx = AttachDllFunc("NtUserCreateWindowEx", HookNtUserCreateWindowEx, "user32.dll");

static CreateProcessInternalWFn OriginalCreateProcessInternalW = (CreateProcessInternalWFn)DetourFindFunction("kernelbase.dll", "CreateProcessInternalW");
static NtUserMessageCallFn OriginalNtUserMessageCall = (NtUserMessageCallFn)DetourFindFunction("win32u.dll", "NtUserMessageCall");

BOOL WINAPI LRCreateProcessInternalW(
_In_opt_ LPCWSTR lpApplicationName,
_Inout_opt_ LPWSTR lpCommandLine,
_In_opt_ LPSECURITY_ATTRIBUTES lpProcessAttributes,
_In_opt_ LPSECURITY_ATTRIBUTES lpThreadAttributes,
_In_ BOOL bInheritHandles,
_In_ DWORD dwCreationFlags,
_In_opt_ LPVOID lpEnvironment,
_In_opt_ LPCWSTR lpCurrentDirectory,
_In_ LPSTARTUPINFOW lpStartupInfo,
_Out_ LPPROCESS_INFORMATION lpProcessInformation
)
{
	return OriginalCreateProcessInternalW(
		NULL,
		lpApplicationName,
		lpCommandLine,
		lpProcessAttributes,
		lpThreadAttributes,
		bInheritHandles,
		dwCreationFlags,
		lpEnvironment,
		lpCurrentDirectory,
		lpStartupInfo,
		lpProcessInformation,
		NULL
	);
}

BOOL WINAPI HookCreateProcessInternalW(
	HANDLE hUserToken,
	LPCWSTR lpApplicationName,
	LPWSTR lpCommandLine,
	LPSECURITY_ATTRIBUTES lpProcessAttributes,
	LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL bInheritHandles,
	DWORD dwCreationFlags,
	LPVOID lpEnvironment,
	LPCWSTR lpCurrentDirectory,
	LPSTARTUPINFOW lpStartupInfo,
	LPPROCESS_INFORMATION lpProcessInformation,
	OPTIONAL PHANDLE hRestrictedUserToken
)
{
	if (!OriginalCreateProcessInternalW(
		hUserToken,
		lpApplicationName,
		lpCommandLine,
		lpProcessAttributes,
		lpThreadAttributes,
		bInheritHandles,
		dwCreationFlags | CREATE_SUSPENDED,
		lpEnvironment,
		lpCurrentDirectory,
		lpStartupInfo,
		lpProcessInformation,
		hRestrictedUserToken))
	{
		return FALSE;
	}
	//MessageBoxW(NULL, lpCommandLine, L"HookCreateProcessInternalW", NULL);
	LPCSTR sz = Original.DllPath;
	DetourUpdateProcessWithDll(lpProcessInformation->hProcess, &sz, 1);
	DetourProcessViaHelperW(lpProcessInformation->dwProcessId, Original.DllPath, LRCreateProcessInternalW);
	if (!(dwCreationFlags & CREATE_SUSPENDED)) {
		ResumeThread(lpProcessInformation->hThread);
	}

	return TRUE;
}

LPVOID AllocateZeroedMemory(SIZE_T size/*eax*/) {
	return HeapAlloc(Original.hHeap, HEAP_ZERO_MEMORY, size);
}

VOID FreeStringInternal(LPVOID pBuffer/*ecx*/)
{
	HeapFree(Original.hHeap, 0, pBuffer);
}

LPWSTR MultiByteToWideCharInternal(LPCSTR lstr, UINT CodePage)
{
	if (CodePage == CP_ACP || CodePage == 0) CodePage = settings.CodePage;
	int lsize = lstrlenA(lstr)/* size without '\0' */, n = 0;
	int wsize = (lsize + 1) << 1;
	LPWSTR wstr = (LPWSTR)HeapAlloc(Original.hHeap, 0, wsize);
	if (wstr) {
		n = OriginalMultiByteToWideChar(CodePage, 0, lstr, lsize, wstr, wsize);
		wstr[n] = L'\0'; // make tail ! 
	}
	return wstr;
}

LPSTR WideCharToMultiByteInternal(LPCWSTR wstr, UINT CodePage)
{
	if (CodePage == CP_ACP || CodePage == 0) CodePage = settings.CodePage;
	int wsize = lstrlenW(wstr)/* size without '\0' */, n = 0;
	int lsize = (wsize + 1) << 1;
	LPSTR lstr = (LPSTR)HeapAlloc(Original.hHeap, 0, lsize);
	if (lstr) {
		n = OriginalWideCharToMultiByte(CodePage, 0, wstr, wsize, lstr, lsize, NULL, NULL);
		lstr[n] = '\0'; // make tail ! 
	}
	return lstr;
}


void AttachFunctions() 
{
	DetourAttach(&(PVOID&)OriginalGetACP, HookGetACP);
	DetourAttach(&(PVOID&)OriginalGetOEMCP, HookGetOEMCP);
	DetourAttach(&(PVOID&)OriginalGetCPInfo, HookGetCPInfo);
	DetourAttach(&(PVOID&)OriginalGetThreadLocale, HookGetThreadLocale);
	DetourAttach(&(PVOID&)OriginalGetSystemDefaultUILanguage, HookGetSystemDefaultUILanguage);
	DetourAttach(&(PVOID&)OriginalGetUserDefaultUILanguage, HookGetUserDefaultUILanguage);
	DetourAttach(&(PVOID&)OriginalGetSystemDefaultLCID, HookGetSystemDefaultLCID);
	DetourAttach(&(PVOID&)OriginalGetUserDefaultLCID, HookGetUserDefaultLCID);
	DetourAttach(&(PVOID&)OriginalGetSystemDefaultLangID, HookGetSystemDefaultLangID);
	DetourAttach(&(PVOID&)OriginalGetUserDefaultLangID, HookGetUserDefaultLangID);
	DetourAttach(&(PVOID&)OriginalMultiByteToWideChar, HookMultiByteToWideChar);
	DetourAttach(&(PVOID&)OriginalWideCharToMultiByte, HookWideCharToMultiByte);

	DetourAttach(&(PVOID&)OriginalCreateWindowExA, HookCreateWindowExA);
	//DetourAttach(&(PVOID&)OriginalDefWindowProcA, HookDefWindowProcA);
	DetourAttach(&(PVOID&)OriginalMessageBoxA, HookMessageBoxA);

	DetourAttach(&(PVOID&)OriginalCharPrevExA, HookCharPrevExA);
	DetourAttach(&(PVOID&)OriginalCharNextExA, HookCharNextExA);
	DetourAttach(&(PVOID&)OriginalIsDBCSLeadByteEx, HookIsDBCSLeadByteEx);
	OriginalSendMessageA_Global = OriginalSendMessageA;
	DetourAttach(&(PVOID&)OriginalSendMessageA, HookSendMessageA);
	
	//DetourAttach(&(PVOID&)OriginalNtCreateUserProcess, HookNtCreateUserProcess);
#ifdef _WIN64
	DetourAttach(&(PVOID&)OriginalCreateProcessA, HookCreateProcessA);
	DetourAttach(&(PVOID&)OriginalCreateProcessW, HookCreateProcessW);
#else
	DetourAttach(&(PVOID&)OriginalCreateProcessInternalW, HookCreateProcessInternalW);
#endif
	
	DetourAttach(&(PVOID&)OriginalWinExec, HookWinExec);
	//DetourAttach(&(PVOID&)OriginalShellExecuteA, HookShellExecuteA);
	//DetourAttach(&(PVOID&)OriginalShellExecuteW, HookShellExecuteW);
	//DetourAttach(&(PVOID&)OriginalShellExecuteExA, HookShellExecuteExA);
	//DetourAttach(&(PVOID&)OriginalShellExecuteExW, HookShellExecuteExW);
	
	
	DetourAttach(&(PVOID&)OriginalSetWindowTextA, HookSetWindowTextA);
	DetourAttach(&(PVOID&)OriginalGetWindowTextA, HookGetWindowTextA);
	
	DetourAttach(&(PVOID&)OriginalIsWindowUnicode, HookIsWindowUnicode);
	DetourAttach(&(PVOID&)OriginalSetWindowLongA, HookSetWindowLongA);
	DetourAttach(&(PVOID&)OriginalSetWindowLongW, HookSetWindowLongW);
	DetourAttach(&(PVOID&)OriginalGetWindowLongA, HookGetWindowLongA);
	DetourAttach(&(PVOID&)OriginalGetWindowLongW, HookGetWindowLongW);
#ifdef _WIN64
	DetourAttach(&(PVOID&)OriginalSetWindowLongPtrA, HookSetWindowLongPtrA);
	DetourAttach(&(PVOID&)OriginalSetWindowLongPtrW, HookSetWindowLongPtrW);
	DetourAttach(&(PVOID&)OriginalGetWindowLongPtrA, HookGetWindowLongPtrA);
	DetourAttach(&(PVOID&)OriginalGetWindowLongPtrW, HookGetWindowLongPtrW);
#endif
	DetourAttach(&(PVOID&)OriginalDirectSoundEnumerateA, HookDirectSoundEnumerateA);
	DetourAttach(&(PVOID&)OriginalCreateFontA, HookCreateFontA);
	DetourAttach(&(PVOID&)OriginalCreateFontW, HookCreateFontW);
	DetourAttach(&(PVOID&)OriginalCreateFontIndirectA, HookCreateFontIndirectA);
	DetourAttach(&(PVOID&)OriginalCreateFontIndirectW, HookCreateFontIndirectW);
	DetourAttach(&(PVOID&)OriginalGetStockObject, HookGetStockObject);
	DetourAttach(&(PVOID&)OriginalCreateFontIndirectExA, HookCreateFontIndirectExA);
	DetourAttach(&(PVOID&)OriginalCreateFontIndirectExW, HookCreateFontIndirectExW);
	DetourAttach(&(PVOID&)OriginalTextOutA, HookTextOutA);
	DetourAttach(&(PVOID&)OriginalDrawTextExA, HookDrawTextExA);
	DetourAttach(&(PVOID&)OriginalExtTextOutA, HookExtTextOutA);
	DetourAttach(&(PVOID&)OriginalDrawTextA, HookDrawTextA);
	DetourAttach(&(PVOID&)OriginalGetClipboardData, HookGetClipboardData);
	DetourAttach(&(PVOID&)OriginalSetClipboardData, HookSetClipboardData);

	DetourAttach(&(PVOID&)OriginalDialogBoxParamA, HookDialogBoxParamA);
	DetourAttach(&(PVOID&)OriginalCreateDialogIndirectParamA, HookCreateDialogIndirectParamA);
	DetourAttach(&(PVOID&)OriginalCreateDialogParamA, HookCreateDialogParamA);
	DetourAttach(&(PVOID&)OriginalDialogBoxIndirectParamA, HookDialogBoxIndirectParamA);
	DetourAttach(&(PVOID&)OriginalLoadStringA, HookLoadStringA);
	DetourAttach(&(PVOID&)OriginalLoadStringW, HookLoadStringW);

	DetourAttach(&(PVOID&)OriginalSendDlgItemMessageA, HookSendDlgItemMessageA);
	DetourAttach(&(PVOID&)OriginalSetDlgItemTextA, HookSetDlgItemTextA);
	DetourAttach(&(PVOID&)OriginalGetDlgItemTextA, HookGetDlgItemTextA);

	DetourAttach(&(PVOID&)OriginalLoadMenuA, HookLoadMenuA);
	DetourAttach(&(PVOID&)OriginalLoadMenuW, HookLoadMenuW);
	DetourAttach(&(PVOID&)OriginalInsertMenuA, HookInsertMenuA);
	DetourAttach(&(PVOID&)OriginalAppendMenuA, HookAppendMenuA);
	DetourAttach(&(PVOID&)OriginalModifyMenuA, HookModifyMenuA);
	DetourAttach(&(PVOID&)OriginalInsertMenuItemA, HookInsertMenuItemA);
	DetourAttach(&(PVOID&)OriginalSetMenuItemInfoA, HookSetMenuItemInfoA);

	DetourAttach(&(PVOID&)OriginalGetTimeZoneInformation, HookGetTimeZoneInformation);
	DetourAttach(&(PVOID&)OriginalCreateDirectoryA, HookCreateDirectoryA);
	DetourAttach(&(PVOID&)OriginalCreateFileA, HookCreateFileA);
	
	DetourAttach(&(PVOID&)OriginalGetLocaleInfoA, HookGetLocaleInfoA);
	DetourAttach(&(PVOID&)OriginalGetLocaleInfoW, HookGetLocaleInfoW);

	if (settings.HookLCID)
	{
		/*DetourAttach(&(PVOID&)OriginalRegisterClassA, HookRegisterClassA);
		DetourAttach(&(PVOID&)OriginalRegisterClassExA, HookRegisterClassExA);*/
	}
	
	Original.CodePage = OriginalGetACP();
	if (settings.HookIME)
	{
		if (Original.CodePage == 936 && (settings.CodePage == 932 || settings.CodePage == 950))
		{
			DetourAttach(&(PVOID&)OriginalImmGetCompositionStringA, HookImmGetCompositionStringA);
			DetourAttach(&(PVOID&)OriginalImmGetCandidateListA, HookImmGetCandidateListA);
		} 
		else
		{
			DetourAttach(&(PVOID&)OriginalImmGetCompositionStringA, HookImmGetCompositionStringA_WM);
			//DetourAttach(&(PVOID&)OriginalImmGetCandidateListA, HookImmGetCandidateListA_WM);
		}
	}
}

void DetachFunctions() 
{
	DetourDetach(&(PVOID&)OriginalGetACP, HookGetACP);
	DetourDetach(&(PVOID&)OriginalGetOEMCP, HookGetOEMCP);
	DetourDetach(&(PVOID&)OriginalGetCPInfo, HookGetCPInfo);
	DetourDetach(&(PVOID&)OriginalGetThreadLocale, HookGetThreadLocale);
	DetourDetach(&(PVOID&)OriginalGetSystemDefaultUILanguage, HookGetSystemDefaultUILanguage);
	DetourDetach(&(PVOID&)OriginalGetUserDefaultUILanguage, HookGetUserDefaultUILanguage);
	DetourDetach(&(PVOID&)OriginalGetSystemDefaultLCID, HookGetSystemDefaultLCID);
	DetourDetach(&(PVOID&)OriginalGetUserDefaultLCID, HookGetUserDefaultLCID);
	DetourDetach(&(PVOID&)OriginalGetSystemDefaultLangID, HookGetSystemDefaultLangID);
	DetourDetach(&(PVOID&)OriginalGetUserDefaultLangID, HookGetUserDefaultLangID);
	DetourDetach(&(PVOID&)OriginalMultiByteToWideChar, HookMultiByteToWideChar);
	DetourDetach(&(PVOID&)OriginalWideCharToMultiByte, HookWideCharToMultiByte);

	DetourDetach(&(PVOID&)OriginalCreateWindowExA, HookCreateWindowExA);
	DetourDetach(&(PVOID&)OriginalDefWindowProcA, HookDefWindowProcA);
	DetourDetach(&(PVOID&)OriginalMessageBoxA, HookMessageBoxA);
	DetourDetach(&(PVOID&)OriginalCharPrevExA, HookCharPrevExA);
	DetourDetach(&(PVOID&)OriginalCharNextExA, HookCharNextExA);
	DetourDetach(&(PVOID&)OriginalIsDBCSLeadByteEx, HookIsDBCSLeadByteEx);
	DetourDetach(&(PVOID&)OriginalSendMessageA, HookSendMessageA);
	
	DetourDetach(&(PVOID&)OriginalWinExec, HookWinExec);
	DetourDetach(&(PVOID&)OriginalCreateProcessA, HookCreateProcessA);
	DetourDetach(&(PVOID&)OriginalCreateProcessW, HookCreateProcessW);

	DetourDetach(&(PVOID&)OriginalLoadStringW, HookLoadStringW);
	DetourDetach(&(PVOID&)OriginalSendDlgItemMessageA, HookSendDlgItemMessageA);
	DetourDetach(&(PVOID&)OriginalSetDlgItemTextA, HookSetDlgItemTextA);
	DetourDetach(&(PVOID&)OriginalGetDlgItemTextA, HookGetDlgItemTextA);
	DetourDetach(&(PVOID&)OriginalLoadMenuA, HookLoadMenuA);
	DetourDetach(&(PVOID&)OriginalLoadMenuW, HookLoadMenuW);
	DetourDetach(&(PVOID&)OriginalInsertMenuA, HookInsertMenuA);
	DetourDetach(&(PVOID&)OriginalAppendMenuA, HookAppendMenuA);
	DetourDetach(&(PVOID&)OriginalModifyMenuA, HookModifyMenuA);
	DetourDetach(&(PVOID&)OriginalInsertMenuItemA, HookInsertMenuItemA);
	DetourDetach(&(PVOID&)OriginalSetMenuItemInfoA, HookSetMenuItemInfoA);

	if (settings.HookLCID)
	{
		/*DetourDetach(&(PVOID&)OriginalRegisterClassA, HookRegisterClassA);
		DetourDetach(&(PVOID&)OriginalRegisterClassExA, HookRegisterClassExA);*/
	}
	//DetourDetach(&(PVOID&)OriginalShellExecuteA, HookShellExecuteA);
	//DetourDetach(&(PVOID&)OriginalShellExecuteW, HookShellExecuteW);
	//DetourDetach(&(PVOID&)OriginalShellExecuteExA, HookShellExecuteExA);
	//DetourDetach(&(PVOID&)OriginalShellExecuteExW, HookShellExecuteExW);

	DetourDetach(&(PVOID&)OriginalSetWindowTextA, HookSetWindowTextA);
	DetourDetach(&(PVOID&)OriginalGetWindowTextA, HookGetWindowTextA);
	
	DetourDetach(&(PVOID&)OriginalIsWindowUnicode, HookIsWindowUnicode);
	DetourDetach(&(PVOID&)OriginalSetWindowLongA, HookSetWindowLongA);
	DetourDetach(&(PVOID&)OriginalSetWindowLongW, HookSetWindowLongW);
	DetourDetach(&(PVOID&)OriginalGetWindowLongA, HookGetWindowLongA);
	DetourDetach(&(PVOID&)OriginalGetWindowLongW, HookGetWindowLongW);
#ifdef _WIN64
	DetourDetach(&(PVOID&)OriginalSetWindowLongPtrA, HookSetWindowLongPtrA);
	DetourDetach(&(PVOID&)OriginalSetWindowLongPtrW, HookSetWindowLongPtrW);
	DetourDetach(&(PVOID&)OriginalGetWindowLongPtrA, HookGetWindowLongPtrA);
	DetourDetach(&(PVOID&)OriginalGetWindowLongPtrW, HookGetWindowLongPtrW);
#endif
	DetourDetach(&(PVOID&)OriginalDirectSoundEnumerateA, HookDirectSoundEnumerateA);
	DetourDetach(&(PVOID&)OriginalCreateFontA, HookCreateFontA);
	DetourDetach(&(PVOID&)OriginalCreateFontW, HookCreateFontW);
	DetourDetach(&(PVOID&)OriginalCreateFontIndirectA, HookCreateFontIndirectA);
	DetourDetach(&(PVOID&)OriginalCreateFontIndirectW, HookCreateFontIndirectW);
	DetourDetach(&(PVOID&)OriginalGetStockObject, HookGetStockObject);
	DetourDetach(&(PVOID&)OriginalCreateFontIndirectExA, HookCreateFontIndirectExA);
	DetourDetach(&(PVOID&)OriginalCreateFontIndirectExW, HookCreateFontIndirectExW);
	DetourDetach(&(PVOID&)OriginalTextOutA, HookTextOutA);
	DetourDetach(&(PVOID&)OriginalDrawTextExA, HookDrawTextExA);
	DetourDetach(&(PVOID&)OriginalExtTextOutA, HookExtTextOutA);
	DetourDetach(&(PVOID&)OriginalDrawTextA, HookDrawTextA);
	DetourDetach(&(PVOID&)OriginalGetClipboardData, HookGetClipboardData);
	DetourDetach(&(PVOID&)OriginalSetClipboardData, HookSetClipboardData);

	DetourDetach(&(PVOID&)OriginalDialogBoxParamA, HookDialogBoxParamA);
	DetourDetach(&(PVOID&)OriginalDialogBoxParamA, HookDialogBoxParamA);
	DetourDetach(&(PVOID&)OriginalCreateDialogIndirectParamA, HookCreateDialogIndirectParamA);
	DetourDetach(&(PVOID&)OriginalCreateDialogParamA, HookCreateDialogParamA);
	DetourDetach(&(PVOID&)OriginalDialogBoxIndirectParamA, HookDialogBoxIndirectParamA);
	DetourDetach(&(PVOID&)OriginalLoadStringA, HookLoadStringA);

	DetourDetach(&(PVOID&)OriginalGetTimeZoneInformation, HookGetTimeZoneInformation);
	DetourDetach(&(PVOID&)OriginalCreateDirectoryA, HookCreateDirectoryA);
	DetourDetach(&(PVOID&)OriginalCreateFileA, HookCreateFileA);

	DetourDetach(&(PVOID&)OriginalGetLocaleInfoA, HookGetLocaleInfoA);
	DetourDetach(&(PVOID&)OriginalGetLocaleInfoW, HookGetLocaleInfoW);

	if (settings.HookLCID)
	{
		DetourDetach(&(PVOID&)OriginalRegisterClassA, HookRegisterClassA);
		DetourDetach(&(PVOID&)OriginalRegisterClassExA, HookRegisterClassExA);
	}

	if (settings.HookIME)
	{
		if (Original.CodePage == 936 && (settings.CodePage == 932 || settings.CodePage == 950))
		{
			DetourDetach(&(PVOID&)OriginalImmGetCompositionStringA, HookImmGetCompositionStringA);
			DetourDetach(&(PVOID&)OriginalImmGetCandidateListA, HookImmGetCandidateListA);
		}
		else
		{
			DetourDetach(&(PVOID&)OriginalImmGetCompositionStringA, HookImmGetCompositionStringA_WM);
			DetourDetach(&(PVOID&)OriginalImmGetCandidateListA, HookImmGetCandidateListA_WM);
		}
	}
}

void FixMenuStrings(HMENU hMenu)
{
	if (!hMenu) return;
	int count = GetMenuItemCount(hMenu);
	for (int i = 0; i < count; i++)
	{
		MENUITEMINFOW mii;
		ZeroMemory(&mii, sizeof(mii));
		mii.cbSize = sizeof(mii);
		mii.fMask = MIIM_STRING | MIIM_SUBMENU;
		if (GetMenuItemInfoW(hMenu, i, TRUE, &mii))
		{
			if (mii.hSubMenu)
			{
				FixMenuStrings(mii.hSubMenu);
			}
			if (mii.cch > 0)
			{
				mii.cch++; // space for null terminator
				mii.dwTypeData = (LPWSTR)HeapAlloc(Original.hHeap, HEAP_ZERO_MEMORY, mii.cch * sizeof(WCHAR));
				if (GetMenuItemInfoW(hMenu, i, TRUE, &mii))
				{
					BOOL usedDefaultChar = FALSE;
					int ansiLen = OriginalWideCharToMultiByte(Original.CodePage, 0, mii.dwTypeData, mii.cch, NULL, 0, NULL, &usedDefaultChar);
					if (ansiLen > 0 && !usedDefaultChar)
					{
						LPSTR ansiStr = (LPSTR)HeapAlloc(Original.hHeap, HEAP_ZERO_MEMORY, ansiLen + 1);
						OriginalWideCharToMultiByte(Original.CodePage, 0, mii.dwTypeData, mii.cch, ansiStr, ansiLen, NULL, NULL);
						
						LPWSTR fixedW = MultiByteToWideCharInternal(ansiStr, settings.CodePage);
						
						MENUITEMINFOW miiSet;
						ZeroMemory(&miiSet, sizeof(miiSet));
						miiSet.cbSize = sizeof(miiSet);
						miiSet.fMask = MIIM_STRING;
						miiSet.dwTypeData = fixedW;
						SetMenuItemInfoW(hMenu, i, TRUE, &miiSet);
						
						FreeStringInternal(fixedW);
						HeapFree(Original.hHeap, 0, ansiStr);
					}
				}
				HeapFree(Original.hHeap, 0, mii.dwTypeData);
			}
		}
	}
}

int WINAPI HookLoadStringW(
	_In_opt_ HINSTANCE hInstance,
	_In_ UINT uID,
	_Out_writes_to_(cchBufferMax, return + 1) LPWSTR lpBuffer,
	_In_ int cchBufferMax)
{
	int ret = OriginalLoadStringW(hInstance, uID, lpBuffer, cchBufferMax);
	if (ret > 0)
	{
		BOOL usedDefaultChar = FALSE;
		int ansiLen = OriginalWideCharToMultiByte(Original.CodePage, 0, lpBuffer, ret, NULL, 0, NULL, &usedDefaultChar);
		if (ansiLen > 0 && !usedDefaultChar)
		{
			LPSTR ansiStr = (LPSTR)HeapAlloc(Original.hHeap, HEAP_ZERO_MEMORY, ansiLen + 1);
			OriginalWideCharToMultiByte(Original.CodePage, 0, lpBuffer, ret, ansiStr, ansiLen, NULL, NULL);
			
			LPWSTR fixedW = MultiByteToWideCharInternal(ansiStr, settings.CodePage);
			int fixedLen = lstrlenW(fixedW);
			
			if (fixedLen <= ret) {
				lstrcpyW(lpBuffer, fixedW);
				ret = fixedLen;
			}
			
			FreeStringInternal(fixedW);
			HeapFree(Original.hHeap, 0, ansiStr);
		}
	}
	return ret;
}

LRESULT WINAPI HookSendDlgItemMessageA(HWND hDlg, int nIDDlgItem, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	HWND hWndControl = GetDlgItem(hDlg, nIDDlgItem);
	if (!hWndControl) return OriginalSendDlgItemMessageA(hDlg, nIDDlgItem, Msg, wParam, lParam);
	return HookSendMessageA(hWndControl, Msg, wParam, lParam);
}

BOOL WINAPI HookSetDlgItemTextA(HWND hDlg, int nIDDlgItem, LPCSTR lpString)
{
	HWND hWndControl = GetDlgItem(hDlg, nIDDlgItem);
	if (!hWndControl) return OriginalSetDlgItemTextA(hDlg, nIDDlgItem, lpString);
	return HookSetWindowTextA(hWndControl, lpString);
}

UINT WINAPI HookGetDlgItemTextA(HWND hDlg, int nIDDlgItem, LPSTR lpString, int cchMax)
{
	HWND hWndControl = GetDlgItem(hDlg, nIDDlgItem);
	if (!hWndControl) return OriginalGetDlgItemTextA(hDlg, nIDDlgItem, lpString, cchMax);
	return HookGetWindowTextA(hWndControl, lpString, cchMax);
}

HMENU WINAPI HookLoadMenuA(HINSTANCE hInstance, LPCSTR lpMenuName)
{
	LPCWSTR lpMenuNameW = IS_INTRESOURCE(lpMenuName) ? (LPCWSTR)lpMenuName : MultiByteToWideCharInternal(lpMenuName, settings.CodePage);
	HMENU ret = LoadMenuW(hInstance, lpMenuNameW);
	if (!IS_INTRESOURCE(lpMenuName)) FreeStringInternal((LPVOID)lpMenuNameW);
	if (ret) FixMenuStrings(ret);
	return ret;
}

HMENU WINAPI HookLoadMenuW(HINSTANCE hInstance, LPCWSTR lpMenuName)
{
	HMENU ret = OriginalLoadMenuW(hInstance, lpMenuName);
	if (ret) FixMenuStrings(ret);
	return ret;
}

BOOL WINAPI HookInsertMenuA(HMENU hMenu, UINT uPosition, UINT uFlags, UINT_PTR uIDNewItem, LPCSTR lpNewItem)
{
	if (!(uFlags & (MF_BITMAP | MF_OWNERDRAW)) && lpNewItem && !IS_INTRESOURCE(lpNewItem)) {
		LPCWSTR lpNewItemW = MultiByteToWideCharInternal(lpNewItem, settings.CodePage);
		BOOL ret = InsertMenuW(hMenu, uPosition, uFlags, uIDNewItem, lpNewItemW);
		FreeStringInternal((LPVOID)lpNewItemW);
		return ret;
	}
	return OriginalInsertMenuA(hMenu, uPosition, uFlags, uIDNewItem, lpNewItem);
}

BOOL WINAPI HookAppendMenuA(HMENU hMenu, UINT uFlags, UINT_PTR uIDNewItem, LPCSTR lpNewItem)
{
	if (!(uFlags & (MF_BITMAP | MF_OWNERDRAW)) && lpNewItem && !IS_INTRESOURCE(lpNewItem)) {
		LPCWSTR lpNewItemW = MultiByteToWideCharInternal(lpNewItem, settings.CodePage);
		BOOL ret = AppendMenuW(hMenu, uFlags, uIDNewItem, lpNewItemW);
		FreeStringInternal((LPVOID)lpNewItemW);
		return ret;
	}
	return OriginalAppendMenuA(hMenu, uFlags, uIDNewItem, lpNewItem);
}

BOOL WINAPI HookModifyMenuA(HMENU hMnu, UINT uPosition, UINT uFlags, UINT_PTR uIDNewItem, LPCSTR lpNewItem)
{
	if (!(uFlags & (MF_BITMAP | MF_OWNERDRAW)) && lpNewItem && !IS_INTRESOURCE(lpNewItem)) {
		LPCWSTR lpNewItemW = MultiByteToWideCharInternal(lpNewItem, settings.CodePage);
		BOOL ret = ModifyMenuW(hMnu, uPosition, uFlags, uIDNewItem, lpNewItemW);
		FreeStringInternal((LPVOID)lpNewItemW);
		return ret;
	}
	return OriginalModifyMenuA(hMnu, uPosition, uFlags, uIDNewItem, lpNewItem);
}

BOOL WINAPI HookInsertMenuItemA(HMENU hmenu, UINT item, BOOL fByPosition, LPCMENUITEMINFOA lpmi)
{
	if (lpmi && (lpmi->fMask & (MIIM_STRING | MIIM_TYPE)) && lpmi->dwTypeData && !IS_INTRESOURCE(lpmi->dwTypeData)) {
		MENUITEMINFOW miiw;
		memcpy(&miiw, lpmi, sizeof(MENUITEMINFOA));
		miiw.cbSize = sizeof(MENUITEMINFOW);
		LPWSTR wstr = MultiByteToWideCharInternal(lpmi->dwTypeData, settings.CodePage);
		miiw.dwTypeData = wstr;
		BOOL ret = InsertMenuItemW(hmenu, item, fByPosition, &miiw);
		FreeStringInternal(wstr);
		return ret;
	}
	return OriginalInsertMenuItemA(hmenu, item, fByPosition, lpmi);
}

BOOL WINAPI HookSetMenuItemInfoA(HMENU hmenu, UINT item, BOOL fByPosition, LPCMENUITEMINFOA lpmi)
{
	if (lpmi && (lpmi->fMask & (MIIM_STRING | MIIM_TYPE)) && lpmi->dwTypeData && !IS_INTRESOURCE(lpmi->dwTypeData)) {
		MENUITEMINFOW miiw;
		memcpy(&miiw, lpmi, sizeof(MENUITEMINFOA));
		miiw.cbSize = sizeof(MENUITEMINFOW);
		LPWSTR wstr = MultiByteToWideCharInternal(lpmi->dwTypeData, settings.CodePage);
		miiw.dwTypeData = wstr;
		BOOL ret = SetMenuItemInfoW(hmenu, item, fByPosition, &miiw);
		FreeStringInternal(wstr);
		return ret;
	}
	return OriginalSetMenuItemInfoA(hmenu, item, fByPosition, lpmi);
}

HWND WINAPI HookCreateWindowExA(
	DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle,
	int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
	HWND ret = OriginalCreateWindowExA(
		dwExStyle,
		lpClassName,
		lpWindowName,
		dwStyle,
		X,
		Y,
		nWidth,
		nHeight,
		hWndParent,
		hMenu,
		hInstance,
		lpParam
	);
	
	if (ret && lpWindowName)
	{
		LPCWSTR wstrlpWindowName = MultiByteToWideCharInternal(lpWindowName, settings.CodePage);
		SetWindowTextW(ret, wstrlpWindowName);
		FreeStringInternal((LPVOID)wstrlpWindowName);
	}
	if (ret && hMenu == NULL && lpClassName != NULL)
	{
		HMENU hMenuLoaded = GetMenu(ret);
		if (hMenuLoaded) FixMenuStrings(hMenuLoaded);
	}
	return ret;
}

int WINAPI HookMessageBoxA(
	_In_opt_ HWND hWnd,
	_In_opt_ LPCSTR lpText,
	_In_opt_ LPCSTR lpCaption,
	_In_ UINT uType)
{
	LPCWSTR wlpText = MultiByteToWideCharInternal(lpText);
	LPCWSTR wlpCaption = MultiByteToWideCharInternal(lpCaption);
	int ret = MessageBoxW(hWnd, wlpText, wlpCaption, uType);
	if (wlpText)
	{
		FreeStringInternal((LPVOID)wlpText);
	}
	if (wlpCaption)
	{
		FreeStringInternal((LPVOID)wlpCaption);
	}
	return ret;
}

UINT WINAPI HookGetACP(void)
{
	return settings.CodePage;
}
UINT WINAPI HookGetOEMCP(void)
{
	return settings.CodePage;
}

BOOL WINAPI HookGetCPInfo(
	UINT       CodePage,
	LPCPINFO  lpCPInfo
)
{
	CodePage = settings.CodePage;
	return OriginalGetCPInfo(CodePage, lpCPInfo);
}

LRESULT WINAPI HookSendMessageA(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
	case WM_NCCREATE:
	{
		return ANSI_INLPCREATESTRUCT(hWnd, uMsg, wParam, lParam);
	}	break;
	case WM_MDICREATE:
	{
		return ANSI_INLPMDICREATESTRUCT(hWnd, uMsg, wParam, lParam);
	}	break;
	case WM_SETTEXT:
	case WM_SETTINGCHANGE:
	case WM_DEVMODECHANGE:
	{
		return ANSI_SETTEXT(hWnd, uMsg, wParam, lParam);
	}	break;
	case EM_REPLACESEL:
	case CB_DIR:
	case LB_DIR:
	case LB_ADDFILE:
	case CB_ADDSTRING:
	case CB_INSERTSTRING:
	case CB_FINDSTRING:
	case CB_SELECTSTRING:
	case CB_FINDSTRINGEXACT:
	case LB_ADDSTRING:
	case LB_INSERTSTRING:
	case LB_SELECTSTRING:
	case LB_FINDSTRING:
	case LB_FINDSTRINGEXACT:
	{
		return ANSI_INSTRING(hWnd, uMsg, wParam, lParam);
	}	break;
	case CB_GETLBTEXT:
	case LB_GETTEXT:
	{
		return ANSI_GETTEXT(hWnd, uMsg, wParam, lParam);
	}	break;
	case WM_GETTEXTLENGTH: // LN327
	case CB_GETLBTEXTLEN:
	case LB_GETTEXTLEN:
	{
		return ANSI_GETTEXTLENGTH(hWnd, uMsg, wParam, lParam);
	}	break;
	case WM_GETTEXT:
	case WM_ASKCBFORMATNAME:
	{
		return ANSI_OUTSTRING(hWnd, uMsg, wParam, lParam);
	}	break;
	case EM_GETLINE:
	{
		return ANSI_GETLINE(hWnd, uMsg, wParam, lParam);
	}	break;
	case 0x1307: // TCM_INSERTITEMA
	case 0x1306: // TCM_SETITEMA
	{
		return ANSI_TABCONTROL(hWnd, uMsg, wParam, lParam);
	}	break;
	}
	return OriginalSendMessageA(hWnd, uMsg, wParam, lParam);
}

int WINAPI HookMultiByteToWideChar(UINT CodePage, DWORD dwFlags,
	LPCSTR lpMultiByteStr, int cbMultiByte, LPWSTR lpWideCharStr, int cchWideChar)
{
	CodePage = (CodePage >= CP_UTF7) ? CodePage : settings.CodePage;
	return OriginalMultiByteToWideChar(CodePage, dwFlags, lpMultiByteStr, cbMultiByte, lpWideCharStr, cchWideChar);
}

int WINAPI HookWideCharToMultiByte(UINT CodePage, DWORD dwFlags,
	LPCWSTR lpWideCharStr, int cchWideChar, LPSTR lpMultiByteStr, int cbMultiByte, LPCSTR lpDefaultChar, LPBOOL lpUsedDefaultChar)
{
	CodePage = (CodePage >= CP_UTF7) ? CodePage : settings.CodePage;
	return OriginalWideCharToMultiByte(CodePage, dwFlags, lpWideCharStr, cchWideChar, lpMultiByteStr, cbMultiByte, lpDefaultChar, lpUsedDefaultChar);
}

UINT WINAPI HookWinExec(
	_In_ LPSTR lpCmdLine,
	_In_ UINT uCmdShow
)
{
	//LRConfigFileMap filemap;
	//filemap.WrtieConfigFileMap(&settings);

	STARTUPINFOA si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(STARTUPINFOA));
	ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
	si.cb = sizeof(STARTUPINFOA);
	
	BOOL ret = CreateProcessA(NULL, lpCmdLine, NULL,
		NULL, FALSE, CREATE_DEFAULT_ERROR_MODE, NULL, NULL,
		&si, &pi);

	//filemap.FreeConfigFileMap();
	if (ret == TRUE)
		return 0x21;
	else return 0;
}

BOOL WINAPI HookCreateProcessA(
	_In_opt_ LPCSTR lpApplicationName,
	_Inout_opt_ LPSTR lpCommandLine,
	_In_opt_ LPSECURITY_ATTRIBUTES lpProcessAttributes,
	_In_opt_ LPSECURITY_ATTRIBUTES lpThreadAttributes,
	_In_ BOOL bInheritHandles,
	_In_ DWORD dwCreationFlags,
	_In_opt_ LPVOID lpEnvironment,
	_In_opt_ LPCSTR lpCurrentDirectory,
	_In_ LPSTARTUPINFOA lpStartupInfo,
	_Out_ LPPROCESS_INFORMATION lpProcessInformation
)
{
	//MessageBoxA(NULL, lpCommandLine, "HookCreateProcessA", NULL);
	return DetourCreateProcessWithDllExA(
		lpApplicationName,
		lpCommandLine,
		lpProcessAttributes,
		lpThreadAttributes,
		bInheritHandles,
		dwCreationFlags,
		lpEnvironment,
		lpCurrentDirectory,
		lpStartupInfo,
		lpProcessInformation, 
		Original.DllPath,
		OriginalCreateProcessA);
}

BOOL WINAPI HookCreateProcessW(
	_In_opt_ LPCWSTR lpApplicationName,
	_Inout_opt_ LPWSTR lpCommandLine,
	_In_opt_ LPSECURITY_ATTRIBUTES lpProcessAttributes,
	_In_opt_ LPSECURITY_ATTRIBUTES lpThreadAttributes,
	_In_ BOOL bInheritHandles,
	_In_ DWORD dwCreationFlags,
	_In_opt_ LPVOID lpEnvironment,
	_In_opt_ LPCWSTR lpCurrentDirectory,
	_In_ LPSTARTUPINFOW lpStartupInfo,
	_Out_ LPPROCESS_INFORMATION lpProcessInformation
)
{
	//MessageBoxW(NULL, lpCommandLine, TEXT("HookCreateProcessW"), NULL);
	if (lpCommandLine && wcsstr(lpCommandLine, L"BlackXchg.aes") != NULL)
	{
		return OriginalCreateProcessW(
			lpApplicationName,
			lpCommandLine,
			lpProcessAttributes,
			lpThreadAttributes,
			bInheritHandles,
			dwCreationFlags,
			lpEnvironment,
			lpCurrentDirectory,
			lpStartupInfo,
			lpProcessInformation
		);
	}
	return DetourCreateProcessWithDllExW(
		lpApplicationName,
		lpCommandLine,
		lpProcessAttributes,
		lpThreadAttributes,
		bInheritHandles,
		dwCreationFlags,
		lpEnvironment,
		lpCurrentDirectory,
		lpStartupInfo,
		lpProcessInformation,
		Original.DllPath,
		OriginalCreateProcessW);
}

HINSTANCE WINAPI HookShellExecuteA(
	_In_opt_ HWND hwnd,
	_In_opt_ LPSTR lpOperation,
	_In_ LPSTR lpFile,
	_In_opt_ LPSTR lpParameters,
	_In_opt_ LPSTR lpDirectory,
	_In_ INT nShowCmd
)
{
	//MessageBoxA(NULL, lpFile, "ShellExecuteA", NULL);

	//LRConfigFileMap filemap;
	//filemap.WrtieConfigFileMap(&settings);

	STARTUPINFOA si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(STARTUPINFOA));
	ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
	si.cb = sizeof(STARTUPINFOA);

	bool ret = DetourCreateProcessWithDllExA(lpFile, lpParameters, NULL,
		NULL, FALSE, CREATE_DEFAULT_ERROR_MODE, NULL, lpDirectory,
		&si, &pi, Original.DllPath, OriginalCreateProcessA);

	//filemap.FreeConfigFileMap();

	return (HINSTANCE)pi.hProcess;
}

HINSTANCE WINAPI HookShellExecuteW(
	_In_opt_ HWND hwnd,
	_In_opt_ LPWSTR lpOperation,
	_In_ LPWSTR lpFile,
	_In_opt_ LPWSTR lpParameters,
	_In_opt_ LPWSTR lpDirectory,
	_In_ INT nShowCmd
)
{
	//MessageBoxW(NULL, TEXT("ShellExecuteW"), TEXT("ShellExecuteW"), NULL);

	//LRConfigFileMap filemap;
	//filemap.WrtieConfigFileMap(&settings);

	STARTUPINFOW si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(STARTUPINFOW));
	ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
	si.cb = sizeof(STARTUPINFOW);

	bool ret = DetourCreateProcessWithDllExW(lpFile, lpParameters, NULL,
		NULL, FALSE, CREATE_DEFAULT_ERROR_MODE, NULL, lpDirectory,
		&si, &pi, Original.DllPath, OriginalCreateProcessW);

	//filemap.FreeConfigFileMap();

	return (HINSTANCE)pi.hProcess;
}

BOOL WINAPI HookShellExecuteExA(
	_Inout_ SHELLEXECUTEINFOA* pExecInfo
)
{
	//MessageBoxA(NULL, pExecInfo->lpFile, "ShellExecuteExA", NULL);

	return OriginalShellExecuteExA(pExecInfo);
}

BOOL WINAPI HookShellExecuteExW(
	_Inout_ SHELLEXECUTEINFOW* pExecInfo
)
{
	//MessageBoxW(NULL, L"ShellExecuteExW", L"ShellExecuteExW", NULL);

	return OriginalShellExecuteExW(pExecInfo);
}

BOOL WINAPI HookSetWindowTextA(
	_In_ HWND hWnd,
	_In_opt_ LPCSTR lpString
)
{
	LPCWSTR wstr = lpString ? MultiByteToWideCharInternal(lpString) : NULL;
	BOOL ret = SetWindowTextW(hWnd, wstr);
	if (wstr) {
		FreeStringInternal((LPVOID)wstr);
	}
	return ret;
}

int WINAPI HookGetWindowTextA(_In_ HWND hWnd, _Out_writes_(nMaxCount) LPSTR lpString, _In_ int nMaxCount)
{
	if (!IsWindowUnicode(hWnd))
	{
		return OriginalGetWindowTextA(hWnd, lpString, nMaxCount);
	}
	int wlen = GetWindowTextLengthW(hWnd) + 1;
	LPWSTR lpStringW = (LPWSTR)AllocateZeroedMemory(wlen * sizeof(wchar_t));
	int wsize = GetWindowTextW(hWnd, lpStringW, wlen);
	int lsize = 0;
	if (wsize > 0)
	{
		lsize = OriginalWideCharToMultiByte(settings.CodePage, 0, lpStringW, wsize + 1, lpString, nMaxCount, NULL, NULL);
		if (lsize > 0) lsize--;
	}
	FreeStringInternal(lpStringW);
	return lsize;
}

LONG WINAPI HookImmGetCompositionStringA(
	HIMC hIMC, 
	DWORD dwIndex,
	LPSTR lpBuf,
	DWORD  dwBufLen
)
{
	LONG ret = OriginalImmGetCompositionStringA(hIMC, dwIndex, lpBuf, dwBufLen);
	if (lpBuf)
	{
		LPWSTR wstr = MultiByteToWideCharInternal(lpBuf, Original.CodePage);
		if (wstr)
		{
			int wsize = lstrlenW(wstr), n = 0;
			int lsize = (wsize + 1) << 1;
			n = OriginalWideCharToMultiByte(settings.CodePage, 0, wstr, wsize, lpBuf, lsize, NULL, NULL);
			dwBufLen = lsize;
			lpBuf[n] = '\0';
		}
		FreeStringInternal((LPVOID)wstr);
	}
	return ret;
}

LONG WINAPI HookImmGetCompositionStringA_WM(
	HIMC hIMC,
	DWORD dwIndex,
	LPSTR lpBuf,
	DWORD  dwBufLen
)
{
	LONG wsize = ImmGetCompositionStringW(hIMC, dwIndex, NULL, 0);
	LPWSTR wstr = (LPWSTR)AllocateZeroedMemory(wsize);
	ImmGetCompositionStringW(hIMC, dwIndex, wstr, wsize);
	LONG lsize = (wsize + 1) << 1;
	if (lpBuf)
	{
		lsize = OriginalWideCharToMultiByte(settings.CodePage, 0, wstr, wsize, lpBuf, lsize, Original.lpDefaultChar, &Original.lpUsedDefaultChar);
		lpBuf[lsize] = '\0'; // make tail ! 
	}
	FreeStringInternal(wstr);
	return lsize;
}

DWORD WINAPI HookImmGetCandidateListA(
	HIMC            hIMC,
	DWORD           deIndex,
	LPCANDIDATELIST lpCandList,
	DWORD           dwBufLen
)
{
	DWORD ret= OriginalImmGetCandidateListA(hIMC, deIndex, lpCandList, dwBufLen);
	if (lpCandList)
	{
		for (int i = 0; i < lpCandList->dwCount; i++)
		{
			LPSTR lstr = (LPSTR)lpCandList + lpCandList->dwOffset[i];
			LPWSTR wstr = MultiByteToWideCharInternal(lstr, Original.CodePage);
			if (wstr)
			{
				int wsize = lstrlenW(wstr), n = 0;
				int lsize = (wsize + 1) << 1;
				n = OriginalWideCharToMultiByte(settings.CodePage, 0, wstr, wsize, lstr, lsize, NULL, NULL);
				dwBufLen = lsize;
				lstr[n] = '\0';
				FreeStringInternal(wstr);
			}
		}
	}
	return ret;
}

DWORD WINAPI HookImmGetCandidateListA_WM(
	HIMC            hIMC,
	DWORD           deIndex,
	LPCANDIDATELIST lpCandList,
	DWORD           dwBufLen
)
{
	DWORD ret = OriginalImmGetCandidateListA(hIMC, deIndex, lpCandList, dwBufLen);
	if (lpCandList)
	{
		DWORD dwBufLenW = ImmGetCandidateListW(hIMC, deIndex, NULL, NULL);
		LPCANDIDATELIST lpCandListW = (LPCANDIDATELIST)AllocateZeroedMemory(dwBufLenW);
		ImmGetCandidateListW(hIMC, deIndex, lpCandListW, dwBufLenW);
		for (int i = 0; i < lpCandList->dwCount; i++)
		{
			LPSTR lstr = (LPSTR)lpCandList + lpCandList->dwOffset[i];
			LPWSTR wstr = (LPWSTR)lpCandListW + lpCandListW->dwOffset[i];
			if (lstr)
			{
				int lsize = lstrlenA(lstr);
				int wsize = wcslen(wstr);
				OriginalWideCharToMultiByte(settings.CodePage, 0, wstr, wsize, lstr, lsize, NULL, NULL);
				//filelog << lstr << "###" << lsize << "###" << wstr << "###" <<wsize << std::endl;
			}
		}
		FreeStringInternal(lpCandListW);
	}
	return ret;
}

HFONT WINAPI HookCreateFontA(
	_In_ int cHeight,
	_In_ int cWidth,
	_In_ int cEscapement,
	_In_ int cOrientation,
	_In_ int cWeight,
	_In_ DWORD bItalic,
	_In_ DWORD bUnderline,
	_In_ DWORD bStrikeOut,
	_In_ DWORD iCharSet,
	_In_ DWORD iOutPrecision,
	_In_ DWORD iClipPrecision,
	_In_ DWORD iQuality,
	_In_ DWORD iPitchAndFamily,
	_In_opt_ LPCSTR pszFaceName
)
{
	LPWSTR pszFaceNameW = MultiByteToWideCharInternal(pszFaceName);
	HFONT ret = CreateFontW(
		cHeight,
		cWidth,
		cEscapement,
		cOrientation,
		cWeight,
		bItalic,
		bUnderline,
		bStrikeOut,
		iCharSet,
		iOutPrecision,
		iClipPrecision,
		iQuality,
		iPitchAndFamily,
		pszFaceNameW
	);
	FreeStringInternal(pszFaceNameW);
	return ret;
}

HFONT WINAPI HookCreateFontW(
	_In_ int cHeight,
	_In_ int cWidth,
	_In_ int cEscapement,
	_In_ int cOrientation,
	_In_ int cWeight,
	_In_ DWORD bItalic,
	_In_ DWORD bUnderline,
	_In_ DWORD bStrikeOut,
	_In_ DWORD iCharSet,
	_In_ DWORD iOutPrecision,
	_In_ DWORD iClipPrecision,
	_In_ DWORD iQuality,
	_In_ DWORD iPitchAndFamily,
	_In_opt_ LPCWSTR pszFaceName
)
{
	//iCharSet = HANGUL_CHARSET;
	return OriginalCreateFontW(
		cHeight,
		cWidth,
		cEscapement,
		cOrientation,
		cWeight,
		bItalic,
		bUnderline,
		bStrikeOut,
		iCharSet,
		iOutPrecision,
		iClipPrecision,
		iQuality,
		iPitchAndFamily,
		pszFaceName
	);
}

//set font characters set
HFONT WINAPI HookCreateFontIndirectA(
	LOGFONTA* lplf
)
{
	//MessageBoxA(NULL, lplf->lfFaceName, "HookCreateFontIndirectA", NULL);
	//lplf->lfCharSet = CHINESEBIG5_CHARSET;
	/*if (strcmp(settings.lfFaceName, "None") != 0)
		strcpy(lplf->lfFaceName, settings.lfFaceName);
	return OriginalCreateFontIndirectA(lplf);*/
	LOGFONTW logfont = { sizeof(LOGFONTW), };
	memcpy(&logfont, lplf, sizeof(LOGFONTW));
	MultiByteToWideChar(settings.CodePage, 0, lplf->lfFaceName, -1, logfont.lfFaceName, LF_FACESIZE);
	return CreateFontIndirectW(&logfont);
}

HFONT WINAPI HookCreateFontIndirectW(
	LOGFONTW* lplf
)
{
	//lplf->lfCharSet = HANGUL_CHARSET;
	return OriginalCreateFontIndirectW(lplf);
}

HFONT WINAPI HookCreateFontIndirectExA(
	ENUMLOGFONTEXDVA* lplf
)
{
	ENUMLOGFONTEXDVW lplfW = { sizeof(ENUMLOGFONTEXDVW), };
	memcpy(&lplfW, lplf, sizeof(ENUMLOGFONTEXDVW));
	MultiByteToWideChar(settings.CodePage, 0, lplf->elfEnumLogfontEx.elfLogFont.lfFaceName, -1, lplfW.elfEnumLogfontEx.elfLogFont.lfFaceName, LF_FACESIZE);
	return OriginalCreateFontIndirectExW(&lplfW);
}

HFONT WINAPI HookCreateFontIndirectExW(
	ENUMLOGFONTEXDVW* lplf
)
{
	//lplf->elfEnumLogfontEx.elfLogFont.lfCharSet = HANGUL_CHARSET;
	return OriginalCreateFontIndirectExW(lplf);
}

BOOL WINAPI HookTextOutA(
	HDC    hdc,
	int    x,
	int    y,
	LPSTR lpString,
	int    c
)
{
	if (lpString && c > 0)
	{
		int wsize = OriginalMultiByteToWideChar(settings.CodePage, 0, lpString, c, NULL, 0);
		if (wsize > 0)
		{
			LPWSTR wstr = (LPWSTR)AllocateZeroedMemory((wsize + 1) * sizeof(WCHAR));
			OriginalMultiByteToWideChar(settings.CodePage, 0, lpString, c, wstr, wsize);
			BOOL ret = TextOutW(hdc, x, y, wstr, wsize);
			FreeStringInternal(wstr);
			return ret;
		}
	}
	return OriginalTextOutA(hdc, x, y, lpString, c);
}

BOOL WINAPI HookExtTextOutA(
	_In_ HDC hdc,
	_In_ int x,
	_In_ int y,
	_In_ UINT options,
	_In_opt_ CONST RECT* lprect,
	_In_reads_opt_(c) LPCSTR lpString,
	_In_ UINT c,
	_In_reads_opt_(c) CONST INT* lpDx
)
{
	if (lpString && c > 0)
	{
		int wsize = OriginalMultiByteToWideChar(settings.CodePage, 0, lpString, c, NULL, 0);
		if (wsize > 0)
		{
			LPWSTR wstr = (LPWSTR)AllocateZeroedMemory((wsize + 1) * sizeof(WCHAR));
			OriginalMultiByteToWideChar(settings.CodePage, 0, lpString, c, wstr, wsize);
			BOOL ret = ExtTextOutW(hdc, x, y, options, lprect, wstr, wsize, lpDx);
			FreeStringInternal(wstr);
			return ret;
		}
	}
	return OriginalExtTextOutA(hdc, x, y, options, lprect, lpString, c, lpDx);
}

int WINAPI HookDrawTextA(
	_In_ HDC hdc,
	_Inout_updates_(cchText) LPCSTR lpchText,
	_In_ int cchText,
	_Inout_ LPRECT lprc,
	_In_ UINT format
)
{
	LPWSTR wstr = MultiByteToWideCharInternal(lpchText);
	if (wstr)
	{
		int wsize = lstrlenW(wstr);
		int ret = DrawTextW(hdc, wstr, wsize, lprc, format);
		FreeStringInternal(wstr);
		return ret;
	}
	return OriginalDrawTextA(hdc, lpchText, cchText, lprc, format);
}

int WINAPI HookDrawTextExA(
	_In_ HDC hdc,
	LPSTR lpchText,
	_In_ int cchText,
	_Inout_ LPRECT lprc,
	_In_ UINT format,
	_In_opt_ LPDRAWTEXTPARAMS lpdtp
)
{
	LPWSTR wstr = MultiByteToWideCharInternal(lpchText);
	if (wstr)
	{
		int wsize = lstrlenW(wstr);
		int ret = DrawTextExW(hdc, wstr, wsize, lprc, format, lpdtp);
		FreeStringInternal(wstr);
		return ret;
	}
	return OriginalDrawTextExA(
		hdc,
		lpchText,
		cchText,
		lprc,
		format,
		lpdtp
	);
}

HANDLE WINAPI HookGetClipboardData(
	UINT uFormat
)
{
	if (uFormat == CF_TEXT)
	{
		HANDLE hClipMemory = OriginalGetClipboardData(CF_UNICODETEXT);
		HANDLE hGlobalMemory = NULL;
		LPWSTR wstr = (LPWSTR)GlobalLock(hClipMemory);
		if (wstr)
		{
			int wsize = lstrlenW(wstr);
			int lsize = (wsize + 1) << 1;
			hGlobalMemory = GlobalAlloc(GHND, lsize);
			if (hGlobalMemory)
			{
				LPSTR lstr = (LPSTR)GlobalLock(hGlobalMemory);
				if (lstr)
				{
					lsize = OriginalWideCharToMultiByte(settings.CodePage, 0, wstr, wsize, lstr, lsize, NULL, NULL);
					lstr[lsize] = '\0';
				}
				GlobalUnlock(hGlobalMemory);
			}
		}
		GlobalUnlock(hClipMemory);
		if (hGlobalMemory)
			return hGlobalMemory;
	}
	return OriginalGetClipboardData(uFormat);
}

HANDLE WINAPI HookSetClipboardData(
	UINT uFormat,
	HANDLE hMem
)
{
	if (uFormat == CF_TEXT)
	{
		HANDLE hGlobalMemory = NULL;
		LPSTR lstr = (LPSTR)GlobalLock(hMem);
		if (lstr)
		{
			int lsize = lstrlenA(lstr);
			int wsize = (lsize + 1) << 1;
			hGlobalMemory = GlobalAlloc(GHND, wsize);
			if (hGlobalMemory)
			{
				LPWSTR wstr = (LPWSTR)GlobalLock(hGlobalMemory);
				if (wstr)
				{
					wsize = OriginalMultiByteToWideChar(settings.CodePage, 0, lstr, lsize, wstr, wsize);
					wstr[wsize] = L'\0';
				}
				GlobalUnlock(hGlobalMemory);
			}
		}
		GlobalUnlock(hMem);
		if (hGlobalMemory)
			return OriginalSetClipboardData(CF_UNICODETEXT, hGlobalMemory);
	}
	return OriginalSetClipboardData(uFormat, hMem);
}

HRESULT WINAPI HookDirectSoundEnumerateA(
	_In_ LPDSENUMCALLBACKA pDSEnumCallback,
	_In_opt_ LPVOID pContext
)
{
	return DirectSoundEnumerateW((LPDSENUMCALLBACKW)pDSEnumCallback,pContext);
}

LPSTR WINAPI HookCharPrevExA(
	_In_ WORD CodePage,
	_In_ LPCSTR lpStart,
	_In_ LPCSTR lpCurrentChar,
	_In_ DWORD dwFlags
)
{
	CodePage = (CodePage >= CP_UTF7) ? CodePage : settings.CodePage;
	return OriginalCharPrevExA(CodePage, lpStart, lpCurrentChar, dwFlags);
}

LPSTR WINAPI HookCharNextExA(
	_In_ WORD CodePage,
	_In_ LPCSTR lpCurrentChar,
	_In_ DWORD dwFlags
)
{
	CodePage = (CodePage >= CP_UTF7) ? CodePage : settings.CodePage;
	return OriginalCharNextExA(CodePage, lpCurrentChar, dwFlags);
}

BOOL WINAPI HookIsDBCSLeadByteEx(
	_In_ UINT  CodePage,
	_In_ BYTE  TestChar
)
{
	CodePage = (CodePage >= CP_UTF7) ? CodePage : settings.CodePage;
	return OriginalIsDBCSLeadByteEx(CodePage, TestChar);
}

INT_PTR WINAPI HookDialogBoxParamA(
	_In_opt_ HINSTANCE hInstance,
	_In_ LPCSTR lpTemplateName,
	_In_opt_ HWND hWndParent,
	_In_opt_ DLGPROC lpDialogFunc,
	_In_ LPARAM dwInitParam
)
{
	LPWSTR lpTemplateNameW = MultiByteToWideCharInternal(lpTemplateName);
	return DialogBoxParamW(
		hInstance, 
		lpTemplateNameW ? lpTemplateNameW : (LPCWSTR)lpTemplateName, 
		hWndParent, 
		lpDialogFunc, 
		dwInitParam
	);
}

HWND WINAPI HookCreateDialogIndirectParamA(
	_In_opt_ HINSTANCE hInstance,
	_In_ LPCDLGTEMPLATEA lpTemplate,
	_In_opt_ HWND hWndParent,
	_In_opt_ DLGPROC lpDialogFunc,
	_In_ LPARAM dwInitParam
)
{
	return CreateDialogIndirectParamW(hInstance, (LPCDLGTEMPLATEW)lpTemplate, hWndParent, lpDialogFunc, dwInitParam);
}

HWND WINAPI HookCreateDialogParamA(
	_In_opt_ HINSTANCE hInstance,
	_In_ LPCSTR lpTemplateName,
	_In_opt_ HWND hWndParent,
	_In_opt_ DLGPROC lpDialogFunc,
	_In_ LPARAM dwInitParam
)
{
	LPCWSTR lpTemplateNameW = IS_INTRESOURCE(lpTemplateName) ? (LPCWSTR)lpTemplateName : MultiByteToWideCharInternal(lpTemplateName);
	HWND ret = CreateDialogParamW(hInstance, lpTemplateNameW, hWndParent, lpDialogFunc, dwInitParam);
	if (!IS_INTRESOURCE(lpTemplateName) && lpTemplateNameW) FreeStringInternal((LPVOID)lpTemplateNameW);
	return ret;
}

INT_PTR WINAPI HookDialogBoxIndirectParamA(
	_In_opt_ HINSTANCE hInstance,
	_In_ LPCDLGTEMPLATEA hDialogTemplate,
	_In_opt_ HWND hWndParent,
	_In_opt_ DLGPROC lpDialogFunc,
	_In_ LPARAM dwInitParam
)
{
	return DialogBoxIndirectParamW(hInstance, (LPCDLGTEMPLATEW)hDialogTemplate, hWndParent, lpDialogFunc, dwInitParam);
}

int WINAPI HookLoadStringA(
	_In_opt_ HINSTANCE hInstance,
	_In_ UINT uID,
	_Out_writes_to_(cchBufferMax, return + 1) LPSTR lpBuffer,
	_In_ int cchBufferMax
)
{
	if (cchBufferMax <= 0) return 0;
	LPWSTR wBuffer = (LPWSTR)AllocateZeroedMemory(cchBufferMax * sizeof(WCHAR));
	if (!wBuffer) return 0;
	
	int wLen = LoadStringW(hInstance, uID, wBuffer, cchBufferMax);
	int aLen = 0;
	if (wLen > 0)
	{
		aLen = OriginalWideCharToMultiByte(settings.CodePage, 0, wBuffer, wLen + 1, lpBuffer, cchBufferMax, NULL, NULL);
		if (aLen > 0) aLen--;
	}
	FreeStringInternal(wBuffer);
	return aLen;
}

ATOM WINAPI HookRegisterClassA(
	_In_ CONST WNDCLASSA* lpWndClass
)
{
	WNDCLASSW lpWndClassW;
	lpWndClassW.style = lpWndClass->style;
	lpWndClassW.lpfnWndProc = lpWndClass->lpfnWndProc;
	lpWndClassW.cbClsExtra = lpWndClass->cbClsExtra;
	lpWndClassW.cbWndExtra = lpWndClass->cbWndExtra;
	lpWndClassW.hInstance = lpWndClass->hInstance;
	lpWndClassW.hIcon = lpWndClass->hIcon;
	lpWndClassW.hCursor = lpWndClass->hCursor;
	lpWndClassW.hbrBackground = lpWndClass->hbrBackground;
	lpWndClassW.lpszMenuName = (lpWndClass->lpszMenuName && !IS_INTRESOURCE(lpWndClass->lpszMenuName)) ? MultiByteToWideCharInternal(lpWndClass->lpszMenuName) : (LPCWSTR)lpWndClass->lpszMenuName;
	lpWndClassW.lpszClassName = (lpWndClass->lpszClassName && !IS_INTRESOURCE(lpWndClass->lpszClassName)) ? MultiByteToWideCharInternal(lpWndClass->lpszClassName) : (LPCWSTR)lpWndClass->lpszClassName;
	
	ATOM ret = RegisterClassW(&lpWndClassW);
	
	if (lpWndClassW.lpszMenuName && !IS_INTRESOURCE(lpWndClassW.lpszMenuName)) FreeStringInternal((LPVOID)lpWndClassW.lpszMenuName);
	if (lpWndClassW.lpszClassName && !IS_INTRESOURCE(lpWndClassW.lpszClassName)) FreeStringInternal((LPVOID)lpWndClassW.lpszClassName);
	return ret;
}

ATOM WINAPI HookRegisterClassExA(
	_In_ CONST WNDCLASSEXA* lpWndClass
)
{
	WNDCLASSEXW lpWndClassW;
	lpWndClassW.cbSize = sizeof(WNDCLASSEXW);
	lpWndClassW.style = lpWndClass->style;
	lpWndClassW.lpfnWndProc = lpWndClass->lpfnWndProc;
	lpWndClassW.cbClsExtra = lpWndClass->cbClsExtra;
	lpWndClassW.cbWndExtra = lpWndClass->cbWndExtra;
	lpWndClassW.hInstance = lpWndClass->hInstance;
	lpWndClassW.hIcon = lpWndClass->hIcon;
	lpWndClassW.hCursor = lpWndClass->hCursor;
	lpWndClassW.hbrBackground = lpWndClass->hbrBackground;
	lpWndClassW.lpszMenuName = (lpWndClass->lpszMenuName && !IS_INTRESOURCE(lpWndClass->lpszMenuName)) ? MultiByteToWideCharInternal(lpWndClass->lpszMenuName) : (LPCWSTR)lpWndClass->lpszMenuName;
	lpWndClassW.lpszClassName = (lpWndClass->lpszClassName && !IS_INTRESOURCE(lpWndClass->lpszClassName)) ? MultiByteToWideCharInternal(lpWndClass->lpszClassName) : (LPCWSTR)lpWndClass->lpszClassName;
	lpWndClassW.hIconSm = lpWndClass->hIconSm;
	
	ATOM ret = RegisterClassExW(&lpWndClassW);
	
	if (lpWndClassW.lpszMenuName && !IS_INTRESOURCE(lpWndClassW.lpszMenuName)) FreeStringInternal((LPVOID)lpWndClassW.lpszMenuName);
	if (lpWndClassW.lpszClassName && !IS_INTRESOURCE(lpWndClassW.lpszClassName)) FreeStringInternal((LPVOID)lpWndClassW.lpszClassName);
	return ret;
}

inline LRESULT CallProcAddress(LPVOID lpProcAddress, HWND hWnd, HWND hMDIClient,
	BOOL bMDIClientEnabled, INT uMsg, WPARAM wParam, LPARAM lParam)
{
	typedef LRESULT(WINAPI* fnWNDProcAddress)(HWND, int, WPARAM, LPARAM);
	typedef LRESULT(WINAPI* fnMDIProcAddress)(HWND, HWND, int, WPARAM, LPARAM);
	// MDI or not ??? 
	return (bMDIClientEnabled) ? ((fnMDIProcAddress)(DWORD_PTR)lpProcAddress)(hWnd, hMDIClient, uMsg, wParam, lParam)
		: ((fnWNDProcAddress)(DWORD_PTR)lpProcAddress)(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK HookDefWindowProcA(
	_In_ HWND hWnd,
	_In_ UINT Msg,
	_In_ WPARAM wParam,
	_In_ LPARAM lParam
)
{
	if(IsWindowUnicode(hWnd))
		return DefWindowProcW(hWnd, Msg, wParam, lParam);
	else
		return OriginalDefWindowProcA(hWnd, Msg, wParam, lParam);
}

DWORD WINAPI HookGetTimeZoneInformation(
	_Out_ LPTIME_ZONE_INFORMATION lpTimeZoneInformation
)
{
	DWORD ret = OriginalGetTimeZoneInformation(lpTimeZoneInformation);
	if (ret != TIME_ZONE_ID_INVALID) {
		// Warning Bias becomes negative!!!
		lpTimeZoneInformation->Bias = -settings.Bias;
	}
	return ret;
}

BOOL WINAPI HookCreateDirectoryA(
	_In_ LPCSTR lpPathName,
	_In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes
)
{
	LPWSTR lpPathNameW = MultiByteToWideCharInternal(lpPathName,Original.CodePage);
	BOOL ret = CreateDirectoryW(lpPathNameW, lpSecurityAttributes);
	if (lpPathNameW)
	{
		FreeStringInternal(lpPathNameW);
	}
	return ret;
}

HANDLE WINAPI HookCreateFileA(
	_In_ LPCSTR lpFileName,
	_In_ DWORD dwDesiredAccess,
	_In_ DWORD dwShareMode,
	_In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	_In_ DWORD dwCreationDisposition,
	_In_ DWORD dwFlagsAndAttributes,
	_In_opt_ HANDLE hTemplateFile
)
{
	LPWSTR lpFileNameW = MultiByteToWideCharInternal(lpFileName,Original.CodePage);
	HANDLE ret = CreateFileW(
		lpFileNameW,
		dwDesiredAccess,
		dwShareMode,
		lpSecurityAttributes,
		dwCreationDisposition,
		dwFlagsAndAttributes,
		hTemplateFile
	);
	if (lpFileNameW)
	{
		FreeStringInternal(lpFileNameW);
	}
	return ret;
}

int WINAPI HookGetLocaleInfoA(
	_In_ LCID Locale,
	_In_ LCTYPE LCType,
	_Out_writes_opt_(cchData) LPSTR lpLCData,
	_In_ int cchData
)
{
	return OriginalGetLocaleInfoA(settings.LCID, LCType, lpLCData, cchData);
}

int WINAPI HookGetLocaleInfoW(
	_In_ LCID Locale,
	_In_ LCTYPE LCType,
	_Out_writes_opt_(cchData) LPWSTR lpLCData,
	_In_ int cchData
)
{
	return OriginalGetLocaleInfoW(settings.LCID, LCType,lpLCData,cchData);
}

HGDIOBJ WINAPI HookGetStockObject(int i)
{
	static HGDIOBJ CachedStockObjects[20] = { 0 };
	if (i >= 0 && i < 20)
	{
		if (i == ANSI_FIXED_FONT || i == ANSI_VAR_FONT || i == DEVICE_DEFAULT_FONT ||
			i == DEFAULT_GUI_FONT || i == OEM_FIXED_FONT || i == SYSTEM_FONT || i == SYSTEM_FIXED_FONT)
		{
			if (CachedStockObjects[i] != NULL)
				return CachedStockObjects[i];

			HGDIOBJ obj = OriginalGetStockObject(i);
			LOGFONTW lf;
			if (GetObjectW(obj, sizeof(lf), &lf))
			{
				CHARSETINFO cs;
				if (TranslateCharsetInfo((DWORD*)(UINT_PTR)settings.CodePage, &cs, TCI_SRCCODEPAGE))
					lf.lfCharSet = cs.ciCharset;
				CachedStockObjects[i] = CreateFontIndirectW(&lf);
				return CachedStockObjects[i] ? CachedStockObjects[i] : obj;
			}
		}
	}
	return OriginalGetStockObject(i);
}

BOOL WINAPI HookIsWindowUnicode(HWND hWnd)
{
	// We return TRUE to force WinForms to use Unicode subclassing.
	// This bypasses the need for ANSI message translation since WinForms handles Unicode natively!
	return TRUE;
}

LONG WINAPI HookSetWindowLongA(HWND hWnd, int nIndex, LONG dwNewLong)
{
	return OriginalSetWindowLongA(hWnd, nIndex, dwNewLong);
}

LONG WINAPI HookSetWindowLongW(HWND hWnd, int nIndex, LONG dwNewLong)
{
	return OriginalSetWindowLongW(hWnd, nIndex, dwNewLong);
}

#ifdef _WIN64
LONG_PTR WINAPI HookSetWindowLongPtrA(HWND hWnd, int nIndex, LONG_PTR dwNewLong)
{
	return OriginalSetWindowLongPtrA(hWnd, nIndex, dwNewLong);
}

LONG_PTR WINAPI HookSetWindowLongPtrW(HWND hWnd, int nIndex, LONG_PTR dwNewLong)
{
	return OriginalSetWindowLongPtrW(hWnd, nIndex, dwNewLong);
}
#endif

LONG WINAPI HookGetWindowLongA(HWND hWnd, int nIndex)
{
	return OriginalGetWindowLongA(hWnd, nIndex);
}

LONG WINAPI HookGetWindowLongW(HWND hWnd, int nIndex)
{
	return OriginalGetWindowLongW(hWnd, nIndex);
}

#ifdef _WIN64
LONG_PTR WINAPI HookGetWindowLongPtrA(HWND hWnd, int nIndex)
{
	return OriginalGetWindowLongPtrA(hWnd, nIndex);
}

LONG_PTR WINAPI HookGetWindowLongPtrW(HWND hWnd, int nIndex)
{
	return OriginalGetWindowLongPtrW(hWnd, nIndex);
}
#endif