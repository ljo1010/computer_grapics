////////////////////////////////////////////////////////////////////////////////
// Filename: LightManager.cpp
////////////////////////////////////////////////////////////////////////////////
#include "LightManager.h"
#include "lightclass.h"
#include <Windows.h>
#include <algorithm>

using namespace DirectX;

LightManager::LightManager()
{
}

LightManager::~LightManager()
{
}

void LightManager::Initialize(LightClass* light)
{
    m_light = light;
    InitDefaults();
    SyncFromLightClass();
}

void LightManager::InitDefaults()
{
    if (!m_light) return;

    m_dirAmbient = m_light->GetAmbientColor();
    m_dirDiffuse = m_light->GetDiffuseColor();
    m_dirDirection = m_light->GetDirection();
    m_specularColor = m_light->GetSpecularColor();
    m_specularPower = m_light->GetSpecularPower();

    // Point lights setup
    m_pointCount = 3;
    m_pointPos[0] = XMFLOAT4(-12.0f, 10.0f, 10.0f, 1.0f);
    m_pointPos[1] = XMFLOAT4(-12.0f, 10.0f, 25.0f, 1.0f);
    m_pointPos[2] = XMFLOAT4(0.0f, 10.0f, 25.0f, 1.0f);

    m_pointDiff[0] = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
    m_pointDiff[1] = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
    m_pointDiff[2] = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);

    // Attenuation
    m_attenKc = 1.0f;
    m_attenKl = 0.045f;
    m_attenKq = 0.0075f;

    m_pointIntensityScale = 1.0f;

    // Toggles
    m_enableAmbient = true;
    m_enableDiffuse = true;
    m_enableSpecular = true;
    m_enablePointLight = true;
    m_prevPToggle = false;
}

void LightManager::SyncFromLightClass()
{
    if (!m_light) return;

    m_dirAmbient = m_light->GetAmbientColor();
    m_dirDiffuse = m_light->GetDiffuseColor();
    m_dirDirection = m_light->GetDirection();
    m_specularColor = m_light->GetSpecularColor();
    m_specularPower = m_light->GetSpecularPower();
}

void LightManager::HandleHotkeys()
{
    // Toggle ambient/diffuse/specular with 5/6/7 keys
    if (GetAsyncKeyState('5') & 0x0001) m_enableAmbient = !m_enableAmbient;
    if (GetAsyncKeyState('6') & 0x0001) m_enableDiffuse = !m_enableDiffuse;
    if (GetAsyncKeyState('7') & 0x0001) m_enableSpecular = !m_enableSpecular;

    // Adjust point light intensity with 8/9 keys
    if (GetAsyncKeyState('8') & 0x8000)
        m_pointIntensityScale = std::clamp(m_pointIntensityScale + 0.02f, 0.0f, 8.0f);
    if (GetAsyncKeyState('9') & 0x8000)
        m_pointIntensityScale = std::clamp(m_pointIntensityScale - 0.02f, 0.0f, 8.0f);
}

void LightManager::TogglePointLight()
{
    bool isDown = (GetAsyncKeyState('P') & 0x8000) != 0;

    if (isDown && !m_prevPToggle)
    {
        m_enablePointLight = !m_enablePointLight;
    }

    m_prevPToggle = isDown;
}

// 섀도우 맵 렌더링용 광원 뷰 행렬 생성 (씬 중심을 바라보는 Directional Light 시점)
void LightManager::GenerateLightViewMatrix(DirectX::XMFLOAT3 sceneCenter, float distance)
{
    using namespace DirectX;
    XMVECTOR dirVec = XMVector3Normalize(XMLoadFloat3(&m_dirDirection));
    XMVECTOR centerVec = XMLoadFloat3(&sceneCenter);
    // 빛의 진행 방향 반대편으로 distance만큼 떨어진 위치에 가상 카메라 배치
    XMVECTOR lightPos = centerVec - dirVec * distance;
    XMVECTOR upVec = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    // 광원 방향이 수직(y축과 평행)일 경우 up 벡터 조정
    if (fabs(m_dirDirection.x) < 0.001f && fabs(m_dirDirection.z) < 0.001f)
    {
        upVec = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    }

    m_lightViewMatrix = XMMatrixLookAtLH(lightPos, centerVec, upVec);
}

// 섀도우 맵 렌더링용 직교 투영 행렬 생성 (방향성 광원용 직교 투영 볼륨)
void LightManager::GenerateLightProjectionMatrix(float sceneWidth, float sceneHeight, float nearZ, float farZ)
{
    m_lightProjMatrix = DirectX::XMMatrixOrthographicLH(sceneWidth, sceneHeight, nearZ, farZ);
}
