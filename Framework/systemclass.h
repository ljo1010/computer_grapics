////////////////////////////////////////////////////////////////////////////////
// Filename: systemclass.h
////////////////////////////////////////////////////////////////////////////////
#ifndef _SYSTEMCLASS_H_
#define _SYSTEMCLASS_H_

///////////////////////////////
// PRE-PROCESSING DIRECTIVES //
///////////////////////////////
#define WIN32_LEAN_AND_MEAN

//////////////
// INCLUDES //
//////////////
#include <windows.h>

///////////////////////
// MY CLASS INCLUDES //
///////////////////////
#include "inputclass.h"
#include "graphicsclass.h"
#include "cpuclass.h"

////////////////////////////////////////////////////////////////////////////////
// Class name: SystemClass
////////////////////////////////////////////////////////////////////////////////
class SystemClass
{
public:
	SystemClass();
	SystemClass(const SystemClass&);
	~SystemClass();
	 
	bool Initialize();
	void Shutdown();
	void Run();

	LRESULT CALLBACK MessageHandler(HWND, UINT, WPARAM, LPARAM);

	// 마우스 커서 잠금(1인칭 모드) 및 해제(ImGui UI 모드) 토글
	void ToggleCursorLock();
	bool IsCursorLocked() const { return m_isCursorLocked; }

private:
	bool Frame();
	void InitializeWindows(int&, int&);
	void ShutdownWindows();

private:
	LPCWSTR   m_applicationName;
	HINSTANCE m_hinstance;
	HWND      m_hwnd;

	InputClass* m_Input;
	GraphicsClass* m_Graphics;
	CpuClass* m_Cpu;

	int   m_fps = 0;
	int   m_cpu = 0;   // CPU ߿ ä 

	bool  m_isCursorLocked = true; // true: 1인칭 조작 모드(커서 잠김), false: ImGui UI 조작 모드(커서 풀림)
};

/////////////////////////
// FUNCTION PROTOTYPES //
/////////////////////////
static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

/////////////
// GLOBALS //
/////////////
static SystemClass* ApplicationHandle = 0;

#endif
