// SkinModel.h
#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>
#include <string>

#include "ModelStructure.h"   // Vertex, Material, HierarchyMesh, NodeInfo, BoneInfo
#include "BaseModel.h"
#include "Animation.h"
#include "textureclass.h"
#include "SkinShaderClass.h"

using namespace DirectX;
using std::vector;
using std::wstring;

// HLSL VertexInputType �� 1:1�� ���ߴ� GPU�� ���� ����ü
struct SkinnedVertexType
{
    XMFLOAT3 position; // float3 POSITION
    XMFLOAT2 tex;      // float2 TEXCOORD
    XMUINT4  boneIdx;  // uint4  BONEID
    XMFLOAT4 weight;   // float4 WEIGHT
};

class SkinModel : public BaseModel
{
public:
    SkinModel();
    virtual ~SkinModel();

    void Release();

    // dt : ������ ��� �ð�(��)
    void Update(float dt);

    // ��Ű�� ���� (GraphicsClass ���� ���)
    void RenderSkinned(
        ID3D11DeviceContext* dc,
        SkinShaderClass* skinShader,
        const XMMATRIX& world,
        const XMMATRIX& view,
        const XMMATRIX& proj,
        ID3D11ShaderResourceView* diffuseTex
    );

    // �⺻ ���� (Draw �� Shader �ʿ��� ȣ��)
    void Render(ID3D11DeviceContext* deviceContext);

    // ModelLoader ���� ȣ���ϴ� ���� �Լ�
    bool CreateModel(
        ID3D11Device* device,
        const Vertex& vertices,
        const std::vector<unsigned long>& indices
    );

    // ���� �ڵ���� ȣȯ�� (���� ������Ʈ���� ModelLoader �� �ε�)
    // ���⼭�� �ؽ�ó�� �ε��ϵ��� ����
    bool LoadFromFbx(ID3D11Device* device, const WCHAR* modelFile, const WCHAR* texFile);

    // ���� �ڵ���� ȣȯ��: ���� ���迡���� SkinShaderClass �� �� ���۸� ��� �����Ƿ�
    // ���⼭�� �ƹ� �͵� �� �ϰ� true ��ȯ
    bool InitializeSkinBuffer(ID3D11Device* device);

    // Animation ����
    void PlayAni(int idx);
    void StopAni();
    void PauseAni();

    void UpdateMeshByMaterial();
    void SetBoneScaleFix(float scale) { _boneScaleFix = scale; }

    vector<HierarchyMesh*>& GetMeshList() { return _meshList; }
    vector<NodeInfo*>& GetNodeList() { return _nodeList; }

    vector<Material>& GetMaterialList() { return _materialList; }
    vector<Animation>& GetAnimationList() { return _aniList; }

    Animation GetAnimation(int num) { return _aniList[num]; }

    bool LoadTexture(ID3D11Device* device, const WCHAR* filename);
    ID3D11ShaderResourceView* GetTexture();

protected:
    // �ִϸ��̼� �� ��� TM �� �� ���(_boneMatrices) ����
    void UpdateNodeTM(float dt);

private:
    int                        _playAniIdx;          // ���� ��� ���� �ִϸ��̼� �ε��� (-1�̸� ����)
    float                      _aniTime;
    std::vector<XMMATRIX>      _boneMatrices;        // GPU�� ���� bone matrix �迭
    float _boneScaleFix = 100.0f;  // 기본값
    vector<Material>           _materialList;
    vector<Animation>          _aniList;
    vector<NodeInfo*>          _nodeList;
    vector<HierarchyMesh*>     _meshList;

    // �ʿ��ϸ� ������ �޽� �׷쿡 ������ ���ܵ�
    vector<vector<HierarchyMesh*>> _meshByMaterial;

    // ��ü �𵨿� �������� ���� ��ǻ�� �ؽ�ó
    TextureClass* m_Texture;
};
