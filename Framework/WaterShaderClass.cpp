#include "WaterShaderClass.h"

WaterShaderClass::WaterShaderClass()
{
}

WaterShaderClass::~WaterShaderClass()
{
    Shutdown();
}

bool WaterShaderClass::Initialize(ID3D11Device* device, HWND hwnd)
{
    return InitializeShader(device, hwnd, L"./data/water.hlsl", L"./data/water.hlsl");
}

void WaterShaderClass::Shutdown()
{
    ShutdownShader();
}

bool WaterShaderClass::InitializeShader(ID3D11Device* device, HWND hwnd, const WCHAR* vsFilename, const WCHAR* psFilename)
{
    HRESULT result;
    ID3D10Blob* errorMessage = nullptr;
    ID3D10Blob* vertexShaderBuffer = nullptr;
    ID3D10Blob* pixelShaderBuffer = nullptr;

    result = D3DCompileFromFile(vsFilename, NULL, NULL, "WaterVertexShader", "vs_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0,
                                &vertexShaderBuffer, &errorMessage);
    if (FAILED(result))
    {
        if (errorMessage) errorMessage->Release();
        return false;
    }

    result = D3DCompileFromFile(psFilename, NULL, NULL, "WaterPixelShader", "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0,
                                &pixelShaderBuffer, &errorMessage);
    if (FAILED(result))
    {
        if (errorMessage) errorMessage->Release();
        if (vertexShaderBuffer) vertexShaderBuffer->Release();
        return false;
    }

    result = device->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, &m_vertexShader);
    if (FAILED(result)) return false;

    result = device->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &m_pixelShader);
    if (FAILED(result)) return false;

    D3D11_INPUT_ELEMENT_DESC polygonLayout[3];
    polygonLayout[0].SemanticName = "POSITION";
    polygonLayout[0].SemanticIndex = 0;
    polygonLayout[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    polygonLayout[0].InputSlot = 0;
    polygonLayout[0].AlignedByteOffset = 0;
    polygonLayout[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    polygonLayout[0].InstanceDataStepRate = 0;

    polygonLayout[1].SemanticName = "TEXCOORD";
    polygonLayout[1].SemanticIndex = 0;
    polygonLayout[1].Format = DXGI_FORMAT_R32G32_FLOAT;
    polygonLayout[1].InputSlot = 0;
    polygonLayout[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
    polygonLayout[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    polygonLayout[1].InstanceDataStepRate = 0;

    polygonLayout[2].SemanticName = "NORMAL";
    polygonLayout[2].SemanticIndex = 0;
    polygonLayout[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    polygonLayout[2].InputSlot = 0;
    polygonLayout[2].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
    polygonLayout[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    polygonLayout[2].InstanceDataStepRate = 0;

    unsigned int numElements = sizeof(polygonLayout) / sizeof(polygonLayout[0]);
    result = device->CreateInputLayout(polygonLayout, numElements, vertexShaderBuffer->GetBufferPointer(),
                                       vertexShaderBuffer->GetBufferSize(), &m_layout);
    if (FAILED(result)) return false;

    vertexShaderBuffer->Release();
    pixelShaderBuffer->Release();

    // 1. Matrix 상수 버퍼
    D3D11_BUFFER_DESC matrixBufferDesc{};
    matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    matrixBufferDesc.ByteWidth = sizeof(MatrixBufferType);
    matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    result = device->CreateBuffer(&matrixBufferDesc, NULL, &m_matrixBuffer);
    if (FAILED(result)) return false;

    // 2. Water 상수 버퍼
    D3D11_BUFFER_DESC waterBufferDesc{};
    waterBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    waterBufferDesc.ByteWidth = sizeof(WaterBufferType);
    waterBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    waterBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    result = device->CreateBuffer(&waterBufferDesc, NULL, &m_waterBuffer);
    if (FAILED(result)) return false;

    // 3. Light 상수 버퍼
    D3D11_BUFFER_DESC lightBufferDesc{};
    lightBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    lightBufferDesc.ByteWidth = sizeof(LightBufferType);
    lightBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    lightBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    result = device->CreateBuffer(&lightBufferDesc, NULL, &m_lightBuffer);
    if (FAILED(result)) return false;

    // 4. Sampler State (타일링 Wrap)
    D3D11_SAMPLER_DESC samplerDesc{};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
    result = device->CreateSamplerState(&samplerDesc, &m_sampleState);
    if (FAILED(result)) return false;

    return true;
}

void WaterShaderClass::ShutdownShader()
{
    if (m_sampleState) { m_sampleState->Release(); m_sampleState = nullptr; }
    if (m_lightBuffer) { m_lightBuffer->Release(); m_lightBuffer = nullptr; }
    if (m_waterBuffer) { m_waterBuffer->Release(); m_waterBuffer = nullptr; }
    if (m_matrixBuffer) { m_matrixBuffer->Release(); m_matrixBuffer = nullptr; }
    if (m_layout) { m_layout->Release(); m_layout = nullptr; }
    if (m_pixelShader) { m_pixelShader->Release(); m_pixelShader = nullptr; }
    if (m_vertexShader) { m_vertexShader->Release(); m_vertexShader = nullptr; }
}

bool WaterShaderClass::Render(ID3D11DeviceContext* deviceContext, int indexCount,
                             const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix,
                             ID3D11ShaderResourceView* normalTexture,
                             const XMFLOAT3& cameraPos, float gameTime,
                             const XMFLOAT4& deepColor, const XMFLOAT4& shallowColor,
                             float waveSpeed, float waveHeight, float waveFrequency, float waterAlpha,
                             const XMFLOAT4& diffuseColor, const XMFLOAT3& lightDir,
                             const XMFLOAT4& specularColor, float specularPower,
                             const XMFLOAT4& fogColor, float fogStart, float fogEnd, bool fogEnabled)
{
    D3D11_MAPPED_SUBRESOURCE mappedResource;

    // 1. Matrix 버퍼 업데이트
    if (SUCCEEDED(deviceContext->Map(m_matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
    {
        MatrixBufferType* dataPtr = (MatrixBufferType*)mappedResource.pData;
        dataPtr->world = XMMatrixTranspose(worldMatrix);
        dataPtr->view = XMMatrixTranspose(viewMatrix);
        dataPtr->projection = XMMatrixTranspose(projectionMatrix);
        deviceContext->Unmap(m_matrixBuffer, 0);
    }

    // 2. Water 버퍼 업데이트
    if (SUCCEEDED(deviceContext->Map(m_waterBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
    {
        WaterBufferType* dataPtr = (WaterBufferType*)mappedResource.pData;
        dataPtr->deepColor = deepColor;
        dataPtr->shallowColor = shallowColor;
        dataPtr->cameraPosition = cameraPos;
        dataPtr->gameTime = gameTime;
        dataPtr->waveSpeed = waveSpeed;
        dataPtr->waveHeight = waveHeight;
        dataPtr->waveFrequency = waveFrequency;
        dataPtr->waterAlpha = waterAlpha;
        deviceContext->Unmap(m_waterBuffer, 0);
    }

    // 3. Light 버퍼 업데이트
    if (SUCCEEDED(deviceContext->Map(m_lightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
    {
        LightBufferType* dataPtr = (LightBufferType*)mappedResource.pData;
        dataPtr->lightDiffuseColor = diffuseColor;
        dataPtr->lightDirection = lightDir;
        dataPtr->specularPower = specularPower;
        dataPtr->specularColor = specularColor;
        dataPtr->fogColor = fogColor;
        dataPtr->fogStart = fogStart;
        dataPtr->fogEnd = fogEnd;
        dataPtr->fogEnabled = fogEnabled ? 1 : 0;
        dataPtr->padding = 0.0f;
        deviceContext->Unmap(m_lightBuffer, 0);
    }

    // 버퍼 바인딩
    deviceContext->VSSetConstantBuffers(0, 1, &m_matrixBuffer);
    deviceContext->VSSetConstantBuffers(1, 1, &m_waterBuffer);
    deviceContext->PSSetConstantBuffers(1, 1, &m_waterBuffer);
    deviceContext->PSSetConstantBuffers(2, 1, &m_lightBuffer);

    deviceContext->PSSetShaderResources(0, 1, &normalTexture);
    deviceContext->PSSetSamplers(0, 1, &m_sampleState);

    deviceContext->IASetInputLayout(m_layout);
    deviceContext->VSSetShader(m_vertexShader, NULL, 0);
    deviceContext->PSSetShader(m_pixelShader, NULL, 0);

    deviceContext->DrawIndexed(indexCount, 0, 0);

    return true;
}