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

    // Point Light Getters
    const DirectX::XMFLOAT4* GetPointPositions() const { return m_pointPos; }
    const DirectX::XMFLOAT4* GetPointDiffuse() const { return m_pointDiff; }
    int GetPointCount() const { return m_pointCount; }

    // Attenuation Getters
    float GetAttenKc() const { return m_attenKc; }
    float GetAttenKl() const { return m_attenKl; }
    float GetAttenKq() const { return m_attenKq; }
    float GetIntensityScale() const { return m_pointIntensityScale; }

    // Toggle Getters
    bool IsAmbientEnabled() const { return m_enableAmbient; }
    bool IsDiffuseEnabled() const { return m_enableDiffuse; }
    bool IsSpecularEnabled() const { return m_enableSpecular; }
    bool IsPointLightEnabled() const { return m_enablePointLight; }

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
};
