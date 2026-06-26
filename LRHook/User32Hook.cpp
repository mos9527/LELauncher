#include"User32Hook.h"
//
//#define FLAG_ON(_V, _F)     (!!((_V) & (_F)))
//
//#define WS_EX_ANSI          0x80000000
//#define WINDOW_FLAG_ANSI    0x00000001
//
//#define CLEAR_FLAG(_V, _F)  ((_V) &= ~(_F))
//
//typedef struct _LARGE_UNICODE_STRING
//{
//	ULONG Length;
//	ULONG MaximumLength : 31;
//	ULONG Ansi : 1;
//
//	union
//	{
//		PWSTR   UnicodeBuffer;
//		PSTR    AnsiBuffer;
//		ULONG64 Buffer;
//	};
//
//} LARGE_UNICODE_STRING, * PLARGE_UNICODE_STRING;
//
//typedef struct _UNICODE_STRING {
//	USHORT Length;
//	USHORT MaximumLength;
//	PWSTR  Buffer;
//} UNICODE_STRING, * PUNICODE_STRING;
//
//typedef struct _STRING {
//	USHORT Length;
//	USHORT MaximumLength;
//	PCHAR Buffer;
//} ANSI_STRING, * PANSI_STRING;
//
//typedef HWND(WINAPI* NtUserCreateWindowExFn)(
//	ULONG                   ExStyle,
//	PLARGE_UNICODE_STRING   ClassName,
//	PLARGE_UNICODE_STRING   ClassVersion,
//	PLARGE_UNICODE_STRING   WindowName,
//	ULONG                   Style,
//	LONG                    X,
//	LONG                    Y,
//	LONG                    Width,
//	LONG                    Height,
//	HWND                    ParentWnd,
//	HMENU                   Menu,
//	PVOID                   Instance,
//	LPVOID                  Param,
//	ULONG                   ShowMode,
//	ULONG                   Unknown1,
//	ULONG                   Unknown2,
//	ULONG                   Unknown3
//	);
//static NtUserCreateWindowExFn OriginalNtUserCreateWindowEx = (NtUserCreateWindowExFn)DetourFindFunction("win32u.dll", "NtUserCreateWindowEx");
//
//typedef LRESULT(WINAPI* NtUserMessageCallFn)(
//	HWND         Window,
//	UINT         Message,
//	WPARAM       wParam,
//	LPARAM       lParam,
//	ULONG_PTR    xParam,
//	ULONG        xpfnProc,
//	ULONG        Flags
//	);
//static NtUserMessageCallFn OriginalNtUserMessageCall = (NtUserMessageCallFn)DetourFindFunction("win32u.dll", "NtUserMessageCall");
//
///*************************************/
///* Unicde to Ansi UserCall Functions */
///*************************************/
//LRESULT NTAPI UNICODE_EMPTY(WNDPROC PrevProc, HWND Window, UINT Message, WPARAM wParam, LPARAM lParam)
//{
//	return CallWindowProcA(PrevProc, Window, Message, wParam, lParam);
//}
//
//LRESULT NTAPI UNICODE_INLPCREATESTRUCT(WNDPROC PrevProc, HWND Window, UINT Message, WPARAM wParam, LPARAM lParam)
//{
//	LRESULT			Result;
//	LPCREATESTRUCTW	CreateStructW;
//	CREATESTRUCTA	CreateStructA;
//
//	CreateStructW = (LPCREATESTRUCTW)lParam;
//
//	CreateStructA.lpszClass = nullptr;
//	CreateStructA.lpszName = nullptr;
//
//	if (CreateStructW)
//	{
//		CreateStructA = *(LPCREATESTRUCTA)CreateStructW;
//		CreateStructA.lpszClass = WideCharToMultiByteInternal(CreateStructW->lpszClass);
//		CreateStructA.lpszName = WideCharToMultiByteInternal(CreateStructW->lpszName);
//
//		filelog << CreateStructA.lpszName << std::endl;
//
//		lParam = (LPARAM)&CreateStructA;
//	}
//
//	Result = CallWindowProcA(PrevProc, Window, Message, wParam, lParam);
//
//	FreeStringInternal((LPVOID)CreateStructA.lpszClass);
//	FreeStringInternal((LPVOID)CreateStructA.lpszName);
//
//	return Result;
//}
//
//LRESULT NTAPI UNICODE_INLPMDICREATESTRUCT(WNDPROC PrevProc, HWND Window, UINT Message, WPARAM wParam, LPARAM lParam)
//{
//	LRESULT				Result;
//	LPMDICREATESTRUCTW	MdiCreateStructW;
//	MDICREATESTRUCTA	MdiCreateStructA;
//
//	MdiCreateStructW = (LPMDICREATESTRUCTW)lParam;
//
//	MdiCreateStructA.szClass = nullptr;
//	MdiCreateStructA.szTitle = nullptr;
//
//	if (MdiCreateStructW)
//	{
//		MdiCreateStructA = *(LPMDICREATESTRUCTA)MdiCreateStructW;
//		MdiCreateStructA.szClass = WideCharToMultiByteInternal(MdiCreateStructW->szClass);
//		MdiCreateStructA.szTitle = WideCharToMultiByteInternal(MdiCreateStructW->szTitle);
//
//		lParam = (LPARAM)&MdiCreateStructA;
//	}
//
//	Result = CallWindowProcA(PrevProc, Window, Message, wParam, lParam);
//
//	FreeStringInternal((LPVOID)MdiCreateStructA.szClass);
//	FreeStringInternal((LPVOID)MdiCreateStructA.szTitle);
//
//	return Result;
//}
//
//LRESULT NTAPI UNICODE_INSTRINGNULL(WNDPROC PrevProc, HWND Window, UINT Message, WPARAM wParam, LPARAM lParam)
//{
//	LRESULT Result;
//	LPWSTR	Unicode;
//	LPSTR	Ansi;
//
//	Unicode = (LPWSTR)lParam;
//
//	MessageBoxW(NULL, Unicode, NULL, NULL);
//
//	Ansi = nullptr;
//
//	if (Unicode)
//	{
//		Ansi = WideCharToMultiByteInternal(Unicode);
//		lParam = (LPARAM)Ansi;
//	}
//
//	Result = CallWindowProcA(PrevProc, Window, Message, wParam, lParam);
//
//	FreeStringInternal(Ansi);
//
//	return Result;
//}
//
//LRESULT NTAPI UNICODE_OUTSTRING(WNDPROC PrevProc, HWND Window, UINT Message, WPARAM wParam, LPARAM lParam)
//{
//	MessageBoxA(NULL, "UNICODE_OUTSTRING", NULL, NULL);
//	LPWSTR	UnicodeBuffer;
//	LPSTR	AnsiBuffer;
//	UINT	UnicodeSize, AnsiSize;
//
//	UnicodeSize = wParam;
//	UnicodeBuffer = (LPWSTR)lParam;
//
//	AnsiSize = UnicodeSize * sizeof(WCHAR);
//	AnsiBuffer = (LPSTR)AllocateZeroedMemory(AnsiSize);
//	if (AnsiBuffer == nullptr)
//		return 0;
//
//	wParam = AnsiSize;
//	lParam = (LPARAM)AnsiBuffer;
//
//	AnsiSize = CallWindowProcA(PrevProc, Window, Message, wParam, lParam);
//
//	if (AnsiSize != 0)
//		WideCharToMultiByte(CP_ACP, 0, UnicodeBuffer, UnicodeSize, AnsiBuffer, AnsiSize, NULL, NULL);
//
//	FreeStringInternal(AnsiBuffer);
//
//	return AnsiSize / sizeof(WCHAR);
//}
//
//LRESULT NTAPI UNICODE_INSTRING(WNDPROC PrevProc, HWND Window, UINT Message, WPARAM wParam, LPARAM lParam)
//{
//	return UNICODE_INSTRINGNULL(PrevProc, Window, Message, wParam, lParam);
//}
//
//LRESULT NTAPI UNICODE_INCNTOUTSTRING(WNDPROC PrevProc, HWND Window, UINT Message, WPARAM wParam, LPARAM lParam)
//{
//	//MessageBoxA(NULL, "UNICODE_INCNTOUTSTRING", NULL, NULL);
//	return CallWindowProcA(PrevProc, Window, Message, wParam, lParam);
//}
//
//LRESULT NTAPI UNICODE_INCBOXSTRING(WNDPROC PrevProc, HWND Window, UINT Message, WPARAM wParam, LPARAM lParam)
//{
//	return UNICODE_INSTRINGNULL(PrevProc, Window, Message, wParam, lParam);
//}
//
//LRESULT NTAPI UNICODE_OUTCBOXSTRING(WNDPROC PrevProc, HWND Window, UINT Message, WPARAM wParam, LPARAM lParam)
//{
//	//MessageBoxA(NULL, "UNICODE_OUTCBOXSTRING", NULL, NULL);
//	return CallWindowProcA(PrevProc, Window, Message, wParam, lParam);
//}
//
//LRESULT NTAPI UNICODE_INLBOXSTRING(WNDPROC PrevProc, HWND Window, UINT Message, WPARAM wParam, LPARAM lParam)
//{
//	return UNICODE_INSTRINGNULL(PrevProc, Window, Message, wParam, lParam);
//}
//
//LRESULT NTAPI UNICODE_OUTLBOXSTRING(WNDPROC PrevProc, HWND Window, UINT Message, WPARAM wParam, LPARAM lParam)
//{
//	return UNICODE_INSTRINGNULL(PrevProc, Window, Message, wParam, lParam);
//}
//
//LRESULT NTAPI UNICODE_INCNTOUTSTRINGNULL(WNDPROC PrevProc, HWND Window, UINT Message, WPARAM wParam, LPARAM lParam)
//{
//	return UNICODE_INSTRINGNULL(PrevProc, Window, Message, wParam, lParam);
//}
//
//LRESULT NTAPI UNICODE_GETDBCSTEXTLENGTHS(WNDPROC PrevProc, HWND Window, UINT Message, WPARAM wParam, LPARAM lParam)
//{
//	//MessageBoxA(NULL, "UNICODE_GETDBCSTEXTLENGTHS", NULL, NULL);
//	return CallWindowProcA(PrevProc, Window, Message, wParam, lParam);
//}
//
/****************************************/
/* Ansi to Unicode KernelCall Functions */
/****************************************/
LRESULT NTAPI ANSI_INLPCREATESTRUCT(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam)
{
	LRESULT			Result;
	LPCREATESTRUCTA CreateStructA;
	CREATESTRUCTW   CreateStructW;

	if (!IsWindowUnicode(Window))
	{
		return OriginalSendMessageA_Global(Window, Message, wParam, lParam);
	}

	CreateStructA = (LPCREATESTRUCTA)lParam;
	CreateStructW.lpszClass = nullptr;
	CreateStructW.lpszName = nullptr;

	if (CreateStructA)
	{
		CreateStructW = *(LPCREATESTRUCTW)CreateStructA;
		CreateStructW.lpszClass = MultiByteToWideCharInternal(CreateStructA->lpszClass);
		CreateStructW.lpszName = MultiByteToWideCharInternal(CreateStructA->lpszName);
		lParam = (LPARAM)&CreateStructW;
	}
	Result = SendMessageW(Window, Message, wParam, lParam);
	FreeStringInternal((LPVOID)CreateStructW.lpszClass);
	FreeStringInternal((LPVOID)CreateStructW.lpszName);

	return Result;
}

LRESULT NTAPI ANSI_INLPMDICREATESTRUCT(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam)
{
	LRESULT				Result;
	LPMDICREATESTRUCTA  MdiCreateStructA;
	MDICREATESTRUCTW    MdiCreateStructW;

	if (!IsWindowUnicode(Window))
	{
		return OriginalSendMessageA_Global(Window, Message, wParam, lParam);
	}

	MdiCreateStructA = (LPMDICREATESTRUCTA)lParam;
	MdiCreateStructW.szClass = nullptr;
	MdiCreateStructW.szTitle = nullptr;

	if (MdiCreateStructA)
	{
		MdiCreateStructW = *(LPMDICREATESTRUCTW)MdiCreateStructA;
		MdiCreateStructW.szClass = MultiByteToWideCharInternal(MdiCreateStructA->szClass);
		MdiCreateStructW.szTitle = MultiByteToWideCharInternal(MdiCreateStructA->szTitle);

		lParam = (LPARAM)&MdiCreateStructW;
	}

	Result = SendMessageW(Window, Message, wParam, lParam);

	FreeStringInternal((LPVOID)MdiCreateStructW.szClass);
	FreeStringInternal((LPVOID)MdiCreateStructW.szTitle);

	return Result;
}

LRESULT NTAPI ANSI_INSTRING(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam)
{
	LRESULT	Result;
	LPSTR	Ansi;
	LPWSTR	Unicode;

	if (!IsWindowUnicode(Window))
	{
		return OriginalSendMessageA_Global(Window, Message, wParam, lParam);
	}

	Ansi = (LPSTR)lParam;
	Unicode = nullptr;

	if (Ansi)
	{
		Unicode = MultiByteToWideCharInternal(Ansi);
		lParam = (LPARAM)Unicode;
	}

	Result = SendMessageW(Window, Message, wParam, lParam);
	FreeStringInternal(Unicode);
	return Result;
}

LRESULT NTAPI ANSI_GETTEXT(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam)
{
	LRESULT Length;
	LPWSTR UnicodeBuffer;
	LPSTR AnsiBuffer;

	if (!IsWindowUnicode(Window))
	{
		return OriginalSendMessageA_Global(Window, Message, wParam, lParam);
	}

	AnsiBuffer = (LPSTR)lParam;

	Length = SendMessageW(Window, Message + 1, wParam, lParam);
	if (Length < 0)
		return Length;

	Length++;
	UnicodeBuffer = (LPWSTR)AllocateZeroedMemory(Length * sizeof(WCHAR));
	if (UnicodeBuffer == nullptr)
		return 0;

	Length = SendMessageW(Window, Message, wParam, (LPARAM)UnicodeBuffer);
	if (Length > 0)
		Length = WideCharToMultiByte(settings.CodePage, 0, UnicodeBuffer, Length, AnsiBuffer, Length * sizeof(WCHAR), NULL, NULL);

	FreeStringInternal(UnicodeBuffer);

	return Length;
}

LRESULT NTAPI ANSI_GETTEXTLENGTH(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam)
{
	LRESULT Length;
	LPWSTR UnicodeBuffer;

	if (!IsWindowUnicode(Window))
	{
		return OriginalSendMessageA_Global(Window, Message, wParam, lParam);
	}

	Length = SendMessageW(Window, Message, wParam, lParam);
	if (Length < 0) 
		return Length;

	Length++;
	UnicodeBuffer = (LPWSTR)AllocateZeroedMemory(Length * sizeof(WCHAR));
	if (UnicodeBuffer == nullptr) 
		return 0;

	wParam = Message == WM_GETTEXTLENGTH ? Length : wParam;

	Length = SendMessageW(Window, Message - 1, wParam, (LPARAM)UnicodeBuffer);
	if (Length > 0)
	{
		Length = WideCharToMultiByte(settings.CodePage, 0, UnicodeBuffer, Length * sizeof(WCHAR), NULL, 0, NULL, NULL);
	}

	FreeStringInternal(UnicodeBuffer);

	return Length;
}

LRESULT NTAPI ANSI_OUTSTRING(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam)
{
	LRESULT	UnicodeSize, AnsiSize;
	LPWSTR	UnicodeBuffer;
	LPSTR	AnsiBuffer;

	if (!IsWindowUnicode(Window))
	{
		return OriginalSendMessageA_Global(Window, Message, wParam, lParam);
	}

	AnsiBuffer = (LPSTR)lParam;
	AnsiSize = wParam;

	UnicodeSize = SendMessageW(Window, WM_GETTEXTLENGTH, wParam, lParam);
	if (UnicodeSize == 0)
		return 0;

	UnicodeSize++;
	UnicodeBuffer = (LPWSTR)AllocateZeroedMemory(UnicodeSize * sizeof(WCHAR));
	if (UnicodeBuffer == nullptr)
		return 0;

	AnsiSize = SendMessageW(Window, Message, UnicodeSize, (LPARAM)UnicodeBuffer);
	AnsiSize = WideCharToMultiByte(settings.CodePage, 0, UnicodeBuffer, (int)UnicodeSize, AnsiBuffer, (int)AnsiSize, NULL, NULL);

	FreeStringInternal(UnicodeBuffer);

	return AnsiSize;
}

LRESULT NTAPI ANSI_GETLINE(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam)
{
	LRESULT AnsiSize, Length;
	LPSTR AnsiBuffer;
	LPWSTR UnicodeBuffer;

	if (!IsWindowUnicode(Window))
	{
		return OriginalSendMessageA_Global(Window, Message, wParam, lParam);
	}

	typedef union
	{
		USHORT  BufferSize;
		WCHAR   UnicodeBuffer[1];
		CHAR    AnsiBuffer[1];

	} MSG_INPUT_COUNT_OUPUT_STRING, * PMSG_INPUT_COUNT_OUPUT_STRING;

	PMSG_INPUT_COUNT_OUPUT_STRING ParamA, ParamW;

	ParamA = (PMSG_INPUT_COUNT_OUPUT_STRING)lParam;

	AnsiSize = ParamA->BufferSize;
	AnsiBuffer = ParamA->AnsiBuffer;

	UnicodeBuffer = (LPWSTR)AllocateZeroedMemory(AnsiSize);
	if (UnicodeBuffer == nullptr)
		return 0;

	ParamW = (PMSG_INPUT_COUNT_OUPUT_STRING)UnicodeBuffer;
	ParamW->BufferSize = AnsiSize;

	Length = SendMessageW(Window, Message, wParam, lParam);

	if (Length != 0)
	{
		Length = WideCharToMultiByte(settings.CodePage, 0, UnicodeBuffer, Length, AnsiBuffer, AnsiSize, NULL, NULL);
	}

	FreeStringInternal(UnicodeBuffer);

	return Length;
}

LRESULT NTAPI ANSI_SETTEXT(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam)
{
	LPSTR Ansi = (LPSTR)lParam;
	LPWSTR Unicode = nullptr;

	if (Ansi)
	{
		Unicode = MultiByteToWideCharInternal(Ansi);
		lParam = (LPARAM)Unicode;
	}

	LRESULT Result = SendMessageW(Window, Message, wParam, lParam);
	FreeStringInternal(Unicode);
	return Result;
}

// Tab control message constants
#ifndef TCM_FIRST
#define TCM_FIRST               0x1300
#endif
#define TCM_INSERTITEMA         (TCM_FIRST + 7)
#define TCM_SETITEMA            (TCM_FIRST + 6)
#define TCM_INSERTITEMW         (TCM_FIRST + 62)
#define TCM_SETITEMW            (TCM_FIRST + 61)

#ifndef TCIF_TEXT
#define TCIF_TEXT               0x0001
#endif

typedef struct {
	UINT mask;
	DWORD dwState;
	DWORD dwStateMask;
	LPSTR pszText;
	int cchTextMax;
	int iImage;
	LPARAM lParam;
} TCITEMA_COMPAT;

typedef struct {
	UINT mask;
	DWORD dwState;
	DWORD dwStateMask;
	LPWSTR pszText;
	int cchTextMax;
	int iImage;
	LPARAM lParam;
} TCITEMW_COMPAT;

LRESULT NTAPI ANSI_TABCONTROL(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam)
{
	TCITEMA_COMPAT* pItemA = (TCITEMA_COMPAT*)lParam;
	if (!pItemA || !(pItemA->mask & TCIF_TEXT) || !pItemA->pszText)
	{
		return OriginalSendMessageA_Global(Window, Message, wParam, lParam);
	}

	LPWSTR wstr = MultiByteToWideCharInternal(pItemA->pszText);
	if (!wstr)
	{
		return OriginalSendMessageA_Global(Window, Message, wParam, lParam);
	}

	TCITEMW_COMPAT itemW;
	memcpy(&itemW, pItemA, sizeof(TCITEMW_COMPAT));
	itemW.pszText = wstr;

	UINT msgW = (Message == TCM_INSERTITEMA) ? TCM_INSERTITEMW : TCM_SETITEMW;
	LRESULT Result = SendMessageW(Window, msgW, wParam, (LPARAM)&itemW);

	FreeStringInternal(wstr);
	return Result;
}
