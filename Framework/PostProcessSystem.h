#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <memory>

#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;

// ============================================================================
// PostProcessSystem: HDR 블룸, 가우시안 블러, ACES 톤매핑, 비네팅 관리자
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

public:
    PostProcessSystem();
    ~PostProcessSystem();

    bool Initialize(ID3D11Device* device, HWND hwnd, int screenWidth, int screenHeight);
    void Shutdown();

    // 씬 렌더링 전 오프스크린 HDR 렌더 타겟 바인딩
    void BindSceneRenderTarget(ID3D11DeviceContext* dc, ID3D11DepthStencilView* dsv);

    // 포스트 프로세싱 실행 및 백버퍼에 최종 합성 출력
    void Render(ID3D11DeviceContext* dc, ID3D11RenderTargetView* backBufferRTV);

    // 파라미터 제어 (ImGui 연동용)
    bool& GetBloomEnabled() { return m_enableBloom; }
    float& GetBloomThreshold() { return m_bloomThreshold; }
    float& GetBloomIntensity() { return m_bloomIntensity; }
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

    // 렌더 타겟 및 셰이더 리소스 뷰
    ID3D11Texture2D* m_sceneTexture = nullptr;
    ID3D11RenderTargetView* m_sceneRTV = nullptr;
    ID3D11ShaderResourceView* m_sceneSRV = nullptr;

    ID3D11Texture2D* m_brightTexture = nullptr;
    ID3D11RenderTargetView* m_brightRTV = nullptr;
    ID3D11ShaderResourceView* m_brightSRV = nullptr;

    ID3D11Texture2D* m_blurTextureX = nullptr;
    ID3D11RenderTargetView* m_blurRTV_X = nullptr;
    ID3D11ShaderResourceView* m_blurSRV_X = nullptr;

    ID3D11Texture2D* m_blurTextureY = nullptr;
    ID3D11RenderTargetView* m_blurRTV_Y = nullptr;
    ID3D11ShaderResourceView* m_blurSRV_Y = nullptr;

    // 셰이더들
    ID3D11VertexShader* m_fullScreenVS = nullptr;
    ID3D11PixelShader* m_brightPassPS = nullptr;
    ID3D11PixelShader* m_blurHorizontalPS = nullptr;
    ID3D11PixelShader* m_blurVerticalPS = nullptr;
    ID3D11PixelShader* m_compositePS = nullptr;

    ID3D11SamplerState* m_samplerClamp = nullptr;
    ID3D11Buffer* m_constantBuffer = nullptr;

    // 뷰포트
    D3D11_VIEWPORT m_screenViewport{};
    D3D11_VIEWPORT m_downsampleViewport{};

    // 제어 파라미터 기본값
    bool  m_enableBloom = true;
    float m_bloomThreshold = 0.88f;
    float m_bloomIntensity = 0.45f;
    bool  m_enableTonemap = false; // 기본은 기존의 쨍한 색감 유지
    float m_vignetteIntensity = 0.20f;
    float m_exposure = 1.0f;
};