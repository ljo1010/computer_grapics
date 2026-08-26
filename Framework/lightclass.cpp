////////////////////////////////////////////////////////////////////////////////
// Filename: lightclass.cpp
////////////////////////////////////////////////////////////////////////////////
#include "lightclass.h"
#include <algorithm> // for std::clamp


//ctor / dtor
LightClass::LightClass()
{
    // Directional (Phong) defaults
    m_ambientColor = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
    m_diffuseColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    m_direction = XMFLOAT3(0.0f, 1.0f, 0.0f); 
    m_specularColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    m_specularPower = 8.0f;

    // Point lights default (off)
    for (int i = 0; i < NUM_LIGHTS; ++i)
    {
        m_pointPosition[i] = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
        m_pointDiffuse[i] = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f); // 0 -> 영향 없음
    }
    m_pointLightCount = 0;

    // Attenuation defaults (no falloff by default except kc)
    m_attenKc = 1.0f;
    m_attenKl = 0.0f;
    m_attenKq = 0.0f;

    // Point intensity scale (press key 8/9)
    m_pointIntensityScale = 1.0f;

    // Toggles (press key 5/6/7)
    m_enableAmbient = true;
    m_enableDiffuse = true;
    m_enableSpecular = true;
}

LightClass::LightClass(const LightClass& other)
{
    *this = other;
}

LightClass::~LightClass()
{
}

// Directional (Phong) setters
void LightClass::SetAmbientColor(float red, float green, float blue, float alpha)
{
    m_ambientColor = XMFLOAT4(red, green, blue, alpha);
}
void LightClass::SetDiffuseColor(float red, float green, float blue, float alpha)
{
    m_diffuseColor = XMFLOAT4(red, green, blue, alpha);
}
void LightClass::SetDirection(float x, float y, float z)
{
    m_direction = XMFLOAT3(x, y, z);
}
void LightClass::SetSpecularColor(float red, float green, float blue, float alpha)
{
    m_specularColor = XMFLOAT4(red, green, blue, alpha);
}
void LightClass::SetSpecularPower(float power)
{
    m_specularPower = power;
}

// Directional (Phong) getters
XMFLOAT4 LightClass::GetAmbientColor() const { return m_ambientColor; }
XMFLOAT4 LightClass::GetDiffuseColor() const { return m_diffuseColor; }
XMFLOAT3 LightClass::GetDirection()    const { return m_direction; }
XMFLOAT4 LightClass::GetSpecularColor()const { return m_specularColor; }
float    LightClass::GetSpecularPower()const { return m_specularPower; }

// Point lights
void LightClass::SetPointLight(int idx,
    float px, float py, float pz,
    float cr, float cg, float cb, float ca)
{
    if (idx < 0 || idx >= NUM_LIGHTS) return;
    m_pointPosition[idx] = XMFLOAT4(px, py, pz, 1.0f);
    m_pointDiffuse[idx] = XMFLOAT4(cr, cg, cb, ca);
}

void LightClass::SetPointLightCount(int count)
{
    m_pointLightCount = std::clamp(count, 0, NUM_LIGHTS);
}
int LightClass::GetPointLightCount() const
{
    return m_pointLightCount;
}

XMFLOAT4 LightClass::GetPointPosition(int idx) const
{
    // 범위에 걸어둔 제한은 가장 가까운 인덱스로 보정
    idx = std::clamp(idx, 0, NUM_LIGHTS - 1);
    return m_pointPosition[idx];
}
XMFLOAT4 LightClass::GetPointDiffuse(int idx) const
{
    idx = std::clamp(idx, 0, NUM_LIGHTS - 1);
    return m_pointDiffuse[idx];
}

// Attenuation
void  LightClass::SetAttenuation(float kc, float kl, float kq)
{
    m_attenKc = kc;
    m_attenKl = kl;
    m_attenKq = kq;
}
float LightClass::GetAttenKc() const { return m_attenKc; }
float LightClass::GetAttenKl() const { return m_attenKl; }
float LightClass::GetAttenKq() const { return m_attenKq; }

// Point intensity scale
void  LightClass::SetPointIntensityScale(float s)
{
    // 음수 방지(필요 시 더 빡세게 clamp 가능)
    m_pointIntensityScale = (s < 0.f) ? 0.f : s;
}
float LightClass::GetPointIntensityScale() const
{
    return m_pointIntensityScale;
}

// Toggles
void LightClass::SetToggleAmbient(bool enabled) { m_enableAmbient = enabled; }
void LightClass::SetToggleDiffuse(bool enabled) { m_enableDiffuse = enabled; }
void LightClass::SetToggleSpecular(bool enabled) { m_enableSpecular = enabled; }

bool LightClass::GetToggleAmbient()  const { return m_enableAmbient; }
bool LightClass::GetToggleDiffuse()  const { return m_enableDiffuse; }
bool LightClass::GetToggleSpecular() const { return m_enableSpecular; }
