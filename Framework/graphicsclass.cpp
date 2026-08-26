////////////////////////////////////////////////////////////////////////////////
// Filename: graphicsclass.cpp
// Description: Refactored to use specialized manager classes
////////////////////////////////////////////////////////////////////////////////
#include "graphicsclass.h"
#include <DirectXMath.h>
#include <algorithm>
#include <Windows.h>

extern D3DClass* g_D3D;

using namespace DirectX;

GraphicsClass::GraphicsClass() {}
GraphicsClass::GraphicsClass(const GraphicsClass& other) {}
GraphicsClass::~GraphicsClass() {}

bool GraphicsClass::Initialize(int screenWidth, int screenHeight, HWND hwnd)
{
    bool result;
    XMMATRIX baseViewMatrix;

    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;

    // ========== D3D ==========
    m_D3D = new D3DClass;
    if (!m_D3D) return false;

    result = m_D3D->Initialize(screenWidth, screenHeight, VSYNC_ENABLED,
        hwnd, FULL_SCREEN, SCREEN_DEPTH, SCREEN_NEAR);
    if (!result) {
        MessageBox(hwnd, L"Could not initialize Direct3D.", L"Error", MB_OK);
        return false;
    }
    g_D3D = m_D3D;

    // ========== Camera ==========
    m_Camera = new CameraClass;
    if (!m_Camera) return false;

    m_Camera->SetPosition(0.0f, 1.7f, -5.0f);
    m_Camera->SetRotation(0.0f, 0.0f, 0.0f);

    // ========== Player Controller ==========
    m_playerController.Initialize(m_Camera);

    // ========== Shaders ==========
    m_MultiTexShader = new MultiTextureShaderClass;
    if (!m_MultiTexShader ||
        !m_MultiTexShader->Initialize(m_D3D->GetDevice(), hwnd,
            L"./data/MultiTextureShader.hlsl")) {
        MessageBox(hwnd, L"Could not initialize multi-texture shader.", L"Error", MB_OK);
        return false;
    }

    m_TextureShader = new TextureShaderClass;
    if (!m_TextureShader || !m_TextureShader->Initialize(m_D3D->GetDevice(), hwnd)) {
        MessageBox(hwnd, L"Could not initialize texture shader.", L"Error", MB_OK);
        return false;
    }

    m_LightShader = new LightShaderClass;
    if (!m_LightShader || !m_LightShader->Initialize(m_D3D->GetDevice(), hwnd)) {
        MessageBox(hwnd, L"Could not initialize the light shader object.", L"Error", MB_OK);
        return false;
    }

    m_SkinShader = new SkinShaderClass;
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

    // ========== UI ==========
    m_Bitmap = new BitmapClass;
    if (!m_Bitmap) return false;

    result = m_Bitmap->Initialize(m_D3D->GetDevice(), screenWidth, screenHeight,
        L"./data/skymap.dds", 256, 256);
    if (!result) {
        MessageBox(hwnd, L"Could not initialize the bitmap object.", L"Error", MB_OK);
        return false;
    }

    m_Camera->Render();
    m_Camera->GetViewMatrix(baseViewMatrix);

    m_Text = new TextClass;
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
    m_Light = new LightClass;
    if (!m_Light) return false;

    m_Light->SetAmbientColor(0.2f, 0.2f, 0.2f, 1.0f);
    m_Light->SetDiffuseColor(1.0f, 1.0f, 1.0f, 1.0f);
    m_Light->SetDirection(1.0f, 0.0f, 1.0f);
    m_Light->SetSpecularColor(1.0f, 1.0f, 1.0f, 1.0f);
    m_Light->SetSpecularPower(8.0f);

    // ========== Light Manager ==========
    m_lightManager.Initialize(m_Light);

    m_scene = SCENE_TITLE;

    return true;
}

void GraphicsClass::Shutdown()
{
    m_sceneManager.Shutdown();

    m_sky.Release();

    if (m_Text) { m_Text->Shutdown(); delete m_Text; m_Text = nullptr; }
    if (m_Bitmap) { m_Bitmap->Shutdown(); delete m_Bitmap; m_Bitmap = nullptr; }

    if (m_TextureShader) { m_TextureShader->Shutdown(); delete m_TextureShader; m_TextureShader = nullptr; }
    if (m_MultiTexShader) { m_MultiTexShader->Shutdown(); delete m_MultiTexShader; m_MultiTexShader = nullptr; }

    if (m_Light) { delete m_Light; m_Light = nullptr; }
    if (m_LightShader) { m_LightShader->Shutdown(); delete m_LightShader; m_LightShader = nullptr; }
    if (m_SkinShader) { m_SkinShader->Shutdown(); delete m_SkinShader; m_SkinShader = nullptr; }

    if (m_Camera) { delete m_Camera; m_Camera = nullptr; }
    if (m_D3D) { m_D3D->Shutdown(); delete m_D3D; m_D3D = nullptr; }
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

    // Check farmer collision
    if (m_sceneManager.IsFarmerVisible())
    {
        XMFLOAT3 farmerPos = m_sceneManager.GetFbxPosition(m_sceneManager.GetFarmerIndex());
        if (m_playerController.CheckFarmerCollision(farmerPos, 1.5f))
        {
            m_sceneManager.SetFarmerVisible(false);
        }
    }

    // Hay pickup logic
    if (m_sceneManager.IsHayVisible() && !m_projectileSystem.HasAmmo())
    {
        XMFLOAT3 hayPos = m_sceneManager.GetFbxPosition(m_sceneManager.GetHayIndex());
        float dx = camPos.x - hayPos.x;
        float dy = camPos.y - hayPos.y;
        float dz = camPos.z - hayPos.z;
        float distSq = dx * dx + dy * dy + dz * dz;
        const float pickupRadius = 2.0f;

        if (distSq < pickupRadius * pickupRadius)
        {
            m_sceneManager.SetHayVisible(false);
            m_projectileSystem.SetHasAmmo(true);
        }
    }

    // Projectile shooting
    static bool prevLButtonDown = false;
    bool currLButtonDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;

    if (currLButtonDown && !prevLButtonDown && m_projectileSystem.HasAmmo())
    {
        m_projectileSystem.Spawn(camPos, m_playerController.GetForward());
    }
    prevLButtonDown = currLButtonDown;

    // Update projectiles and check pig collision
    bool pigHit = false;
    XMFLOAT3 pigPos = m_sceneManager.GetFbxPosition(m_sceneManager.GetPigIndex());
    m_projectileSystem.Update(1.0f / 60.0f, pigPos, 1.0f, pigHit);

    if (pigHit)
    {
        m_sceneManager.SetPigVisible(false);
    }

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

    // Update UI text
    if (m_Text)
    {
        int polyCount = 0;
        int objCount = (int)m_sceneManager.GetFbxCount();

        for (size_t i = 0; i < m_sceneManager.GetFbxCount(); ++i)
        {
            FbxModelClass* f = m_sceneManager.GetFbxModel(i);
            if (f) polyCount += f->GetIndexCount() / 3;
        }

        m_Text->SetSceneInfo(
            m_fps, m_cpu,
            polyCount, objCount,
            m_screenWidth, m_screenHeight,
            m_D3D->GetDeviceContext()
        );
    }

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
// Render - Main scene rendering
//////////////////////////////////////////////////////////////////
bool GraphicsClass::Render()
{
    XMMATRIX worldMatrix, viewMatrix, projectionMatrix, orthoMatrix;

    m_D3D->BeginScene(0.0f, 0.0f, 0.0f, 1.0f);

    m_Camera->Render();
    m_Camera->GetViewMatrix(viewMatrix);
    m_D3D->GetWorldMatrix(worldMatrix);
    m_D3D->GetProjectionMatrix(projectionMatrix);
    m_D3D->GetOrthoMatrix(orthoMatrix);

    // ========== Skybox ==========
    {
        XMFLOAT3 camPos = m_Camera->GetPosition();
        m_sky.Update(camPos, viewMatrix, projectionMatrix);
        m_sky.Draw();
        m_D3D->GetDeviceContext()->OMSetDepthStencilState(nullptr, 0);
        m_D3D->GetDeviceContext()->RSSetState(nullptr);
    }

    m_D3D->TurnZBufferOn();

    // ========== Plane (Ground) ==========
    FbxModelClass* planeFbx = m_sceneManager.GetPlane();
    if (planeFbx && m_MultiTexShader)
    {
        XMFLOAT3 planePos = m_sceneManager.GetPlanePosition();
        float scale = 0.3f;
        XMMATRIX localWorld =
            XMMatrixScaling(scale, 1.0f, scale) *
            XMMatrixTranslation(planePos.x, planePos.y, planePos.z);

        planeFbx->Render(m_D3D->GetDeviceContext());

        XMFLOAT4 ambientColor = XMFLOAT4(0.35f, 0.35f, 0.35f, 1.0f);
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
            m_D3D->GetDeviceContext(),
            planeFbx->GetIndexCount(),
            localWorld, viewMatrix, projectionMatrix,
            m_sceneManager.GetTex0(),
            m_sceneManager.GetTex1(),
            m_sceneManager.GetTexAlpha(),
            0.8f,
            ambientColor,
            p0Pos, p0Color, p0Range,
            p1Pos, p1Color, p1Range
        );
    }

    // ========== FBX Models ==========
    size_t fenceIndex = m_sceneManager.GetFenceIndex();
    size_t treeIndex = m_sceneManager.GetTreeIndex();
    size_t hayIndex = m_sceneManager.GetHayIndex();
    size_t pigIndex = m_sceneManager.GetPigIndex();

    for (size_t i = 0; i < m_sceneManager.GetFbxCount(); ++i)
    {
        // Skip instanced models
        if (i == fenceIndex || i == treeIndex || i == hayIndex)
            continue;

        // Visibility checks
        if (i == 0 && !m_sceneManager.IsFarmerVisible())
            continue;
        if (i == pigIndex && !m_sceneManager.IsPigVisible())
            continue;

        FbxModelClass* f = m_sceneManager.GetFbxModel(i);
        if (!f) continue;

        XMFLOAT3 p = m_sceneManager.GetFbxPosition(i);
        float scale = 1.0f;
        XMMATRIX fbxWorld =
            XMMatrixScaling(scale, scale, scale) *
            XMMatrixTranslation(p.x, p.y, p.z);

        f->Render(m_D3D->GetDeviceContext());

        if (m_LightShader)
        {
            m_LightShader->RenderEx(
                m_D3D->GetDeviceContext(),
                f->GetIndexCount(),
                fbxWorld, viewMatrix, projectionMatrix,
                f->GetTexture(),
                m_Camera->GetPosition(),
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
                m_lightManager.IsSpecularEnabled()
            );
        }
    }

    // ========== Fence Instances ==========
    if (fenceIndex < m_sceneManager.GetFbxCount())
    {
        FbxModelClass* fenceModel = m_sceneManager.GetFbxModel(fenceIndex);
        if (fenceModel && fenceModel->GetInstanceCount() > 0 && m_TextureShader)
        {
            fenceModel->RenderInstanced(m_D3D->GetDeviceContext());
            XMMATRIX fenceWorld = XMMatrixIdentity();
            m_TextureShader->RenderInstanced(
                m_D3D->GetDeviceContext(),
                fenceModel->GetIndexCount(),
                fenceModel->GetInstanceCount(),
                fenceWorld, viewMatrix, projectionMatrix,
                fenceModel->GetTexture()
            );
        }
    }

    // ========== Tree Instances ==========
    if (treeIndex < m_sceneManager.GetFbxCount())
    {
        FbxModelClass* treeModel = m_sceneManager.GetFbxModel(treeIndex);
        if (treeModel && treeModel->GetInstanceCount() > 0 && m_TextureShader)
        {
            treeModel->RenderInstanced(m_D3D->GetDeviceContext());
            XMMATRIX world = XMMatrixIdentity();
            m_TextureShader->RenderInstanced(
                m_D3D->GetDeviceContext(),
                treeModel->GetIndexCount(),
                treeModel->GetInstanceCount(),
                world, viewMatrix, projectionMatrix,
                treeModel->GetTexture()
            );
        }
    }

    // ========== Hay (Ground + Projectiles) ==========
    if (hayIndex < m_sceneManager.GetFbxCount())
    {
        FbxModelClass* hayModel = m_sceneManager.GetFbxModel(hayIndex);
        if (hayModel && m_LightShader)
        {
            // Ground hay
            if (m_sceneManager.IsHayVisible())
            {
                XMFLOAT3 p = m_sceneManager.GetFbxPosition(hayIndex);
                XMMATRIX world =
                    XMMatrixScaling(1.0f, 1.0f, 1.0f) *
                    XMMatrixTranslation(p.x, p.y, p.z);

                hayModel->Render(m_D3D->GetDeviceContext());
                m_LightShader->RenderEx(
                    m_D3D->GetDeviceContext(),
                    hayModel->GetIndexCount(),
                    world, viewMatrix, projectionMatrix,
                    hayModel->GetTexture(),
                    m_Camera->GetPosition(),
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
                    m_lightManager.IsSpecularEnabled()
                );
            }

            // Projectiles
            for (const auto& proj : m_projectileSystem.GetProjectiles())
            {
                XMMATRIX world =
                    XMMatrixScaling(0.7f, 0.7f, 0.7f) *
                    XMMatrixTranslation(proj.pos.x, proj.pos.y, proj.pos.z);

                hayModel->Render(m_D3D->GetDeviceContext());
                m_LightShader->RenderEx(
                    m_D3D->GetDeviceContext(),
                    hayModel->GetIndexCount(),
                    world, viewMatrix, projectionMatrix,
                    hayModel->GetTexture(),
                    m_Camera->GetPosition(),
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
                    m_lightManager.IsSpecularEnabled()
                );
            }
        }
    }

    // ========== Skin Models ==========
    SkinModel* cowModel = m_sceneManager.GetCowModel();
    //if (cowModel && m_SkinShader)
    //{
    //    float scale = 3.0f;
    //    XMMATRIX cowWorld =
    //        XMMatrixScaling(scale, scale, scale) *
    //        XMMatrixTranslation(0.0f, 1.0f, 5.0f);

    //    cowModel->RenderSkinned(
    //        m_D3D->GetDeviceContext(),
    //        m_SkinShader,
    //        cowWorld, viewMatrix, projectionMatrix,
    //        m_sceneManager.GetCowTexture()
    //    );
    //}

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
            m_D3D->GetDeviceContext(),
            m_SkinShader,
            girlWorld, viewMatrix, projectionMatrix,
            m_sceneManager.GetFarmGirlTexture()
        );
    }

    // ========== 2D UI ==========
    m_D3D->TurnZBufferOff();
    m_D3D->TurnOnAlphaBlending();

    if (m_Text)
    {
        m_Text->SetTitleVisible(false);
        m_Text->Render(m_D3D->GetDeviceContext(), worldMatrix, orthoMatrix);
    }

    m_D3D->TurnOffAlphaBlending();
    m_D3D->TurnZBufferOn();

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
        m_Text->SetTitleLine(1, "Goal : Pick hay and throw to pig", dc);
        m_Text->SetTitleLine(2, "Control : WASD move, Mouse rotate, Mouse left click = Shoot hay", dc);
        m_Text->SetTitleLine(3, "Developer : C093199 Jae Wook-Lee", dc);
        m_Text->SetTitleLine(4, "[Enter] Start  [Esc] Exit", dc);

        m_Text->Render(dc, worldMatrix, orthoMatrix);
    }

    m_D3D->TurnOffAlphaBlending();
    m_D3D->TurnZBufferOn();

    m_D3D->EndScene();
    return true;
}
