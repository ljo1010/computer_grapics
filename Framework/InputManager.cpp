#include "pch.h"
#include "InputManager.h"

HRESULT InputManager::Init()
{
	InitDirectInput(_hInstance, _hWnd);
	return S_OK;
}

void InputManager::release(void)
{
}

bool InputManager::InitDirectInput(HINSTANCE hInstance, HWND hwnd)
{
	HRESULT hr;

	hr = DirectInput8Create(hInstance,
		DIRECTINPUT_VERSION,
		IID_IDirectInput8,
		(void**)&DirectInput,
		NULL);

	hr = DirectInput->CreateDevice(GUID_SysKeyboard,
		&DIKeyboard,
		NULL);

	hr = DirectInput->CreateDevice(GUID_SysMouse,
		&DIMouse,
		NULL);

	hr = DIKeyboard->SetDataFormat(&c_dfDIKeyboard);
	hr = DIKeyboard->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);

	hr = DIMouse->SetDataFormat(&c_dfDIMouse);
	hr = DIMouse->SetCooperativeLevel(hwnd, DISCL_EXCLUSIVE | DISCL_NOWINKEY | DISCL_FOREGROUND);

	return true;
}

void InputManager::DetectInput()
{
	DIMOUSESTATE mouseCurrState;

	BYTE keyboardState[256];

	DIKeyboard->Acquire();
	DIMouse->Acquire();

	DIMouse->GetDeviceState(sizeof(DIMOUSESTATE), &mouseCurrState);

	DIKeyboard->GetDeviceState(sizeof(keyboardState), (LPVOID)&keyboardState);

	if (keyboardState[DIK_ESCAPE] & 0x80)
		PostMessage(_hWnd, WM_DESTROY, 0, 0);

	if (keyboardState[DIK_A] & 0x80) //аб
	{
		m_Horizontal = -1;
	}
	else if (keyboardState[DIK_D] & 0x80) //©Л
	{
		m_Horizontal = 1;
	}
	else m_Horizontal = 0;

	if (keyboardState[DIK_W] & 0x80) //╩С
	{
		m_Vertical = 1;
	}
	else if (keyboardState[DIK_S] & 0x80) //го
	{
		m_Vertical = -1;
	}
	else m_Vertical = 0;

	if (keyboardState[DIK_LSHIFT] & 0x80)
	{
		m_Shift = 1;
	}
	else m_Shift = 0;

	if ((mouseCurrState.lX != mouseLastState.lX) || (mouseCurrState.lY != mouseLastState.lY))
	{
		m_Yaw += mouseLastState.lX * 0.01f;

		m_Pitch += mouseCurrState.lY * 0.01f;

		mouseLastState = mouseCurrState;
	}

	return;
}
