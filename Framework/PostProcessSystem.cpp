#include "PostProcessSystem.h"

PostProcessSystem::PostProcessSystem()
{
}

PostProcessSystem::~PostProcessSystem()
{
    Shutdown();
}

bool PostProcessSystem::Initialize(ID3D11Device* device, HWND hwnd, int screenWidth, int screenHeight)
{
    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;
    m_downWidth = screenWidth / 2;
    m_downHeight = screenHeight / 2;

    // 뷰포트 설정
    m_screenViewport.Width = (float)screenWidth;
    m_screenViewport.Height = (float)screenHeight;
    m_screenViewport.MinDepth = 0.0f;
    m_screenViewport.MaxDepth = 1.0f;
    m_screenViewport.TopLeftX = 0.0f;
    m_screenViewport.TopLeftY = 0.0f;

    m_downsampleViewport.Width = (float)m_downWidth;
    m_downsampleViewport.Height = (float)m_downHeight;
    m_downsampleViewport.MinDepth = 0.0f;
    m_downsampleViewport.MaxDepth = 1.0f;
    m_downsampleViewport.TopLeftX = 0.0f;
    m_downsampleViewport.TopLeftY = 0.0f;

    // 1. 렌더 타겟 텍스처 생성
    if (!CreateRenderTargets(device))
    {
        return false;
    }

    // 2. 셰이더 및 상수 버퍼 생성
    if (!CreateShaders(device, hwnd))
    {
        return false;
    }

    return true;
}

bool PostProcessSystem::CreateRenderTargets(ID3D11Device* device)
{
    HRESULT hr;

    auto makeRTV = [&](int width, int height, ID3D11Texture2D** tex, ID3D11RenderTargetView** rtv, ID3D11ShaderResourceView** srv) -> bool
    {
        D3D11_TEXTURE2D_DESC texDesc{};
        texDesc.Width = width;
        texDesc.Height = height;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.Format = DXGI_FORMAT_R11G11B10_FLOAT; // HDR 포맷
        texDesc.SampleDesc.Count = 1;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

        hr = device->CreateTexture2D(&texDesc, nullptr, tex);
        if (FAILED(hr)) return false;

        D3D11_RENDER_TARGET_VIEW_DESC rtvDesc{};
        rtvDesc.Format = texDesc.Format;
        rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

        hr = device->CreateRenderTargetView(*tex, &rtvDesc, rtv);
        if (FAILED(hr)) return false;

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = texDesc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;

        hr = device->CreateShaderResourceView(*tex, &srvDesc, srv);
        if (FAILED(hr)) return false;

        return true;
    };

    // 1. 풀해상도 메인 씬 HDR RTV
    if (!makeRTV(m_screenWidth, m_screenHeight, &m_sceneTexture, &m_sceneRTV, &m_sceneSRV)) return false;

    // 2. 하프해상도 Bright Pass 및 블러 RTV들
    if (!makeRTV(m_downWidth, m_downHeight, &m_brightTexture, &m_brightRTV, &m_brightSRV)) return false;
    if (!makeRTV(m_downWidth, m_downHeight, &m_blurTextureX, &m_blurRTV_X, &m_blurSRV_X)) return false;
    if (!makeRTV(m_downWidth, m_downHeight, &m_blurTextureY, &m_blurRTV_Y, &m_blurSRV_Y)) return false;

    return true;
}

bool PostProcessSystem::CreateShaders(ID3D11Device* device, HWND hwnd)
{
    HRESULT hr;
    ID3D10Blob* vsBlob = nullptr;
    ID3D10Blob* psBlob = nullptr;
    ID3D10Blob* errorBlob = nullptr;

    const WCHAR* filename = L"./data/postprocess.hlsl";

    // 1. FullScreenVS
    hr = D3DCompileFromFile(filename, NULL, NULL, "FullScreenVS", "vs_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, &vsBlob, &errorBlob);
    if (FAILED(hr)) { if (errorBlob) errorBlob->Release(); return false; }
    hr = device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), NULL, &m_fullScreenVS);
    vsBlob->Release();
    if (FAILED(hr)) return false;

    auto compilePS = [&](const char* entryPoint, ID3D11PixelShader** ps) -> bool
    {
        hr = D3DCompileFromFile(filename, NULL, NULL, entryPoint, "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, &psBlob, &errorBlob);
        if (FAILED(hr)) { if (errorBlob) errorBlob->Release(); return false; }
        hr = device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), NULL, ps);
        psBlob->Release();
        return SUCCEEDED(hr);
    };

    if (!compilePS("BrightPassPS", &m_brightPassPS)) return false;
    if (!compilePS("BlurHorizontalPS", &m_blurHorizontalPS)) return false;
    if (!compilePS("BlurVerticalPS", &m_blurVerticalPS)) return false;
    if (!compilePS("CompositePS", &m_compositePS)) return false;

    // 상수 버퍼 생성
    D3D11_BUFFER_DESC cbDesc{};
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.ByteWidth = sizeof(PostProcessBufferType);
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    hr = device->CreateBuffer(&cbDesc, NULL, &m_constantBuffer);
    if (FAILED(hr)) return false;

    // 샘플러 생성 (Linear Clamp)
    D3D11_SAMPLER_DESC sampDesc{};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    hr = device->CreateSamplerState(&sampDesc, &m_samplerClamp);
    if (FAILED(hr)) return false;

    return true;
}

void PostProcessSystem::Shutdown()
{
    if (m_samplerClamp) { m_samplerClamp->Release(); m_samplerClamp = nullptr; }
    if (m_constantBuffer) { m_constantBuffer->Release(); m_constantBuffer = nullptr; }

    if (m_compositePS) { m_compositePS->Release(); m_compositePS = nullptr; }
    if (m_blurVerticalPS) { m_blurVerticalPS->Release(); m_blurVerticalPS = nullptr; }
    if (m_blurHorizontalPS) { m_blurHorizontalPS->Release(); m_blurHorizontalPS = nullptr; }
    if (m_brightPassPS) { m_brightPassPS->Release(); m_brightPassPS = nullptr; }
    if (m_fullScreenVS) { m_fullScreenVS->Release(); m_fullScreenVS = nullptr; }

    if (m_blurSRV_Y) { m_blurSRV_Y->Release(); m_blurSRV_Y = nullptr; }
    if (m_blurRTV_Y) { m_blurRTV_Y->Release(); m_blurRTV_Y = nullptr; }
    if (m_blurTextureY) { m_blurTextureY->Release(); m_blurTextureY = nullptr; }

    if (m_blurSRV_X) { m_blurSRV_X->Release(); m_blurSRV_X = nullptr; }
    if (m_blurRTV_X) { m_blurRTV_X->Release(); m_blurRTV_X = nullptr; }
    if (m_blurTextureX) { m_blurTextureX->Release(); m_blurTextureX = nullptr; }

    if (m_brightSRV) { m_brightSRV->Release(); m_brightSRV = nullptr; }
    if (m_brightRTV) { m_brightRTV->Release(); m_brightRTV = nullptr; }
    if (m_brightTexture) { m_brightTexture->Release(); m_brightTexture = nullptr; }

    if (m_sceneSRV) { m_sceneSRV->Release(); m_sceneSRV = nullptr; }
    if (m_sceneRTV) { m_sceneRTV->Release(); m_sceneRTV = nullptr; }
    if (m_sceneTexture) { m_sceneTexture->Release(); m_sceneTexture = nullptr; }
}

void PostProcessSystem::BindSceneRenderTarget(ID3D11DeviceContext* dc, ID3D11DepthStencilView* dsv)
{
    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    dc->ClearRenderTargetView(m_sceneRTV, clearColor);
    dc->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);

    dc->OMSetRenderTargets(1, &m_sceneRTV, dsv);
    dc->RSSetViewports(1, &m_screenViewport);
}

void PostProcessSystem::RenderFullScreenQuad(ID3D11DeviceContext* dc)
{
    dc->IASetInputLayout(nullptr);
    dc->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
    dc->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
    dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    dc->VSSetShader(m_fullScreenVS, nullptr, 0);
    dc->Draw(3, 0); // 풀스크린 트라이앵글
}

void PostProcessSystem::Render(ID3D11DeviceContext* dc, ID3D11RenderTargetView* backBufferRTV)
{
    D3D11_MAPPED_SUBRESOURCE mapped;
    if (SUCCEEDED(dc->Map(m_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        PostProcessBufferType* p = (PostProcessBufferType*)mapped.pData;
        p->screenSize = XMFLOAT2((float)m_downWidth, (float)m_downHeight);
        p->bloomThreshold = m_bloomThreshold;
        p->bloomIntensity = m_bloomIntensity;
        p->enableBloom = m_enableBloom ? 1 : 0;
        p->enableTonemap = m_enableTonemap ? 1 : 0;
        p->vignetteIntensity = m_vignetteIntensity;
        p->exposure = m_exposure;
        dc->Unmap(m_constantBuffer, 0);
    }

    dc->PSSetConstantBuffers(0, 1, &m_constantBuffer);
    dc->PSSetSamplers(0, 1, &m_samplerClamp);

    ID3D11ShaderResourceView* nullSRV[2] = { nullptr, nullptr };

    if (m_enableBloom)
    {
        // ------------------------------------------------------------
        // Pass 1: Bright Pass (고휘도 영역 추출)
        // ------------------------------------------------------------
        dc->OMSetRenderTargets(1, &m_brightRTV, nullptr);
        dc->RSSetViewports(1, &m_downsampleViewport);
        dc->PSSetShaderResources(0, 1, &m_sceneSRV);
        dc->PSSetShader(m_brightPassPS, nullptr, 0);
        RenderFullScreenQuad(dc);
        dc->PSSetShaderResources(0, 1, nullSRV);

        // ------------------------------------------------------------
        // Pass 2: Blur Horizontal
        // ------------------------------------------------------------
        dc->OMSetRenderTargets(1, &m_blurRTV_X, nullptr);
        dc->PSSetShaderResources(0, 1, &m_brightSRV);
        dc->PSSetShader(m_blurHorizontalPS, nullptr, 0);
        RenderFullScreenQuad(dc);
        dc->PSSetShaderResources(0, 1, nullSRV);

        // ------------------------------------------------------------
        // Pass 3: Blur Vertical
        // ------------------------------------------------------------
        dc->OMSetRenderTargets(1, &m_blurRTV_Y, nullptr);
        dc->PSSetShaderResources(0, 1, &m_blurSRV_X);
        dc->PSSetShader(m_blurVerticalPS, nullptr, 0);
        RenderFullScreenQuad(dc);
        dc->PSSetShaderResources(0, 1, nullSRV);
    }

    // ------------------------------------------------------------
    // Pass 4: Composite + Tone Mapping + Vignette -> 백버퍼 출력
    // ------------------------------------------------------------
    dc->OMSetRenderTargets(1, &backBufferRTV, nullptr);
    dc->RSSetViewports(1, &m_screenViewport);

    ID3D11ShaderResourceView* srvs[2] = { m_sceneSRV, m_enableBloom ? m_blurSRV_Y : nullptr };
    dc->PSSetShaderResources(0, 2, srvs);
    dc->PSSetShader(m_compositePS, nullptr, 0);

    RenderFullScreenQuad(dc);

    // 언바인드
    dc->PSSetShaderResources(0, 2, nullSRV);
}