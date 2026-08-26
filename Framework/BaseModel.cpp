//====================================================================================
//      BaseModel.cpp - ���� ����
//====================================================================================
#include "BaseModel.h"
#include <cfloat>
#include <algorithm>
#include <cmath>

using std::vector;

// ��Ű�׿� ���� ����ü
// HLSL �Է� ���̾ƿ�: POSITION, TEXCOORD, BONEID, WEIGHT �� ����
struct SkinnedVertex
{
    XMFLOAT3 position;
    XMFLOAT2 tex;
    XMUINT4  boneIdx;
    XMFLOAT4 weight;
};

// -----------------------------
// ���� / �Ҹ�
// -----------------------------
BaseModel::BaseModel()
{
    m_vertexBuffer = nullptr;
    m_indexBuffer = nullptr;

    m_vertexStride = 0;
    m_vertexOffset = 0;
    m_indexCount = 0;

    m_position = XMFLOAT3(0.0f, 0.0f, 0.0f);
    m_rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
    m_scale = XMFLOAT3(1.0f, 1.0f, 1.0f);

    _tmModel = XMMatrixIdentity();

    m_aabbWorld = {};
    m_sphere.centerOffset = XMFLOAT3(0, 0, 0);
    m_sphere.radius = 0.0f;

    m_primitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
}

BaseModel::~BaseModel()
{
    Release();
}

void BaseModel::Release()
{
    if (m_vertexBuffer)
    {
        m_vertexBuffer->Release();
        m_vertexBuffer = nullptr;
    }

    if (m_indexBuffer)
    {
        m_indexBuffer->Release();
        m_indexBuffer = nullptr;
    }

    m_boundingBoxVertsLocal.clear();
    m_indexCount = 0;
}

// -----------------------------
// Ʈ������
// -----------------------------
void BaseModel::SetPosition(float x, float y, float z)
{
    m_position = XMFLOAT3(x, y, z);
    UpdateModelMatrix();
    UpdateAabb();
}

void BaseModel::SetRotation(float x, float y, float z)
{
    m_rotation = XMFLOAT3(x, y, z);
    UpdateModelMatrix();
    UpdateAabb();
}

void BaseModel::SetScale(float x, float y, float z)
{
    m_scale = XMFLOAT3(x, y, z);
    UpdateModelMatrix();
    UpdateAabb();
}

XMMATRIX BaseModel::UpdateModelMatrix()
{
    XMMATRIX S = XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z);
    XMMATRIX R = XMMatrixRotationRollPitchYaw(
        m_rotation.x, m_rotation.y, m_rotation.z);
    XMMATRIX T = XMMatrixTranslation(
        m_position.x, m_position.y, m_position.z);

    _tmModel = S * R * T;
    return _tmModel;
}

// -----------------------------
// ���� ����
// -----------------------------
void BaseModel::SetVertexBuffer(ID3D11Buffer* vb, UINT stride, UINT offset)
{
    m_vertexBuffer = vb;
    m_vertexStride = stride;
    m_vertexOffset = offset;
}

void BaseModel::SetIndexBuffer(ID3D11Buffer* ib, UINT indexCount)
{
    m_indexBuffer = ib;
    m_indexCount = indexCount;
}

void BaseModel::SetRenderBuffers(ID3D11DeviceContext* dc)
{
    if (!dc) return;

    // ���� ����
    if (m_vertexBuffer)
    {
        dc->IASetVertexBuffers(0, 1, &m_vertexBuffer, &m_vertexStride, &m_vertexOffset);
    }

    // �ε��� ����
    if (m_indexBuffer)
    {
        dc->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);
    }

    dc->IASetPrimitiveTopology(m_primitiveType);
}

// -----------------------------
// �ٿ�� ���� ����
// -----------------------------
void BaseModel::BuildBoundingVolumesFromVertices(const vector<XMFLOAT3>& positions)
{
    if (positions.empty())
        return;

    // 1) ���� min/max
    XMFLOAT3 minV(FLT_MAX, FLT_MAX, FLT_MAX);
    XMFLOAT3 maxV(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    for (const auto& p : positions)
    {
        minV.x = min(minV.x, p.x);
        minV.y = min(minV.y, p.y);
        minV.z = min(minV.z, p.z);

        maxV.x = max(maxV.x, p.x);
        maxV.y = max(maxV.y, p.y);
        maxV.z = max(maxV.z, p.z);
    }

    // 2) ���� AABB 8�� �ڳ�
    m_boundingBoxVertsLocal.clear();
    m_boundingBoxVertsLocal.reserve(8);

    m_boundingBoxVertsLocal.push_back(XMFLOAT3(minV.x, minV.y, minV.z));
    m_boundingBoxVertsLocal.push_back(XMFLOAT3(maxV.x, minV.y, minV.z));
    m_boundingBoxVertsLocal.push_back(XMFLOAT3(minV.x, maxV.y, minV.z));
    m_boundingBoxVertsLocal.push_back(XMFLOAT3(maxV.x, maxV.y, minV.z));
    m_boundingBoxVertsLocal.push_back(XMFLOAT3(minV.x, minV.y, maxV.z));
    m_boundingBoxVertsLocal.push_back(XMFLOAT3(maxV.x, minV.y, maxV.z));
    m_boundingBoxVertsLocal.push_back(XMFLOAT3(minV.x, maxV.y, maxV.z));
    m_boundingBoxVertsLocal.push_back(XMFLOAT3(maxV.x, maxV.y, maxV.z));

    // 3) ���� ���Ǿ�
    XMFLOAT3 center(
        (minV.x + maxV.x) * 0.5f,
        (minV.y + maxV.y) * 0.5f,
        (minV.z + maxV.z) * 0.5f);

    m_sphere.centerOffset = center;

    XMVECTOR c = XMLoadFloat3(&center);
    float maxRadiusSq = 0.0f;
    for (const auto& p : positions)
    {
        XMVECTOR v = XMLoadFloat3(&p);
        float dSq = XMVectorGetX(XMVector3LengthSq(v - c));
        maxRadiusSq = max(maxRadiusSq, dSq);
    }
    m_sphere.radius = std::sqrt(maxRadiusSq);

    // 4) ���� ���� ��� �������� ���� AABB ���
    UpdateAabb();
}

void BaseModel::UpdateAabb()
{
    if (m_boundingBoxVertsLocal.empty())
        return;

    Collision::CalculateAABB(m_boundingBoxVertsLocal, _tmModel, m_aabbWorld);
}

// -----------------------------
// CreateModel: Vertex & indices �� GPU ����
// -----------------------------
bool BaseModel::CreateModel(ID3D11Device* device,
    Vertex& vertices,
    vector<unsigned long>& indices,
    D3D11_PRIMITIVE_TOPOLOGY primitiveType)
{
    if (!device) return false;
    if (vertices.position.empty()) return false;
    if (indices.empty())          return false;

    m_primitiveType = primitiveType;

    const size_t vCount = vertices.position.size();

    // bone/weight/uv ���� ����
    if (vertices.uv.size() < vCount) vertices.uv.resize(vCount, XMFLOAT2(0, 0));
    if (vertices.boneidx.size() < vCount) vertices.boneidx.resize(vCount, XMUINT4(0, 0, 0, 0));
    if (vertices.weight.size() < vCount) vertices.weight.resize(vCount, XMFLOAT4(0, 0, 0, 0));

    // 1) SkinnedVertex �迭 �����
    std::vector<SkinnedVertex> vtx(vCount);
    for (size_t i = 0; i < vCount; ++i)
    {
        vtx[i].position = vertices.position[i];
        vtx[i].tex = vertices.uv[i];
        vtx[i].boneIdx = vertices.boneidx[i];
        vtx[i].weight = vertices.weight[i];
    }

    // 2) Vertex Buffer ����
    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.Usage = D3D11_USAGE_DEFAULT;
    vbDesc.ByteWidth = static_cast<UINT>(sizeof(SkinnedVertex) * vCount);
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbDesc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA vbData = {};
    vbData.pSysMem = vtx.data();

    ID3D11Buffer* vb = nullptr;
    HRESULT hr = device->CreateBuffer(&vbDesc, &vbData, &vb);
    if (FAILED(hr))
        return false;

    // 3) Index Buffer ����
    D3D11_BUFFER_DESC ibDesc = {};
    ibDesc.Usage = D3D11_USAGE_DEFAULT;
    ibDesc.ByteWidth = static_cast<UINT>(sizeof(unsigned long) * indices.size());
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibDesc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA ibData = {};
    ibData.pSysMem = indices.data();

    ID3D11Buffer* ib = nullptr;
    hr = device->CreateBuffer(&ibDesc, &ibData, &ib);
    if (FAILED(hr))
    {
        vb->Release();
        return false;
    }

    // 4) ���� ���� ���� �� ��ü
    if (m_vertexBuffer) m_vertexBuffer->Release();
    if (m_indexBuffer)  m_indexBuffer->Release();

    m_vertexBuffer = vb;
    m_indexBuffer = ib;

    m_vertexStride = sizeof(SkinnedVertex);
    m_vertexOffset = 0;
    m_indexCount = static_cast<UINT>(indices.size());

    // 5) �浹 ���� ����
    BuildBoundingVolumesFromVertices(vertices.position);

    return true;
}
