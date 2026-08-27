#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>
#include <memory>
#include "WaterShaderClass.h"

using namespace DirectX;

class WaterClass
{
private:
    struct VertexType
    {
        XMFLOAT3 position;
        XMFLOAT2 texture;
        XMFLOAT3 normal;
    };

public:
    WaterClass();
    ~WaterClass();

    bool Initialize(ID3D11Device* device, HWND hwnd, float width = 28.0f, float height = 28.0f, int gridSegments = 40);
    void Shutdown();
    void Render(ID3D11DeviceContext* deviceContext,
                const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix,
                const XMFLOAT3& cameraPos, float gameTime,
                const XMFLOAT4& diffuseColor, const XMFLOAT3& lightDir,
                const XMFLOAT4& specularColor, float specularPower,
                const XMFLOAT4& fogColor, float fogStart, float fogEnd, bool fogEnabled);

    // 파라미터 Getter / Setter (ImGui 실시간 제어 연동)
    bool IsEnabled() const { return m_enabled; }
    void SetEnabled(bool enabled) { m_enabled = enabled; }

    XMFLOAT3& GetPosition() { return m_position; }
    float& GetScale() { return m_scale; }
    XMFLOAT4& GetDeepColor() { return m_deepColor; }
    XMFLOAT4& GetShallowColor() { return m_shallowColor; }
    float& GetWaveSpeed() { return m_waveSpeed; }
    float& GetWaveHeight() { return m_waveHeight; }
    float& GetWaveFrequency() { return m_waveFrequency; }
    float& GetWaterAlpha() { return m_waterAlpha; }

private:
    bool InitializeBuffers(ID3D11Device* device, float width, float height, int gridSegments);
    void ShutdownBuffers();

private:
    ID3D11Buffer* m_vertexBuffer = nullptr;
    ID3D11Buffer* m_indexBuffer = nullptr;
    int m_vertexCount = 0;
    int m_indexCount = 0;

    ID3D11ShaderResourceView* m_normalTexture = nullptr;
    std::unique_ptr<WaterShaderClass> m_waterShader;

    // 수면 제어 파라미터
    bool     m_enabled = true;
    XMFLOAT3 m_position = XMFLOAT3(-14.0f, 0.05f, 9.0f); // 서쪽 아늑한 연못 최적 위치
    float    m_scale = 1.0f;
    XMFLOAT4 m_deepColor = XMFLOAT4(0.04f, 0.30f, 0.44f, 1.0f);     // 맑고 시원한 깊은 에메랄드빛
    XMFLOAT4 m_shallowColor = XMFLOAT4(0.28f, 0.76f, 0.88f, 1.0f);  // 햇빛을 머금은 밝고 영롱한 청록빛
    float    m_waveSpeed = 0.75f;
    float    m_waveHeight = 0.035f; // 아담한 연못에 어울리는 잔잔하고 부드러운 파고
    float    m_waveFrequency = 1.4f;
    float    m_waterAlpha = 0.85f;
};