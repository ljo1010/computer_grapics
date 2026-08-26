////////////////////////////////////////////////////////////////////////////////
// Filename: SceneManager.cpp
////////////////////////////////////////////////////////////////////////////////
#include "SceneManager.h"
#include "FbxModelClass.h"
#include "SkinModel.h"
#include "ModelLoader.h"
#include "DDSTextureLoader.h"
#include <assimp/postprocess.h>

using namespace DirectX;

SceneManager::SceneManager()
{
    m_CowPos = XMFLOAT3(0.0f, 0.7f, 5.0f);
    m_FarmGirlPos = XMFLOAT3(3.0f, 0.7f, 4.0f);
    m_PlanePos = XMFLOAT3(0.0f, -0.2f, 0.0f);
}

SceneManager::~SceneManager()
{
}

bool SceneManager::Initialize(ID3D11Device* device, HWND hwnd)
{
    // Load textures first
    if (!LoadTextures(device, hwnd))
        return false;

    // Load FBX models
    if (!LoadFbxModels(device, hwnd))
        return false;

    // Load skin models
    if (!LoadSkinModels(device, hwnd))
        return false;

    // Setup instancing for fence and trees
    SetupInstancing(device);

    return true;
}

bool SceneManager::LoadTextures(ID3D11Device* device, HWND hwnd)
{
    // Multi-texture for plane
    if (FAILED(CreateDDSTextureFromFile(device,
        L"./data/dirttexture.dds", nullptr, &m_Tex0))) {
        MessageBox(hwnd, L"Failed to load dirttexture.dds", L"Error", MB_OK);
        return false;
    }
    if (FAILED(CreateDDSTextureFromFile(device,
        L"./data/dungeontextile.dds", nullptr, &m_Tex1))) {
        MessageBox(hwnd, L"Failed to load dungeontextile.dds", L"Error", MB_OK);
        return false;
    }
    if (FAILED(CreateDDSTextureFromFile(device,
        L"./data/alphatexture.dds", nullptr, &m_TexAlpha))) {
        MessageBox(hwnd, L"Failed to load alphatexture.dds", L"Error", MB_OK);
        return false;
    }

    // PBR textures for tractor
    if (FAILED(CreateDDSTextureFromFile(device,
        L"./data/tractor/Tractor_metalic.dds", nullptr, &m_MetallicMap))) {
        MessageBox(hwnd, L"Failed to load Tractor_metalic.dds", L"Error", MB_OK);
        return false;
    }
    if (FAILED(CreateDDSTextureFromFile(device,
        L"./data/tractor/Tractor_roughness.dds", nullptr, &m_RoughnessMap))) {
        MessageBox(hwnd, L"Failed to load Tractor_roughness.dds", L"Error", MB_OK);
        return false;
    }

    // Cow texture
    if (FAILED(CreateDDSTextureFromFile(device,
        L"./data/cow/Cow.dds", nullptr, &m_CowTex))) {
        MessageBox(hwnd, L"Failed to load cow texture.", L"Error", MB_OK);
        return false;
    }

    // Farm girl texture
    if (FAILED(CreateDDSTextureFromFile(device,
        L"./data/farm_girl/Farm_girl.dds", nullptr, &m_FarmGirlTex))) {
        MessageBox(hwnd, L"Failed to load Farm_girl.dds", L"Error", MB_OK);
        return false;
    }

    return true;
}

bool SceneManager::LoadFbxModels(ID3D11Device* device, HWND hwnd)
{
    // Helper lambda for adding FBX models
    auto addFbx = [&](const wchar_t* fbx, const wchar_t* tex, XMFLOAT3 pos) -> bool
    {
        FbxModelClass* m = new FbxModelClass;
        if (!m) return false;

        if (!m->Initialize(device, fbx, tex)) {
            MessageBox(hwnd, L"Could not initialize FBX model.", L"Error", MB_OK);
            delete m;
            return false;
        }

        m_fbxModels.push_back(m);
        m_fbxPositions.push_back(pos);
        return true;
    };

    // Plane (ground)
    m_PlaneFbx = new FbxModelClass;
    if (!m_PlaneFbx || !m_PlaneFbx->Initialize(device,
        L"./data/plane/Plane.fbx", L"./data/plane/Plane.dds")) {
        MessageBox(hwnd, L"Could not initialize plane fbx.", L"Error", MB_OK);
        return false;
    }

    // Index 0: Farmer
    if (!addFbx(L"./data/farmer/Farmer.fbx", L"./data/farmer/Farmer.dds",
        XMFLOAT3(6.0f, 0.0f, 2.0f))) return false;

    // Index 1: Barn
    if (!addFbx(L"./data/barn/Barn.fbx", L"./data/barn/Barn.dds",
        XMFLOAT3(-10.0f, 0.0f, 20.0f))) return false;

    // Index 2: Horse
    if (!addFbx(L"./data/horse/Horse.fbx", L"./data/horse/Horse.dds",
        XMFLOAT3(5.0f, 0.0f, 7.0f))) return false;

    // Index 3: Pig
    if (!addFbx(L"./data/pig/Pig.fbx", L"./data/pig/Pig.dds",
        XMFLOAT3(-4.0f, 0.0f, 8.0f))) return false;

    // Index 4: Chicken
    if (!addFbx(L"./data/chicken/Chicken.fbx", L"./data/chicken/Chicken.dds",
        XMFLOAT3(-8.0f, 0.0f, 15.0f))) return false;

    // Index 5: Goat
    if (!addFbx(L"./data/goat/Goat.fbx", L"./data/goat/Goat.dds",
        XMFLOAT3(0.0f, 0.2f, 10.0f))) return false;

    // Index 6: Windmill
    if (!addFbx(L"./data/windmill/Windmill.fbx", L"./data/windmill/Windmill.dds",
        XMFLOAT3(-10.0f, 0.0f, 0.0f))) return false;

    // Index 7: Hay
    if (!addFbx(L"./data/hay/Hay.fbx", L"./data/hay/Hay.dds",
        XMFLOAT3(-7.0f, 0.0f, 18.0f))) return false;
    m_hayIndex = 7;

    // Index 8: Fence
    if (!addFbx(L"./data/fence/Fence.fbx", L"./data/fence/Fence.dds",
        XMFLOAT3(10.0f, 0.4f, 10.0f))) return false;
    m_fenceIndex = 8;

    // Index 9: Tractor
    if (!addFbx(L"./data/tractor/Tractor.fbx", L"./data/tractor/Tractor.dds",
        XMFLOAT3(10.0f, 0.0f, 3.0f))) return false;

    // Index 10: Tree
    if (!addFbx(L"./data/tree/Tree.fbx", L"./data/tree/Tree.dds",
        XMFLOAT3(2.0f, 0.0f, 9.0f))) return false;
    m_treeIndex = 10;

    return true;
}

bool SceneManager::LoadSkinModels(ID3D11Device* device, HWND hwnd)
{
    ModelLoader loader;
    unsigned int flag =
        aiProcess_ConvertToLeftHanded |
        aiProcess_GenNormals |
        aiProcess_CalcTangentSpace;

    // Cow model
    m_CowModel = loader.LoadModel(
        L"./data/cow/Cow_walking.fbx",
        flag,
        device
    );
    if (!m_CowModel) {
        MessageBox(hwnd, L"Failed to load Cow_walking.fbx", L"Error", MB_OK);
        return false;
    }
    m_CowModel->SetBoneScaleFix(100.0f);

    auto& cowAniList = m_CowModel->GetAnimationList();
    if (!cowAniList.empty()) {
        cowAniList[0].SetRepeat(true);
        cowAniList[0].Play();
        m_CowModel->PlayAni(0);
    }

    // Farm girl animation 1
    m_FarmGirlAni1 = loader.LoadModel(
        L"./data/farm_girl/Farm_girl_ani1.fbx",
        flag,
        device
    );
    if (!m_FarmGirlAni1) {
        MessageBox(hwnd, L"Failed to load Farm_girl_ani1.fbx", L"Error", MB_OK);
        return false;
    }
    m_FarmGirlAni1->SetBoneScaleFix(10000.0f);

    auto& list1 = m_FarmGirlAni1->GetAnimationList();
    if (!list1.empty()) {
        for (auto& a : list1) a.Stop();
        list1[0].SetRepeat(true);
        list1[0].Play();
        m_FarmGirlAni1->PlayAni(0);
    }

    // Farm girl animation 2
    m_FarmGirlAni2 = loader.LoadModel(
        L"./data/farm_girl/Farm_girl_ani2.fbx",
        flag,
        device
    );
    if (!m_FarmGirlAni2) {
        MessageBox(hwnd, L"Failed to load Farm_girl_ani2.fbx", L"Error", MB_OK);
        return false;
    }

    auto& list2 = m_FarmGirlAni2->GetAnimationList();
    if (!list2.empty()) {
        for (auto& a : list2) a.Stop();
        list2[0].SetRepeat(true);
        list2[0].Play();
        m_FarmGirlAni2->PlayAni(0);
    }

    // Default to animation 1
    m_FarmGirlCurrent = m_FarmGirlAni1;
    m_FarmGirlAniIndex = 1;

    return true;
}

void SceneManager::SetupInstancing(ID3D11Device* device)
{
    // Fence instances
    if (m_fenceIndex < m_fbxModels.size())
    {
        std::vector<XMFLOAT3> fencePositions = {
            XMFLOAT3(10.0f, 0.4f, 38.0f),
            XMFLOAT3(12.0f, 0.4f, 38.0f),
            XMFLOAT3(14.0f, 0.4f, 38.0f),
            XMFLOAT3(16.0f, 0.4f, 38.0f),
            XMFLOAT3(18.0f, 0.4f, 38.0f),
            XMFLOAT3(20.0f, 0.4f, 38.0f),
            XMFLOAT3(22.0f, 0.4f, 38.0f),
            XMFLOAT3(24.0f, 0.4f, 38.0f),
            XMFLOAT3(10.0f, 0.4f, 38.0f),
            XMFLOAT3(12.0f, 0.4f, 38.0f),
            XMFLOAT3(14.0f, 0.4f, 38.0f),
            XMFLOAT3(16.0f, 0.4f, 38.0f),
            XMFLOAT3(8.0f, 0.4f, 38.0f),
            XMFLOAT3(6.0f, 0.4f, 38.0f),
            XMFLOAT3(4.0f, 0.4f, 38.0f),
            XMFLOAT3(2.0f, 0.4f, 38.0f),
            XMFLOAT3(-2.0f, 0.4f, 38.0f),
            XMFLOAT3(-4.0f, 0.4f, 38.0f),
            XMFLOAT3(-6.0f, 0.4f, 38.0f),
            XMFLOAT3(-8.0f, 0.4f, 38.0f),
            XMFLOAT3(-10.0f, 0.4f, 38.0f),
            XMFLOAT3(-12.0f, 0.4f, 38.0f),
            XMFLOAT3(-14.0f, 0.4f, 38.0f),
            XMFLOAT3(-16.0f, 0.4f, 38.0f),
            XMFLOAT3(-18.0f, 0.4f, 38.0f),
            XMFLOAT3(-20.0f, 0.4f, 38.0f),
            XMFLOAT3(-22.0f, 0.4f, 38.0f),
            XMFLOAT3(-24.0f, 0.4f, 38.0f),
            XMFLOAT3(-26.0f, 0.4f, 38.0f),
            XMFLOAT3(-28.0f, 0.4f, 38.0f),
            XMFLOAT3(-30.0f, 0.4f, 38.0f),
            XMFLOAT3(-32.0f, 0.4f, 38.0f),
        };

        FbxModelClass* fenceModel = m_fbxModels[m_fenceIndex];
        fenceModel->InitializeInstanceBuffer(device, fencePositions);

        // Setup fence colliders
        m_fenceColliders.clear();
        const float fenceRadius = 0.8f;
        for (const auto& p : fencePositions)
        {
            FenceCollider col;
            col.center = p;
            col.radius = fenceRadius;
            m_fenceColliders.push_back(col);
        }
    }

    // Tree instances
    if (m_treeIndex < m_fbxModels.size())
    {
        std::vector<XMFLOAT3> treePositions = {
            XMFLOAT3(2.0f, 0.0f, 9.0f),
            XMFLOAT3(-3.0f, 0.0f, 12.0f),
            XMFLOAT3(6.0f, 0.0f, 15.0f),
            XMFLOAT3(-6.0f, 0.0f, 7.0f),
            XMFLOAT3(15.0f, 0.0f, 10.0f),
            XMFLOAT3(20.0f, 0.0f, 15.0f),
            XMFLOAT3(25.0f, 0.0f, 20.0f),
            XMFLOAT3(18.0f, 0.0f, 25.0f),
            XMFLOAT3(22.0f, 0.0f, 30.0f),
            XMFLOAT3(28.0f, 0.0f, 35.0f),
            XMFLOAT3(-15.0f, 0.0f, 12.0f),
            XMFLOAT3(-20.0f, 0.0f, 18.0f),
            XMFLOAT3(-25.0f, 0.0f, 24.0f),
            XMFLOAT3(-18.0f, 0.0f, 30.0f),
            XMFLOAT3(-12.0f, 0.0f, 32.0f),
            XMFLOAT3(0.0f, 0.0f, 30.0f),
            XMFLOAT3(-8.0f, 0.0f, -10.0f),
            XMFLOAT3(8.0f, 0.0f, -12.0f),
            XMFLOAT3(-15.0f, 0.0f, -18.0f),
            XMFLOAT3(15.0f, 0.0f, -20.0f),
            XMFLOAT3(18.0f, 0.0f, -5.0f),
            XMFLOAT3(-18.0f, 0.0f, -6.0f),
            XMFLOAT3(25.0f, 0.0f, 5.0f),
            XMFLOAT3(-25.0f, 0.0f, 8.0f),
        };

        FbxModelClass* treeModel = m_fbxModels[m_treeIndex];
        treeModel->InitializeInstanceBuffer(device, treePositions);
    }
}

void SceneManager::Shutdown()
{
    // Plane
    if (m_PlaneFbx) {
        m_PlaneFbx->Shutdown();
        delete m_PlaneFbx;
        m_PlaneFbx = nullptr;
    }

    // FBX models
    for (auto* f : m_fbxModels) {
        if (f) {
            f->Shutdown();
            delete f;
        }
    }
    m_fbxModels.clear();
    m_fbxPositions.clear();

    // Skin models
    if (m_CowModel) {
        delete m_CowModel;
        m_CowModel = nullptr;
    }
    if (m_FarmGirlAni1) {
        delete m_FarmGirlAni1;
        m_FarmGirlAni1 = nullptr;
    }
    if (m_FarmGirlAni2) {
        delete m_FarmGirlAni2;
        m_FarmGirlAni2 = nullptr;
    }
    m_FarmGirlCurrent = nullptr;

    // Textures
    if (m_Tex0) { m_Tex0->Release(); m_Tex0 = nullptr; }
    if (m_Tex1) { m_Tex1->Release(); m_Tex1 = nullptr; }
    if (m_TexAlpha) { m_TexAlpha->Release(); m_TexAlpha = nullptr; }
    if (m_CowTex) { m_CowTex->Release(); m_CowTex = nullptr; }
    if (m_FarmGirlTex) { m_FarmGirlTex->Release(); m_FarmGirlTex = nullptr; }
    if (m_MetallicMap) { m_MetallicMap->Release(); m_MetallicMap = nullptr; }
    if (m_RoughnessMap) { m_RoughnessMap->Release(); m_RoughnessMap = nullptr; }

    m_fenceColliders.clear();
}

void SceneManager::Update(float dt)
{
    // Update cow animation
    if (m_CowModel)
    {
        m_CowModel->Update(dt);
    }

    // Update farm girl animation
    if (m_FarmGirlCurrent)
    {
        m_FarmGirlCurrent->Update(dt);
    }
}

FbxModelClass* SceneManager::GetFbxModel(size_t index)
{
    if (index < m_fbxModels.size())
        return m_fbxModels[index];
    return nullptr;
}

XMFLOAT3 SceneManager::GetFbxPosition(size_t index) const
{
    if (index < m_fbxPositions.size())
        return m_fbxPositions[index];
    return XMFLOAT3(0, 0, 0);
}

void SceneManager::SwitchFarmGirlAnimation(int index)
{
    if (index == 1 && m_FarmGirlAni1)
    {
        m_FarmGirlCurrent = m_FarmGirlAni1;
        m_FarmGirlAniIndex = 1;
    }
    else if (index == 2 && m_FarmGirlAni2)
    {
        m_FarmGirlCurrent = m_FarmGirlAni2;
        m_FarmGirlAniIndex = 2;
    }
}
