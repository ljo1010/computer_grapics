////////////////////////////////////////////////////////////////////////////////
// Filename: SceneManager.h
// Description: Manages all scene objects (FBX models, skin models, textures)
////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <vector>
#include <DirectXMath.h>
#include <d3d11.h>
#include "PlayerController.h"  // For FenceCollider

class FbxModelClass;
class SkinModel;
class ModelLoader;

class SceneManager
{
public:
    SceneManager();
    ~SceneManager();

    bool Initialize(ID3D11Device* device, HWND hwnd);
    void Shutdown();
    void Update(float dt);

    // ========== FBX Models ==========
    FbxModelClass* GetFbxModel(size_t index);
    DirectX::XMFLOAT3 GetFbxPosition(size_t index) const;
    size_t GetFbxCount() const { return m_fbxModels.size(); }

    // ========== Skin Models ==========
    SkinModel* GetCowModel() { return m_CowModel; }
    SkinModel* GetCurrentFarmGirl() { return m_FarmGirlCurrent; }
    void SwitchFarmGirlAnimation(int index);
    DirectX::XMFLOAT3 GetCowPosition() const { return m_CowPos; }
    DirectX::XMFLOAT3 GetFarmGirlPosition() const { return m_FarmGirlPos; }

    // ========== Plane (Ground) ==========
    FbxModelClass* GetPlane() { return m_PlaneFbx; }
    DirectX::XMFLOAT3 GetPlanePosition() const { return m_PlanePos; }

    // ========== Visibility ==========
    void SetFarmerVisible(bool v) { m_farmerVisible = v; }
    void SetPigVisible(bool v) { m_pigVisible = v; }
    void SetHayVisible(bool v) { m_hayVisible = v; }
    bool IsFarmerVisible() const { return m_farmerVisible; }
    bool IsPigVisible() const { return m_pigVisible; }
    bool IsHayVisible() const { return m_hayVisible; }

    // ========== Textures ==========
    ID3D11ShaderResourceView* GetCowTexture() { return m_CowTex; }
    ID3D11ShaderResourceView* GetFarmGirlTexture() { return m_FarmGirlTex; }
    ID3D11ShaderResourceView* GetTex0() { return m_Tex0; }
    ID3D11ShaderResourceView* GetTex1() { return m_Tex1; }
    ID3D11ShaderResourceView* GetTexAlpha() { return m_TexAlpha; }
    ID3D11ShaderResourceView* GetMetallicMap() { return m_MetallicMap; }
    ID3D11ShaderResourceView* GetRoughnessMap() { return m_RoughnessMap; }

    // ========== Special Indices ==========
    size_t GetHayIndex() const { return m_hayIndex; }
    size_t GetPigIndex() const { return m_pigIndex; }
    size_t GetFenceIndex() const { return m_fenceIndex; }
    size_t GetTreeIndex() const { return m_treeIndex; }
    size_t GetFarmerIndex() const { return 0; }

    // ========== Fence Colliders ==========
    const std::vector<FenceCollider>& GetFenceColliders() const { return m_fenceColliders; }

private:
    bool LoadFbxModels(ID3D11Device* device, HWND hwnd);
    bool LoadSkinModels(ID3D11Device* device, HWND hwnd);
    bool LoadTextures(ID3D11Device* device, HWND hwnd);
    void SetupInstancing(ID3D11Device* device);

private:
    // ========== FBX Models ==========
    std::vector<FbxModelClass*> m_fbxModels;
    std::vector<DirectX::XMFLOAT3> m_fbxPositions;

    // ========== Skin Models ==========
    SkinModel* m_CowModel = nullptr;
    SkinModel* m_FarmGirlAni1 = nullptr;
    SkinModel* m_FarmGirlAni2 = nullptr;
    SkinModel* m_FarmGirlCurrent = nullptr;
    int m_FarmGirlAniIndex = 0;
    DirectX::XMFLOAT3 m_CowPos;
    DirectX::XMFLOAT3 m_FarmGirlPos;

    // ========== Plane ==========
    FbxModelClass* m_PlaneFbx = nullptr;
    DirectX::XMFLOAT3 m_PlanePos;

    // ========== Textures ==========
    ID3D11ShaderResourceView* m_Tex0 = nullptr;
    ID3D11ShaderResourceView* m_Tex1 = nullptr;
    ID3D11ShaderResourceView* m_TexAlpha = nullptr;
    ID3D11ShaderResourceView* m_CowTex = nullptr;
    ID3D11ShaderResourceView* m_FarmGirlTex = nullptr;
    ID3D11ShaderResourceView* m_MetallicMap = nullptr;
    ID3D11ShaderResourceView* m_RoughnessMap = nullptr;

    // ========== Visibility ==========
    bool m_farmerVisible = true;
    bool m_pigVisible = true;
    bool m_hayVisible = true;

    // ========== Indices ==========
    size_t m_hayIndex = 7;
    size_t m_pigIndex = 3;
    size_t m_fenceIndex = 8;
    size_t m_treeIndex = 10;

    // ========== Collision ==========
    std::vector<FenceCollider> m_fenceColliders;
};
