////////////////////////////////////////////////////////////////////////////////
// Filename: ShadowMapClass.h
// 설명: 방향성 조명(Directional Light)의 섀도우 맵(Depth Stencil 텍스처)을
//       생성하고 관리하는 클래스입니다.
////////////////////////////////////////////////////////////////////////////////
#ifndef _SHADOWMAPCLASS_H_
#define _SHADOWMAPCLASS_H_

#include <d3d11.h>
#include <directxmath.h>

class ShadowMapClass
{
public:
    ShadowMapClass();
    ~ShadowMapClass();

    // 섀도우 맵 텍스처, DSV, SRV 초기화 (기본 2048x2048 해상도)
    bool Initialize(ID3D11Device* device, int width, int height);
    void Shutdown();

    // 1st Pass: 깊이 렌더링을 위해 DSV를 바인딩하고 렌더 타깃을 nullptr로 설정
    void BindDsvAndSetNullRenderTarget(ID3D11DeviceContext* deviceContext);

    // 2nd Pass: 메인 셰이더에서 깊이 비교를 위해 섀도우 맵 SRV 반환
    ID3D11ShaderResourceView* GetShaderResourceView() const { return m_depthMapSRV; }

    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }

private:
    int m_width;
    int m_height;

    ID3D11Texture2D* m_depthMap;
    ID3D11DepthStencilView* m_depthMapDSV;
    ID3D11ShaderResourceView* m_depthMapSRV;
    D3D11_VIEWPORT m_viewport;
};

#endif
