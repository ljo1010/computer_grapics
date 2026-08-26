#include "FbxModelClass.h"

#include <string>
#include <vector>

using namespace DirectX;
using std::vector;

// ������ wstring �� ansi string ��ȯ (FBX ��ο� �ѱ� ���ٰ� ����)
static std::string WStringToAnsi(const std::wstring& ws)
{
    int len = WideCharToMultiByte(CP_ACP, 0, ws.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string s(len, 0);
    WideCharToMultiByte(CP_ACP, 0, ws.c_str(), -1, &s[0], len, nullptr, nullptr);
    if (!s.empty() && s.back() == '\0') s.pop_back();
    return s;
}

//-----------------------------------------------------------------------------
// ������ / �Ҹ���
//-----------------------------------------------------------------------------
FbxModelClass::FbxModelClass()
{
    m_vertexBuffer = nullptr;
    m_indexBuffer = nullptr;
    m_vertexCount = 0;
    m_indexCount = 0;

    m_instanceBuffer = nullptr;   // �ν��Ͻ� ����
    m_instanceCount = 0;         // �ν��Ͻ� ����

    m_Texture = nullptr;
}

FbxModelClass::~FbxModelClass()
{
    Shutdown();
}

//-----------------------------------------------------------------------------
// Initialize : FBX + �ؽ�ó �ε�
//-----------------------------------------------------------------------------
bool FbxModelClass::Initialize(ID3D11Device* device,
    const wchar_t* fbxFilename,
    const wchar_t* textureFilename)
{
    // 1) ��� ��ȯ
    std::wstring ws(fbxFilename);
    std::string  path = WStringToAnsi(ws);

    // 2) Assimp�� FBX �б�
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        path,
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_ConvertToLeftHanded
    );

    if (!scene || !scene->HasMeshes())
        return false;

    // ù ��° �޽� �ϳ��� ���
    const aiMesh* mesh = scene->mMeshes[0];

    // 3) ����/�ε��� ���� ����
    if (!InitializeBuffers(device, mesh))
        return false;

    // 4) �ؽ�ó �ε� (���� ModelClass�� ������ ���)
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
// ����/�ε��� ���� ����
//-----------------------------------------------------------------------------
bool FbxModelClass::InitializeBuffers(ID3D11Device* device, const aiMesh* mesh)
{
    m_vertexCount = static_cast<int>(mesh->mNumVertices);
    m_indexCount = static_cast<int>(mesh->mNumFaces * 3);

    vector<VertexType>    vertices(m_vertexCount);
    vector<unsigned long> indices(m_indexCount);

    // ----- ���� ä��� -----
    for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
    {
        // ��ġ
        vertices[i].position = XMFLOAT3(
            mesh->mVertices[i].x,
            mesh->mVertices[i].y,
            mesh->mVertices[i].z);

        // ���
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

        // �ؽ�ó ��ǥ (ä�� 0 ���)
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

    // ----- �ε��� ä��� -----
    int index = 0;
    for (unsigned int f = 0; f < mesh->mNumFaces; ++f)
    {
        const aiFace& face = mesh->mFaces[f];
        // Triangulate �����Ƿ� 3�� �ε������ ����
        if (face.mNumIndices == 3)
        {
            indices[index++] = face.mIndices[0];
            indices[index++] = face.mIndices[1];
            indices[index++] = face.mIndices[2];
        }
        // 3�� �ƴϸ� ����
    }

    // ----- D3D ���� ���� -----
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
// �ν��Ͻ� ���� ���� (instanceCount ����, ��ġ�� ���ο��� ���Ƿ� ��ġ ����)
//-----------------------------------------------------------------------------
bool FbxModelClass::InitializeInstanceBuffer(
    ID3D11Device* device,
    const std::vector<XMFLOAT3>& instancePositions)
{
    // ���� ���� ������ ����
    if (m_instanceBuffer)
    {
        m_instanceBuffer->Release();
        m_instanceBuffer = nullptr;
    }

    // �ν��Ͻ� ���� ����
    m_instanceCount = static_cast<int>(instancePositions.size());
    if (m_instanceCount <= 0)
        return true;   // �ν��Ͻ��� 0�̸� �׳� ����

    // CPU �� �ӽ� ����
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
// ���� ����
//-----------------------------------------------------------------------------
void FbxModelClass::ShutdownBuffers()
{
    // �ν��Ͻ� ����
    if (m_instanceBuffer)
    {
        m_instanceBuffer->Release();
        m_instanceBuffer = nullptr;
    }
    m_instanceCount = 0;

    // �ε���/���� ����
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
// �Ϲ� ���� (���ν��Ͻ�) - ���� ���� �״��
//-----------------------------------------------------------------------------
void FbxModelClass::Render(ID3D11DeviceContext* deviceContext)
{
    RenderBuffers(deviceContext);
}

void FbxModelClass::RenderBuffers(ID3D11DeviceContext* deviceContext)
{
    unsigned int stride = sizeof(VertexType);
    unsigned int offset = 0;

    // ���� ���� / �ε��� ���� ���ε�
    deviceContext->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
    deviceContext->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);

    // �ﰢ�� ����Ʈ�� �׸���
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // ���⼭�� Draw ȣ�� ���� (�׷��Ƚ�/���̴� �ʿ��� DrawIndexed ȣ���ϴ� ���� ����)
}

//-----------------------------------------------------------------------------
// �ν��Ͻ� ������ : IA�� (���� ���� + �ν��Ͻ� ����) ���ε�
//  - GraphicsClass �Ǵ� ShaderClass �ʿ���
//    DrawIndexedInstanced(m_indexCount, m_instanceCount, 0, 0, 0);
//    �� ȣ���ϴ� ������ ���� ��.
//-----------------------------------------------------------------------------
void FbxModelClass::RenderInstanced(ID3D11DeviceContext* deviceContext)
{
    if (!m_vertexBuffer || !m_indexBuffer || !m_instanceBuffer || m_instanceCount <= 0)
        return;

    unsigned int strides[2];
    unsigned int offsets[2];
    ID3D11Buffer* buffers[2];

    strides[0] = sizeof(VertexType);   // Stream 0: ����
    strides[1] = sizeof(InstanceType); // Stream 1: �ν��Ͻ�

    offsets[0] = 0;
    offsets[1] = 0;

    buffers[0] = m_vertexBuffer;    // slot 0
    buffers[1] = m_instanceBuffer;  // slot 1

    deviceContext->IASetVertexBuffers(0, 2, buffers, strides, offsets);
    deviceContext->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

//-----------------------------------------------------------------------------
// �ؽ�ó �ε� / ����
//-----------------------------------------------------------------------------
bool FbxModelClass::LoadTexture(ID3D11Device* device, const wchar_t* filename)
{
    m_Texture = new TextureClass;
    if (!m_Texture)
        return false;

    bool result = m_Texture->Initialize(device, filename);
    if (!result)
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
