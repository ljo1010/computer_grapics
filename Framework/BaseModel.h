//====================================================================================
//      ## BaseModel ##
//      - ���� �� ���̽� Ŭ���� (���ؽ�/�ε��� ���� + ���� ��ȯ + �浹)
//      - �� ������ SetName / CreateModel / GetIndexCount ���� ����
//====================================================================================
#pragma once

#include <vector>
#include <string>
#include <d3d11.h>
#include <DirectXMath.h>

#include "ModelStructure.h"     // Vertex, HierarchyMesh, Material, ...
#include "CollisionHelper.h"    // AABB, BoundingSphereData, Collision::CalculateAABB

using namespace DirectX;

class BaseModel
{
public:
    BaseModel();
    virtual ~BaseModel();

    // ���ҽ� ����
    virtual void Release();

    // -----------------------------
    // ���� ��ȯ
    // -----------------------------
    void SetPosition(float x, float y, float z);
    void SetRotation(float x, float y, float z);   // rad ����
    void SetScale(float x, float y, float z);

    // _tmModel ���� + ����
    XMMATRIX UpdateModelMatrix();

    XMMATRIX        GetModelMatrix() const { return _tmModel; }
    const XMMATRIX& GetWorldMatrix() const { return _tmModel; }

    // -----------------------------
    // �浹 ����
    // -----------------------------
    const AABB& GetAabb()   const { return m_aabbWorld; }
    const BoundingSphereData& GetSphere() const { return m_sphere; }

    // ���� ��� ���� �� AABB ����
    void UpdateAabb();

    // -----------------------------
    // �̸� / �ε��� ����
    // -----------------------------
    void SetName(const std::wstring& name) { m_name = name; }
    const std::wstring& GetName() const { return m_name; }

    int GetIndexCount() const { return static_cast<int>(m_indexCount); }

    // -----------------------------
    // Vertex/Index �� GPU ���� ����
    //  - ModelLoader ���� ȣ��
    // -----------------------------
    bool CreateModel(ID3D11Device* device,
        Vertex& vertices,
        std::vector<unsigned long>& indices,
        D3D11_PRIMITIVE_TOPOLOGY primitiveType =
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // �Ļ� Ŭ����(SkinModel ��)���� override�ؼ� ��� ����
    virtual void Render(ID3D11DeviceContext* dc) {}

protected:
    // IA �ܰ迡 ���ؽ�/�ε��� ���� ���ε�
    void SetRenderBuffers(ID3D11DeviceContext* dc);

    // �ʿ��ϸ� ���� ���۸� ����
    void SetVertexBuffer(ID3D11Buffer* vb, UINT stride, UINT offset = 0);
    void SetIndexBuffer(ID3D11Buffer* ib, UINT indexCount);

    // ���� position ����Ʈ�κ��� AABB / Sphere ����
    void BuildBoundingVolumesFromVertices(const std::vector<XMFLOAT3>& positions);

protected:
    // GPU ����
    ID3D11Buffer* m_vertexBuffer;
    ID3D11Buffer* m_indexBuffer;

    UINT          m_vertexStride;
    UINT          m_vertexOffset;
    UINT          m_indexCount;

    // ���� ��ȯ
    XMFLOAT3      m_position;
    XMFLOAT3      m_rotation;
    XMFLOAT3      m_scale;
    XMMATRIX      _tmModel;

    // �浹��
    std::vector<XMFLOAT3> m_boundingBoxVertsLocal; // ���� AABB 8��
    AABB                  m_aabbWorld;
    BoundingSphereData    m_sphere;                // centerOffset, radius

    // ��Ÿ
    std::wstring          m_name;
    D3D11_PRIMITIVE_TOPOLOGY m_primitiveType;
};
