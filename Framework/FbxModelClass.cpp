// Filename: FbxModelClass.cpp
////////////////////////////////////////////////////////////////////////////////
#include "FbxModelClass.h"

#include <vector>
#include <string>
#include <windows.h>

using namespace std;
using namespace DirectX;

static std::string WStringToAnsi(const std::wstring& ws)
{
    if (ws.empty()) return std::string();
    int size = WideCharToMultiByte(CP_ACP, 0, ws.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string s(size - 1, 0);
    WideCharToMultiByte(CP_ACP, 0, ws.c_str(), -1, &s[0], size, nullptr, nullptr);
    return s;
}

//-----------------------------------------------------------------------------
// 생성자 / 소멸자
//-----------------------------------------------------------------------------
FbxModelClass::FbxModelClass()
{
    m_vertexBuffer = nullptr;
    m_indexBuffer = nullptr;
    m_vertexCount = 0;
    m_indexCount = 0;

    m_instanceBuffer = nullptr;   // 인스턴스 버퍼
    m_instanceCount = 0;         // 인스턴스 개수

    m_Texture = nullptr;
}

FbxModelClass::~FbxModelClass()
{
    Shutdown();
}

//-----------------------------------------------------------------------------
// Initialize : FBX + 텍스처 로드
//-----------------------------------------------------------------------------
bool FbxModelClass::Initialize(ID3D11Device* device,
    const wchar_t* fbxFilename,
    const wchar_t* textureFilename)
{
    // 1) 경로 변환
    std::wstring ws(fbxFilename);
    std::string  path = WStringToAnsi(ws);

    // 2) Assimp로 FBX 읽기
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        path,
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_ConvertToLeftHanded
    );

    if (!scene || !scene->HasMeshes())
        return false;

    // 첫 번째 메시 사용
    const aiMesh* mesh = scene->mMeshes[0];

    // 3) 정점/인덱스 버퍼 생성
    if (!InitializeBuffers(device, mesh))
        return false;

    // 4) 텍스처 로드
    if (!LoadTexture(device, textureFilename))
        return false;

    return true;
}

//-----------------------------------------------------------------------------
// Shutdown
//-----------------------------------------------------------------------------
void FbxModelClass::Shutdown()
{
    ReleaseTexture();
    ShutdownBuffers();
}

//-----------------------------------------------------------------------------
// 정점/인덱스 버퍼 생성
//-----------------------------------------------------------------------------
bool FbxModelClass::InitializeBuffers(ID3D11Device* device, const aiMesh* mesh)
{
    m_vertexCount = static_cast<int>(mesh->mNumVertices);
    m_indexCount = static_cast<int>(mesh->mNumFaces * 3);

    vector<VertexType>    vertices(m_vertexCount);
    vector<unsigned long> indices(m_indexCount);

    // ----- 정점 채우기 -----
    for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
    {
        // 위치
        vertices[i].position = XMFLOAT3(
            mesh->mVertices[i].x,
            mesh->mVertices[i].y,
            mesh->mVertices[i].z);

        // 법선
        if (mesh->HasNormals())
        {
            vertices[i].normal = XMFLOAT3(
                mesh->mNormals[i].x,
                mesh->mNormals[i].y,
                mesh->mNormals[i].z);
        }
        else
        {
            vertices[i].normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
        }

        // 텍스처 좌표 (채널 0 사용)
        if (mesh->HasTextureCoords(0))
        {
            vertices[i].tex = XMFLOAT2(
                mesh->mTextureCoords[0][i].x,
                mesh->mTextureCoords[0][i].y);
        }
        else
        {
            vertices[i].tex = XMFLOAT2(0.0f, 0.0f);
        }
    }

    // ----- 인덱스 채우기 -----
    int index = 0;
    for (unsigned int f = 0; f < mesh->mNumFaces; ++f)
    {
        const aiFace& face = mesh->mFaces[f];
        // Triangulate 되었으므로 3개 인덱스만 사용
        if (face.mNumIndices == 3)
        {
            indices[index++] = face.mIndices[0];
            indices[index++] = face.mIndices[1];
            indices[index++] = face.mIndices[2];
        }
    }

    // ----- D3D 버퍼 생성 -----
    HRESULT result;

    // Vertex buffer
    D3D11_BUFFER_DESC        vertexBufferDesc;
    D3D11_SUBRESOURCE_DATA   vertexData;

    ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));
    vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexBufferDesc.ByteWidth = sizeof(VertexType) * m_vertexCount;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.CPUAccessFlags = 0;
    vertexBufferDesc.MiscFlags = 0;
    vertexBufferDesc.StructureByteStride = 0;

    vertexData.pSysMem = vertices.data();
    vertexData.SysMemPitch = 0;
    vertexData.SysMemSlicePitch = 0;

    result = device->CreateBuffer(&vertexBufferDesc, &vertexData, &m_vertexBuffer);
    if (FAILED(result))
        return false;

    // Index buffer
    D3D11_BUFFER_DESC        indexBufferDesc;
    D3D11_SUBRESOURCE_DATA   indexData;

    ZeroMemory(&indexBufferDesc, sizeof(indexBufferDesc));
    indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    indexBufferDesc.ByteWidth = sizeof(unsigned long) * m_indexCount;
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexBufferDesc.CPUAccessFlags = 0;
    indexBufferDesc.MiscFlags = 0;
    indexBufferDesc.StructureByteStride = 0;

    indexData.pSysMem = indices.data();
    indexData.SysMemPitch = 0;
    indexData.SysMemSlicePitch = 0;

    result = device->CreateBuffer(&indexBufferDesc, &indexData, &m_indexBuffer);
    if (FAILED(result))
        return false;

    return true;
}

//-----------------------------------------------------------------------------
// 인스턴스 버퍼 생성
//-----------------------------------------------------------------------------
bool FbxModelClass::InitializeInstanceBuffer(
    ID3D11Device* device,
    const std::vector<XMFLOAT3>& instancePositions)
{
    if (m_instanceBuffer)
    {
        m_instanceBuffer->Release();
        m_instanceBuffer = nullptr;
    }

    m_instanceCount = static_cast<int>(instancePositions.size());
    if (m_instanceCount <= 0)
        return true;

    std::vector<InstanceType> instances(m_instanceCount);

    for (int i = 0; i < m_instanceCount; ++i)
    {
        instances[i].instancePos = instancePositions[i];
    }

    D3D11_BUFFER_DESC desc = {};
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.ByteWidth = sizeof(InstanceType) * m_instanceCount;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA data = {};
    data.pSysMem = instances.data();
    data.SysMemPitch = 0;
    data.SysMemSlicePitch = 0;

    HRESULT hr = device->CreateBuffer(&desc, &data, &m_instanceBuffer);
    if (FAILED(hr))
    {
        m_instanceBuffer = nullptr;
        m_instanceCount = 0;
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
// 버퍼 해제
//-----------------------------------------------------------------------------
void FbxModelClass::ShutdownBuffers()
{
    if (m_instanceBuffer)
    {
        m_instanceBuffer->Release();
        m_instanceBuffer = nullptr;
    }
    m_instanceCount = 0;

    if (m_indexBuffer)
    {
        m_indexBuffer->Release();
        m_indexBuffer = nullptr;
    }

    if (m_vertexBuffer)
    {
        m_vertexBuffer->Release();
        m_vertexBuffer = nullptr;
    }

    m_vertexCount = 0;
    m_indexCount = 0;
}

//-----------------------------------------------------------------------------
// 일반 렌더 (논인스턴싱)
//-----------------------------------------------------------------------------
void FbxModelClass::Render(ID3D11DeviceContext* deviceContext)
{
    RenderBuffers(deviceContext);
}

void FbxModelClass::RenderBuffers(ID3D11DeviceContext* deviceContext)
{
    unsigned int stride = sizeof(VertexType);
    unsigned int offset = 0;

    deviceContext->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
    deviceContext->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

//-----------------------------------------------------------------------------
// 인스턴스 렌더용
//-----------------------------------------------------------------------------
void FbxModelClass::RenderInstanced(ID3D11DeviceContext* deviceContext)
{
    if (!m_vertexBuffer || !m_indexBuffer || !m_instanceBuffer || m_instanceCount <= 0)
        return;

    unsigned int strides[2];
    unsigned int offsets[2];
    ID3D11Buffer* buffers[2];

    strides[0] = sizeof(VertexType);
    strides[1] = sizeof(InstanceType);

    offsets[0] = 0;
    offsets[1] = 0;

    buffers[0] = m_vertexBuffer;
    buffers[1] = m_instanceBuffer;

    deviceContext->IASetVertexBuffers(0, 2, buffers, strides, offsets);
    deviceContext->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

//-----------------------------------------------------------------------------
// 텍스처 로드 / 해제
//-----------------------------------------------------------------------------
bool FbxModelClass::LoadTexture(ID3D11Device* device, const wchar_t* filename)
{
    m_Texture = new TextureClass;
    if (!m_Texture)
        return false;

    if (!m_Texture->Initialize(device, filename))
        return false;

    return true;
}

void FbxModelClass::ReleaseTexture()
{
    if (m_Texture)
    {
        m_Texture->Shutdown();
        delete m_Texture;
        m_Texture = nullptr;
    }
}