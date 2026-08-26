////////////////////////////////////////////////////////////////////////////////
// Filename: modelclass.cpp
////////////////////////////////////////////////////////////////////////////////
#include "modelclass.h"
#include <vector>    // �߰�

ModelClass::ModelClass()
{
    // BaseModel �ʿ��� ���� ����� ������, 0���� �� �� �� �ʱ�ȭ�ϴ� �� ���� ����
    m_vertexBuffer = 0;
    m_indexBuffer = 0;
    m_Texture = 0;
    m_model = 0;
    m_instanceBuffer = 0;
    m_textureCount = 0;
    m_normalCount = 0;
    m_faceCount = 0;
}

ModelClass::ModelClass(const ModelClass& other)
{
}

ModelClass::~ModelClass()
{
}

bool ModelClass::Initialize(ID3D11Device* device, const WCHAR* modelFilename, const WCHAR* textureFilename)
{
    bool result;

    // Load in the model data.
    result = LoadModel(modelFilename);
    if (!result)
    {
        OutputDebugString(L"[ModelClass] LoadModel failed.\n");
        return false;
    }

    // Initialize the vertex and index buffers.
    result = InitializeBuffers(device);
    if (!result)
    {
        return false;
    }

    // Load the texture for this model.
    result = LoadTexture(device, textureFilename);
    if (!result)
    {
        return false;
    }

    return true;
}

void ModelClass::Shutdown()
{
    // Release the model texture.
    ReleaseTexture();

    // Shutdown the vertex and index buffers.
    ShutdownBuffers();

    // Release the model data.
    ReleaseModel();

    return;
}

void ModelClass::Render(ID3D11DeviceContext* deviceContext)
{
    // Put the vertex and index buffers on the graphics pipeline to prepare them for drawing.
    RenderBuffers(deviceContext);

    return;
}

int ModelClass::GetIndexCount()
{
    return m_indexCount;
}

ID3D11ShaderResourceView* ModelClass::GetTexture()
{
    return m_Texture->GetTexture();
}

bool ModelClass::InitializeBuffers(ID3D11Device* device)
{
    VertexType* vertices;
    unsigned long* indices;
    D3D11_BUFFER_DESC vertexBufferDesc, indexBufferDesc;
    D3D11_SUBRESOURCE_DATA vertexData, indexData;
    HRESULT result;
    int i;

    // Create the vertex array.
    vertices = new VertexType[m_vertexCount];
    if (!vertices)
    {
        return false;
    }

    // Create the index array.
    indices = new unsigned long[m_indexCount];
    if (!indices)
    {
        return false;
    }

    // Load the vertex array and index array with data.
    for (i = 0; i < m_vertexCount; i++)
    {
        vertices[i].position = XMFLOAT3(m_model[i].x, m_model[i].y, m_model[i].z);
        vertices[i].texture = XMFLOAT2(m_model[i].tu, m_model[i].tv);
        vertices[i].normal = XMFLOAT3(m_model[i].nx, m_model[i].ny, m_model[i].nz);

        indices[i] = i;
    }

    // --------------------------------------------------------------------
    // [�߰�] �浹�� Bounding Volume ���� (BaseModel�� �Լ� ���)
    // --------------------------------------------------------------------
    {
        std::vector<XMFLOAT3> positions;
        positions.reserve(m_vertexCount);
        for (int k = 0; k < m_vertexCount; ++k)
        {
            positions.push_back(vertices[k].position);
        }
        // BaseModel �� ���: m_boundingBoxVertsLocal, m_aabbWorld, m_sphere ����
        BuildBoundingVolumesFromVertices(positions);
    }
    // --------------------------------------------------------------------

    // Set up the description of the static vertex buffer.
    vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexBufferDesc.ByteWidth = sizeof(VertexType) * m_vertexCount;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.CPUAccessFlags = 0;
    vertexBufferDesc.MiscFlags = 0;
    vertexBufferDesc.StructureByteStride = 0;

    // Give the subresource structure a pointer to the vertex data.
    vertexData.pSysMem = vertices;
    vertexData.SysMemPitch = 0;
    vertexData.SysMemSlicePitch = 0;

    // Now create the vertex buffer.
    result = device->CreateBuffer(&vertexBufferDesc, &vertexData, &m_vertexBuffer);
    if (FAILED(result)) {
        wchar_t msg[128];
        swprintf_s(msg, L"[ModelClass] CreateBuffer failed (vertex) HRESULT=0x%08X\n", result);
        OutputDebugString(msg);
        return false;
    }

    // Set up the description of the static index buffer.
    indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    indexBufferDesc.ByteWidth = sizeof(unsigned long) * m_indexCount;
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexBufferDesc.CPUAccessFlags = 0;
    indexBufferDesc.MiscFlags = 0;
    indexBufferDesc.StructureByteStride = 0;

    // Give the subresource structure a pointer to the index data.
    indexData.pSysMem = indices;
    indexData.SysMemPitch = 0;
    indexData.SysMemSlicePitch = 0;

    // Create the index buffer.
    result = device->CreateBuffer(&indexBufferDesc, &indexData, &m_indexBuffer);
    if (FAILED(result))
    {
        return false;
    }

    // Release the arrays now that the vertex and index buffers have been created and loaded.
    delete[] vertices;
    vertices = 0;

    delete[] indices;
    indices = 0;

    return true;
}

void ModelClass::ShutdownBuffers()
{
    // ���⼭ VB/IB�� ���� Release�ϴ� ������ �״�� �ֵ� �ǰ�,
    // BaseModel::~BaseModel()���� Release()�� �� �� �� ȣ���ص�
    // �����Ͱ� 0���� ���õǾ� �־ �ߺ� ������ �Ͼ�� �ʴ´�.

    // Release the index buffer.
    if (m_indexBuffer)
    {
        m_indexBuffer->Release();
        m_indexBuffer = 0;
    }

    // Release the vertex buffer.
    if (m_vertexBuffer)
    {
        m_vertexBuffer->Release();
        m_vertexBuffer = 0;
    }

    return;
}

void ModelClass::RenderBuffers(ID3D11DeviceContext* deviceContext)
{
    unsigned int stride;
    unsigned int offset;

    // Set vertex buffer stride and offset.
    stride = sizeof(VertexType);
    offset = 0;

    // Set the vertex buffer to active in the input assembler so it can be rendered.
    deviceContext->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);

    // Set the index buffer to active in the input assembler so it can be rendered.
    deviceContext->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);

    // Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    return;
}

bool ModelClass::LoadTexture(ID3D11Device* device, const WCHAR* filename)
{
    bool result;

    // Create the texture object.
    m_Texture = new TextureClass;
    if (!m_Texture)
    {
        return false;
    }

    // Initialize the texture object.
    result = m_Texture->Initialize(device, filename);
    if (!result)
    {
        return false;
    }

    return true;
}

void ModelClass::ReleaseTexture()
{
    // Release the texture object.
    if (m_Texture)
    {
        m_Texture->Shutdown();
        delete m_Texture;
        m_Texture = 0;
    }

    return;
}

bool ModelClass::LoadModel(const WCHAR* filename)
{
    OutputDebugString(L"[ModelClass] LoadModel start.\n");
    bool ok = ReadFileCounts(filename);
    if (!ok) {
        OutputDebugString(L"[ModelClass] ReadFileCounts failed (file not found?)\n");
    }
    return ok;
}

// �ν��Ͻ� ���� �ʱ�ȭ
bool ModelClass::InitializeInstanceBuffer(ID3D11Device* device, int count)
{
    m_instanceCount = count;
    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.ByteWidth = sizeof(DirectX::XMMATRIX) * count;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    return SUCCEEDED(device->CreateBuffer(&bd, nullptr, &m_instanceBuffer));
}

// �� ������ world �迭�� ���� ����
bool ModelClass::UpdateInstanceBuffer(ID3D11DeviceContext* ctx, const std::vector<DirectX::XMMATRIX>& worlds)
{
    if ((int)worlds.size() != m_instanceCount) return false;
    D3D11_MAPPED_SUBRESOURCE mr;
    if (FAILED(ctx->Map(m_instanceBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mr))) return false;
    memcpy(mr.pData, worlds.data(), sizeof(DirectX::XMMATRIX) * m_instanceCount);
    ctx->Unmap(m_instanceBuffer, 0);
    return true;
}

// �ν��Ͻ� ��ο�
bool ModelClass::RenderInstanced(
    ID3D11DeviceContext* ctx, int idxCount,
    const std::vector<DirectX::XMMATRIX>& worlds,
    ID3D11ShaderResourceView* tex,
    ID3D11Buffer* matrixCB,
    const DirectX::XMMATRIX& view,
    const DirectX::XMMATRIX& proj)
{
    UpdateInstanceBuffer(ctx, worlds);
    // VAO: slot0=vertex, slot1=instance
    UINT strides[2] = { sizeof(VertexType), sizeof(DirectX::XMMATRIX) };
    UINT offsets[2] = { 0, 0 };
    ID3D11Buffer* bufs[2] = { m_vertexBuffer, m_instanceBuffer };
    ctx->IASetVertexBuffers(0, 2, bufs, strides, offsets);
    ctx->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // �������(view/proj) ������Ʈ
    struct CB { DirectX::XMMATRIX view, proj; };
    CB cb;
    cb.view = DirectX::XMMatrixTranspose(view);
    cb.proj = DirectX::XMMatrixTranspose(proj);
    D3D11_MAPPED_SUBRESOURCE mr;
    ctx->Map(matrixCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mr);
    memcpy(mr.pData, &cb, sizeof(cb));
    ctx->Unmap(matrixCB, 0);
    ctx->VSSetConstantBuffers(0, 1, &matrixCB);

    ctx->PSSetShaderResources(0, 1, &tex);
    ctx->DrawIndexedInstanced(idxCount, m_instanceCount, 0, 0, 0);
    return true;
}

void ModelClass::ReleaseModel()
{
    if (m_model)
    {
        delete[] m_model;
        m_model = 0;
    }

    return;
}

bool ModelClass::ReadFileCounts(const WCHAR* filename)
{
    ifstream fin;
    char input;
    // Initialize the counts.
    int vertexCount = 0;
    int textureCount = 0;
    int normalCount = 0;
    int faceCount = 0;

    // Open the file.
    fin.open(filename);
    if (fin.fail() == true)
    {
        return false;
    }

    // Read from the file and continue to read until the end of the file is reached.
    fin.get(input);
    while (!fin.eof())
    {
        if (input == 'v')
        {
            fin.get(input);
            if (input == ' ') { vertexCount++; }
            if (input == 't') { textureCount++; }
            if (input == 'n') { normalCount++; }
        }

        if (input == 'f')
        {
            fin.get(input);
            if (input == ' ') { faceCount++; }
        }

        while (input != '\n')
        {
            fin.get(input);
            if (fin.eof())
                break;
        }

        fin.get(input);
    }
    fin.close();

    LoadDataStructures(filename, vertexCount, textureCount, normalCount, faceCount);

    return true;
}

bool ModelClass::LoadDataStructures(const WCHAR* filename, int vertexCount, int textureCount, int normalCount, int faceCount)
{
    XMFLOAT3* vertices, * texcoords, * normals;
    FaceType* faces;
    ifstream fin;
    int vertexIndex, texcoordIndex, normalIndex, faceIndex, vIndex, tIndex, nIndex;
    char input, input2;

    // Initialize the four data structures.
    vertices = new XMFLOAT3[vertexCount];
    if (!vertices)
    {
        return false;
    }

    texcoords = new XMFLOAT3[textureCount];
    if (!texcoords)
    {
        return false;
    }

    normals = new XMFLOAT3[normalCount];
    if (!normals)
    {
        return false;
    }

    faces = new FaceType[faceCount];
    if (!faces)
    {
        return false;
    }

    vertexIndex = 0;
    texcoordIndex = 0;
    normalIndex = 0;
    faceIndex = 0;

    fin.open(filename);
    if (fin.fail() == true)
    {
        return false;
    }

    fin.get(input);
    while (!fin.eof())
    {
        if (input == 'v')
        {
            fin.get(input);

            if (input == ' ')
            {
                fin >> vertices[vertexIndex].x >> vertices[vertexIndex].y >>
                    vertices[vertexIndex].z;

                vertices[vertexIndex].z = vertices[vertexIndex].z * -1.0f;
                vertexIndex++;
            }

            if (input == 't')
            {
                fin >> texcoords[texcoordIndex].x >> texcoords[texcoordIndex].y;
                texcoords[texcoordIndex].y = 1.0f - texcoords[texcoordIndex].y;
                texcoordIndex++;
            }

            if (input == 'n')
            {
                fin >> normals[normalIndex].x >> normals[normalIndex].y >>
                    normals[normalIndex].z;

                normals[normalIndex].z = normals[normalIndex].z * -1.0f;
                normalIndex++;
            }
        }

        if (input == 'f')
        {
            fin.get(input);
            if (input == ' ')
            {
                fin >> faces[faceIndex].vIndex3 >> input2 >> faces[faceIndex].tIndex3 >>
                    input2 >> faces[faceIndex].nIndex3 >> faces[faceIndex].vIndex2 >> input2 >>
                    faces[faceIndex].tIndex2 >> input2 >> faces[faceIndex].nIndex2 >>
                    faces[faceIndex].vIndex1 >> input2 >> faces[faceIndex].tIndex1 >> input2 >>
                    faces[faceIndex].nIndex1;
                faceIndex++;
            }
        }

        while (input != '\n')
        {
            fin.get(input);
            if (fin.eof())
                break;
        }

        fin.get(input);
    }

    m_vertexCount = faceCount * 3;
    m_indexCount = m_vertexCount;

    m_model = new ModelType[m_vertexCount];
    if (!m_model)
    {
        return false;
    }

    for (int i = 0; i < faceIndex; i++)
    {
        vIndex = faces[i].vIndex1 - 1;
        tIndex = faces[i].tIndex1 - 1;
        nIndex = faces[i].nIndex1 - 1;

        m_model[i * 3].x = vertices[vIndex].x;
        m_model[i * 3].y = vertices[vIndex].y;
        m_model[i * 3].z = vertices[vIndex].z;
        m_model[i * 3].tu = texcoords[tIndex].x;
        m_model[i * 3].tv = texcoords[tIndex].y;
        m_model[i * 3].nx = normals[nIndex].x;
        m_model[i * 3].ny = normals[nIndex].y;
        m_model[i * 3].nz = normals[nIndex].z;

        vIndex = faces[i].vIndex2 - 1;
        tIndex = faces[i].tIndex2 - 1;
        nIndex = faces[i].nIndex2 - 1;

        m_model[i * 3 + 1].x = vertices[vIndex].x;
        m_model[i * 3 + 1].y = vertices[vIndex].y;
        m_model[i * 3 + 1].z = vertices[vIndex].z;
        m_model[i * 3 + 1].tu = texcoords[tIndex].x;
        m_model[i * 3 + 1].tv = texcoords[tIndex].y;
        m_model[i * 3 + 1].nx = normals[nIndex].x;
        m_model[i * 3 + 1].ny = normals[nIndex].y;
        m_model[i * 3 + 1].nz = normals[nIndex].z;

        vIndex = faces[i].vIndex3 - 1;
        tIndex = faces[i].tIndex3 - 1;
        nIndex = faces[i].nIndex3 - 1;

        m_model[i * 3 + 2].x = vertices[vIndex].x;
        m_model[i * 3 + 2].y = vertices[vIndex].y;
        m_model[i * 3 + 2].z = vertices[vIndex].z;
        m_model[i * 3 + 2].tu = texcoords[tIndex].x;
        m_model[i * 3 + 2].tv = texcoords[tIndex].y;
        m_model[i * 3 + 2].nx = normals[nIndex].x;
        m_model[i * 3 + 2].ny = normals[nIndex].y;
        m_model[i * 3 + 2].nz = normals[nIndex].z;
    }

    if (vertices) { delete[] vertices; vertices = 0; }
    if (texcoords) { delete[] texcoords; texcoords = 0; }
    if (normals) { delete[] normals; normals = 0; }
    if (faces) { delete[] faces; faces = 0; }

    return true;
}
