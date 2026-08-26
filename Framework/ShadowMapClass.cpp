////////////////////////////////////////////////////////////////////////////////
// Filename: ShadowMapClass.cpp
////////////////////////////////////////////////////////////////////////////////
#include "ShadowMapClass.h"

ShadowMapClass::ShadowMapClass()
    : m_width(0), m_height(0),
      m_depthMap(nullptr),
      m_depthMapDSV(nullptr),
      m_depthMapSRV(nullptr)
{
    ZeroMemory(&m_viewport, sizeof(D3D11_VIEWPORT));
}

ShadowMapClass::~ShadowMapClass()
{
    Shutdown();
}

bool ShadowMapClass::Initialize(ID3D11Device* device, int width, int height)
{
    if (!device || width <= 0 || height <= 0) return false;

    m_width = width;
    m_height = height;

    // 1. 섀도우 맵용 2D 텍스처 생성 (32비트 고정밀 깊이 버퍼: DXGI_FORMAT_R32_TYPELESS)
    D3D11_TEXTURE2D_DESC texDesc{};
    texDesc.Width = m_width;
    texDesc.Height = m_height;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags = 0;

    HRESULT hr = device->CreateTexture2D(&texDesc, nullptr, &m_depthMap);
    if (FAILED(hr)) return false;

    // 2. 뎁스 스텐실 뷰(DSV) 생성 - 1st Pass에서 광원 깊이 기록용
    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
    dsvDesc.Flags = 0;
    dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0;

    hr = device->CreateDepthStencilView(m_depthMap, &dsvDesc, &m_depthMapDSV);
    if (FAILED(hr)) return false;

    // 3. 셰이더 리소스 뷰(SRV) 생성 - 2nd Pass에서 메인 셰이더 깊이 비교용
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    hr = device->CreateShaderResourceView(m_depthMap, &srvDesc, &m_depthMapSRV);
    if (FAILED(hr)) return false;

    // 4. 섀도우 맵 전용 뷰포트 설정
    m_viewport.TopLeftX = 0.0f;
    m_viewport.TopLeftY = 0.0f;
    m_viewport.Width = static_cast<float>(m_width);
    m_viewport.Height = static_cast<float>(m_height);
    m_viewport.MinDepth = 0.0f;
    m_viewport.MaxDepth = 1.0f;

    return true;
}

void ShadowMapClass::Shutdown()
{
    if (m_depthMapSRV)
    {
        m_depthMapSRV->Release();
        m_depthMapSRV = nullptr;
    }
    if (m_depthMapDSV)
    {
        m_depthMapDSV->Release();
        m_depthMapDSV = nullptr;
    }
    if (m_depthMap)
    {
        m_depthMap->Release();
        m_depthMap = nullptr;
    }
}

void ShadowMapClass::BindDsvAndSetNullRenderTarget(ID3D11DeviceContext* deviceContext)
{
    if (!deviceContext || !m_depthMapDSV) return;

    // 컬러 렌더 타깃 없이 오직 Depth 버퍼만 바인딩 (그림자 렌더링 최적화)
    ID3D11RenderTargetView* renderTargets[1] = { nullptr };
    deviceContext->OMSetRenderTargets(1, renderTargets, m_depthMapDSV);

    // 섀도우 맵 뷰포트 바인딩
    deviceContext->RSSetViewports(1, &m_viewport);

    // 깊이 버퍼 1.0f로 초기화
    deviceContext->ClearDepthStencilView(m_depthMapDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);
}
