////////////////////////////////////////////////////////////////////////////////
// Filename: graphicsclass.cpp
// Description: Refactored to use specialized manager classes + Dear ImGui Debug UI
////////////////////////////////////////////////////////////////////////////////
#include "graphicsclass.h"
#include <DirectXMath.h>
#include <algorithm>
#include <Windows.h>

// Dear ImGui 헤더 인클루드
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

// 이벤트 버스 헤더
#include "Event.h"
#include "EventBus.h"

extern D3DClass* g_D3D;

using namespace DirectX;

GraphicsClass::GraphicsClass() {}
GraphicsClass::~GraphicsClass() {}

bool GraphicsClass::Initialize(int screenWidth, int screenHeight, HWND hwnd)
{
    bool result;
    XMMATRIX baseViewMatrix;

    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;

    // ========== D3D (Smart Pointer RAII) ==========
    m_D3D = std::make_unique<D3DClass>();
    if (!m_D3D) return false;

    result = m_D3D->Initialize(screenWidth, screenHeight, VSYNC_ENABLED,
        hwnd, FULL_SCREEN, SCREEN_DEPTH, SCREEN_NEAR);
    if (!result) {
        MessageBox(hwnd, L"Could not initialize Direct3D.", L"Error", MB_OK);
        return false;
    }
    g_D3D = m_D3D.get();

    // ========== Camera (Smart Pointer RAII) ==========
    m_Camera = std::make_unique<CameraClass>();
    if (!m_Camera) return false;

    m_Camera->SetPosition(0.0f, 1.7f, -5.0f);
    m_Camera->SetRotation(0.0f, 0.0f, 0.0f);

    // ========== Player Controller ==========
    m_playerController.Initialize(m_Camera.get());

    // ========== Shadow Mapping (실시간 그림자) ==========
    m_ShadowMap = std::make_unique<ShadowMapClass>();
    if (!m_ShadowMap || !m_ShadowMap->Initialize(m_D3D->GetDevice(), 2048, 2048)) {
        MessageBox(hwnd, L"Could not initialize Shadow Map.", L"Error", MB_OK);
        return false;
    }

    m_DepthShader = std::make_unique<DepthShaderClass>();
    if (!m_DepthShader || !m_DepthShader->Initialize(m_D3D->GetDevice(), hwnd)) {
        MessageBox(hwnd, L"Could not initialize Depth Shader.", L"Error", MB_OK);
        return false;
    }

    // ========== Shaders (Smart Pointer RAII) ==========
    m_MultiTexShader = std::make_unique<MultiTextureShaderClass>();
    if (!m_MultiTexShader ||
        !m_MultiTexShader->Initialize(m_D3D->GetDevice(), hwnd,
            L"./data/MultiTextureShader.hlsl")) {
        MessageBox(hwnd, L"Could not initialize multi-texture shader.", L"Error", MB_OK);
        return false;
    }

    m_TextureShader = std::make_unique<TextureShaderClass>();
    if (!m_TextureShader || !m_TextureShader->Initialize(m_D3D->GetDevice(), hwnd)) {
        MessageBox(hwnd, L"Could not initialize texture shader.", L"Error", MB_OK);
        return false;
    }

    m_LightShader = std::make_unique<LightShaderClass>();
    if (!m_LightShader || !m_LightShader->Initialize(m_D3D->GetDevice(), hwnd)) {
        MessageBox(hwnd, L"Could not initialize the light shader object.", L"Error", MB_OK);
        return false;
    }

    m_SkinShader = std::make_unique<SkinShaderClass>();
    if (!m_SkinShader || !m_SkinShader->Initialize(m_D3D->GetDevice(), hwnd)) {
        MessageBox(hwnd, L"Could not initialize skin shader.", L"Error", MB_OK);
        return false;
    }

    // ========== Scene Manager ==========
    if (!m_sceneManager.Initialize(m_D3D->GetDevice(), hwnd)) {
        MessageBox(hwnd, L"Could not initialize scene manager.", L"Error", MB_OK);
        return false;
    }

    // Pass fence colliders to player controller
    m_playerController.SetFenceColliders(m_sceneManager.GetFenceColliders());

    // ========== Animal Quest System ==========
    m_questSystem.Initialize();

    // ========== Particle System (동물 먹이 반응 파티클) ==========
    if (!m_particleSystem.Initialize(m_D3D->GetDevice(), hwnd, L"./data/particle.hlsl")) {
        MessageBox(hwnd, L"Could not initialize particle system.", L"Error", MB_OK);
        return false;
    }

    // ========== Sound Manager (BGM 및 효과음 이벤트 연동) ==========
    m_soundManager.Initialize(hwnd);

    // ========== Water System (연못 수면 렌더링 시스템) ==========
    if (!m_water.Initialize(m_D3D->GetDevice(), hwnd, 10.0f, 10.0f, 32))
    {
        MessageBox(hwnd, L"Could not initialize water system.", L"Error", MB_OK);
        return false;
    }

    // ========== UI ==========
    m_Bitmap = std::make_unique<BitmapClass>();
    if (!m_Bitmap) return false;

    result = m_Bitmap->Initialize(m_D3D->GetDevice(), screenWidth, screenHeight,
        L"./data/skymap.dds", 256, 256);
    if (!result) {
        MessageBox(hwnd, L"Could not initialize the bitmap object.", L"Error", MB_OK);
        return false;
    }

    m_Camera->Render();
    m_Camera->GetViewMatrix(baseViewMatrix);

    m_Text = std::make_unique<TextClass>();
    if (!m_Text) return false;

    result = m_Text->Initialize(m_D3D->GetDevice(), m_D3D->GetDeviceContext(),
        hwnd, screenWidth, screenHeight, baseViewMatrix);
    if (!result) {
        MessageBox(hwnd, L"Could not initialize the text object.", L"Error", MB_OK);
        return false;
    }

    // ========== Skybox ==========
    if (!m_sky.Init(m_D3D->GetDevice(), m_D3D->GetDeviceContext(),
        L"./data/skymap.dds"))
        return false;

    // ========== Light ==========
    m_Light = std::make_unique<LightClass>();
    if (!m_Light) return false;

    m_Light->SetAmbientColor(0.25f, 0.25f, 0.25f, 1.0f);
    m_Light->SetDiffuseColor(1.0f, 1.0f, 1.0f, 1.0f);
    m_Light->SetDirection(0.5f, -1.0f, 0.5f); // 위에서 아래로 비추는 자연스러운 태양광 방향
    m_Light->SetSpecularColor(1.0f, 1.0f, 1.0f, 1.0f);
    m_Light->SetSpecularPower(16.0f);

    // ========== Light Manager ==========
    m_lightManager.Initialize(m_Light.get());

    // ========== Dear ImGui 초기화 ==========
    if (!InitImGui(hwnd))
    {
        MessageBox(hwnd, L"Could not initialize Dear ImGui.", L"Error", MB_OK);
        return false;
    }

    // 인트로 화면을 건너뛰고 즉시 3D 메인 게임 씬으로 시작
    m_scene = SCENE_MAIN;

    return true;
}

void GraphicsClass::Shutdown()
{
    // ImGui 해제
    ShutdownImGui();

    m_soundManager.Shutdown();
    m_water.Shutdown();
    m_particleSystem.Shutdown();

    if (m_DepthShader) { m_DepthShader->Shutdown(); m_DepthShader.reset(); }
    if (m_ShadowMap) { m_ShadowMap->Shutdown(); m_ShadowMap.reset(); }

    m_sceneManager.Shutdown();

    m_sky.Release();

    if (m_Text) { m_Text->Shutdown(); m_Text.reset(); }
    if (m_Bitmap) { m_Bitmap->Shutdown(); m_Bitmap.reset(); }

    if (m_TextureShader) { m_TextureShader->Shutdown(); m_TextureShader.reset(); }
    if (m_MultiTexShader) { m_MultiTexShader->Shutdown(); m_MultiTexShader.reset(); }

    m_Light.reset();
    if (m_LightShader) { m_LightShader->Shutdown(); m_LightShader.reset(); }
    if (m_SkinShader) { m_SkinShader->Shutdown(); m_SkinShader.reset(); }

    m_Camera.reset();
    g_D3D = nullptr;
    if (m_D3D) { m_D3D->Shutdown(); m_D3D.reset(); }
}

//////////////////////////////////////////////////////////////////
// Frame - Main game loop
//////////////////////////////////////////////////////////////////
bool GraphicsClass::Frame(int mouseDX, int mouseDY)
{
    // ========== Title Scene ==========
    if (m_scene == SCENE_TITLE)
    {
        if (GetAsyncKeyState(VK_RETURN) & 0x8000)
        {
            m_scene = SCENE_MAIN;
        }
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
        {
            return false;
        }
        if (m_scene == SCENE_TITLE)
        {
            return RenderTitle();
        }
    }

    // ========== Main Scene ==========

    // Update player controller
    m_playerController.Update(mouseDX, mouseDY);
    XMFLOAT3 camPos = m_playerController.GetPosition();

    // 시간 누적 (수면 파도 및 셰이더 애니메이션용)
    m_gameTime += 1.0f / 60.0f;

    // 1. 건초 줍기 처리 (맵의 건초 더미에 다가가면 획득)
    m_questSystem.TryPickupHay(camPos);

    // 2. [E] 키: 가까이 있는 동물에게 직접 먹이 주기
    static bool prevEKeyDown = false;
    bool currEKeyDown = (GetAsyncKeyState('E') & 0x8000) != 0;
    if (currEKeyDown && !prevEKeyDown)
    {
        std::string fedAnimalName;
        m_questSystem.TryFeedNearAnimal(camPos, fedAnimalName);
    }
    prevEKeyDown = currEKeyDown;

    // 3. [R] 키: 퀘스트 재시작 (동물들 다시 배고픔 상태로)
    static bool prevRKeyDown = false;
    bool currRKeyDown = (GetAsyncKeyState('R') & 0x8000) != 0;
    if (currRKeyDown && !prevRKeyDown)
    {
        m_questSystem.ResetQuest();
    }
    prevRKeyDown = currRKeyDown;

    // [M] 키: 사운드 전체 음소거 토글
    static bool prevMKeyDown = false;
    bool currMKeyDown = (GetAsyncKeyState('M') & 0x8000) != 0;
    if (currMKeyDown && !prevMKeyDown)
    {
        m_soundManager.ToggleMute();
    }
    prevMKeyDown = currMKeyDown;

    // 4. [F] 키: 건초 던지기 발사 (마우스 좌클릭 제거, 오직 F키로만 발사)
    static bool prevFKeyDown = false;
    bool currFKeyDown = (GetAsyncKeyState('F') & 0x8000) != 0;

    bool isShootTriggered = (currFKeyDown && !prevFKeyDown);

    if (isShootTriggered && m_questSystem.CanShoot())
    {
        m_projectileSystem.Spawn(camPos, m_playerController.GetForward());
        m_questSystem.ConsumeAmmo();
        EventBus::Get().Publish(HayThrownEvent{});
    }
    prevFKeyDown = currFKeyDown;

    // 5. 투사체 이동 및 퀘스트(동물 피격) 업데이트
    m_projectileSystem.UpdateMotionOnly(1.0f / 60.0f);
    m_questSystem.Update(1.0f / 60.0f, camPos, m_projectileSystem.GetProjectilesRef());

    // 6. 파티클 이펙트 업데이트
    m_particleSystem.Update(1.0f / 60.0f);

    // 7. 실시간 태양 자전 / 일주 운동 (그림자가 시간에 따라 실시간 회전)
    m_lightManager.UpdateSunOrbit(1.0f / 60.0f);

    // Update scene (animations)
    m_sceneManager.Update(1.0f / 30.0f);

    // Farm girl animation switching
    if (GetAsyncKeyState('1') & 0x0001)
    {
        m_sceneManager.SwitchFarmGirlAnimation(1);
    }
    if (GetAsyncKeyState('2') & 0x0001)
    {
        m_sceneManager.SwitchFarmGirlAnimation(2);
    }

    // Light hotkeys
    m_lightManager.HandleHotkeys();
    m_lightManager.TogglePointLight();

    return Render();
}

void GraphicsClass::SetCameraMove(float forward, float right, float up, float dtSeconds)
{
    m_playerController.SetMovement(forward, right, up, dtSeconds);
}

void GraphicsClass::SetMouseSensitivity(float yawSens, float pitchSens)
{
    m_playerController.SetSensitivity(yawSens, pitchSens);
}

void GraphicsClass::SetInvertY(bool invert)
{
    m_playerController.SetInvertY(invert);
}

//////////////////////////////////////////////////////////////////
// Render Context Builder
//////////////////////////////////////////////////////////////////
RenderContext GraphicsClass::BuildRenderContext()
{
    RenderContext ctx;
    ctx.deviceContext = m_D3D->GetDeviceContext();

    m_Camera->Render();
    m_Camera->GetViewMatrix(ctx.viewMatrix);
    m_D3D->GetWorldMatrix(ctx.worldMatrix);
    m_D3D->GetProjectionMatrix(ctx.projectionMatrix);
    m_D3D->GetOrthoMatrix(ctx.orthoMatrix);

    m_lightManager.GetLightViewMatrix(ctx.lightViewMatrix);
    m_lightManager.GetLightProjectionMatrix(ctx.lightProjectionMatrix);

    ctx.cameraPosition = m_Camera ? m_Camera->GetPosition() : XMFLOAT3(0, 0, 0);
    ctx.shadowMapSRV = m_ShadowMap ? m_ShadowMap->GetShaderResourceView() : nullptr;
    ctx.shadowBias = m_lightManager.GetShadowBias();
    ctx.shadowIntensity = m_lightManager.GetShadowIntensity();
    ctx.enableShadow = m_lightManager.IsShadowEnabled();
    ctx.enablePCF = m_lightManager.IsPcfEnabled();
    ctx.wireframe = m_wireframeMode;

    return ctx;
}

//////////////////////////////////////////////////////////////////
// [Pass 1] Depth Shadow Pass: 광원 시점 깊이(Shadow Map) 렌더링
//////////////////////////////////////////////////////////////////
void GraphicsClass::ExecuteShadowPass()
{
    if (!m_ShadowMap || !m_DepthShader) return;

    // 1. 플레이어 카메라 위치를 추적하는 Directional Light 뷰/직교 투영 행렬 생성
    XMFLOAT3 camPos = m_Camera ? m_Camera->GetPosition() : XMFLOAT3(0, 0, 0);
    m_lightManager.GenerateLightViewMatrix(XMFLOAT3(camPos.x, 0.0f, camPos.z), 100.0f);
    m_lightManager.GenerateLightProjectionMatrix(140.0f, 140.0f, 1.0f, 220.0f);

    XMMATRIX lightViewMatrix, lightProjectionMatrix;
    m_lightManager.GetLightViewMatrix(lightViewMatrix);
    m_lightManager.GetLightProjectionMatrix(lightProjectionMatrix);

    // 2. 섀도우 맵 Depth 버퍼 바인딩 및 뷰포트 설정
    m_ShadowMap->BindDsvAndSetNullRenderTarget(m_D3D->GetDeviceContext());

    size_t hayIndex = m_sceneManager.GetHayIndex();
    size_t pigIndex = m_sceneManager.GetPigIndex();
    size_t fenceIndex = m_sceneManager.GetFenceIndex();
    size_t treeIndex = m_sceneManager.GetTreeIndex();

    // 3. 정적 FBX 모델들의 Depth 렌더링 (헛간, 돼지, 말, 닭, 염소 등)
    for (size_t i = 0; i < m_sceneManager.GetFbxCount(); ++i)
    {
        if (i == fenceIndex || i == treeIndex || i == hayIndex) continue;
        if (i == 0 && !m_sceneManager.IsFarmerVisible()) continue;

        FbxModelClass* f = m_sceneManager.GetFbxModel(i);
        if (!f) continue;

        XMFLOAT3 p = m_sceneManager.GetFbxPosition(i);
        float rotY = 0.0f;

        // 퀘스트 동물 위치 및 뜀뛰기 모션 동기화
        for (const auto& a : m_questSystem.GetAnimals())
        {
            if (a.fbxIndex == i)
            {
                p = a.currentPos;
                rotY = a.rotationOffset;
                break;
            }
        }

        XMMATRIX fbxWorld = XMMatrixRotationY(rotY) * XMMatrixTranslation(p.x, p.y, p.z);

        f->Render(m_D3D->GetDeviceContext());
        m_DepthShader->Render(m_D3D->GetDeviceContext(), f->GetIndexCount(), fbxWorld, lightViewMatrix, lightProjectionMatrix);
    }

    // 4. 짚더미(Hay) 및 발사체(Projectiles) Depth 렌더링
    if (hayIndex < m_sceneManager.GetFbxCount())
    {
        FbxModelClass* hayModel = m_sceneManager.GetFbxModel(hayIndex);
        if (hayModel)
        {
            if (m_questSystem.IsHayAvailable())
            {
                XMFLOAT3 p = m_questSystem.GetHayPosition();
                XMMATRIX world = XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(p.x, p.y, p.z);
                hayModel->Render(m_D3D->GetDeviceContext());
                m_DepthShader->Render(m_D3D->GetDeviceContext(), hayModel->GetIndexCount(), world, lightViewMatrix, lightProjectionMatrix);
            }

            const auto& projs = m_projectileSystem.GetProjectiles();
            for (const auto& pr : projs)
            {
                XMMATRIX world = XMMatrixScaling(0.5f, 0.5f, 0.5f) * XMMatrixTranslation(pr.pos.x, pr.pos.y, pr.pos.z);
                hayModel->Render(m_D3D->GetDeviceContext());
                m_DepthShader->Render(m_D3D->GetDeviceContext(), hayModel->GetIndexCount(), world, lightViewMatrix, lightProjectionMatrix);
            }
        }
    }

    // 섀도우 패스 완료 후 OM 깊이 버퍼 언바인드
    ID3D11RenderTargetView* nullRTV = nullptr;
    m_D3D->GetDeviceContext()->OMSetRenderTargets(1, &nullRTV, nullptr);
}

//////////////////////////////////////////////////////////////////
// [Pass 2] Skybox & Environment Pass
//////////////////////////////////////////////////////////////////
void GraphicsClass::ExecuteSkyboxPass(const RenderContext& ctx)
{
    if (m_showSkybox)
    {
        m_sky.Update(ctx.cameraPosition, ctx.viewMatrix, ctx.projectionMatrix);
        m_sky.Draw();
        m_D3D->GetDeviceContext()->OMSetDepthStencilState(nullptr, 0);
        m_D3D->GetDeviceContext()->RSSetState(nullptr);
    }
}

//////////////////////////////////////////////////////////////////
// [Pass 3] MultiTexture Terrain Pass (지형, 그림자, 거리 안개)
//////////////////////////////////////////////////////////////////
void GraphicsClass::ExecuteTerrainPass(const RenderContext& ctx)
{
    m_D3D->TurnZBufferOn();
    m_D3D->SetWireframe(ctx.wireframe);

    FbxModelClass* planeFbx = m_sceneManager.GetPlane();
    if (planeFbx && m_MultiTexShader)
    {
        XMFLOAT3 planePos = m_sceneManager.GetPlanePosition();
        float scale = 0.3f;
        XMMATRIX localWorld =
            XMMatrixScaling(scale, 1.0f, scale) *
            XMMatrixTranslation(planePos.x, planePos.y, planePos.z);

        planeFbx->Render(ctx.deviceContext);

        XMFLOAT4 ambientColor = XMFLOAT4(0.12f, 0.12f, 0.12f, 1.0f);
        XMFLOAT3 p0Pos, p1Pos;
        XMFLOAT4 p0Color, p1Color;
        float p0Range, p1Range;

        if (m_lightManager.IsPointLightEnabled())
        {
            p0Pos = XMFLOAT3(-7.0f, 3.0f, 18.0f);
            p0Color = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
            p0Range = 8.0f;
            p1Pos = XMFLOAT3(-4.0f, 3.0f, 8.0f);
            p1Color = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
            p1Range = 8.0f;
        }
        else
        {
            p0Pos = p1Pos = XMFLOAT3(0, 0, 0);
            p0Color = p1Color = XMFLOAT4(0, 0, 0, 0);
            p0Range = p1Range = 0.0f;
        }

        m_MultiTexShader->Render(
            ctx.deviceContext,
            planeFbx->GetIndexCount(),
            localWorld, ctx.viewMatrix, ctx.projectionMatrix,
            m_sceneManager.GetTex0(),
            m_sceneManager.GetTex1(),
            m_sceneManager.GetTexAlpha(),
            0.8f,
            ambientColor,
            p0Pos, p0Color, p0Range,
            p1Pos, p1Color, p1Range,
            m_lightManager.GetAmbient(),
            m_lightManager.GetDiffuse(),
            m_lightManager.GetDirection(),
            ctx.shadowMapSRV,
            ctx.lightViewMatrix,
            ctx.lightProjectionMatrix,
            ctx.shadowBias,
            ctx.shadowIntensity,
            ctx.enableShadow,
            ctx.enablePCF,
            ctx.cameraPosition,
            m_lightManager.GetFogColor(),
            m_lightManager.GetFogStart(),
            m_lightManager.GetFogEnd(),
            m_lightManager.IsFogEnabled()
        );
    }
}

//////////////////////////////////////////////////////////////////
// [Pass 4] Static & Dynamic Mesh Pass (소품, 동물, 건초)
//////////////////////////////////////////////////////////////////
void GraphicsClass::ExecuteMeshPass(const RenderContext& ctx)
{
    size_t fenceIndex = m_sceneManager.GetFenceIndex();
    size_t treeIndex = m_sceneManager.GetTreeIndex();
    size_t hayIndex = m_sceneManager.GetHayIndex();
    size_t pigIndex = m_sceneManager.GetPigIndex();

    for (size_t i = 0; i < m_sceneManager.GetFbxCount(); ++i)
    {
        if (i == fenceIndex || i == treeIndex || i == hayIndex) continue;
        if (i == 0 && !m_sceneManager.IsFarmerVisible()) continue;
        if (i == pigIndex && !m_sceneManager.IsPigVisible()) continue;

        FbxModelClass* f = m_sceneManager.GetFbxModel(i);
        if (!f) continue;

        XMFLOAT3 p = m_sceneManager.GetFbxPosition(i);
        float rotY = 0.0f;

        for (const auto& a : m_questSystem.GetAnimals())
        {
            if (a.fbxIndex == i)
            {
                p = a.currentPos;
                rotY = a.rotationOffset;
                break;
            }
        }

        XMMATRIX fbxWorld = XMMatrixRotationY(rotY) * XMMatrixTranslation(p.x, p.y, p.z);

        f->Render(ctx.deviceContext);

        if (m_LightShader)
        {
            m_LightShader->RenderEx(
                ctx.deviceContext,
                f->GetIndexCount(),
                fbxWorld, ctx.viewMatrix, ctx.projectionMatrix,
                f->GetTexture(),
                ctx.cameraPosition,
                m_lightManager.GetAmbient(),
                m_lightManager.GetDiffuse(),
                m_lightManager.GetDirection(),
                m_lightManager.GetSpecularColor(),
                m_lightManager.GetSpecularPower(),
                m_lightManager.GetPointPositions(),
                m_lightManager.GetPointDiffuse(),
                m_lightManager.GetPointCount(),
                m_lightManager.GetAttenKc(),
                m_lightManager.GetAttenKl(),
                m_lightManager.GetAttenKq(),
                m_lightManager.GetIntensityScale(),
                m_lightManager.IsAmbientEnabled(),
                m_lightManager.IsDiffuseEnabled(),
                m_lightManager.IsSpecularEnabled(),
                ctx.shadowMapSRV,
                ctx.lightViewMatrix,
                ctx.lightProjectionMatrix,
                ctx.shadowBias,
                ctx.shadowIntensity,
                ctx.enableShadow,
                ctx.enablePCF,
                m_lightManager.GetFogColor(),
                m_lightManager.GetFogStart(),
                m_lightManager.GetFogEnd(),
                m_lightManager.IsFogEnabled()
            );
        }
    }

    // Hay & Projectiles
    if (hayIndex < m_sceneManager.GetFbxCount())
    {
        FbxModelClass* hayModel = m_sceneManager.GetFbxModel(hayIndex);
        if (hayModel && m_LightShader)
        {
            if (m_questSystem.IsHayAvailable())
            {
                XMFLOAT3 p = m_questSystem.GetHayPosition();
                XMMATRIX world = XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(p.x, p.y, p.z);
                hayModel->Render(ctx.deviceContext);
                m_LightShader->RenderEx(
                    ctx.deviceContext, hayModel->GetIndexCount(),
                    world, ctx.viewMatrix, ctx.projectionMatrix,
                    hayModel->GetTexture(), ctx.cameraPosition,
                    m_lightManager.GetAmbient(), m_lightManager.GetDiffuse(), m_lightManager.GetDirection(),
                    m_lightManager.GetSpecularColor(), m_lightManager.GetSpecularPower(),
                    m_lightManager.GetPointPositions(), m_lightManager.GetPointDiffuse(), m_lightManager.GetPointCount(),
                    m_lightManager.GetAttenKc(), m_lightManager.GetAttenKl(), m_lightManager.GetAttenKq(),
                    m_lightManager.GetIntensityScale(), m_lightManager.IsAmbientEnabled(), m_lightManager.IsDiffuseEnabled(), m_lightManager.IsSpecularEnabled(),
                    ctx.shadowMapSRV, ctx.lightViewMatrix, ctx.lightProjectionMatrix,
                    ctx.shadowBias, ctx.shadowIntensity, ctx.enableShadow, ctx.enablePCF,
                    m_lightManager.GetFogColor(), m_lightManager.GetFogStart(), m_lightManager.GetFogEnd(), m_lightManager.IsFogEnabled()
                );
            }

            for (const auto& proj : m_projectileSystem.GetProjectiles())
            {
                XMMATRIX world = XMMatrixScaling(0.7f, 0.7f, 0.7f) * XMMatrixTranslation(proj.pos.x, proj.pos.y, proj.pos.z);
                hayModel->Render(ctx.deviceContext);
                m_LightShader->RenderEx(
                    ctx.deviceContext, hayModel->GetIndexCount(),
                    world, ctx.viewMatrix, ctx.projectionMatrix,
                    hayModel->GetTexture(), ctx.cameraPosition,
                    m_lightManager.GetAmbient(), m_lightManager.GetDiffuse(), m_lightManager.GetDirection(),
                    m_lightManager.GetSpecularColor(), m_lightManager.GetSpecularPower(),
                    m_lightManager.GetPointPositions(), m_lightManager.GetPointDiffuse(), m_lightManager.GetPointCount(),
                    m_lightManager.GetAttenKc(), m_lightManager.GetAttenKl(), m_lightManager.GetAttenKq(),
                    m_lightManager.GetIntensityScale(), m_lightManager.IsAmbientEnabled(), m_lightManager.IsDiffuseEnabled(), m_lightManager.IsSpecularEnabled(),
                    ctx.shadowMapSRV, ctx.lightViewMatrix, ctx.lightProjectionMatrix,
                    ctx.shadowBias, ctx.shadowIntensity, ctx.enableShadow, ctx.enablePCF,
                    m_lightManager.GetFogColor(), m_lightManager.GetFogStart(), m_lightManager.GetFogEnd(), m_lightManager.IsFogEnabled()
                );
            }
        }
    }
}

//////////////////////////////////////////////////////////////////
// [Pass 5] Hardware Instancing Pass (울타리, 나무 대량 렌더링)
//////////////////////////////////////////////////////////////////
void GraphicsClass::ExecuteInstancingPass(const RenderContext& ctx)
{
    size_t fenceIndex = m_sceneManager.GetFenceIndex();
    if (fenceIndex < m_sceneManager.GetFbxCount())
    {
        FbxModelClass* fenceModel = m_sceneManager.GetFbxModel(fenceIndex);
        if (fenceModel && fenceModel->GetInstanceCount() > 0 && m_TextureShader)
        {
            fenceModel->RenderInstanced(ctx.deviceContext);
            XMMATRIX fenceWorld = XMMatrixIdentity();
            m_TextureShader->RenderInstanced(
                ctx.deviceContext,
                fenceModel->GetIndexCount(),
                fenceModel->GetInstanceCount(),
                fenceWorld, ctx.viewMatrix, ctx.projectionMatrix,
                fenceModel->GetTexture()
            );
        }
    }

    size_t treeIndex = m_sceneManager.GetTreeIndex();
    if (treeIndex < m_sceneManager.GetFbxCount())
    {
        FbxModelClass* treeModel = m_sceneManager.GetFbxModel(treeIndex);
        if (treeModel && treeModel->GetInstanceCount() > 0 && m_TextureShader)
        {
            treeModel->RenderInstanced(ctx.deviceContext);
            XMMATRIX world = XMMatrixIdentity();
            m_TextureShader->RenderInstanced(
                ctx.deviceContext,
                treeModel->GetIndexCount(),
                treeModel->GetInstanceCount(),
                world, ctx.viewMatrix, ctx.projectionMatrix,
                treeModel->GetTexture()
            );
        }
    }
}

//////////////////////////////////////////////////////////////////
// [Pass 6] Skinned Animation Pass (캐릭터 스키닝)
//////////////////////////////////////////////////////////////////
void GraphicsClass::ExecuteSkinnedPass(const RenderContext& ctx)
{
    SkinModel* farmGirl = m_sceneManager.GetCurrentFarmGirl();
    if (farmGirl && m_SkinShader)
    {
        float scale = 0.01f;
        float rotY = XMConvertToRadians(180.0f);
        XMMATRIX girlWorld =
            XMMatrixScaling(scale, scale, scale) *
            XMMatrixRotationY(rotY) *
            XMMatrixTranslation(0.0f, 0.7f, 1.0f);

        farmGirl->RenderSkinned(
            ctx.deviceContext,
            m_SkinShader.get(),
            girlWorld, ctx.viewMatrix, ctx.projectionMatrix,
            m_sceneManager.GetFarmGirlTexture()
        );
    }
}

//////////////////////////////////////////////////////////////////
// [Pass 7] Water Surface Pass (에메랄드빛 일렁이는 연못 수면)
//////////////////////////////////////////////////////////////////
void GraphicsClass::ExecuteWaterPass(const RenderContext& ctx)
{
    if (!m_water.IsEnabled()) return;

    m_D3D->TurnOnAlphaBlending();

    m_water.Render(
        ctx.deviceContext,
        ctx.viewMatrix, ctx.projectionMatrix,
        ctx.cameraPosition, m_gameTime,
        m_lightManager.GetDiffuse(), m_lightManager.GetDirection(),
        m_lightManager.GetSpecularColor(), m_lightManager.GetSpecularPower(),
        m_lightManager.GetFogColor(), m_lightManager.GetFogStart(), m_lightManager.GetFogEnd(),
        m_lightManager.IsFogEnabled()
    );

    m_D3D->TurnOffAlphaBlending();
}

//////////////////////////////////////////////////////////////////
// [Pass 8] Particle Pass (절차적 별빛 빌보드 파티클)
//////////////////////////////////////////////////////////////////
void GraphicsClass::ExecuteParticlePass(const RenderContext& ctx)
{
    m_particleSystem.Render(ctx.deviceContext, ctx.viewMatrix, ctx.projectionMatrix, ctx.cameraPosition);
}

//////////////////////////////////////////////////////////////////
// [Pass 9] UI & ImGui Pass (2D HUD 및 디버그 패널)
//////////////////////////////////////////////////////////////////
void GraphicsClass::ExecuteUIPass(const RenderContext& ctx)
{
    m_D3D->SetWireframe(false);
    RenderImGui();
}

//////////////////////////////////////////////////////////////////
// Render - Main Render Pipeline Coordinator
//////////////////////////////////////////////////////////////////
bool GraphicsClass::Render()
{
    // 1. [Pass 1: Depth Shadow Pass] 광원 시점 섀도우 맵 깊이 버퍼 기록
    ExecuteShadowPass();

    // 2. 메인 백버퍼 클리어 및 프레임 렌더 컨텍스트 생성
    m_D3D->BeginScene(0.0f, 0.0f, 0.0f, 1.0f);
    RenderContext ctx = BuildRenderContext();

    // 3. [Pass 2: Skybox Pass] 대기 및 하늘 배경 렌더링
    ExecuteSkyboxPass(ctx);

    // 4. [Pass 3: MultiTexture Terrain Pass] 바닥 지형 스플래팅 및 그림자/안개 합성
    ExecuteTerrainPass(ctx);

    // 5. [Pass 4: Static/Dynamic Mesh Pass] 농장 소품, 동물, 건초 발사체 렌더링
    ExecuteMeshPass(ctx);

    // 6. [Pass 5: Hardware Instancing Pass] 울타리/나무 하드웨어 인스턴싱 대량 렌더링
    ExecuteInstancingPass(ctx);

    // 7. [Pass 6: Skinned Animation Pass] 농장 소녀 스켈레탈 본 변형 애니메이션
    ExecuteSkinnedPass(ctx);

    // 8. [Pass 7: Water Surface Pass] 에메랄드빛 일렁이는 연못 수면 렌더링
    ExecuteWaterPass(ctx);

    // 9. [Pass 8: Particle Billboard Pass] 동물 먹이 반응 절차적 별빛 파티클 렌더링
    ExecuteParticlePass(ctx);

    // 10. [Pass 9: UI & ImGui Debug Pass] 인게임 퀘스트 HUD 및 실시간 디버그 패널
    ExecuteUIPass(ctx);

    m_D3D->EndScene();
    return true;
}

//////////////////////////////////////////////////////////////////
// RenderTitle - Title screen rendering
//////////////////////////////////////////////////////////////////
bool GraphicsClass::RenderTitle()
{
    XMMATRIX worldMatrix, orthoMatrix;

    m_D3D->BeginScene(0.0f, 0.0f, 0.2f, 1.0f);

    m_D3D->GetWorldMatrix(worldMatrix);
    m_D3D->GetOrthoMatrix(orthoMatrix);

    m_D3D->TurnZBufferOff();
    m_D3D->TurnOnAlphaBlending();

    if (m_Text)
    {
        ID3D11DeviceContext* dc = m_D3D->GetDeviceContext();

        m_Text->SetTitleVisible(true);
        m_Text->SetTitleLine(0, "Farm simulator", dc);
        m_Text->SetTitleLine(1, "Goal : Feed hungry farm animals with hay", dc);
        m_Text->SetTitleLine(2, "Control : WASD Move, Mouse Look, [F / Click] Shoot, [E] Feed, [R] Reset", dc);
        m_Text->SetTitleLine(3, "Developer : C093199 Jae Wook-Lee", dc);
        m_Text->SetTitleLine(4, "[Enter] Start  [Esc] Exit", dc);

        m_Text->Render(dc, worldMatrix, orthoMatrix);
    }

    m_D3D->TurnOffAlphaBlending();
    m_D3D->TurnZBufferOn();

    m_D3D->EndScene();
    return true;
}

//////////////////////////////////////////////////////////////////
// Dear ImGui 초기화 (InitImGui)
// DirectX 11 디바이스 및 Win32 창 핸들과 연동합니다.
//////////////////////////////////////////////////////////////////
bool GraphicsClass::InitImGui(HWND hwnd)
{
    // 1. ImGui 컨텍스트 생성 및 버전 검증
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // 키보드 탐색 활성화

    // 2. 한글 폰트 로드 (맑은 고딕 또는 굴림) - 한글 깨짐 완전 방지
    if (GetFileAttributesW(L"C:\\Windows\\Fonts\\malgun.ttf") != INVALID_FILE_ATTRIBUTES)
    {
        io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\malgun.ttf", 16.0f, NULL, io.Fonts->GetGlyphRangesKorean());
    }
    else if (GetFileAttributesW(L"C:\\Windows\\Fonts\\gulim.ttc") != INVALID_FILE_ATTRIBUTES)
    {
        io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\gulim.ttc", 16.0f, NULL, io.Fonts->GetGlyphRangesKorean());
    }
    else
    {
        io.Fonts->AddFontDefault();
    }

    // 3. 모던 다크 테마 적용
    ImGui::StyleColorsDark();

    // UI 스타일 커스텀 (모서리 둥글게, 여백 조절)
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.WindowBorderSize = 1.0f;

    // 4. Win32 및 DirectX 11 플랫폼 백엔드 초기화
    if (!ImGui_ImplWin32_Init(hwnd))
    {
        return false;
    }

    if (!ImGui_ImplDX11_Init(m_D3D->GetDevice(), m_D3D->GetDeviceContext()))
    {
        return false;
    }

    return true;
}

//////////////////////////////////////////////////////////////////
// Dear ImGui 리소스 해제 (ShutdownImGui)
//////////////////////////////////////////////////////////////////
void GraphicsClass::ShutdownImGui()
{
    if (ImGui::GetCurrentContext())
    {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }
}

//////////////////////////////////////////////////////////////////
// RenderQuestHUD - 인게임 퀘스트 HUD 오버레이
//////////////////////////////////////////////////////////////////
void GraphicsClass::RenderQuestHUD()
{
    ImGuiIO& io = ImGui::GetIO();
    float screenW = io.DisplaySize.x;
    float screenH = io.DisplaySize.y;

    ImGuiWindowFlags hudFlags = ImGuiWindowFlags_NoDecoration |
                               ImGuiWindowFlags_AlwaysAutoResize |
                               ImGuiWindowFlags_NoSavedSettings |
                               ImGuiWindowFlags_NoFocusOnAppearing |
                               ImGuiWindowFlags_NoNav |
                               ImGuiWindowFlags_NoMove;

    // [Tab/F1]을 눌러 커서가 해제된 UI 조작 모드가 아니라면(1인칭 게임 모드),
    // HUD가 마우스 클릭이나 호버를 일체 가로채지 않도록 마우스 입력을 100% 비활성화/투과
    if (m_isCursorLocked)
    {
        hudFlags |= ImGuiWindowFlags_NoMouseInputs | ImGuiWindowFlags_NoNavInputs;
    }

    // ========== 상단 퀘스트 진행도 HUD ==========
    ImGui::SetNextWindowPos(ImVec2(screenW * 0.5f, 15.0f), ImGuiCond_Always, ImVec2(0.5f, 0.0f));
    ImGui::SetNextWindowBgAlpha(0.80f);

    if (ImGui::Begin("##QuestHUDOverlay", nullptr, hudFlags))
    {
        ImGui::TextColored(ImVec4(1.0f, 0.88f, 0.35f, 1.0f), u8"[농장 퀘스트] 배고픈 동물들에게 건초를 먹여주세요!");
        ImGui::Separator();

        int fed = m_questSystem.GetFedCount();
        int total = m_questSystem.GetTotalCount();
        float progress = (total > 0) ? ((float)fed / (float)total) : 0.0f;

        // 진행도 바
        char buf[64];
        sprintf_s(buf, "%d / %d (%.0f%%)", fed, total, progress * 100.0f);
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.95f, 0.35f, 0.45f, 1.0f));
        ImGui::ProgressBar(progress, ImVec2(340, 20), buf);
        ImGui::PopStyleColor();

        // 탄약 및 리스폰 상태
        int ammo = m_questSystem.GetAmmo();
        int maxAmmo = m_questSystem.GetMaxAmmo();
        ImGui::Text(u8"보유 건초: %d / %d", ammo, maxAmmo);
        ImGui::SameLine();
        if (ammo > 0)
        {
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.5f, 1.0f), u8"[발사 가능]");
        }
        else
        {
            ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), u8"[건초 더미에서 보충 필요!]");
        }

        // 가까운 동물 상호작용 프롬프트 안내
        XMFLOAT3 camPos = m_Camera ? m_Camera->GetPosition() : XMFLOAT3(0, 0, 0);
        std::string prompt;
        if (m_questSystem.GetNearestAnimalPrompt(camPos, prompt))
        {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.2f, 1.0f), u8">> %s", prompt.c_str());
        }

        ImGui::Spacing();
        ImGui::TextDisabled(u8"[F: 건초 던지기]  [E: 직접 먹이기]  [R: 퀘스트 리셋]  [Tab/F1: 디버그 패널]");
    }
    ImGui::End();

    // ========== 미션 완료 (Mission Complete) 축하 팝업 ==========
    if (m_questSystem.IsCompleted())
    {
        ImGui::SetNextWindowPos(ImVec2(screenW * 0.5f, screenH * 0.45f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowBgAlpha(0.92f);

        if (ImGui::Begin("##MissionCompleteModal", nullptr, hudFlags))
        {
            ImGui::SetWindowFontScale(1.3f);
            ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.2f, 1.0f), u8"[ MISSION COMPLETE! ]");
            ImGui::SetWindowFontScale(1.0f);
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::Text(u8"축하합니다! 농장의 모든 동물들(말, 돼지, 닭, 염소)이 배부르고 행복해졌습니다!");
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.7f, 1.0f), u8"[R 키]를 누르면 퀘스트를 언제든 다시 시작할 수 있습니다.");
        }
        ImGui::End();
    }
}

//////////////////////////////////////////////////////////////////
// Dear ImGui 디버그 컨트롤 패널 렌더링 (RenderImGui)
//////////////////////////////////////////////////////////////////
void GraphicsClass::RenderImGui()
{
    // ImGui 새 프레임 시작
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // 마우스 커서 해제(UI 조작 모드) 시 ImGui가 화면에 직접 마우스 커서를 그리도록 설정
    ImGuiIO& io = ImGui::GetIO();
    io.MouseDrawCursor = (m_showImGui && !m_isCursorLocked);

    // 1. 인게임 퀘스트 HUD 오버레이 렌더링 (항상 표시)
    RenderQuestHUD();

    // 2. 디버그 컨트롤 패널 렌더링 (m_showImGui 일 때 표시)
    if (m_showImGui)
    {
        // 윈도우 초기 위치 및 크기 설정
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(400, 560), ImGuiCond_FirstUseEver);

        ImGuiWindowFlags panelFlags = 0;
        if (m_isCursorLocked)
        {
            panelFlags |= ImGuiWindowFlags_NoMouseInputs | ImGuiWindowFlags_NoNavInputs;
        }

        if (ImGui::Begin(u8"Pawdy Engine Debug Panel [Tab/F1: 커서 해제/잠금]", &m_showImGui, panelFlags))
        {
            // ------------------------------------------------------------
            // 1. 동물 먹이주기 퀘스트 상태 (Quest System)
            // ------------------------------------------------------------
            if (ImGui::CollapsingHeader(u8"동물 먹이주기 퀘스트 상태 (Quest)", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Text(u8"진행도: %d / %d", m_questSystem.GetFedCount(), m_questSystem.GetTotalCount());
                ImGui::Text(u8"보유 건초 탄약: %d / %d", m_questSystem.GetAmmo(), m_questSystem.GetMaxAmmo());
                ImGui::Text(u8"건초 더미 상태: %s", m_questSystem.IsHayAvailable() ? u8"스폰됨 (획득 가능)" : u8"리스폰 대기 중...");

                if (ImGui::Button(u8"퀘스트 리셋 (R 키)"))
                {
                    m_questSystem.ResetQuest();
                }

                ImGui::Separator();
                ImGui::TextDisabled(u8"[개별 동물 실시간 상태 (FSM)]");
                for (const auto& a : m_questSystem.GetAnimals())
                {
                    ImGui::BulletText("%s: %s", a.name.c_str(), a.GetStateName());
                }
                ImGui::Separator();
            }

            // ------------------------------------------------------------
            // 2. 실시간 성능 모니터링 (Performance & Stats)
            // ------------------------------------------------------------
            if (ImGui::CollapsingHeader(u8"성능 및 해상도 (Performance)", ImGuiTreeNodeFlags_DefaultOpen))
            {
                float frameTimeMs = (m_fps > 0) ? (1000.0f / m_fps) : 0.0f;
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "FPS: %d (%.2f ms/frame)", m_fps, frameTimeMs);
                ImGui::Text(u8"CPU 점유율: %d %%", m_cpu);
                ImGui::Text(u8"현재 해상도: %d x %d", m_screenWidth, m_screenHeight);
                ImGui::Separator();
            }

            // ------------------------------------------------------------
            // 3. 실시간 그림자 설정 (Shadow Mapping) & 태양 자전
            // ------------------------------------------------------------
            if (ImGui::CollapsingHeader(u8"실시간 그림자 및 태양 자전 (Shadow & Sun)", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Checkbox(u8"실시간 그림자 ON (Shadow Map)", m_lightManager.GetShadowEnabledPtr());
                ImGui::SameLine();
                ImGui::Checkbox(u8"3x3 PCF 소프트 섀도우 ON", m_lightManager.GetPcfEnabledPtr());

                ImGui::Checkbox(u8"태양 실시간 자전 ON (Sun Orbit)", m_lightManager.GetSunAutoRotatePtr());
                if (m_lightManager.IsSunAutoRotate())
                {
                    ImGui::SliderFloat(u8"자전 속도 (Speed)", m_lightManager.GetSunRotateSpeedPtr(), 0.05f, 2.0f, "%.2f");
                }
                else
                {
                    ImGui::SliderFloat(u8"태양 궤도 각도 (Angle)", m_lightManager.GetSunAnglePtr(), 0.0f, 6.283f, "%.2f");
                }

                ImGui::SliderFloat(u8"그림자 농도 (Darkness)", m_lightManager.GetShadowIntensityPtr(), 0.0f, 1.0f, "%.2f");
                ImGui::SliderFloat(u8"그림자 바이어스 (Bias)", m_lightManager.GetShadowBiasPtr(), 0.0001f, 0.0080f, "%.4f");
                ImGui::Text(u8"섀도우 맵 해상도: 2048 x 2048 (D32_FLOAT)");
                ImGui::Separator();
            }

            // ------------------------------------------------------------
            // 4. 대기 거리 안개 효과 (Distance Fog)
            // ------------------------------------------------------------
            if (ImGui::CollapsingHeader(u8"대기 거리 안개 효과 (Distance Fog)", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Checkbox(u8"거리 안개 ON (Fog Enabled)", m_lightManager.GetFogEnabledPtr());
                ImGui::ColorEdit4(u8"안개 색상 (Fog Color)", (float*)m_lightManager.GetFogColorPtr());
                ImGui::SliderFloat(u8"안개 시작 거리 (Start)", m_lightManager.GetFogStartPtr(), 0.0f, 50.0f, "%.1f m");
                ImGui::SliderFloat(u8"안개 최대 거리 (End)", m_lightManager.GetFogEndPtr(), 20.0f, 150.0f, "%.1f m");
                ImGui::TextDisabled(u8"원경의 산과 지형이 부드러운 대기 안개로 자연스럽게 융합됩니다.");
                ImGui::Separator();
            }

            // ------------------------------------------------------------
            // 5. 파티클 이펙트 시스템 (Particle System)
            // ------------------------------------------------------------
            if (ImGui::CollapsingHeader(u8"동물 먹이 반응 파티클 (Particle System)", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Text(u8"현재 활성 파티클: %d 개", m_particleSystem.GetActiveCount());
                if (ImGui::Button(u8"파티클 방출 테스트 (Spawn Sparkles)"))
                {
                    XMFLOAT3 p = m_playerController.GetPosition();
                    XMFLOAT3 f;
                    XMStoreFloat3(&f, m_playerController.GetForward());
                    m_particleSystem.SpawnFeedParticles(XMFLOAT3(p.x + f.x * 3.0f, p.y + 1.0f, p.z + f.z * 3.0f), 30);
                }
                ImGui::SameLine();
                if (ImGui::Button(u8"파티클 모두 제거 (Clear)"))
                {
                    m_particleSystem.Clear();
                }
                ImGui::Separator();
            }

            // ------------------------------------------------------------
            // 6. 방향성 조명 및 퐁 셰이딩 제어 (Directional Light Controls)
            // ------------------------------------------------------------
            if (ImGui::CollapsingHeader(u8"조명 및 셰이더 조절 (Lighting)", ImGuiTreeNodeFlags_DefaultOpen))
            {
                DirectX::XMFLOAT3* dir = m_lightManager.GetDirectionPtr();
                if (ImGui::SliderFloat3(u8"광원 방향 (Dir)", (float*)dir, -1.0f, 1.0f, "%.2f"))
                {
                    if (dir->x == 0 && dir->y == 0 && dir->z == 0) dir->y = -1.0f;
                }

                DirectX::XMFLOAT4* amb = m_lightManager.GetAmbientPtr();
                ImGui::ColorEdit4(u8"주변광 (Ambient)", (float*)amb);

                DirectX::XMFLOAT4* diff = m_lightManager.GetDiffusePtr();
                ImGui::ColorEdit4(u8"확산광 (Diffuse)", (float*)diff);

                DirectX::XMFLOAT4* spec = m_lightManager.GetSpecularColorPtr();
                ImGui::ColorEdit4(u8"반사광 (Specular)", (float*)spec);

                float* specPow = m_lightManager.GetSpecularPowerPtr();
                ImGui::SliderFloat(u8"광택 강도 (Shininess)", specPow, 1.0f, 128.0f, "%.1f");

                ImGui::Checkbox(u8"주변광 ON", m_lightManager.GetAmbientEnabledPtr());
                ImGui::SameLine();
                ImGui::Checkbox(u8"확산광 ON", m_lightManager.GetDiffuseEnabledPtr());
                ImGui::Checkbox(u8"반사광 ON", m_lightManager.GetSpecularEnabledPtr());
                ImGui::SameLine();
                ImGui::Checkbox(u8"포인트 라이트 ON", m_lightManager.GetPointLightEnabledPtr());

                ImGui::Separator();
            }

            // ------------------------------------------------------------
            // 5. 카메라 및 플레이어 실시간 좌표 (Camera / Player Info)
            // ------------------------------------------------------------
            if (ImGui::CollapsingHeader(u8"카메라 및 플레이어 정보 (Camera)", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (m_Camera)
                {
                    XMFLOAT3 pos = m_Camera->GetPosition();
                    XMFLOAT3 rot = m_Camera->GetRotation();
                    ImGui::Text(u8"카메라 위치: (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);
                    ImGui::Text(u8"카메라 회전: (Pitch: %.1f, Yaw: %.1f)", rot.x, rot.y);
                }
                ImGui::Separator();
            }

            // ------------------------------------------------------------
            // 6. 렌더링 모드 옵션 (Render Settings)
            // ------------------------------------------------------------
            if (ImGui::CollapsingHeader(u8"렌더링 모드 옵션 (Render Settings)", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Checkbox(u8"와이어프레임 뷰 (Wireframe)", &m_wireframeMode);
                ImGui::Checkbox(u8"스카이박스 표시 (Skybox)", &m_showSkybox);
                ImGui::Separator();
            }

            // ------------------------------------------------------------
            // 7. 오디오 및 효과음 시스템 (Sound System - EventBus 연동)
            // ------------------------------------------------------------
            if (ImGui::CollapsingHeader(u8"오디오 및 효과음 (Audio & Sound)", ImGuiTreeNodeFlags_DefaultOpen))
            {
                bool isMuted = m_soundManager.IsMuted();
                if (ImGui::Checkbox(u8"전체 음소거 (Mute) [M 키]", &isMuted))
                {
                    m_soundManager.SetMuted(isMuted);
                }

                if (ImGui::Button(u8"던지기 효과음"))
                {
                    m_soundManager.PlayThrowSound();
                }
                ImGui::SameLine();
                if (ImGui::Button(u8"먹이주기 벨"))
                {
                    m_soundManager.PlayFeedSound();
                }
                ImGui::SameLine();
                if (ImGui::Button(u8"승리 팡파레"))
                {
                    m_soundManager.PlayCompleteSound();
                }
                ImGui::Separator();
            }

            // ------------------------------------------------------------
            // 8. 수면 및 연못 시스템 (Water Surface & Pond System)
            // ------------------------------------------------------------
            if (ImGui::CollapsingHeader(u8"수면 및 연못 렌더링 (Water System)", ImGuiTreeNodeFlags_DefaultOpen))
            {
                bool waterEnabled = m_water.IsEnabled();
                if (ImGui::Checkbox(u8"연못 수면 활성화 (Enable Water)", &waterEnabled))
                {
                    m_water.SetEnabled(waterEnabled);
                }

                if (waterEnabled)
                {
                    XMFLOAT3& waterPos = m_water.GetPosition();
                    float posArray[3] = { waterPos.x, waterPos.y, waterPos.z };
                    if (ImGui::DragFloat3(u8"연못 위치 (Pos X/Y/Z)", posArray, 0.1f, -50.0f, 50.0f, "%.2f"))
                    {
                        waterPos = XMFLOAT3(posArray[0], posArray[1], posArray[2]);
                    }

                    ImGui::SliderFloat(u8"연못 크기 (Scale)", &m_water.GetScale(), 0.2f, 3.0f, "%.2f");
                    ImGui::SliderFloat(u8"파도 속도 (Wave Speed)", &m_water.GetWaveSpeed(), 0.0f, 3.0f, "%.2f");
                    ImGui::SliderFloat(u8"파도 높이 (Wave Height)", &m_water.GetWaveHeight(), 0.0f, 0.3f, "%.3f");
                    ImGui::SliderFloat(u8"파도 주파수 (Wave Freq)", &m_water.GetWaveFrequency(), 0.1f, 5.0f, "%.2f");
                    ImGui::SliderFloat(u8"수면 투명도 (Alpha)", &m_water.GetWaterAlpha(), 0.1f, 1.0f, "%.2f");

                    XMFLOAT4& deep = m_water.GetDeepColor();
                    float deepCol[3] = { deep.x, deep.y, deep.z };
                    if (ImGui::ColorEdit3(u8"깊은 물빛 (Deep Color)", deepCol))
                    {
                        deep = XMFLOAT4(deepCol[0], deepCol[1], deepCol[2], 1.0f);
                    }

                    XMFLOAT4& shallow = m_water.GetShallowColor();
                    float shallowCol[3] = { shallow.x, shallow.y, shallow.z };
                    if (ImGui::ColorEdit3(u8"반사 물빛 (Shallow Color)", shallowCol))
                    {
                        shallow = XMFLOAT4(shallowCol[0], shallowCol[1], shallowCol[2], 1.0f);
                    }
                }
                ImGui::Separator();
            }

            // ------------------------------------------------------------
            // 9. 단축키 안내
            // ------------------------------------------------------------
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), u8"[Tab/F1]: 커서 해제  [M]: 음소거 토글");
        }
        ImGui::End();
    }

    // ImGui 정점/인덱스 드로우 데이터 빌드 및 Direct3D 11 파이프라인에 렌더링
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

//////////////////////////////////////////////////////////////////
// OnResize - 창 크기 변경 시 해상도 및 렌더 타깃/투영행렬 갱신
//////////////////////////////////////////////////////////////////
void GraphicsClass::OnResize(int width, int height)
{
    if (width <= 0 || height <= 0) return;
    if (m_screenWidth == width && m_screenHeight == height) return;

    m_screenWidth = width;
    m_screenHeight = height;

    if (m_D3D)
    {
        m_D3D->Resize(width, height, SCREEN_NEAR, SCREEN_DEPTH);
    }
}


