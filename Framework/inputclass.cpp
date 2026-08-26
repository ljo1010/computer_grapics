////////////////////////////////////////////////////////////////////////////////
// Filename: inputclass.cpp
////////////////////////////////////////////////////////////////////////////////
#include "inputclass.h"

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <xaudio2.h>
#include <windows.h>
#include <mmreg.h>
#include <cstdio>
#include <cstring>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "xaudio2.lib")

InputClass::InputClass()
{
    m_directInput = nullptr;
    m_keyboard = nullptr;
    m_mouse = nullptr;
    std::memset(m_keyboardState, 0, sizeof(m_keyboardState));
    std::memset(&m_mouseState, 0, sizeof(m_mouseState));
    m_screenWidth = m_screenHeight = 0;
    m_mouseX = m_mouseY = 0;
    std::memset(m_keys, 0, sizeof(m_keys));

    m_xaudio2 = nullptr;
    m_masterVoice = nullptr;
    m_bgmVoice = nullptr;
    std::memset(&m_wfx, 0, sizeof(m_wfx));
    std::memset(&m_xaBuffer, 0, sizeof(m_xaBuffer));
}

InputClass::InputClass(const InputClass& other) { (void)other; }
InputClass::~InputClass() { ShutdownSound(); ShutdownDirectInput(); }

void InputClass::Initialize() { for (bool& k : m_keys) k = false; }
void InputClass::KeyDown(unsigned int key) { if (key < 256) m_keys[key] = true; }
void InputClass::KeyUp(unsigned int key) { if (key < 256) m_keys[key] = false; }
bool InputClass::IsKeyDown(unsigned int key) { return (key < 256) ? m_keys[key] : false; }

bool InputClass::InitializeDirectInput(HINSTANCE hInst, HWND hwnd, int w, int h)
{
    m_screenWidth = w; m_screenHeight = h; m_mouseX = 0; m_mouseY = 0;
    if (FAILED(DirectInput8Create(hInst, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&m_directInput, nullptr))) return false;

    if (FAILED(m_directInput->CreateDevice(GUID_SysKeyboard, &m_keyboard, nullptr))) return false;
    if (FAILED(m_keyboard->SetDataFormat(&c_dfDIKeyboard))) return false;
    //if (FAILED(m_keyboard->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_EXCLUSIVE))) return false;
    if (FAILED(m_keyboard->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE))) return false;

    if (FAILED(m_keyboard->Acquire())) return false;

    if (FAILED(m_directInput->CreateDevice(GUID_SysMouse, &m_mouse, nullptr))) return false;
    if (FAILED(m_mouse->SetDataFormat(&c_dfDIMouse))) return false;
    if (FAILED(m_mouse->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE))) return false;
    if (FAILED(m_mouse->Acquire())) return false;

    return true;
}

void InputClass::ShutdownDirectInput()
{
    if (m_mouse) { m_mouse->Unacquire(); m_mouse->Release(); m_mouse = nullptr; }
    if (m_keyboard) { m_keyboard->Unacquire(); m_keyboard->Release(); m_keyboard = nullptr; }
    if (m_directInput) { m_directInput->Release(); m_directInput = nullptr; }
}

bool InputClass::Frame()
{
    if (!ReadKeyboard()) return false;
    if (!ReadMouse()) return false;
    ProcessInput();
    return true;
}

bool InputClass::ReadKeyboard()
{
    if (!m_keyboard) return false;
    HRESULT hr = m_keyboard->GetDeviceState(sizeof(m_keyboardState), (LPVOID)&m_keyboardState);
    if (FAILED(hr)) {
        if (hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED) { m_keyboard->Acquire(); return true; }
        return false;
    }
    return true;
}

bool InputClass::ReadMouse()
{
    if (!m_mouse) return false;
    HRESULT hr = m_mouse->GetDeviceState(sizeof(DIMOUSESTATE), (LPVOID)&m_mouseState);
    if (FAILED(hr)) {
        if (hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED) { m_mouse->Acquire(); return true; }
        return false;
    }
    return true;
}

void InputClass::ProcessInput()
{
    m_mouseX += m_mouseState.lX;
    m_mouseY += m_mouseState.lY;
    if (m_mouseX < 0) m_mouseX = 0;
    if (m_mouseY < 0) m_mouseY = 0;
    if (m_mouseX > m_screenWidth)  m_mouseX = m_screenWidth;
    if (m_mouseY > m_screenHeight) m_mouseY = m_screenHeight;
}

bool InputClass::IsEscapePressed() const
{
    return (m_keyboardState[DIK_ESCAPE] & 0x80) != 0;
}

void InputClass::GetMouseLocation(int& x, int& y) const
{
    x = m_mouseX; y = m_mouseY;
}

void InputClass::GetMouseDelta(int& dx, int& dy) const {
    dx = m_mouseState.lX;
    dy = m_mouseState.lY;
}

// ---------- XAudio2 ----------
bool InputClass::InitializeSound(HWND /*hwnd*/, const wchar_t* wavPath)
{
    if (FAILED(XAudio2Create(&m_xaudio2, 0))) return false;
    if (FAILED(m_xaudio2->CreateMasteringVoice(&m_masterVoice))) return false;

    if (!LoadWavFile(wavPath, m_wfx, m_audioData)) return false;

    m_xaBuffer = {};
    m_xaBuffer.AudioBytes = static_cast<UINT32>(m_audioData.size());
    m_xaBuffer.pAudioData = m_audioData.data();
    m_xaBuffer.Flags = 0;

    if (FAILED(m_xaudio2->CreateSourceVoice(&m_bgmVoice, &m_wfx))) return false;
    return true;
}

void InputClass::ShutdownSound()
{
    if (m_bgmVoice) { m_bgmVoice->Stop(); m_bgmVoice->DestroyVoice(); m_bgmVoice = nullptr; }
    if (m_masterVoice) { m_masterVoice->DestroyVoice(); m_masterVoice = nullptr; }
    if (m_xaudio2) { m_xaudio2->Release(); m_xaudio2 = nullptr; }
    m_audioData.clear();
    std::memset(&m_wfx, 0, sizeof(m_wfx));
    std::memset(&m_xaBuffer, 0, sizeof(m_xaBuffer));
}

bool InputClass::PlayBGM(bool loop)
{
    if (!m_bgmVoice) return false;

    XAUDIO2_BUFFER buf = m_xaBuffer;
    buf.LoopBegin = 0;
    buf.LoopLength = 0;

    if (loop) {
        buf.LoopCount = XAUDIO2_LOOP_INFINITE;
        buf.Flags = 0;  // 루프 버퍼
    }
    else {
        buf.LoopCount = 0;
        buf.Flags = XAUDIO2_END_OF_STREAM; // 한 번만
    }

    m_bgmVoice->FlushSourceBuffers();
    if (FAILED(m_bgmVoice->SubmitSourceBuffer(&buf))) return false;
    if (FAILED(m_bgmVoice->Start())) return false;
    return true;
}


bool InputClass::IsDIKeyDown(unsigned int dik) const
{
    // DirectInput 스캔코드(DIK_*)는 0~255 범위
    if (dik < 256) {
        return (m_keyboardState[dik] & 0x80) != 0;
    }
    return false;
}

void InputClass::StopBGM()
{
    if (m_bgmVoice) { m_bgmVoice->Stop(); m_bgmVoice->FlushSourceBuffers(); }
}

// ---------- WAV loader ----------
#pragma pack(push,1)
struct RiffHeader { char id[4]; uint32_t size; char type[4]; };
struct ChunkHeader { char id[4]; uint32_t size; };
#pragma pack(pop)

static bool read_all(FILE* f, void* dst, size_t sz) { return fread(dst, 1, sz, f) == sz; }

bool InputClass::LoadWavFile(const wchar_t* path, WAVEFORMATEX& wfx, std::vector<BYTE>& audio)
{
    FILE* fp = nullptr;
    _wfopen_s(&fp, path, L"rb");
    if (!fp) return false;

    RiffHeader riff{};
    if (!read_all(fp, &riff, sizeof(riff)) || std::strncmp(riff.id, "RIFF", 4) != 0 || std::strncmp(riff.type, "WAVE", 4) != 0) {
        fclose(fp); return false;
    }

    bool fmtFound = false, dataFound = false;
    BYTE* dataBuf = nullptr;
    uint32_t dataSize = 0;

    while (!fmtFound || !dataFound)
    {
        ChunkHeader ch{};
        if (!read_all(fp, &ch, sizeof(ch))) break;

        if (std::strncmp(ch.id, "fmt ", 4) == 0)
        {
            if (ch.size >= sizeof(WAVEFORMATEX)) {
                if (!read_all(fp, &wfx, sizeof(WAVEFORMATEX))) { fclose(fp); return false; }
                if (ch.size > sizeof(WAVEFORMATEX)) fseek(fp, long(ch.size - sizeof(WAVEFORMATEX)), SEEK_CUR);
            }
            else {
                BYTE tmp[64] = { 0 };
                size_t toRead = ch.size < sizeof(tmp) ? ch.size : sizeof(tmp);
                if (!read_all(fp, tmp, toRead)) { fclose(fp); return false; }
                std::memcpy(&wfx, tmp, toRead);
                if (ch.size > toRead) fseek(fp, long(ch.size - toRead), SEEK_CUR);
            }
            fmtFound = true;
        }
        else if (std::strncmp(ch.id, "data", 4) == 0)
        {
            dataSize = ch.size;
            dataBuf = new BYTE[dataSize];
            if (!read_all(fp, dataBuf, dataSize)) { delete[] dataBuf; fclose(fp); return false; }
            dataFound = true;
        }
        else {
            fseek(fp, long(ch.size), SEEK_CUR);
        }
    }

    fclose(fp);
    if (!fmtFound || !dataFound) { delete[] dataBuf; return false; }

    audio.assign(dataBuf, dataBuf + dataSize);
    delete[] dataBuf;
    return true;
}
