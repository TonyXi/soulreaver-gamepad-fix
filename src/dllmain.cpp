#include "stdafx.h"
#include <stdio.h>

void SoulReaverHook();

FARPROC p[7] = { 0 };

BOOL hooked = FALSE;

//DirectInputCreateA
extern "C" __declspec(naked) void __stdcall __E__0__()
{
	if (!hooked)
	{
		SoulReaverHook();
		hooked = true;
	}
	__asm
	{
		jmp p[0];
	}
}
// DirectInputCreateEx
extern "C" __declspec(naked) void __stdcall __E__1__()
{
	__asm
	{
		jmp p[1];
	}
}
// DirectInputCreateW
extern "C" __declspec(naked) void __stdcall __E__2__()
{
	__asm
	{
		jmp p[2];
	}
}
// DllCanUnloadNow
extern "C" __declspec(naked) void __stdcall __E__3__()
{
	__asm
	{
		jmp p[3];
	}
}
// DllGetClassObject
extern "C" __declspec(naked) void __stdcall __E__4__()
{
	__asm
	{
		jmp p[4];
	}
}
//DllRegisterServer
extern "C" __declspec(naked) void __stdcall __E__5__()
{
	__asm
	{
		jmp p[5];
	}
}
//DllUnregisterServer
extern "C" __declspec(naked) void __stdcall __E__6__()
{
	__asm
	{
		jmp p[6];
	}
}

typedef struct DIJOYSTATE {
	LONG    lX;                     /* x-axis position              */
	LONG    lY;                     /* y-axis position              */
	LONG    lZ;                     /* z-axis position              */
	LONG    lRx;                    /* x-axis rotation              */
	LONG    lRy;                    /* y-axis rotation              */
	LONG    lRz;                    /* z-axis rotation              */
	LONG    rglSlider[2];           /* extra axes positions         */
	DWORD   rgdwPOV[4];             /* POV directions               */
	BYTE    rgbButtons[32];         /* 32 buttons                   */
} DIJOYSTATE, *LPDIJOYSTATE;

void LoadFunctions() {

	char * sz_buffer = new char[255];
	char * sz_systemDirectory = new char[255];
	memset(sz_systemDirectory, 0, 255);

	GetSystemDirectory(sz_systemDirectory, 255);

	sprintf(sz_buffer, "%s\\dinput.dll", sz_systemDirectory);

	HMODULE hL = LoadLibrary(sz_buffer);
	if (hL == 0)
	{
		return;
	}

	p[0] = GetProcAddress(hL, "DirectInputCreateA");
	p[1] = GetProcAddress(hL, "DirectInputCreateEx");
	p[2] = GetProcAddress(hL, "DirectInputCreateW");
	p[3] = GetProcAddress(hL, "DllCanUnloadNow");
	p[4] = GetProcAddress(hL, "DllGetClassObject");
	p[5] = GetProcAddress(hL, "DllRegisterServer");
	p[6] = GetProcAddress(hL, "DllUnregisterServer");

}

void SoulReaverGamePadFix()
{
	int result;

	DIJOYSTATE *dijoystate;

	__asm
	{
		push edx;
		push 0x50;
		mov dijoystate, EDX;
		MOV ECX, DWORD PTR DS : [EAX];
		PUSH EAX;
		CALL DWORD PTR DS : [ECX + 0x24];
		mov result, eax
	}

	if (dijoystate->lX > -500 && dijoystate->lX < 500)
	{
		dijoystate->lX = 0;
	}

	if (dijoystate->lY > -500 && dijoystate->lY < 500)
	{
		dijoystate->lY = 0;
	}

	__asm mov eax, result;
}

void SoulReaverHook()
{
	DWORD dwOldProtect, dwNewProtect, dwNewCall, dwNewAddress, dwAddress;

	dwAddress = 0x00478201;
	dwNewAddress = (DWORD)&SoulReaverGamePadFix;

	BYTE opCALL[9] = { 0xE8, 0x00, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90 };
	dwNewCall = dwNewAddress - dwAddress - 5;
	memcpy(&opCALL[1], &dwNewCall, 4);
	VirtualProtectEx(GetCurrentProcess(), (LPVOID)dwAddress, 9, PAGE_EXECUTE_WRITECOPY, &dwOldProtect);
	memcpy((LPVOID)dwAddress, &opCALL, 9);
	VirtualProtectEx(GetCurrentProcess(), (LPVOID)dwAddress, 9, dwOldProtect, &dwNewProtect);

}

__declspec(dllexport) INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//case WM_INITDIALOG
	//WNDPROC baseListViewProc = (WNDPROC)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	//return CallWindowProc(baseListViewHeaderProc, hWnd, msg, wParam, lParam);
	//WNDPROC baseEditProc = (WNDPROC)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	//return CallWindowProc(baseEditProc, hWnd, msg, wParam, lParam);
	//m_xBaseProc = (WNDPROC)SetWindowLongPtr(m_hWnd, GWLP_WNDPROC, (LONG_PTR)ListViewProc);
	//SetWindowLongPtr(m_hWnd, GWLP_USERDATA, (LONG_PTR)this);
	//SetWindowSubclass(m_hWnd, ListViewProc, 0, (DWORD_PTR)this);
	//return DefSubclassProc(hDlg, uMsg, wParam, lParam);

	const int DLG_OK_BUTTON = 1;
	const int DLG_CANCEL_BUTTON = 2;
	const int DLG_WINDOWED_CHECKBOX = 1002;

	// Anything in the 2000s should be safe to use fo my own IDs
	const int DLG_PARAMS_LABEL = 2000;
	const int DLG_PARAMS_BOX = 2001;

	if (uMsg == WM_INITDIALOG)
	{
		INT_PTR result = CallWindowProcA(reinterpret_cast<WNDPROC>(0x0047B060), hDlg, uMsg, wParam, lParam);

		SetWindowPos(hDlg, 0, 100, 100, 420, 410, SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER);

		HWND hRenderDeviceLabel = GetDlgItem(hDlg, 1011);
		HFONT hFont = (HFONT)SendMessageA(hRenderDeviceLabel, WM_GETFONT, 0, 0);
		HINSTANCE hInst = (HINSTANCE)GetWindowLongA(hDlg, GWL_HINSTANCE);

		HWND hProcessLabel = CreateWindowExA(0, "STATIC", "Parameters (Kain2.arg)", WS_CHILD | WS_VISIBLE,
			10, 250, 200, 20,
			hDlg, (HMENU)DLG_PARAMS_LABEL, hInst, 0
		);
		SendDlgItemMessageA(hDlg, DLG_PARAMS_LABEL, WM_SETFONT, (WPARAM)hFont, 0);

		HWND hParamsBox = CreateWindowExA(
			WS_EX_CLIENTEDGE, "EDIT", 0, WS_CHILD | WS_VISIBLE | ES_LEFT | ES_MULTILINE,
			10, 270, 390, 50, hDlg, (HMENU)DLG_PARAMS_BOX, 0, 0
		);
		SendDlgItemMessageA(hDlg, DLG_PARAMS_BOX, WM_SETFONT, (WPARAM)hFont, 0);
		SendDlgItemMessageA(hDlg, DLG_PARAMS_BOX, EM_LIMITTEXT, 256, 0);

		FILE* paramsFile;
		paramsFile = fopen("kain2.arg", "r");
		if (paramsFile)
		{
			fseek(paramsFile, 0, SEEK_END);
			long size = ftell(paramsFile);
			fseek(paramsFile, 0, SEEK_SET);
			if (size > 255) size = 255;

			char szParams[256];
			memset(szParams, 0, 256);
			fread(szParams, size, 1, paramsFile);
			fclose(paramsFile);
			SetDlgItemTextA(hDlg, DLG_PARAMS_BOX, szParams);
		}

		HWND hOKButton = GetDlgItem(hDlg, DLG_OK_BUTTON);
		HWND hCancelButton = GetDlgItem(hDlg, DLG_CANCEL_BUTTON);

		RECT rect;

		GetWindowRect(hOKButton, &rect);
		MapWindowPoints(HWND_DESKTOP, hDlg, (LPPOINT)&rect, 2);
		SetWindowPos(hOKButton, 0, /*rect.left*/ 10, /*rect.top + 200*/ 340, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);

		GetWindowRect(hCancelButton, &rect);
		MapWindowPoints(HWND_DESKTOP, hDlg, (LPPOINT)&rect, 2);
		SetWindowPos(hCancelButton, 0, /*rect.left*/ 100, /*rect.top + 200*/ 340, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);

		return result;
	}

	if (uMsg == WM_COMMAND)
	{
		switch (HIWORD(wParam))
		{
		case BN_CLICKED:
		{
			switch (LOWORD(wParam))
			{
			case DLG_OK_BUTTON:
			{
				INT_PTR result = CallWindowProc(reinterpret_cast<WNDPROC>(0x0047B060), hDlg, uMsg, wParam, lParam);
				char* szParams = reinterpret_cast<char*>(0x00C651E0);
				memset(szParams, 0, 256);
				GetDlgItemTextA(hDlg, DLG_PARAMS_BOX, szParams, 256);

				int* registryValues = reinterpret_cast<int*>(0x00C53C58);
				int* windowedValue = reinterpret_cast<int*>(*registryValues + 28);
				if (IsDlgButtonChecked(hDlg, DLG_WINDOWED_CHECKBOX) == BST_CHECKED)
				{
					*windowedValue = TRUE;
				}
				else
				{
					*windowedValue = FALSE;
				}
				return result;
			}
			case DLG_WINDOWED_CHECKBOX:
			{
				// char* cWindowedFlag = reinterpret_cast<char*>(0x00C53C74); // dword_C53C58 + 28
				//int oldState = IsDlgButtonChecked(hDlg, DLG_WINDOWED_CHECKBOX);
				//CheckDlgButton(hDlg, DLG_WINDOWED_CHECKBOX, (oldState != BST_CHECKED) ? BST_CHECKED : BST_UNCHECKED);
				return TRUE;
			}
			default:
				break;
			}
			break;
		}
		default:
			break;
		}
	}

	return CallWindowProc(reinterpret_cast<WNDPROC>(0x0047B060), hDlg, uMsg, wParam, lParam);
}

__declspec(dllexport) void Initialise()
{
	int* functionPointer = reinterpret_cast<int*>(0x0047AF01);
	*functionPointer = reinterpret_cast<int>(&DialogProc);

	char* dialogIdPointer = reinterpret_cast<char*>(0x0047AF0D);
	*dialogIdPointer = 102;

	char* windowStyleExPointer = reinterpret_cast<char*>(0x0047B86E);
	*windowStyleExPointer = 0;

	char* neverWindowedPointer = reinterpret_cast<char*>(0x00475563);
	*neverWindowedPointer = 0xEB;

	HANDLE process = GetCurrentProcess();
	DWORD_PTR processAffinityMask = 0x0000000F;
	BOOL success = SetProcessAffinityMask(process, processAffinityMask);

	LoadFunctions();
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
	)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		Initialise();
		break;

	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}