#include "WaterClass.h"
#include "DDSTextureLoader.h"

WaterClass::WaterClass()
{
    m_waterShader = std::make_unique<WaterShaderClass>();
}

WaterClass::~WaterClass()
{
    Shutdown();
}

bool WaterClass::Initialize(ID3D11Device* device, HWND hwnd, float width, float height, int gridSegments)
{
    // 1. 그리드 정점 및 인덱스 버퍼 생성
    if (!InitializeBuffers(device, width, height, gridSegments))
    {
        return false;
    }

    // 2. 물결 노멀맵 로드
    HRESULT hr = CreateDDSTextureFromFile(device, L"./data/water_normal.dds", nullptr, &m_normalTexture);
    if (FAILED(hr))
    {
        return false;
    }

    // 3. 물 전용 셰이더 초기화
    if (!m_waterShader || !m_waterShader->Initialize(device, hwnd))
    {
        return false;
    }

    return true;
}

void WaterClass::Shutdown()
{
    if (m_waterShader)
    {
        m_waterShader->Shutdown();
        m_waterShader.reset();
    }

    if (m_normalTexture)
    {
        m_normalTexture->Release();
        m_normalTexture = nullptr;
    }

    ShutdownBuffers();
}

bool WaterClass::InitializeBuffers(ID3D11Device* device, float width, float height, int gridSegments)
{
    int numVerticesAcross = gridSegments + 1;
    m_vertexCount = numVerticesAcross * numVerticesAcross;
    m_indexCount = gridSegments * gridSegments * 6;

    std::vector<VertexType> vertices(m_vertexCount);
    std::vector<unsigned int> indices(m_indexCount);

    float halfW = width * 0.5f;
    float halfH = height * 0.5f;
    float dx = width / (float)gridSegments;
    float dz = height / (float)gridSegments;

    // 정점 배열 채우기
    int vIdx = 0;
    for (int z = 0; z <= gridSegments; ++z)
    {
        float posZ = -halfH + z * dz;
        float texV = (float)z / (float)gridSegments * 4.0f; // 4회 타일링

        for (int x = 0; x <= gridSegments; ++x)
        {
            float posX = -halfW + x * dx;
            float texU = (float)x / (float)gridSegments * 4.0f;

            vertices[vIdx].position = XMFLOAT3(posX, 0.0f, posZ);
            vertices[vIdx].texture = XMFLOAT2(texU, texV);
            vertices[vIdx].normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
            vIdx++;
        }
    }

    // 인덱스 배열 채우기 (시계방향 삼각형)
    int iIdx = 0;
    for (int z = 0; z < gridSegments; ++z)
    {
        for (int x = 0; x < gridSegments; ++x)
        {
            int row1 = z * numVerticesAcross + x;
            int row2 = (z + 1) * numVerticesAcross + x;

            // Triangle 1
            indices[iIdx++] = row1;
            indices[iIdx++] = row2;
            indices[iIdx++] = row1 + 1;

            // Triangle 2
            indices[iIdx++] = row1 + 1;
            indices[iIdx++] = row2;
            indices[iIdx++] = row2 + 1;
        }
    }

    // 버텍스 버퍼 생성
    D3D11_BUFFER_DESC vertexBufferDesc{};
    vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexBufferDesc.ByteWidth = sizeof(VertexType) * m_vertexCount;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vertexData{};
    vertexData.pSysMem = vertices.data();

    HRESULT hr = device->CreateBuffer(&vertexBufferDesc, &vertexData, &m_vertexBuffer);
    if (FAILED(hr)) return false;

    // 인덱스 버퍼 생성
    D3D11_BUFFER_DESC indexBufferDesc{};
    indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    indexBufferDesc.ByteWidth = sizeof(unsigned int) * m_indexCount;
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA indexData{};
    indexData.pSysMem = indices.data();

    hr = device->CreateBuffer(&indexBufferDesc, &indexData, &m_indexBuffer);
    if (FAILED(hr)) return false;

    return true;
}

void WaterClass::ShutdownBuffers()
{
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
}

void WaterClass::Render(ID3D11DeviceContext* deviceContext,
                        const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix,
                        const XMFLOAT3& cameraPos, float gameTime,
                        const XMFLOAT4& diffuseColor, const XMFLOAT3& lightDir,
                        const XMFLOAT4& specularColor, float specularPower,
                        const XMFLOAT4& fogColor, float fogStart, float fogEnd, bool fogEnabled)
{
    if (!m_enabled || !m_waterShader) return;

    unsigned int stride = sizeof(VertexType);
    unsigned int offset = 0;

    deviceContext->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
    deviceContext->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    XMMATRIX worldMatrix = XMMatrixScaling(m_scale, 1.0f, m_scale) *
                           XMMatrixTranslation(m_position.x, m_position.y, m_position.z);

    m_waterShader->Render(
        deviceContext, m_indexCount,
        worldMatrix, viewMatrix, projectionMatrix,
        m_normalTexture,
        cameraPos, gameTime,
        m_deepColor, m_shallowColor,
        m_waveSpeed, m_waveHeight, m_waveFrequency, m_waterAlpha,
        diffuseColor, lightDir,
        specularColor, specularPower,
        fogColor, fogStart, fogEnd, fogEnabled
    );
}