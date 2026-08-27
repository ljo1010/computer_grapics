#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;

class WaterShaderClass
{
private:
    struct MatrixBufferType
    {
        XMMATRIX world;
        XMMATRIX view;
        XMMATRIX projection;
    };

    struct WaterBufferType
    {
        XMFLOAT4 deepColor;
        XMFLOAT4 shallowColor;
        XMFLOAT3 cameraPosition;
        float gameTime;
        float waveSpeed;
        float waveHeight;
        float waveFrequency;
        float waterAlpha;
    };

    struct LightBufferType
    {
        XMFLOAT4 lightDiffuseColor;
        XMFLOAT3 lightDirection;
        float specularPower;
        XMFLOAT4 specularColor;
        XMFLOAT4 fogColor;
        float fogStart;
        float fogEnd;
        int fogEnabled;
        float padding;
    };

public:
    WaterShaderClass();
    ~WaterShaderClass();

    bool Initialize(ID3D11Device* device, HWND hwnd);
    void Shutdown();
    bool Render(ID3D11DeviceContext* deviceContext, int indexCount,
                const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix,
                ID3D11ShaderResourceView* normalTexture,
                const XMFLOAT3& cameraPos, float gameTime,
                const XMFLOAT4& deepColor, const XMFLOAT4& shallowColor,
                float waveSpeed, float waveHeight, float waveFrequency, float waterAlpha,
                const XMFLOAT4& diffuseColor, const XMFLOAT3& lightDir,
                const XMFLOAT4& specularColor, float specularPower,
                const XMFLOAT4& fogColor, float fogStart, float fogEnd, bool fogEnabled);

private:
    bool InitializeShader(ID3D11Device* device, HWND hwnd, const WCHAR* vsFilename, const WCHAR* psFilename);
    void ShutdownShader();

private:
    ID3D11VertexShader* m_vertexShader = nullptr;
    ID3D11PixelShader* m_pixelShader = nullptr;
    ID3D11InputLayout* m_layout = nullptr;
    ID3D11SamplerState* m_sampleState = nullptr;

    ID3D11Buffer* m_matrixBuffer = nullptr;
    ID3D11Buffer* m_waterBuffer = nullptr;
    ID3D11Buffer* m_lightBuffer = nullptr;
};