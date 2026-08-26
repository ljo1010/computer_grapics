#include "MultiTextureShaderClass.h"
#include <d3dcompiler.h>

using namespace DirectX;

MultiTextureShaderClass::MultiTextureShaderClass()
{
}

MultiTextureShaderClass::~MultiTextureShaderClass()
{
    Shutdown();
}

bool MultiTextureShaderClass::Initialize(ID3D11Device* device, HWND hwnd, const std::wstring& file)
{
    return InitializeShader(device, hwnd, file);
}

void MultiTextureShaderClass::Shutdown()
{
    ShutdownShader();
}

// =============================
// Render (지형 + 조명 + 실시간 섀도우)
// =============================
bool MultiTextureShaderClass::Render(
    ID3D11DeviceContext* dc, int indexCount,
    const XMMATRIX& world, const XMMATRIX& view, const XMMATRIX& proj,
    ID3D11ShaderResourceView* tex0,
    ID3D11ShaderResourceView* tex1,
    ID3D11ShaderResourceView* texAlpha,
    float alphaStrength,
    const XMFLOAT4& ambient,
    const XMFLOAT3& pointPos0,
    const XMFLOAT4& pointColor0,
    float pointRange0,
    const XMFLOAT3& pointPos1,
    const XMFLOAT4& pointColor1,
    float pointRange1,
    const XMFLOAT4& dirAmbient,
    const XMFLOAT4& dirDiffuse,
    const XMFLOAT3& dirDirection,
    ID3D11ShaderResourceView* shadowMapSRV,
    const XMMATRIX& lightView,
    const XMMATRIX& lightProj,
    float shadowBias,
    float shadowIntensity,
    bool enableShadow,
    bool enablePCF,
    // Distance Fog (대기 거리 안개)
    const XMFLOAT3& cameraPos,
    const XMFLOAT4& fogColor,
    float fogStart,
    float fogEnd,
    bool enableFog)
{
    if (!SetShaderParameters(dc, world, view, proj,
        tex0, tex1, texAlpha,
        alphaStrength,
        ambient,
        pointPos0, pointColor0, pointRange0,
        pointPos1, pointColor1, pointRange1,
        dirAmbient, dirDiffuse, dirDirection,
        shadowMapSRV, lightView, lightProj,
        shadowBias, shadowIntensity, enableShadow, enablePCF,
        cameraPos, fogColor, fogStart, fogEnd, enableFog))
        return false;

    RenderShader(dc, indexCount);
    return true;
}

// =============================
// SetShaderParameters
// =============================
bool MultiTextureShaderClass::SetShaderParameters(
    ID3D11DeviceContext* dc,
    const XMMATRIX& world, const XMMATRIX& view, const XMMATRIX& proj,
    ID3D11ShaderResourceView* tex0,
    ID3D11ShaderResourceView* tex1,
    ID3D11ShaderResourceView* texAlpha,
    float alphaStrength,
    const XMFLOAT4& ambient,
    const XMFLOAT3& pointPos0,
    const XMFLOAT4& pointColor0,
    float pointRange0,
    const XMFLOAT3& pointPos1,
    const XMFLOAT4& pointColor1,
    float pointRange1,
    const XMFLOAT4& dirAmbient,
    const XMFLOAT4& dirDiffuse,
    const XMFLOAT3& dirDirection,
    ID3D11ShaderResourceView* shadowMapSRV,
    const XMMATRIX& lightView,
    const XMMATRIX& lightProj,
    float shadowBias,
    float shadowIntensity,
    bool enableShadow,
    bool enablePCF,
    // Distance Fog (대기 거리 안개)
    const XMFLOAT3& cameraPos,
    const XMFLOAT4& fogColor,
    float fogStart,
    float fogEnd,
    bool enableFog)
{
    // --- b0 : MatrixBuffer ---
    {
        D3D11_MAPPED_SUBRESOURCE map{};
        if (FAILED(dc->Map(m_matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map)))
            return false;

        auto* mb = reinterpret_cast<MatrixBufferType*>(map.pData);
        mb->world = XMMatrixTranspose(world);
        mb->view = XMMatrixTranspose(view);
        mb->proj = XMMatrixTranspose(proj);
        dc->Unmap(m_matrixBuffer, 0);

        dc->VSSetConstantBuffers(0, 1, &m_matrixBuffer);
    }

    // --- b1 : BlendBuffer ---
    {
        D3D11_MAPPED_SUBRESOURCE map{};
        if (FAILED(dc->Map(m_blendBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map)))
            return false;

        auto* bb = reinterpret_cast<BlendBufferType*>(map.pData);
        const float tile = 3.0f;
        bb->alphaStrength = alphaStrength;
        bb->uvTile = tile;
        bb->pad = XMFLOAT2(0.0f, 0.0f);
        dc->Unmap(m_blendBuffer, 0);

        dc->VSSetConstantBuffers(1, 1, &m_blendBuffer);
        dc->PSSetConstantBuffers(1, 1, &m_blendBuffer);
    }

    // --- b2 : LightBuffer (ambient + point2개) ---
    {
        D3D11_MAPPED_SUBRESOURCE map{};
        if (FAILED(dc->Map(m_lightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map)))
            return false;

        auto* lb = reinterpret_cast<LightBufferType*>(map.pData);
        lb->ambientColor = ambient;

        lb->pointPos0 = pointPos0;
        lb->pointRange0 = pointRange0;
        lb->pointColor0 = pointColor0;

        lb->pointPos1 = pointPos1;
        lb->pointRange1 = pointRange1;
        lb->pointColor1 = pointColor1;

        dc->Unmap(m_lightBuffer, 0);
        dc->PSSetConstantBuffers(2, 1, &m_lightBuffer);
    }

    // --- b3 : ShadowBuffer ---
    if (m_shadowBuffer)
    {
        D3D11_MAPPED_SUBRESOURCE map{};
        if (SUCCEEDED(dc->Map(m_shadowBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map)))
        {
            auto* sb = reinterpret_cast<ShadowBufferType*>(map.pData);
            sb->lightViewMatrix = XMMatrixTranspose(lightView);
            sb->lightProjectionMatrix = XMMatrixTranspose(lightProj);
            sb->shadowBias = shadowBias;
            sb->enableShadow = enableShadow ? 1 : 0;
            sb->enablePCF = enablePCF ? 1 : 0;
            sb->shadowIntensity = shadowIntensity;
            dc->Unmap(m_shadowBuffer, 0);

            dc->PSSetConstantBuffers(3, 1, &m_shadowBuffer);
        }
    }

    // --- b4 : DirLightBuffer ---
    if (m_dirLightBuffer)
    {
        D3D11_MAPPED_SUBRESOURCE map{};
        if (SUCCEEDED(dc->Map(m_dirLightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map)))
        {
            auto* dlb = reinterpret_cast<DirLightBufferType*>(map.pData);
            dlb->dirAmbient = dirAmbient;
            dlb->dirDiffuse = dirDiffuse;
            dlb->dirDirection = dirDirection;
            dlb->_dirPad = 0.0f;
            dc->Unmap(m_dirLightBuffer, 0);

            dc->PSSetConstantBuffers(4, 1, &m_dirLightBuffer);
        }
    }

    // --- b5 : FogBuffer (대기 거리 안개) ---
    if (m_fogBuffer)
    {
        D3D11_MAPPED_SUBRESOURCE map{};
        if (SUCCEEDED(dc->Map(m_fogBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map)))
        {
            auto* fb = reinterpret_cast<FogBufferType*>(map.pData);
            fb->cameraPos = cameraPos;
            fb->fogStart = fogStart;
            fb->fogColor = fogColor;
            fb->fogEnd = fogEnd;
            fb->enableFog = enableFog ? 1 : 0;
            fb->_fogPad = XMFLOAT2(0.0f, 0.0f);
            dc->Unmap(m_fogBuffer, 0);

            dc->PSSetConstantBuffers(5, 1, &m_fogBuffer);
        }
    }

    // --- 텍스처 & 샘플러 (t0: dirt, t1: dungeon, t2: alpha, t3: shadowMap) ---
    {
        ID3D11ShaderResourceView* srvs[3] = { tex0, tex1, texAlpha };
        dc->PSSetShaderResources(0, 3, srvs);
        if (shadowMapSRV)
        {
            dc->PSSetShaderResources(3, 1, &shadowMapSRV);
        }
        dc->PSSetSamplers(0, 1, &m_samplerState);
    }

    return true;
}

// =============================
// InitializeShader
// =============================
bool MultiTextureShaderClass::InitializeShader(ID3D11Device* device, HWND hwnd, const std::wstring& file)
{
    ID3DBlob* vs = nullptr;
    ID3DBlob* ps = nullptr;
    ID3DBlob* err = nullptr;

    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(_DEBUG)
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    HRESULT hr = D3DCompileFromFile(
        file.c_str(), nullptr, nullptr,
        "VS_main", "vs_5_0", flags, 0, &vs, &err);
    if (FAILED(hr)) {
        if (err) { OutputDebugStringA((char*)err->GetBufferPointer()); err->Release(); }
        MessageBoxW(hwnd, (L"Vertex Shader compile failed:\n" + file).c_str(),
            L"Shader Error", MB_OK | MB_ICONERROR);
        return false;
    }

    hr = D3DCompileFromFile(
        file.c_str(), nullptr, nullptr,
        "PS_main", "ps_5_0", flags, 0, &ps, &err);
    if (FAILED(hr)) {
        if (err) { OutputDebugStringA((char*)err->GetBufferPointer()); err->Release(); }
        MessageBoxW(hwnd, (L"Pixel Shader compile failed:\n" + file).c_str(),
            L"Shader Error", MB_OK | MB_ICONERROR);
        vs->Release();
        return false;
    }

    device->CreateVertexShader(vs->GetBufferPointer(), vs->GetBufferSize(), nullptr, &m_vertexShader);
    device->CreatePixelShader(ps->GetBufferPointer(), ps->GetBufferSize(), nullptr, &m_pixelShader);

    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION",   0, DXGI_FORMAT_R32G32B32_FLOAT,   0, 0,                           D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",   0, DXGI_FORMAT_R32G32_FLOAT,      0, 12,                          D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BONEID",     0, DXGI_FORMAT_R32G32B32A32_UINT, 0, 20,                          D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "WEIGHT",     0, DXGI_FORMAT_R32G32B32A32_FLOAT,0, 36,                          D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    device->CreateInputLayout(layout, 2, vs->GetBufferPointer(), vs->GetBufferSize(), &m_layout);

    vs->Release();
    ps->Release();

    // ---- constant buffers
    D3D11_BUFFER_DESC bd{};
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    bd.ByteWidth = sizeof(MatrixBufferType);
    device->CreateBuffer(&bd, nullptr, &m_matrixBuffer);

    bd.ByteWidth = sizeof(BlendBufferType);
    device->CreateBuffer(&bd, nullptr, &m_blendBuffer);

    bd.ByteWidth = sizeof(LightBufferType);
    device->CreateBuffer(&bd, nullptr, &m_lightBuffer);

    // Shadow buffer (b3)
    bd.ByteWidth = sizeof(ShadowBufferType);
    device->CreateBuffer(&bd, nullptr, &m_shadowBuffer);

    // Dir light buffer (b4)
    bd.ByteWidth = sizeof(DirLightBufferType);
    device->CreateBuffer(&bd, nullptr, &m_dirLightBuffer);

    // Fog buffer (b5)
    bd.ByteWidth = sizeof(FogBufferType);
    device->CreateBuffer(&bd, nullptr, &m_fogBuffer);

    // ---- sampler
    D3D11_SAMPLER_DESC sd{};
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sd.MinLOD = 0;
    sd.MaxLOD = D3D11_FLOAT32_MAX;
    device->CreateSamplerState(&sd, &m_samplerState);

    return true;
}

// =============================
// ShutdownShader
// =============================
void MultiTextureShaderClass::ShutdownShader()
{
    if (m_fogBuffer)      m_fogBuffer->Release(), m_fogBuffer = nullptr;
    if (m_samplerState)   m_samplerState->Release(), m_samplerState = nullptr;
    if (m_dirLightBuffer) m_dirLightBuffer->Release(), m_dirLightBuffer = nullptr;
    if (m_shadowBuffer)   m_shadowBuffer->Release(), m_shadowBuffer = nullptr;
    if (m_lightBuffer)    m_lightBuffer->Release(), m_lightBuffer = nullptr;
    if (m_blendBuffer)    m_blendBuffer->Release(), m_blendBuffer = nullptr;
    if (m_matrixBuffer)   m_matrixBuffer->Release(), m_matrixBuffer = nullptr;
    if (m_layout)         m_layout->Release(), m_layout = nullptr;
    if (m_pixelShader)    m_pixelShader->Release(), m_pixelShader = nullptr;
    if (m_vertexShader)   m_vertexShader->Release(), m_vertexShader = nullptr;
}

// =============================
// RenderShader
// =============================
void MultiTextureShaderClass::RenderShader(ID3D11DeviceContext* dc, int indexCount)
{
    dc->IASetInputLayout(m_layout);
    dc->VSSetShader(m_vertexShader, nullptr, 0);
    dc->PSSetShader(m_pixelShader, nullptr, 0);
    dc->DrawIndexed(indexCount, 0, 0);
}
