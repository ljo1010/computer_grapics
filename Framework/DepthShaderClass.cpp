////////////////////////////////////////////////////////////////////////////////
// Filename: DepthShaderClass.cpp
////////////////////////////////////////////////////////////////////////////////
#include "DepthShaderClass.h"

DepthShaderClass::DepthShaderClass()
    : m_vertexShader(nullptr),
      m_pixelShader(nullptr),
      m_layout(nullptr),
      m_matrixBuffer(nullptr)
{
}

DepthShaderClass::~DepthShaderClass()
{
    Shutdown();
}

bool DepthShaderClass::Initialize(ID3D11Device* device, HWND hwnd)
{
    return InitializeShader(device, hwnd, L"./data/depth.hlsl", L"./data/depth.hlsl");
}

void DepthShaderClass::Shutdown()
{
    ShutdownShader();
}

bool DepthShaderClass::Render(ID3D11DeviceContext* deviceContext, int indexCount,
                              XMMATRIX worldMatrix, XMMATRIX lightViewMatrix, XMMATRIX lightProjectionMatrix)
{
    if (!SetShaderParameters(deviceContext, worldMatrix, lightViewMatrix, lightProjectionMatrix))
    {
        return false;
    }

    RenderShader(deviceContext, indexCount);
    return true;
}

bool DepthShaderClass::InitializeShader(ID3D11Device* device, HWND hwnd, const WCHAR* vsFilename, const WCHAR* psFilename)
{
    ID3D10Blob* errorMessage = nullptr;
    ID3D10Blob* vertexShaderBuffer = nullptr;
    ID3D10Blob* pixelShaderBuffer = nullptr;

    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(DEBUG) || defined(_DEBUG)
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    // 1. 버텍스 셰이더 컴파일
    HRESULT result = D3DCompileFromFile(vsFilename, NULL, NULL, "VS_Depth", "vs_5_0", flags, 0,
                                        &vertexShaderBuffer, &errorMessage);
    if (FAILED(result))
    {
        if (errorMessage)
        {
            OutputDebugStringA((char*)errorMessage->GetBufferPointer());
            MessageBoxA(hwnd, (char*)errorMessage->GetBufferPointer(), "Depth VS Error", MB_OK);
            errorMessage->Release();
        }
        else
        {
            MessageBox(hwnd, vsFilename, L"Missing Depth Vertex Shader File", MB_OK);
        }
        return false;
    }

    // 2. 픽셀 셰이더 컴파일
    result = D3DCompileFromFile(psFilename, NULL, NULL, "PS_Depth", "ps_5_0", flags, 0,
                                &pixelShaderBuffer, &errorMessage);
    if (FAILED(result))
    {
        if (errorMessage)
        {
            OutputDebugStringA((char*)errorMessage->GetBufferPointer());
            MessageBoxA(hwnd, (char*)errorMessage->GetBufferPointer(), "Depth PS Error", MB_OK);
            errorMessage->Release();
        }
        else
        {
            MessageBox(hwnd, psFilename, L"Missing Depth Pixel Shader File", MB_OK);
        }
        return false;
    }

    // 3. 셰이더 객체 생성
    result = device->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, &m_vertexShader);
    if (FAILED(result)) return false;

    result = device->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &m_pixelShader);
    if (FAILED(result)) return false;

    // 4. 입력 레이아웃 생성 (POSITION)
    D3D11_INPUT_ELEMENT_DESC polygonLayout[1];
    polygonLayout[0].SemanticName = "POSITION";
    polygonLayout[0].SemanticIndex = 0;
    polygonLayout[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    polygonLayout[0].InputSlot = 0;
    polygonLayout[0].AlignedByteOffset = 0;
    polygonLayout[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    polygonLayout[0].InstanceDataStepRate = 0;

    UINT numElements = sizeof(polygonLayout) / sizeof(polygonLayout[0]);
    result = device->CreateInputLayout(polygonLayout, numElements, vertexShaderBuffer->GetBufferPointer(),
                                       vertexShaderBuffer->GetBufferSize(), &m_layout);
    if (FAILED(result)) return false;

    vertexShaderBuffer->Release();
    pixelShaderBuffer->Release();

    // 5. 상수 버퍼 생성 (MatrixBuffer: World, LightView, LightProj)
    D3D11_BUFFER_DESC matrixBufferDesc{};
    matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    matrixBufferDesc.ByteWidth = sizeof(MatrixBufferType);
    matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    matrixBufferDesc.MiscFlags = 0;
    matrixBufferDesc.StructureByteStride = 0;

    result = device->CreateBuffer(&matrixBufferDesc, NULL, &m_matrixBuffer);
    if (FAILED(result)) return false;

    return true;
}

void DepthShaderClass::ShutdownShader()
{
    if (m_matrixBuffer) { m_matrixBuffer->Release(); m_matrixBuffer = nullptr; }
    if (m_layout) { m_layout->Release(); m_layout = nullptr; }
    if (m_pixelShader) { m_pixelShader->Release(); m_pixelShader = nullptr; }
    if (m_vertexShader) { m_vertexShader->Release(); m_vertexShader = nullptr; }
}

bool DepthShaderClass::SetShaderParameters(ID3D11DeviceContext* deviceContext,
                                          XMMATRIX worldMatrix, XMMATRIX lightViewMatrix, XMMATRIX lightProjectionMatrix)
{
    // 전치 행렬 변환 (HLSL column-major)
    XMMATRIX worldCopy = XMMatrixTranspose(worldMatrix);
    XMMATRIX lightViewCopy = XMMatrixTranspose(lightViewMatrix);
    XMMATRIX lightProjCopy = XMMatrixTranspose(lightProjectionMatrix);

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT result = deviceContext->Map(m_matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (FAILED(result)) return false;

    MatrixBufferType* dataPtr = (MatrixBufferType*)mappedResource.pData;
    dataPtr->world = worldCopy;
    dataPtr->lightView = lightViewCopy;
    dataPtr->lightProjection = lightProjCopy;

    deviceContext->Unmap(m_matrixBuffer, 0);

    deviceContext->VSSetConstantBuffers(0, 1, &m_matrixBuffer);

    return true;
}

void DepthShaderClass::RenderShader(ID3D11DeviceContext* deviceContext, int indexCount)
{
    deviceContext->IASetInputLayout(m_layout);
    deviceContext->VSSetShader(m_vertexShader, NULL, 0);
    deviceContext->PSSetShader(m_pixelShader, NULL, 0);
    deviceContext->DrawIndexed(indexCount, 0, 0);
}
