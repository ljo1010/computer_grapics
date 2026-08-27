#pragma once

#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>
#include <string>
#include <memory>

#pragma comment(lib, "dsound.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "winmm.lib")

// ============================================================================
// SoundManager: DirectSound 기반 BGM 및 이벤트 연동 효과음 관리자 (Observer Pattern)
// ============================================================================
class SoundManager
{
private:
    struct WaveHeaderType
    {
        char chunkId[4];
        unsigned long chunkSize;
        char format[4];
        char subChunkId[4];
        unsigned long subChunkSize;
        unsigned short audioFormat;
        unsigned short numChannels;
        unsigned long sampleRate;
        unsigned long bytesPerSecond;
        unsigned short blockAlign;
        unsigned short bitsPerSample;
        char dataChunkId[4];
        unsigned long dataSize;
    };

public:
    SoundManager();
    ~SoundManager();

    bool Initialize(HWND hwnd);
    void Shutdown();

    // 오디오 재생 함수
    void PlayBGM(bool loop = true);
    void StopBGM();
    void PlayThrowSound();
    void PlayFeedSound();
    void PlayCompleteSound();

    // 음소거 및 볼륨 제어 (ImGui 연동용)
    void SetMuted(bool mute);
    bool IsMuted() const { return m_isMuted; }
    void ToggleMute() { SetMuted(!m_isMuted); }

private:
    bool InitializeDirectSound(HWND hwnd);
    void ShutdownDirectSound();

    bool LoadWaveFile(const char* filename, IDirectSoundBuffer8** secondaryBuffer);
    void ShutdownWaveBuffer(IDirectSoundBuffer8** buffer);
    void PlayBuffer(IDirectSoundBuffer8* buffer, bool loop = false);

private:
    IDirectSound8* m_DirectSound = nullptr;
    IDirectSoundBuffer* m_primaryBuffer = nullptr;

    // 사운드 버퍼들
    IDirectSoundBuffer8* m_bgmBuffer = nullptr;
    IDirectSoundBuffer8* m_throwBuffer = nullptr;
    IDirectSoundBuffer8* m_feedBuffer = nullptr;
    IDirectSoundBuffer8* m_completeBuffer = nullptr;

    bool m_isMuted = false;
};