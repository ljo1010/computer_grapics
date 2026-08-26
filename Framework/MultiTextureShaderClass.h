#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include <string>

class MultiTextureShaderClass
{
public:
    MultiTextureShaderClass();
    ~MultiTextureShaderClass();

    bool Initialize(ID3D11Device* device, HWND hwnd, const std::wstring& file);
    void Shutdown();

    bool Render(
        ID3D11DeviceContext* dc,
        int indexCount,
        const DirectX::XMMATRIX& world,
        const DirectX::XMMATRIX& view,
        const DirectX::XMMATRIX& proj,
        ID3D11ShaderResourceView* tex0,
        ID3D11ShaderResourceView* tex1,
        ID3D11ShaderResourceView* texAlpha,
        float alphaStrength,
        const DirectX::XMFLOAT4& ambient,
        const DirectX::XMFLOAT3& pointPos0,
        const DirectX::XMFLOAT4& pointColor0,
        float pointRange0,
        const DirectX::XMFLOAT3& pointPos1,
        const DirectX::XMFLOAT4& pointColor1,
        float pointRange1,
        // Directional Light & Shadow Mapping
        const DirectX::XMFLOAT4& dirAmbient = DirectX::XMFLOAT4(0, 0, 0, 1),
        const DirectX::XMFLOAT4& dirDiffuse = DirectX::XMFLOAT4(1, 1, 1, 1),
        const DirectX::XMFLOAT3& dirDirection = DirectX::XMFLOAT3(0, -1, 0),
        ID3D11ShaderResourceView* shadowMapSRV = nullptr,
        const DirectX::XMMATRIX& lightView = DirectX::XMMatrixIdentity(),
        const DirectX::XMMATRIX& lightProj = DirectX::XMMatrixIdentity(),
        float shadowBias = 0.0015f,
        float shadowIntensity = 0.85f,
        bool enableShadow = true,
        bool enablePCF = true
    );

private:
    bool InitializeShader(ID3D11Device* device, HWND hwnd, const std::wstring& file);
    void ShutdownShader();

    bool SetShaderParameters(
        ID3D11DeviceContext* dc,
        const DirectX::XMMATRIX& world,
        const DirectX::XMMATRIX& view,
        const DirectX::XMMATRIX& proj,
        ID3D11ShaderResourceView* tex0,
        ID3D11ShaderResourceView* tex1,
        ID3D11ShaderResourceView* texAlpha,
        float alphaStrength,
        const DirectX::XMFLOAT4& ambient,
        const DirectX::XMFLOAT3& pointPos0,
        const DirectX::XMFLOAT4& pointColor0,
        float pointRange0,
        const DirectX::XMFLOAT3& pointPos1,
        const DirectX::XMFLOAT4& pointColor1,
        float pointRange1,
        const DirectX::XMFLOAT4& dirAmbient,
        const DirectX::XMFLOAT4& dirDiffuse,
        const DirectX::XMFLOAT3& dirDirection,
        ID3D11ShaderResourceView* shadowMapSRV,
        const DirectX::XMMATRIX& lightView,
        const DirectX::XMMATRIX& lightProj,
        float shadowBias,
        float shadowIntensity,
        bool enableShadow,
        bool enablePCF);

    void RenderShader(ID3D11DeviceContext* dc, int indexCount);

private:
    struct MatrixBufferType
    {
        DirectX::XMMATRIX world;
        DirectX::XMMATRIX view;
        DirectX::XMMATRIX proj;
    };

    struct BlendBufferType
    {
        float alphaStrength;
        float uvTile;
        DirectX::XMFLOAT2 pad;
    };

    struct LightBufferType
    {
        DirectX::XMFLOAT4 ambientColor;

        DirectX::XMFLOAT3 pointPos0;
        float             pointRange0;

        DirectX::XMFLOAT4 pointColor0;

        DirectX::XMFLOAT3 pointPos1;
        float             pointRange1;

        DirectX::XMFLOAT4 pointColor1;
    };

    struct ShadowBufferType
    {
        DirectX::XMMATRIX lightViewMatrix;
        DirectX::XMMATRIX lightProjectionMatrix;
        float shadowBias;
        int enableShadow;
        int enablePCF;
        float shadowIntensity;
    };

    struct DirLightBufferType
    {
        DirectX::XMFLOAT4 dirAmbient;
        DirectX::XMFLOAT4 dirDiffuse;
        DirectX::XMFLOAT3 dirDirection;
        float _dirPad;
    };

    ID3D11VertexShader* m_vertexShader = nullptr;
    ID3D11PixelShader* m_pixelShader = nullptr;
    ID3D11InputLayout* m_layout = nullptr;
    ID3D11Buffer* m_matrixBuffer = nullptr;
    ID3D11Buffer* m_blendBuffer = nullptr;
    ID3D11Buffer* m_lightBuffer = nullptr;
    ID3D11Buffer* m_shadowBuffer = nullptr;
    ID3D11Buffer* m_dirLightBuffer = nullptr;
    ID3D11SamplerState* m_samplerState = nullptr;
};
