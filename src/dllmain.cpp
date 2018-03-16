#include "stdafx.h"
#include <stdio.h>
#include <xinput.h>

#define PLAYSTATION3_GAMEPAD_TRIANGLE 0
#define PLAYSTATION3_GAMEPAD_CIRCLE   1
#define PLAYSTATION3_GAMEPAD_CROSS    2
#define PLAYSTATION3_GAMEPAD_SQUARE   3
#define PLAYSTATION3_GAMEPAD_L2       4
#define PLAYSTATION3_GAMEPAD_R2       5
#define PLAYSTATION3_GAMEPAD_L1       6
#define PLAYSTATION3_GAMEPAD_R1       7
#define PLAYSTATION3_GAMEPAD_SELECT   8
#define PLAYSTATION3_GAMEPAD_START    9
#define PLAYSTATION3_GAMEPAD_L3       10
#define PLAYSTATION3_GAMEPAD_R3       11
#define PLAYSTATION3_GAMEPAD_PS       12

#define PLAYSTATION4_GAMEPAD_SQUARE   0
#define PLAYSTATION4_GAMEPAD_CROSS    1
#define PLAYSTATION4_GAMEPAD_CIRCLE   2
#define PLAYSTATION4_GAMEPAD_TRIANGLE 3
#define PLAYSTATION4_GAMEPAD_L1       4
#define PLAYSTATION4_GAMEPAD_R1       5
#define PLAYSTATION4_GAMEPAD_L2       6
#define PLAYSTATION4_GAMEPAD_R2       7
#define PLAYSTATION4_GAMEPAD_SHARE    8
#define PLAYSTATION4_GAMEPAD_OPTIONS  9
#define PLAYSTATION4_GAMEPAD_L3       10
#define PLAYSTATION4_GAMEPAD_R3       11
#define PLAYSTATION4_GAMEPAD_PS       12
#define PLAYSTATION4_GAMEPAD_PAD      13

typedef struct DIJOYSTATE {
	LONG    lX;             /* x-axis position      */
	LONG    lY;             /* y-axis position      */
	LONG    lZ;             /* z-axis position      */
	LONG    lRx;            /* x-axis rotation      */
	LONG    lRy;            /* y-axis rotation      */
	LONG    lRz;            /* z-axis rotation      */
	LONG    rglSlider[2];   /* extra axes positions */
	DWORD   rgdwPOV[4];     /* POV directions       */
	BYTE    rgbButtons[32]; /* 32 buttons           */
} DIJOYSTATE, *LPDIJOYSTATE;

typedef struct DIDEVICEINSTANCEA
{
	DWORD   dwSize;
	GUID    guidInstance;
	GUID    guidProduct;
	DWORD   dwDevType;
	CHAR    tszInstanceName[MAX_PATH];
	CHAR    tszProductName[MAX_PATH];
#if(DIRECTINPUT_VERSION >= 0x0500)
	GUID    guidFFDriver;
	WORD    wUsagePage;
	WORD    wUsage;
#endif /* DIRECTINPUT_VERSION >= 0x0500 */
} DIDEVICEINSTANCEA, *LPDIDEVICEINSTANCEA;
typedef DIDEVICEINSTANCEA DIDEVICEINSTANCE;
typedef LPDIDEVICEINSTANCEA LPDIDEVICEINSTANCE;
typedef const DIDEVICEINSTANCEA* LPCDIDEVICEINSTANCEA;
typedef const DIDEVICEINSTANCE* LPCDIDEVICEINSTANCE;

typedef BOOL(FAR PASCAL* LPDIENUMDEVICESCALLBACKA)(LPCDIDEVICEINSTANCEA, LPVOID);
#define LPDIENUMDEVICESCALLBACK  LPDIENUMDEVICESCALLBACKA

typedef struct DIDEVICEOBJECTINSTANCEA
{
	DWORD   dwSize;
	GUID    guidType;
	DWORD   dwOfs;
	DWORD   dwType;
	DWORD   dwFlags;
	CHAR    tszName[MAX_PATH];
#if(DIRECTINPUT_VERSION >= 0x0500)
	DWORD   dwFFMaxForce;
	DWORD   dwFFForceResolution;
	WORD    wCollectionNumber;
	WORD    wDesignatorIndex;
	WORD    wUsagePage;
	WORD    wUsage;
	DWORD   dwDimension;
	WORD    wExponent;
	WORD    wReportId;
#endif /* DIRECTINPUT_VERSION >= 0x0500 */
} DIDEVICEOBJECTINSTANCEA, *LPDIDEVICEOBJECTINSTANCEA;
typedef DIDEVICEOBJECTINSTANCEA DIDEVICEOBJECTINSTANCE;
typedef LPDIDEVICEOBJECTINSTANCEA LPDIDEVICEOBJECTINSTANCE;
typedef const DIDEVICEOBJECTINSTANCEA* LPCDIDEVICEOBJECTINSTANCEA;
typedef const DIDEVICEOBJECTINSTANCE* LPCDIDEVICEOBJECTINSTANCE;

typedef BOOL(FAR PASCAL* LPDIENUMDEVICEOBJECTSCALLBACKA)(LPCDIDEVICEOBJECTINSTANCEA, LPVOID);
#define LPDIENUMDEVICEOBJECTSCALLBACK  LPDIENUMDEVICEOBJECTSCALLBACKA

BOOL SupportsXInput(const GUID* guid)
{
	UINT i, count = 0;
	RAWINPUTDEVICELIST* ridl;
	BOOL result = FALSE;

	if (GetRawInputDeviceList(NULL, &count, sizeof(RAWINPUTDEVICELIST)) != 0)
		return FALSE;

	ridl = new RAWINPUTDEVICELIST[count];

	if (GetRawInputDeviceList(ridl, &count, sizeof(RAWINPUTDEVICELIST)) == (UINT)-1)
	{
		delete[] ridl;
		return FALSE;
	}

	for (i = 0; i < count; i++)
	{
		RID_DEVICE_INFO rdi;
		char name[256];
		UINT size;

		if (ridl[i].dwType != RIM_TYPEHID)
			continue;

		ZeroMemory(&rdi, sizeof(rdi));
		rdi.cbSize = sizeof(rdi);
		size = sizeof(rdi);

		if ((INT)GetRawInputDeviceInfoA(ridl[i].hDevice,
			RIDI_DEVICEINFO,
			&rdi, &size) == -1)
		{
			continue;
		}

		if (MAKELONG(rdi.hid.dwVendorId, rdi.hid.dwProductId) != (LONG)guid->Data1)
			continue;

		memset(name, 0, sizeof(name));
		size = sizeof(name);

		if ((INT)GetRawInputDeviceInfoA(ridl[i].hDevice,
			RIDI_DEVICENAME,
			name, &size) == -1)
		{
			break;
		}

		name[sizeof(name) - 1] = '\0';
		if (strstr(name, "IG_"))
		{
			result = TRUE;
			break;
		}
	}

	delete[] ridl;
	return result;
}

FARPROC p[7] = { 0 };

bool ps3PadFound = false;
bool ps4PadFound = false;
bool xBox360PadFound = false;
int xBox360PadSlot = -1;

template<typename T>
void WriteToAddress(int address, T value)
{
	HANDLE process = GetCurrentProcess();
	T* pointer = reinterpret_cast<T*>(address);
	DWORD oldProtect, newProtect;
	VirtualProtectEx(process, pointer, sizeof(T), PAGE_EXECUTE_WRITECOPY, &oldProtect);
	*pointer = value;
	VirtualProtectEx(process, pointer, sizeof(T), oldProtect, &newProtect);
}

//DirectInputCreateA
extern "C" __declspec(naked) void __stdcall __E__0__()
{
	__asm jmp p[0];
}

// DirectInputCreateEx
extern "C" __declspec(naked) void __stdcall __E__1__()
{
	__asm jmp p[1];
}

// DirectInputCreateW
extern "C" __declspec(naked) void __stdcall __E__2__()
{
	__asm jmp p[2];
}

// DllCanUnloadNow
extern "C" __declspec(naked) void __stdcall __E__3__()
{
	__asm jmp p[3];
}

// DllGetClassObject
extern "C" __declspec(naked) void __stdcall __E__4__()
{
	__asm jmp p[4];
}

//DllRegisterServer
extern "C" __declspec(naked) void __stdcall __E__5__()
{
	__asm jmp p[5];
}

//DllUnregisterServer
extern "C" __declspec(naked) void __stdcall __E__6__()
{
	__asm jmp p[6];
}

__declspec(dllexport) void __stdcall SoulReaverGamePadFix()
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

	if (xBox360PadFound)
	{
		XINPUT_STATE state;
		ZeroMemory(&state, sizeof(XINPUT_STATE));
		XInputGetState(xBox360PadSlot, &state);

		WORD buttons = state.Gamepad.wButtons;

		CHAR up = ((buttons & XINPUT_GAMEPAD_DPAD_UP) != 0) ? -1 : 0;
		CHAR down = ((buttons & XINPUT_GAMEPAD_DPAD_DOWN) != 0) ? 1 : 0;
		CHAR left = ((buttons & XINPUT_GAMEPAD_DPAD_LEFT) != 0) ? -1 : 0;
		CHAR right = ((buttons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0) ? 1 : 0;
		BYTE jump = ((buttons & XINPUT_GAMEPAD_A) != 0) ? 128 : 0;
		BYTE devour = ((buttons & XINPUT_GAMEPAD_B) != 0) ? 128 : 0;
		BYTE action = ((buttons & XINPUT_GAMEPAD_X) != 0) ? 128 : 0;
		BYTE aim = ((buttons & XINPUT_GAMEPAD_Y) != 0) ? 128 : 0;
		BYTE crouch = ((buttons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0) ? 128 : 0;
		BYTE sneak = ((buttons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0) ? 128 : 0;
		BYTE glyph = ((buttons & XINPUT_GAMEPAD_BACK) != 0) ? 128 : 0;
		BYTE pause = ((buttons & XINPUT_GAMEPAD_START) != 0) ? 128 : 0;
		BYTE panLeft = (state.Gamepad.bLeftTrigger >= 128) ? 128 : 0;
		BYTE panRight = (state.Gamepad.bRightTrigger >= 128) ? 128 : 0;

		short leftStickX = state.Gamepad.sThumbLX;
		short leftStickY = state.Gamepad.sThumbLY;
		short rightStickX = state.Gamepad.sThumbRX;
		short rightStickY = state.Gamepad.sThumbRY;

		if (leftStickX < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
		{
			leftStickX = -1;
		}
		else if (leftStickX > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
		{
			leftStickX = 1;
		}
		else
		{
			leftStickX = left + right;
		}

		if (leftStickY < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
		{
			leftStickY = 1;
		}
		else if (leftStickY > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
		{
			leftStickY = -1;
		}
		else
		{
			leftStickY = up + down;
		}

		if (rightStickX < -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE)
		{
			rightStickX = -1;
		}
		else if (rightStickX > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE)
		{
			rightStickX = 1;
		}
		else
		{
			rightStickX = 0;
		}

		if (rightStickY < -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE)
		{
			rightStickY = 1;
		}
		else if (rightStickY > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE)
		{
			rightStickY = -1;
		}
		else
		{
			rightStickY = 0;
		}

		dijoystate->lX = leftStickX * 500;
		dijoystate->lY = leftStickY * 500;
		dijoystate->lZ = 0;
		dijoystate->lRx = 0;
		dijoystate->lRy = 0;
		dijoystate->lRz = 0;
		dijoystate->rglSlider[0] = 0;
		dijoystate->rglSlider[1] = 0;
		dijoystate->rgdwPOV[0] = 0;
		dijoystate->rgdwPOV[1] = 0;
		dijoystate->rgdwPOV[2] = 0;
		dijoystate->rgdwPOV[3] = 0;
		dijoystate->rgbButtons[0] = action;
		dijoystate->rgbButtons[1] = jump;
		dijoystate->rgbButtons[2] = devour;
		dijoystate->rgbButtons[3] = aim;
		dijoystate->rgbButtons[4] = crouch;
		dijoystate->rgbButtons[5] = sneak;
		dijoystate->rgbButtons[6] = panLeft;
		dijoystate->rgbButtons[7] = panRight;
		dijoystate->rgbButtons[8] = glyph;
		dijoystate->rgbButtons[9] = pause;
	}
	else if (ps3PadFound || ps4PadFound)
	{
		BYTE jump = 0;
		BYTE devour = 0;
		BYTE action = 0;
		BYTE aim = 0;
		BYTE crouch = 0;
		BYTE sneak = 0;
		BYTE glyph = 0;
		BYTE pause = 0;
		BYTE panLeft = 0;
		BYTE panRight = 0;

		if (ps3PadFound)
		{
			jump = dijoystate->rgbButtons[PLAYSTATION3_GAMEPAD_CROSS];
			devour = dijoystate->rgbButtons[PLAYSTATION3_GAMEPAD_CIRCLE];
			action = dijoystate->rgbButtons[PLAYSTATION3_GAMEPAD_SQUARE];
			aim = dijoystate->rgbButtons[PLAYSTATION3_GAMEPAD_TRIANGLE];
			crouch = dijoystate->rgbButtons[PLAYSTATION3_GAMEPAD_L1];
			sneak = dijoystate->rgbButtons[PLAYSTATION3_GAMEPAD_R1];
			glyph = dijoystate->rgbButtons[PLAYSTATION3_GAMEPAD_SELECT];
			pause = dijoystate->rgbButtons[PLAYSTATION3_GAMEPAD_START];
			panLeft = dijoystate->rgbButtons[PLAYSTATION3_GAMEPAD_L2];
			panRight = dijoystate->rgbButtons[PLAYSTATION3_GAMEPAD_R2];
		}
		else if (ps4PadFound)
		{
			jump = dijoystate->rgbButtons[PLAYSTATION4_GAMEPAD_CROSS];
			devour = dijoystate->rgbButtons[PLAYSTATION4_GAMEPAD_CIRCLE];
			action = dijoystate->rgbButtons[PLAYSTATION4_GAMEPAD_SQUARE];
			aim = dijoystate->rgbButtons[PLAYSTATION4_GAMEPAD_TRIANGLE];
			crouch = dijoystate->rgbButtons[PLAYSTATION4_GAMEPAD_L1];
			sneak = dijoystate->rgbButtons[PLAYSTATION4_GAMEPAD_R1];
			glyph = dijoystate->rgbButtons[PLAYSTATION4_GAMEPAD_SHARE];
			pause = dijoystate->rgbButtons[PLAYSTATION4_GAMEPAD_OPTIONS];
			panLeft = dijoystate->rgbButtons[PLAYSTATION4_GAMEPAD_L2];
			panRight = dijoystate->rgbButtons[PLAYSTATION4_GAMEPAD_R2];
		}

		dijoystate->lRx = 0;
		dijoystate->lRy = 0;
		dijoystate->lRz = 0;
		dijoystate->rglSlider[0] = 0;
		dijoystate->rglSlider[1] = 0;
		dijoystate->rgdwPOV[1] = 0;
		dijoystate->rgdwPOV[2] = 0;
		dijoystate->rgdwPOV[3] = 0;
		dijoystate->rgbButtons[0] = action;
		dijoystate->rgbButtons[1] = jump;
		dijoystate->rgbButtons[2] = devour;
		dijoystate->rgbButtons[3] = aim;
		dijoystate->rgbButtons[4] = crouch;
		dijoystate->rgbButtons[5] = sneak;
		dijoystate->rgbButtons[6] = panLeft;
		dijoystate->rgbButtons[7] = panRight;
		dijoystate->rgbButtons[8] = glyph;
		dijoystate->rgbButtons[9] = pause;

		if (dijoystate->rgdwPOV[0] == 22500 || dijoystate->rgdwPOV[0] == 27000 || dijoystate->rgdwPOV[0] == 31500)
		{
			dijoystate->lX = -500;
		}
		else if (dijoystate->rgdwPOV[0] == 4500 || dijoystate->rgdwPOV[0] == 9000 || dijoystate->rgdwPOV[0] == 13500)
		{
			dijoystate->lX = 500;
		}
		else if (dijoystate->lX > -500 && dijoystate->lX < 500)
		{
			dijoystate->lX = 0;
		}

		if (dijoystate->rgdwPOV[0] == 0 || dijoystate->rgdwPOV[0] == 4500 || dijoystate->rgdwPOV[0] == 31500)
		{
			dijoystate->lY = -500;
		}
		else if (dijoystate->rgdwPOV[0] == 13500 || dijoystate->rgdwPOV[0] == 18000 || dijoystate->rgdwPOV[0] == 22500)
		{
			dijoystate->lY = 500;
		}
		else if (dijoystate->lY > -500 && dijoystate->lY < 500)
		{
			dijoystate->lY = 0;
		}

		dijoystate->lZ = 0;

		dijoystate->rgdwPOV[0] = 0;
	}
	else
	{
		dijoystate->lX = 0;
		dijoystate->lY = 0;
		dijoystate->lZ = 0;
		dijoystate->lRx = 0;
		dijoystate->lRy = 0;
		dijoystate->lRz = 0;
		dijoystate->rglSlider[0] = 0;
		dijoystate->rglSlider[1] = 0;
		dijoystate->rgdwPOV[0] = 0;
		dijoystate->rgdwPOV[1] = 0;
		dijoystate->rgdwPOV[2] = 0;
		dijoystate->rgdwPOV[3] = 0;
		dijoystate->rgbButtons[0] = 0;
		dijoystate->rgbButtons[1] = 0;
		dijoystate->rgbButtons[2] = 0;
		dijoystate->rgbButtons[3] = 0;
		dijoystate->rgbButtons[4] = 0;
		dijoystate->rgbButtons[5] = 0;
		dijoystate->rgbButtons[6] = 0;
		dijoystate->rgbButtons[7] = 0;
		dijoystate->rgbButtons[8] = 0;
		dijoystate->rgbButtons[9] = 0;
	}

	__asm mov eax, result;
}

__declspec(dllexport) INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	const int DLG_OK_BUTTON = 1;
	const int DLG_CANCEL_BUTTON = 2;
	const int DLG_WINDOWED_CHECKBOX = 1002;
	const int DLG_RENDER_DEVICE_COMBO = 1003;
	const int DLG_SOUND_OUTPUT_DEVICE_COMBO = 1010;
	const int DLG_RENDER_DEVICE_LABEL = 1011;
	const int DLG_SOUND_OUTPUT_DEVICE_LABEL = 1013;

	// Anything in the 2000s should be safe to use fo my own IDs
	const int DLG_PARAMS_LABEL = 2000;
	const int DLG_PARAMS_BOX = 2001;

	if (uMsg == WM_INITDIALOG)
	{
		INT_PTR result = CallWindowProcA(reinterpret_cast<WNDPROC>(0x0047B060), hDlg, uMsg, wParam, lParam);

		HINSTANCE hInst = (HINSTANCE)GetWindowLongA(hDlg, GWL_HINSTANCE);

		HWND hOKButton = GetDlgItem(hDlg, DLG_OK_BUTTON);
		HWND hCancelButton = GetDlgItem(hDlg, DLG_CANCEL_BUTTON);
		HWND hRenderDeviceCombo = GetDlgItem(hDlg, DLG_RENDER_DEVICE_COMBO);
		HWND hSoundOutputDeviceCombo = GetDlgItem(hDlg, DLG_SOUND_OUTPUT_DEVICE_COMBO);
		HWND hRenderDeviceLabel = GetDlgItem(hDlg, DLG_RENDER_DEVICE_LABEL);
		HWND hSoundOutputDeviceLabel = GetDlgItem(hDlg, DLG_SOUND_OUTPUT_DEVICE_LABEL);

		HFONT hFont = (HFONT)SendMessageA(hRenderDeviceLabel, WM_GETFONT, 0, 0);

		RECT windowRect;
		RECT clientRect;
		RECT okRect;
		RECT soundOutputDeviceComboRect;
		RECT renderDeviceLabelRect;
		RECT soundOutputDeviceLabelRect;

		GetWindowRect(hDlg, &windowRect);

		GetClientRect(hDlg, &clientRect);
		MapWindowPoints(HWND_DESKTOP, hDlg, (LPPOINT)&clientRect, 2);
		GetWindowRect(hOKButton, &okRect);
		MapWindowPoints(HWND_DESKTOP, hDlg, (LPPOINT)&okRect, 2);
		GetWindowRect(hSoundOutputDeviceCombo, &soundOutputDeviceComboRect);
		MapWindowPoints(HWND_DESKTOP, hDlg, (LPPOINT)&soundOutputDeviceComboRect, 2);
		GetWindowRect(hRenderDeviceLabel, &renderDeviceLabelRect);
		MapWindowPoints(HWND_DESKTOP, hDlg, (LPPOINT)&renderDeviceLabelRect, 2);
		GetWindowRect(hSoundOutputDeviceLabel, &soundOutputDeviceLabelRect);
		MapWindowPoints(HWND_DESKTOP, hDlg, (LPPOINT)&soundOutputDeviceLabelRect, 2);

		int windowWidth = windowRect.right - windowRect.left;
		int windowHeight = (windowRect.bottom - windowRect.top) - (clientRect.bottom - clientRect.top);
		int buttonHeight = okRect.bottom - okRect.top;
		int labelMargin = soundOutputDeviceLabelRect.left;
		int labelWidth = soundOutputDeviceLabelRect.right - soundOutputDeviceLabelRect.left;
		int labelHeight = soundOutputDeviceLabelRect.bottom - soundOutputDeviceLabelRect.top;
		int comboWidth = soundOutputDeviceComboRect.right - soundOutputDeviceComboRect.left;
		int comboHeight = soundOutputDeviceComboRect.bottom - soundOutputDeviceComboRect.top;
		int comboMargin = soundOutputDeviceComboRect.left;
		int spacing = renderDeviceLabelRect.top;
		int labelSpacing = soundOutputDeviceComboRect.top - soundOutputDeviceLabelRect.top;
		int top = soundOutputDeviceComboRect.bottom;

		top += spacing;

		HWND hParamsLabel = CreateWindowExA(0, "STATIC", "Parameters (Kain2.arg)", WS_CHILD | WS_VISIBLE,
			labelMargin, top, labelWidth, labelHeight,
			hDlg, (HMENU)DLG_PARAMS_LABEL, hInst, 0
		);
		SendDlgItemMessageA(hDlg, DLG_PARAMS_LABEL, WM_SETFONT, (WPARAM)hFont, 0);

		top += labelSpacing;

		HWND hParamsBox = CreateWindowExA(
			WS_EX_CLIENTEDGE, "EDIT", 0, WS_CHILD | WS_VISIBLE | ES_LEFT | ES_MULTILINE,
			comboMargin, top, comboWidth, (5 * comboHeight) / 2, hDlg, (HMENU)DLG_PARAMS_BOX, 0, 0
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

		top += (5 * comboHeight) / 2;

		top += spacing;

		SetWindowPos(hOKButton, 0, comboMargin, top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);
		SetWindowPos(hCancelButton, 0, comboMargin + okRect.right, top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);

		top += buttonHeight;

		top += spacing;

		top += windowHeight;

		SetWindowPos(hDlg, 0, 0, 0, windowWidth, top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER);

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

__declspec(dllexport) BOOL PASCAL EvalDevice(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef)
{
	DWORD oldJoystick = *reinterpret_cast<DWORD*>(0x00C52FB0);
	LPDIENUMDEVICESCALLBACK enumDevicesPointer = reinterpret_cast<LPDIENUMDEVICESCALLBACK>(0x00478460);
	BOOL result = enumDevicesPointer(lpddi, pvRef);
	DWORD newJoystick = *reinterpret_cast<DWORD*>(0x00C52FB0);

	if (oldJoystick == 0 && newJoystick != 0)
	{
		// 02140e6f-0000-0000-0000-504944564944
		GUID ps3GUID = { 0x02140e6f, 0, 0, { '\0', '\0', 'P', 'I', 'D', 'V', 'I', 'D' } };
		// 05c4054c-0000-0000-0000-504944564944
		GUID ps4GUID = { 0x05c4054c, 0, 0, { '\0', '\0', 'P', 'I', 'D', 'V', 'I', 'D' } };

		if (lpddi->guidProduct == ps3GUID)
		{
			ps3PadFound = true;
		}
		else if (lpddi->guidProduct == ps4GUID)
		{
			ps4PadFound = true;
		}
		else if (!SupportsXInput(&lpddi->guidProduct))
		{
			xBox360PadFound = false;
		}
	}

	return result;
}

__declspec(dllexport) BOOL PASCAL EvalObject(LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef)
{
	LPDIENUMDEVICEOBJECTSCALLBACK enumDeviceObjectsPointer = reinterpret_cast<LPDIENUMDEVICEOBJECTSCALLBACK>(0x00478700);
	return enumDeviceObjectsPointer(lpddoi, pvRef);
}

bool LoadFunctions()
{
	char * sz_buffer = new char[255];
	char * sz_systemDirectory = new char[255];
	memset(sz_systemDirectory, 0, 255);

	GetSystemDirectory(sz_systemDirectory, 255);

	sprintf(sz_buffer, "%s\\dinput.dll", sz_systemDirectory);

	HMODULE hL = LoadLibrary(sz_buffer);
	if (hL == 0)
	{
		return false;
	}

	p[0] = GetProcAddress(hL, "DirectInputCreateA");
	p[1] = GetProcAddress(hL, "DirectInputCreateEx");
	p[2] = GetProcAddress(hL, "DirectInputCreateW");
	p[3] = GetProcAddress(hL, "DllCanUnloadNow");
	p[4] = GetProcAddress(hL, "DllGetClassObject");
	p[5] = GetProcAddress(hL, "DllRegisterServer");
	p[6] = GetProcAddress(hL, "DllUnregisterServer");

	return true;
}

void InitializeDirectInput()
{
	WriteToAddress(0x004780DC, EvalDevice);
	WriteToAddress(0x00478503, EvalObject);

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

void InitializeXInput()
{
	for (DWORD i = 0; i < XUSER_MAX_COUNT && xBox360PadSlot == -1; i++)
	{
		XINPUT_STATE state;
		ZeroMemory(&state, sizeof(XINPUT_STATE));
		if (XInputGetState(i, &state) == ERROR_SUCCESS)
		{
			xBox360PadSlot = i;
		}
	}

	if (xBox360PadSlot != -1)
	{
		xBox360PadFound = true;
	}
}

__declspec(dllexport) void Initialize()
{
	WriteToAddress(0x0047AF01, DialogProc); // Setup Dialog Function
	WriteToAddress(0x0047AF0D, (char)102);  // Setup Dialog ID
	WriteToAddress(0x0047B86E, (char)0);    // Setup Dialog WindowStyleEx
	WriteToAddress(0x00475563, (BYTE)0xEB); // Remove always fullscreen developer hack

	// Cursor
	//char* showCursorPointer = reinterpret_cast<char*>(0x0047BA89);
	//*showCursorPointer = 1;
	//char* setCursorPointer = reinterpret_cast<char*>(0x0047BA91);
	//*setCursorPointer = 1;

	HANDLE process = GetCurrentProcess();
	DWORD_PTR processAffinityMask = 0x0000000F;
	BOOL success = SetProcessAffinityMask(process, processAffinityMask);

	if (LoadFunctions())
	{
		InitializeDirectInput();
		InitializeXInput();
	}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		Initialize();
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		break;
	}

	return TRUE;
}