////////////////////////////////////////////////////////////////////////////////
// Filename: lightclass.h
////////////////////////////////////////////////////////////////////////////////
#ifndef _LIGHTCLASS_H_
#define _LIGHTCLASS_H_

//////////////
// INCLUDES //
//////////////
#include <DirectXMath.h>
using namespace DirectX;

// 반드시 HLSL의 NUM_LIGHTS와 동일하게 유지
#ifndef NUM_LIGHTS
#define NUM_LIGHTS 3
#endif

////////////////////////////////////////////////////////////////////////////////
// Class name: LightClass
////////////////////////////////////////////////////////////////////////////////
class LightClass
{
public:
    LightClass();
    LightClass(const LightClass&);
    ~LightClass();

    // ===== 기존 Directional(Phong) Light 인터페이스 =====
    void SetAmbientColor(float r, float g, float b, float a);
    void SetDiffuseColor(float r, float g, float b, float a);      // Directional diffuse
    void SetDirection(float x, float y, float z);                  // Directional dir (씬을 향하는 방향)
    void SetSpecularColor(float r, float g, float b, float a);
    void SetSpecularPower(float p);

    XMFLOAT4 GetAmbientColor() const;
    XMFLOAT4 GetDiffuseColor() const;
    XMFLOAT3 GetDirection() const;
    XMFLOAT4 GetSpecularColor() const;
    float    GetSpecularPower() const;

    // ===== Point Lights (최대 NUM_LIGHTS) =====
    // 색상은 "diffuse" 성분만 사용 (스펙큘러 색은 공통 specularColor 사용)
    void SetPointLight(int idx, float px, float py, float pz,
                       float cr, float cg, float cb, float ca = 1.0f);
    // 사용 중인 포인트 라이트 개수 지정(렌더 시 편의용, 0~NUM_LIGHTS)
    void SetPointLightCount(int count);
    int  GetPointLightCount() const;

    // 개별 항목 접근자 (바인딩 시 memcpy 등에 사용)
    XMFLOAT4 GetPointPosition(int idx) const;  // xyz=pos, w=1
    XMFLOAT4 GetPointDiffuse (int idx) const;  // rgba

    // ===== Attenuation (kc + kl*d + kq*d^2)^-1 =====
    void  SetAttenuation(float kc, float kl, float kq);
    float GetAttenKc() const;
    float GetAttenKl() const;
    float GetAttenKq() const;

    // ===== 포인트 라이트 강도 스케일 (키 8/9로 조절) =====
    void  SetPointIntensityScale(float s);
    float GetPointIntensityScale() const;

    // ===== 토글 (키 5/6/7) =====
    void SetToggleAmbient(bool enabled);
    void SetToggleDiffuse(bool enabled);
    void SetToggleSpecular(bool enabled);

    bool GetToggleAmbient() const;
    bool GetToggleDiffuse() const;
    bool GetToggleSpecular() const;

private:
    // Directional(Phong) 공통
    XMFLOAT4 m_ambientColor   = XMFLOAT4(0,0,0,1);
    XMFLOAT4 m_diffuseColor   = XMFLOAT4(1,1,1,1);   // 방향광 diffuse
    XMFLOAT3 m_direction      = XMFLOAT3(0,-1,0);    // 씬을 향하는 방향
    XMFLOAT4 m_specularColor  = XMFLOAT4(1,1,1,1);
    float    m_specularPower  = 32.0f;

    // Point lights
    XMFLOAT4 m_pointPosition[NUM_LIGHTS];            // xyz pos, w=1
    XMFLOAT4 m_pointDiffuse [NUM_LIGHTS];            // rgba
    int      m_pointLightCount = 0;                  // 사용 개수(0~NUM_LIGHTS)

    // Attenuation -> 빛의 거리에 따라 약해지는 정도
    float m_attenKc = 1.0f;
    float m_attenKl = 0.0f;
    float m_attenKq = 0.0f;

    // 강도intensity 스케일 (키 8/9)
    float m_pointIntensityScale = 1.0f;

    // 토글 (키 5/6/7)
    bool m_enableAmbient  = true;
    bool m_enableDiffuse  = true;
    bool m_enableSpecular = true;
};

#endif // _LIGHTCLASS_H_
