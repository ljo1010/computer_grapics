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

// New managers
#include "LightManager.h"
#include "ProjectileSystem.h"
#include "PlayerController.h"
#include "SceneManager.h"

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

    // Frame
    bool Frame(int mouseDX, int mouseDY);

    // Input settings
    void SetCameraMove(float forward, float right, float up, float dtSeconds);
    void SetMouseSensitivity(float yawSens, float pitchSens);
    void SetInvertY(bool invert);
    void SetPerformance(int fps, int cpu) { m_fps = fps; m_cpu = cpu; }

private:
    // Render functions
    bool Render();
    bool RenderTitle();

    // Scene state
    enum SceneType { SCENE_TITLE, SCENE_MAIN };
    SceneType m_scene;

private:
    // ========== Core ==========
    D3DClass* m_D3D = nullptr;
    CameraClass* m_Camera = nullptr;

    // ========== Managers (NEW) ==========
    LightManager m_lightManager;
    ProjectileSystem m_projectileSystem;
    PlayerController m_playerController;
    SceneManager m_sceneManager;

    // ========== Shaders ==========
    TextureShaderClass* m_TextureShader = nullptr;
    MultiTextureShaderClass* m_MultiTexShader = nullptr;
    LightShaderClass* m_LightShader = nullptr;
    SkinShaderClass* m_SkinShader = nullptr;

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
};

#endif // _GRAPHICSCLASS_H_
