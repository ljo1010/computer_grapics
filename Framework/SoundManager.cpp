#include "SoundManager.h"
#include "Event.h"
#include "EventBus.h"
#include <stdio.h>

SoundManager::SoundManager()
{
}

SoundManager::~SoundManager()
{
    Shutdown();
}

bool SoundManager::Initialize(HWND hwnd)
{
    if (!InitializeDirectSound(hwnd))
    {
        return false;
    }

    // 1. 배경음악 및 효과음 로드
    LoadWaveFile("./data/Lofi-bgm.wav", &m_bgmBuffer);
    LoadWaveFile("./data/throw.wav", &m_throwBuffer);
    LoadWaveFile("./data/feed.wav", &m_feedBuffer);
    LoadWaveFile("./data/complete.wav", &m_completeBuffer);

    // 2. [옵저버 패턴] 이벤트 버스 구독 (게임플레이 이벤트에 자동 사운드 반응)
    EventBus::Get().Subscribe<HayThrownEvent>([this](const HayThrownEvent&) {
        PlayThrowSound();
    });

    EventBus::Get().Subscribe<AnimalFedEvent>([this](const AnimalFedEvent& e) {
        if (e.isAllCompleted)
        {
            PlayCompleteSound();
        }
        else
        {
            PlayFeedSound();
        }
    });

    // 3. 배경음악 무한 루프 재생 시작
    PlayBGM(true);

    return true;
}

void SoundManager::Shutdown()
{
    StopBGM();

    ShutdownWaveBuffer(&m_completeBuffer);
    ShutdownWaveBuffer(&m_feedBuffer);
    ShutdownWaveBuffer(&m_throwBuffer);
    ShutdownWaveBuffer(&m_bgmBuffer);

    ShutdownDirectSound();
}

void SoundManager::PlayBGM(bool loop)
{
    if (m_isMuted || !m_bgmBuffer) return;
    m_bgmBuffer->SetCurrentPosition(0);
    m_bgmBuffer->SetVolume(DSBVOLUME_MIN + 7500); // 편안한 배경음악 음량 (약 -25dB)
    m_bgmBuffer->Play(0, 0, loop ? DSBPLAY_LOOPING : 0);
}

void SoundManager::StopBGM()
{
    if (m_bgmBuffer)
    {
        m_bgmBuffer->Stop();
    }
}

void SoundManager::PlayThrowSound()
{
    if (m_isMuted || !m_throwBuffer) return;
    PlayBuffer(m_throwBuffer, false);
}

void SoundManager::PlayFeedSound()
{
    if (m_isMuted || !m_feedBuffer) return;
    PlayBuffer(m_feedBuffer, false);
}

void SoundManager::PlayCompleteSound()
{
    if (m_isMuted || !m_completeBuffer) return;
    PlayBuffer(m_completeBuffer, false);
}

void SoundManager::PlayBuffer(IDirectSoundBuffer8* buffer, bool loop)
{
    if (!buffer) return;
    buffer->SetCurrentPosition(0);
    buffer->SetVolume(DSBVOLUME_MAX - 600); // 맑고 또렷한 효과음 볼륨
    buffer->Play(0, 0, loop ? DSBPLAY_LOOPING : 0);
}

void SoundManager::SetMuted(bool mute)
{
    m_isMuted = mute;
    if (m_isMuted)
    {
        StopBGM();
    }
    else
    {
        PlayBGM(true);
    }
}

bool SoundManager::InitializeDirectSound(HWND hwnd)
{
    HRESULT hr = DirectSoundCreate8(NULL, &m_DirectSound, NULL);
    if (FAILED(hr)) return false;

    hr = m_DirectSound->SetCooperativeLevel(hwnd, DSSCL_PRIORITY);
    if (FAILED(hr)) return false;

    // 프라이머리 버퍼 설정
    DSBUFFERDESC bufferDesc{};
    bufferDesc.dwSize = sizeof(DSBUFFERDESC);
    bufferDesc.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRLVOLUME;

    hr = m_DirectSound->CreateSoundBuffer(&bufferDesc, &m_primaryBuffer, NULL);
    if (FAILED(hr)) return false;

    WAVEFORMATEX waveFormat{};
    waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    waveFormat.nSamplesPerSec = 44100;
    waveFormat.wBitsPerSample = 16;
    waveFormat.nChannels = 2;
    waveFormat.nBlockAlign = (waveFormat.wBitsPerSample / 8) * waveFormat.nChannels;
    waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;

    hr = m_primaryBuffer->SetFormat(&waveFormat);
    if (FAILED(hr)) return false;

    return true;
}

void SoundManager::ShutdownDirectSound()
{
    if (m_primaryBuffer)
    {
        m_primaryBuffer->Release();
        m_primaryBuffer = nullptr;
    }

    if (m_DirectSound)
    {
        m_DirectSound->Release();
        m_DirectSound = nullptr;
    }
}

bool SoundManager::LoadWaveFile(const char* filename, IDirectSoundBuffer8** secondaryBuffer)
{
    FILE* filePtr = nullptr;
    fopen_s(&filePtr, filename, "rb");
    if (!filePtr) return false;

    WaveHeaderType waveFileHeader;
    size_t count = fread(&waveFileHeader, sizeof(waveFileHeader), 1, filePtr);
    if (count != 1) { fclose(filePtr); return false; }

    // RIFF 및 WAVE 헤더 검사
    if ((waveFileHeader.chunkId[0] != 'R') || (waveFileHeader.chunkId[1] != 'I') ||
        (waveFileHeader.chunkId[2] != 'F') || (waveFileHeader.chunkId[3] != 'F') ||
        (waveFileHeader.format[0] != 'W') || (waveFileHeader.format[1] != 'A') ||
        (waveFileHeader.format[2] != 'V') || (waveFileHeader.format[3] != 'E'))
    {
        fclose(filePtr);
        return false;
    }

    WAVEFORMATEX waveFormat{};
    waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    waveFormat.nSamplesPerSec = waveFileHeader.sampleRate;
    waveFormat.wBitsPerSample = waveFileHeader.bitsPerSample;
    waveFormat.nChannels = waveFileHeader.numChannels;
    waveFormat.nBlockAlign = (waveFormat.wBitsPerSample / 8) * waveFormat.nChannels;
    waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;

    DSBUFFERDESC bufferDesc{};
    bufferDesc.dwSize = sizeof(DSBUFFERDESC);
    bufferDesc.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_GLOBALFOCUS;
    bufferDesc.dwBufferBytes = waveFileHeader.dataSize;
    bufferDesc.lpwfxFormat = &waveFormat;

    IDirectSoundBuffer* tempBuffer = nullptr;
    HRESULT hr = m_DirectSound->CreateSoundBuffer(&bufferDesc, &tempBuffer, NULL);
    if (FAILED(hr)) { fclose(filePtr); return false; }

    hr = tempBuffer->QueryInterface(IID_IDirectSoundBuffer8, (void**)secondaryBuffer);
    tempBuffer->Release();
    if (FAILED(hr)) { fclose(filePtr); return false; }

    unsigned char* waveData = new unsigned char[waveFileHeader.dataSize];
    fseek(filePtr, sizeof(WaveHeaderType), SEEK_SET);
    fread(waveData, 1, waveFileHeader.dataSize, filePtr);
    fclose(filePtr);

    unsigned char* bufferPtr = nullptr;
    unsigned long bufferSize = 0;
    hr = (*secondaryBuffer)->Lock(0, waveFileHeader.dataSize, (void**)&bufferPtr, (DWORD*)&bufferSize, NULL, 0, 0);
    if (FAILED(hr)) { delete[] waveData; return false; }

    memcpy(bufferPtr, waveData, waveFileHeader.dataSize);
    (*secondaryBuffer)->Unlock((void*)bufferPtr, bufferSize, NULL, 0);

    delete[] waveData;
    return true;
}

void SoundManager::ShutdownWaveBuffer(IDirectSoundBuffer8** buffer)
{
    if (buffer && *buffer)
    {
        (*buffer)->Release();
        *buffer = nullptr;
    }
}