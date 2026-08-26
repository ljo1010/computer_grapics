////////////////////////////////////////////////////////////////////////////////
// Filename: inputclass.h
////////////////////////////////////////////////////////////////////////////////
#ifndef _INPUTCLASS_H_
#define _INPUTCLASS_H_

///////////////////////////////
// PRE-PROCESSING DIRECTIVES //
///////////////////////////////
#define DIRECTINPUT_VERSION 0x0800

/////////////
// LINKING //
/////////////
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "xaudio2.lib")

//////////////
// INCLUDES //
//////////////
#include <dinput.h>
#include <xaudio2.h>
#include <vector>
#include <windows.h>

////////////////////////////////////////////////////////////////////////////////
// Class name: InputClass
////////////////////////////////////////////////////////////////////////////////
class InputClass
{
public:
	InputClass();
	InputClass(const InputClass&);
	~InputClass();

	// 초기화/종료
	bool InitializeDirectInput(HINSTANCE, HWND, int, int);
	void ShutdownDirectInput();
	bool Frame();

	// 키보드/마우스
	bool IsEscapePressed() const;
	void GetMouseLocation(int&, int&) const;
	bool IsDIKeyDown(unsigned int dik) const;
	void GetMouseDelta(int& dx, int& dy) const;

	// 간단 키 배열
	void Initialize();
	void KeyDown(unsigned int);
	void KeyUp(unsigned int);
	bool IsKeyDown(unsigned int);

	// 사운드
	bool InitializeSound(HWND hwnd, const wchar_t* wavPath);
	void ShutdownSound();
	bool PlayBGM(bool loop);
	void StopBGM();


private:
	bool ReadKeyboard();
	bool ReadMouse();
	void ProcessInput();

	bool LoadWavFile(const wchar_t* wavPath, WAVEFORMATEX& wfx, std::vector<BYTE>& audioData);

private:
	// DirectInput
	IDirectInput8* m_directInput;
	IDirectInputDevice8* m_keyboard;
	IDirectInputDevice8* m_mouse;

	unsigned char m_keyboardState[256];
	DIMOUSESTATE m_mouseState;

	int m_screenWidth, m_screenHeight;
	int m_mouseX, m_mouseY;

	// 단순 키 상태
	bool m_keys[256];

	// XAudio2
	IXAudio2* m_xaudio2;
	IXAudio2MasteringVoice* m_masterVoice;
	IXAudio2SourceVoice* m_bgmVoice;
	WAVEFORMATEX            m_wfx;
	XAUDIO2_BUFFER          m_xaBuffer;
	std::vector<BYTE>       m_audioData;
};

#endif
