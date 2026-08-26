////////////////////////////////////////////////////////////////////////////////
// Filename: graphicsclass.h
// Description: Main graphics class - now delegates to specialized managers
////////////////////////////////////////////////////////////////////////////////
#ifndef _GRAPHICSCLASS_H_
#define _GRAPHICSCLASS_H_

////////////////
// STD/DirectX
////////////////
#include <vector>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <d3d11.h>

using namespace DirectX;

///////////////////////
// MY CLASS INCLUDES //
///////////////////////
#include "d3dclass.h"
#include "cameraclass.h"
#include "textureshaderclass.h"
#include "bitmapclass.h"
#include "lightshaderclass.h"
#include "lightclass.h"
#include "textclass.h"
#include "modelclass.h"
#include "Skybox.h"
#include "inputclass.h"
#include "multitextureshaderclass.h"
#include "SkinModel.h"
#include "FbxModelClass.h"
#include "SkinShaderClass.h"

// New managers & Shadow System
#include "LightManager.h"
#include "ProjectileSystem.h"
#include "AnimalQuestSystem.h"
#include "PlayerController.h"
#include "SceneManager.h"
#include "ShadowMapClass.h"
#include "DepthShaderClass.h"
#include "ParticleSystem.h"

/////////////
// GLOBALS //
/////////////
const bool  FULL_SCREEN = false;
const bool  VSYNC_ENABLED = true;
const float SCREEN_DEPTH = 1000.0f;
const float SCREEN_NEAR = 0.1f;

////////////////////////////////////////////////////////////////////////////////
// Class name: GraphicsClass
////////////////////////////////////////////////////////////////////////////////
class GraphicsClass
{
public:
    GraphicsClass();
    GraphicsClass(const GraphicsClass&);
    ~GraphicsClass();

    // Init / Shutdown
    bool Initialize(int screenWidth, int screenHeight, HWND hwnd);
    void Shutdown();

    // 창 크기 변경(Resize) 처리 함수
    void OnResize(int width, int height);

    // Frame
    bool Frame(int mouseDX, int mouseDY);

    // ImGui 디버그 패널 토글
    void ToggleImGui() { m_showImGui = !m_showImGui; }
    void SetImGuiVisible(bool visible) { m_showImGui = visible; }
    bool IsImGuiVisible() const { return m_showImGui; }

    // 마우스 커서 락 상태 동기화 (ImGui 마우스 커서 렌더링용)
    void SetCursorLocked(bool locked) { m_isCursorLocked = locked; }
    bool IsCursorLocked() const { return m_isCursorLocked; }

    // Input settings
    void SetCameraMove(float forward, float right, float up, float dtSeconds);
    void SetMouseSensitivity(float yawSens, float pitchSens);
    void SetInvertY(bool invert);
    void SetPerformance(int fps, int cpu) { m_fps = fps; m_cpu = cpu; }

private:
    // Render functions (2-Pass 렌더링 구조)
    bool Render();
    bool RenderShadowPass();
    bool RenderTitle();
    void RenderQuestHUD(); // 인게임 퀘스트 HUD 오버레이

    // Scene state
    enum SceneType { SCENE_TITLE, SCENE_MAIN };
    SceneType m_scene;

private:
    // ========== Core ==========
    D3DClass* m_D3D = nullptr;
    CameraClass* m_Camera = nullptr;

    // ========== Managers ==========
    LightManager m_lightManager;
    ProjectileSystem m_projectileSystem;
    AnimalQuestSystem m_questSystem;
    PlayerController m_playerController;
    SceneManager m_sceneManager;
    ParticleSystem m_particleSystem;

    // ========== Shaders ==========
    TextureShaderClass* m_TextureShader = nullptr;
    MultiTextureShaderClass* m_MultiTexShader = nullptr;
    LightShaderClass* m_LightShader = nullptr;
    SkinShaderClass* m_SkinShader = nullptr;
    DepthShaderClass* m_DepthShader = nullptr;

    // ========== Shadow Mapping ==========
    ShadowMapClass* m_ShadowMap = nullptr;

    // ========== Light ==========
    LightClass* m_Light = nullptr;

    // ========== UI ==========
    BitmapClass* m_Bitmap = nullptr;
    TextClass* m_Text = nullptr;
    Skybox m_sky;

    // ========== Screen ==========
    int m_screenWidth = 0;
    int m_screenHeight = 0;

    // ========== Performance ==========
    int m_fps = 0;
    int m_cpu = 0;

    // ========== Dear ImGui 디버그 UI ==========
    bool InitImGui(HWND hwnd);       // ImGui 초기화
    void ShutdownImGui();            // ImGui 정리
    void RenderImGui();              // ImGui 프레임 렌더링 및 컨트롤 패널 UI 정의

    bool m_showImGui = true;         // ImGui 디버그 윈도우 표시 여부
    bool m_isCursorLocked = true;    // 마우스 커서 잠금 여부
    bool m_wireframeMode = false;    // 와이어프레임 모드 토글 (true: Wireframe, false: Solid)
    bool m_showSkybox = true;        // 스카이박스 렌더링 토글
};

#endif // _GRAPHICSCLASS_H_
