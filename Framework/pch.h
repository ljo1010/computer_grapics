#pragma once

#include <wrl.h>
/////////////
// LINKING //
/////////////
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

//////////////
// INCLUDES //
//////////////
#include <dxgi.h>
#include <d3dcommon.h>
#include <d3d11.h>
#include <directxmath.h>
#include <d3dcompiler.h>
using namespace DirectX;

#include <iostream>
#include <string>
#include <locale>
#include <cstdlib>
#include <vector>
#include <list>
#include <map>
#include <atlstr.h>
#include <atlconv.h>
using namespace std;

#define DIRECTINPUT_VERSION 0x0800

//using namespace Microsoft::WRL;

#define WM_UPDATE_COMMAND	WM_USER + 4
//====================================================================
//			## 메크로함수 ## (클래스 동적할당된 부분 해제)
//====================================================================
#define SAFE_DELETE(p)			{if(p) {delete(p); (p) = NULL;}}
#define SAFE_RELEASE(p)			{if(p) {(p)->Release(); (p) = NULL;}}

//====================================================================
//			## 매크로 변수 ##
//====================================================================
#define WINSIZEX	1600
#define WINSIZEY	900

//====================================================================
//			## 싱글톤으로 사용되는 매니저 ##
//====================================================================
#include "DeviceManager.h"


#include "RMTexture.h"
#include "RMShader.h"
#include "RMModel.h"
#include "TimeManager.h"
#include "SceneManager.h"
#include "MYUTIL.h"
#include "Camera.h"
#include "InputManager.h"

#define DEVICEMANAGER DeviceManager::GetSingleton()
#define RM_MODEL RMModel::GetSingleton()
#define RM_SHADER RMShader::GetSingleton()
#define RM_TEXTURE RMTexture::GetSingleton()
#define TIMEMANAGER TimeManager::GetSingleton()
#define SCENEMANAGER SceneManager::GetSingleton()
#define INPUTMANAGER InputManager::GetSingleton()

extern HWND		_hWnd;
extern Camera	_mainCam;
extern HINSTANCE _hInstance;
