#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <memory>

#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;

// ============================================================================
// PostProcessSystem: HDR 블룸, 방사형 갓 레이(빛줄기), ACES 톤매핑, 비네팅 관리자
// ============================================================================
class PostProcessSystem
{
private:
    struct PostProcessBufferType
    {
        XMFLOAT2 screenSize;
        float    bloomThreshold;
        float    bloomIntensity;

        int      enableBloom;
        int      enableTonemap;
        float    vignetteIntensity;
        float    exposure;
    };

    struct GodRaysBufferType
    {
        XMFLOAT2 sunScreenPos;
        float    sunVisibility;
        float    rayIntensity;

        float    rayDecay;
        float    rayDensity;
        float    rayWeight;
        int      enableGodRays;

        XMFLOAT4 rayColor;
    };

public:
    PostProcessSystem();
    ~PostProcessSystem();

    bool Initialize(ID3D11Device* device, HWND hwnd, int screenWidth, int screenHeight);
    void Shutdown();

    // 씬 렌더링 전 오프스크린 HDR 렌더 타겟 바인딩
    void BindSceneRenderTarget(ID3D11DeviceContext* dc, ID3D11DepthStencilView* dsv);

    // 태양의 3D 방향을 2D 스크린 좌표로 투영
    void UpdateSun(XMFLOAT3 sunDirection, const XMMATRIX& view, const XMMATRIX& proj, XMFLOAT3 camPos);

    // 포스트 프로세싱 실행 및 백버퍼에 최종 합성 출력
    void Render(ID3D11DeviceContext* dc, ID3D11RenderTargetView* backBufferRTV);

    // [블룸 파라미터 제어]
    bool& GetBloomEnabled() { return m_enableBloom; }
    float& GetBloomThreshold() { return m_bloomThreshold; }
    float& GetBloomIntensity() { return m_bloomIntensity; }

    // [갓 레이 파라미터 제어]
    bool& GetGodRaysEnabled() { return m_enableGodRays; }
    float& GetRayIntensity() { return m_rayIntensity; }
    float& GetRayDecay() { return m_rayDecay; }
    float& GetRayDensity() { return m_rayDensity; }
    float& GetRayWeight() { return m_rayWeight; }
    XMFLOAT4& GetRayColor() { return m_rayColor; }

    // [톤매핑 및 비네팅 제어]
    bool& GetTonemapEnabled() { return m_enableTonemap; }
    float& GetVignetteIntensity() { return m_vignetteIntensity; }
    float& GetExposure() { return m_exposure; }

private:
    bool CreateRenderTargets(ID3D11Device* device);
    bool CreateShaders(ID3D11Device* device, HWND hwnd);
    void RenderFullScreenQuad(ID3D11DeviceContext* dc);

private:
    int m_screenWidth = 0;
    int m_screenHeight = 0;
    int m_downWidth = 0;
    int m_downHeight = 0;

    // 1. 메인 씬 렌더 타겟
    ID3D11Texture2D* m_sceneTexture = nullptr;
    ID3D11RenderTargetView* m_sceneRTV = nullptr;
    ID3D11ShaderResourceView* m_sceneSRV = nullptr;

    // 2. 블룸용 렌더 타겟
    ID3D11Texture2D* m_brightTexture = nullptr;
    ID3D11RenderTargetView* m_brightRTV = nullptr;
    ID3D11ShaderResourceView* m_brightSRV = nullptr;

    ID3D11Texture2D* m_blurTextureX = nullptr;
    ID3D11RenderTargetView* m_blurRTV_X = nullptr;
    ID3D11ShaderResourceView* m_blurSRV_X = nullptr;

    ID3D11Texture2D* m_blurTextureY = nullptr;
    ID3D11RenderTargetView* m_blurRTV_Y = nullptr;
    ID3D11ShaderResourceView* m_blurSRV_Y = nullptr;

    // 3. 갓 레이(God Rays)용 렌더 타겟
    ID3D11Texture2D* m_rayOcclusionTexture = nullptr;
    ID3D11RenderTargetView* m_rayOcclusionRTV = nullptr;
    ID3D11ShaderResourceView* m_rayOcclusionSRV = nullptr;

    ID3D11Texture2D* m_rayRadialTexture = nullptr;
    ID3D11RenderTargetView* m_rayRadialRTV = nullptr;
    ID3D11ShaderResourceView* m_rayRadialSRV = nullptr;

    // 셰이더들
    ID3D11VertexShader* m_fullScreenVS = nullptr;
    ID3D11PixelShader*  m_brightPassPS = nullptr;
    ID3D11PixelShader*  m_blurHorizontalPS = nullptr;
    ID3D11PixelShader*  m_blurVerticalPS = nullptr;
    ID3D11PixelShader*  m_sunOcclusionPS = nullptr;
    ID3D11PixelShader*  m_radialBlurRayPS = nullptr;
    ID3D11PixelShader*  m_compositePS = nullptr;

    ID3D11SamplerState* m_samplerClamp = nullptr;
    ID3D11Buffer*       m_postProcessBuffer = nullptr;
    ID3D11Buffer*       m_godRaysBuffer = nullptr;

    // 뷰포트
    D3D11_VIEWPORT m_screenViewport{};
    D3D11_VIEWPORT m_downsampleViewport{};

    // 블룸 기본 파라미터
    bool  m_enableBloom = true;
    float m_bloomThreshold = 0.88f;
    float m_bloomIntensity = 0.45f;

    // 갓 레이(God Rays) 기본 파라미터
    bool     m_enableGodRays = true;
    XMFLOAT2 m_sunScreenPos = XMFLOAT2(0.5f, 0.3f);
    float    m_sunVisibility = 1.0f;
    float    m_rayIntensity = 1.15f;
    float    m_rayDecay = 0.965f;
    float    m_rayDensity = 0.82f;
    float    m_rayWeight = 0.28f;
    XMFLOAT4 m_rayColor = XMFLOAT4(1.0f, 0.88f, 0.62f, 1.0f); // 따스한 황금빛 햇살 톤

    // 톤매핑 및 비네팅 기본 파라미터
    bool  m_enableTonemap = false;
    float m_vignetteIntensity = 0.20f;
    float m_exposure = 1.0f;
};