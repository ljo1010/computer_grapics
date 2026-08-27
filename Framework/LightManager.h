////////////////////////////////////////////////////////////////////////////////
// Filename: LightManager.h
// Description: Manages lighting system (directional + point lights)
////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <DirectXMath.h>

#ifndef NUM_LIGHTS
#define NUM_LIGHTS 3
#endif

class LightClass;

class LightManager
{
public:
    LightManager();
    ~LightManager();

    void Initialize(LightClass* light);
    void HandleHotkeys();
    void TogglePointLight();

    // Directional Light Getters
    DirectX::XMFLOAT4 GetAmbient() const { return m_dirAmbient; }
    DirectX::XMFLOAT4 GetDiffuse() const { return m_dirDiffuse; }
    DirectX::XMFLOAT3 GetDirection() const { return m_dirDirection; }
    DirectX::XMFLOAT4 GetSpecularColor() const { return m_specularColor; }
    float GetSpecularPower() const { return m_specularPower; }

    // Directional Light Setters (ImGui 실시간 제어용)
    void SetAmbient(const DirectX::XMFLOAT4& ambient) { m_dirAmbient = ambient; }
    void SetDiffuse(const DirectX::XMFLOAT4& diffuse) { m_dirDiffuse = diffuse; }
    void SetDirection(const DirectX::XMFLOAT3& dir) { m_dirDirection = dir; }
    void SetSpecularColor(const DirectX::XMFLOAT4& spec) { m_specularColor = spec; }
    void SetSpecularPower(float power) { m_specularPower = power; }

    // ImGui에서 포인터/레퍼런스로 직접 조작할 수 있는 Getter
    DirectX::XMFLOAT4* GetAmbientPtr() { return &m_dirAmbient; }
    DirectX::XMFLOAT4* GetDiffusePtr() { return &m_dirDiffuse; }
    DirectX::XMFLOAT3* GetDirectionPtr() { return &m_dirDirection; }
    DirectX::XMFLOAT4* GetSpecularColorPtr() { return &m_specularColor; }
    float* GetSpecularPowerPtr() { return &m_specularPower; }

    // Point Light Getters
    const DirectX::XMFLOAT4* GetPointPositions() const { return m_pointPos; }
    const DirectX::XMFLOAT4* GetPointDiffuse() const { return m_pointDiff; }
    int GetPointCount() const { return m_pointCount; }

    // Attenuation Getters
    float GetAttenKc() const { return m_attenKc; }
    float GetAttenKl() const { return m_attenKl; }
    float GetAttenKq() const { return m_attenKq; }
    float GetIntensityScale() const { return m_pointIntensityScale; }

    // Toggle Getters & Setters
    bool IsAmbientEnabled() const { return m_enableAmbient; }
    bool IsDiffuseEnabled() const { return m_enableDiffuse; }
    bool IsSpecularEnabled() const { return m_enableSpecular; }
    bool IsPointLightEnabled() const { return m_enablePointLight; }

    bool* GetAmbientEnabledPtr() { return &m_enableAmbient; }
    bool* GetDiffuseEnabledPtr() { return &m_enableDiffuse; }
    bool* GetSpecularEnabledPtr() { return &m_enableSpecular; }
    bool* GetPointLightEnabledPtr() { return &m_enablePointLight; }

    // ===== 실시간 섀도우 매핑(Shadow Mapping) 관련 메서드 =====
    void GenerateLightViewMatrix(DirectX::XMFLOAT3 sceneCenter = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), float distance = 50.0f);
    void GenerateLightProjectionMatrix(float sceneWidth = 60.0f, float sceneHeight = 60.0f, float nearZ = 1.0f, float farZ = 120.0f);

    void GetLightViewMatrix(DirectX::XMMATRIX& lightView) const { lightView = m_lightViewMatrix; }
    void GetLightProjectionMatrix(DirectX::XMMATRIX& lightProj) const { lightProj = m_lightProjMatrix; }

    float* GetShadowBiasPtr() { return &m_shadowBias; }
    float GetShadowBias() const { return m_shadowBias; }
    void SetShadowBias(float bias) { m_shadowBias = bias; }

    float* GetShadowIntensityPtr() { return &m_shadowIntensity; }
    float GetShadowIntensity() const { return m_shadowIntensity; }
    void SetShadowIntensity(float intensity) { m_shadowIntensity = intensity; }

    bool* GetShadowEnabledPtr() { return &m_shadowEnabled; }
    bool IsShadowEnabled() const { return m_shadowEnabled; }
    void SetShadowEnabled(bool enabled) { m_shadowEnabled = enabled; }

    bool* GetPcfEnabledPtr() { return &m_pcfEnabled; }
    bool IsPcfEnabled() const { return m_pcfEnabled; }
    void SetPcfEnabled(bool enabled) { m_pcfEnabled = enabled; }

    // Sun Orbit (실시간 태양 자전/일주 운동)
    void UpdateSunOrbit(float dt);
    bool* GetSunAutoRotatePtr() { return &m_sunAutoRotate; }
    bool IsSunAutoRotate() const { return m_sunAutoRotate; }
    void SetSunAutoRotate(bool autoRotate) { m_sunAutoRotate = autoRotate; }

    float* GetSunRotateSpeedPtr() { return &m_sunRotateSpeed; }
    float GetSunRotateSpeed() const { return m_sunRotateSpeed; }
    void SetSunRotateSpeed(float speed) { m_sunRotateSpeed = speed; }

    float* GetSunAnglePtr() { return &m_sunAngle; }
    float GetSunAngle() const { return m_sunAngle; }
    void SetSunAngle(float angle) { m_sunAngle = angle; }

    // ===== 거리 기반 대기 안개 효과 (Distance Fog) =====
    DirectX::XMFLOAT4 GetFogColor() const { return m_fogColor; }
    DirectX::XMFLOAT4* GetFogColorPtr() { return &m_fogColor; }
    void SetFogColor(const DirectX::XMFLOAT4& color) { m_fogColor = color; }

    float GetFogStart() const { return m_fogStart; }
    float* GetFogStartPtr() { return &m_fogStart; }
    void SetFogStart(float start) { m_fogStart = start; }

    float GetFogEnd() const { return m_fogEnd; }
    float* GetFogEndPtr() { return &m_fogEnd; }
    void SetFogEnd(float end) { m_fogEnd = end; }

    bool IsFogEnabled() const { return m_fogEnabled; }
    bool* GetFogEnabledPtr() { return &m_fogEnabled; }
    void SetFogEnabled(bool enabled) { m_fogEnabled = enabled; }

private:
    void InitDefaults();
    void SyncFromLightClass();

private:
    LightClass* m_light = nullptr;

    // Directional Light
    DirectX::XMFLOAT4 m_dirAmbient = DirectX::XMFLOAT4(0, 0, 0, 1);
    DirectX::XMFLOAT4 m_dirDiffuse = DirectX::XMFLOAT4(1, 1, 1, 1);
    DirectX::XMFLOAT3 m_dirDirection = DirectX::XMFLOAT3(0, -1, 0);
    DirectX::XMFLOAT4 m_specularColor = DirectX::XMFLOAT4(1, 1, 1, 1);
    float m_specularPower = 32.0f;

    // Sun Orbit / Rotation
    bool m_sunAutoRotate = true;   // 실시간 태양 자전 활성화 (기본 ON)
    float m_sunRotateSpeed = 0.25f; // 회전 속도 (초당 라디안)
    float m_sunAngle = 0.8f;       // 현재 태양 공전 각도

    // Distance Fog (거리 기반 대기 안개)
    DirectX::XMFLOAT4 m_fogColor = DirectX::XMFLOAT4(0.72f, 0.76f, 0.85f, 1.0f); // 하늘색 조화 대기 안개
    float m_fogStart = 45.0f;     // 안개 시작 거리 (농장 마당 45m 내부는 쨍하고 선명하게 유지)
    float m_fogEnd = 135.0f;      // 안개 최대 거리 (135m 원경에서 대기 안개에 융합)
    bool m_fogEnabled = true;     // 안개 기본 ON

    // Point Lights
    int m_pointCount = 0;
    DirectX::XMFLOAT4 m_pointPos[NUM_LIGHTS] = {};
    DirectX::XMFLOAT4 m_pointDiff[NUM_LIGHTS] = {};

    // Attenuation
    float m_attenKc = 1.0f;
    float m_attenKl = 0.0f;
    float m_attenKq = 0.0f;
    float m_pointIntensityScale = 1.0f;

    // Toggles
    bool m_enableAmbient = true;
    bool m_enableDiffuse = true;
    bool m_enableSpecular = true;
    bool m_enablePointLight = true;
    bool m_prevPToggle = false;

    // Shadow Mapping
    DirectX::XMMATRIX m_lightViewMatrix = DirectX::XMMatrixIdentity();
    DirectX::XMMATRIX m_lightProjMatrix = DirectX::XMMatrixIdentity();
    float m_shadowBias = 0.0015f;
    float m_shadowIntensity = 0.85f; // 그림자 기본 짙기 (85% 어둡게 차단하여 선명한 그림자 표현)
    bool m_shadowEnabled = true;
    bool m_pcfEnabled = true;
};
