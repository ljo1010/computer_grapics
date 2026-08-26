////////////////////////////////////////////////////////////////////////////////
// Filename: lightshaderclass.cpp  (UNIFIED: directional + point lights)
////////////////////////////////////////////////////////////////////////////////
#include "lightshaderclass.h"
#include <cstring>      // std::memset
#include <algorithm>    // std::min

using std::min;

LightShaderClass::LightShaderClass()
{
    m_vertexShader = nullptr;
    m_pixelShader = nullptr;
    m_layout = nullptr;
    m_sampleState = nullptr;
    m_matrixBuffer = nullptr;
    m_lightBuffer = nullptr;
    m_cameraBuffer = nullptr;

    // added
    m_pointPosBuffer = nullptr;
    m_pointColorBuffer = nullptr;
    m_attenuationBuffer = nullptr;
    m_toggleBuffer = nullptr;
}

LightShaderClass::LightShaderClass(const LightShaderClass& other)
{
    // not used
}

LightShaderClass::~LightShaderClass()
{
}

bool LightShaderClass::Initialize(ID3D11Device* device, HWND hwnd)
{
    // 통합 HLSL 로드 경로
    return InitializeShader(device, hwnd, L"./data/mergephongpoint.hlsl");
}

void LightShaderClass::Shutdown()
{
    ShutdownShader();
}

bool LightShaderClass::Render(ID3D11DeviceContext* deviceContext, int indexCount,
    XMMATRIX worldMatrix, XMMATRIX viewMatrix, XMMATRIX projectionMatrix,
    ID3D11ShaderResourceView* texture,
    XMFLOAT3 cameraPosition,
    XMFLOAT4 ambientColor,
    XMFLOAT4 diffuseColor,
    XMFLOAT3 lightDirection,
    XMFLOAT4 specularColor,
    float    specularPower)
{
    // 레거시 경로: 포인트 라이트/감쇠/토글은 기본값으로 채워 셰이더에 전달
    if (!SetShaderParameters(deviceContext, worldMatrix, viewMatrix, projectionMatrix,
        texture, cameraPosition, ambientColor, diffuseColor,
        lightDirection, specularColor, specularPower))
        return false;

    RenderShader(deviceContext, indexCount);
    return true;
}

bool LightShaderClass::RenderEx(ID3D11DeviceContext* deviceContext, int indexCount,
    XMMATRIX worldMatrix, XMMATRIX viewMatrix, XMMATRIX projectionMatrix,
    ID3D11ShaderResourceView* texture,
    // directional
    XMFLOAT3 cameraPosition,
    XMFLOAT4 ambientColor,
    XMFLOAT4 directionalDiffuse,
    XMFLOAT3 lightDirection,
    XMFLOAT4 specularColor,
    float    specularPower,
    // points
    const XMFLOAT4* pointPositions,
    const XMFLOAT4* pointDiffuse,
    int             pointCount,
    // attenuation & scale
    float kc, float kl, float kq,
    float pointIntensityScale,
    // toggles
    bool enableAmbient,
    bool enableDiffuse,
    bool enableSpecular,
    // Shadow Mapping
    ID3D11ShaderResourceView* shadowMapSRV,
    XMMATRIX lightViewMatrix,
    XMMATRIX lightProjMatrix,
    float shadowBias,
    float shadowIntensity,
    bool enableShadow,
    bool enablePCF,
    // Distance Fog (대기 거리 안개)
    XMFLOAT4 fogColor,
    float fogStart,
    float fogEnd,
    bool enableFog)
{
    if (!SetShaderParametersEx(deviceContext, worldMatrix, viewMatrix, projectionMatrix,
        texture, cameraPosition, ambientColor, directionalDiffuse,
        lightDirection, specularColor, specularPower,
        pointPositions, pointDiffuse, pointCount,
        kc, kl, kq, pointIntensityScale,
        enableAmbient, enableDiffuse, enableSpecular,
        shadowMapSRV, lightViewMatrix, lightProjMatrix, shadowBias, shadowIntensity, enableShadow, enablePCF,
        fogColor, fogStart, fogEnd, enableFog))
        return false;

    RenderShader(deviceContext, indexCount);
    return true;
}

bool LightShaderClass::InitializeShader(ID3D11Device* device, HWND hwnd, const WCHAR* hlslFile)
{
    HRESULT result;
    ID3D10Blob* errorMessage = nullptr;
    ID3D10Blob* vertexShaderBuffer = nullptr;
    ID3D10Blob* pixelShaderBuffer = nullptr;

    // 1) Compile shaders
    result = D3DCompileFromFile(hlslFile, NULL, NULL,
        "LightVertexShader", "vs_5_0",
        D3D10_SHADER_ENABLE_STRICTNESS, 0,
        &vertexShaderBuffer, &errorMessage);
    if (FAILED(result))
    {
        if (errorMessage) OutputShaderErrorMessage(errorMessage, hwnd, hlslFile);
        else MessageBox(hwnd, hlslFile, L"Missing Shader File", MB_OK);
        return false;
    }

    result = D3DCompileFromFile(hlslFile, NULL, NULL,
        "LightPixelShader", "ps_5_0",
        D3D10_SHADER_ENABLE_STRICTNESS, 0,
        &pixelShaderBuffer, &errorMessage);
    if (FAILED(result))
    {
        if (errorMessage) OutputShaderErrorMessage(errorMessage, hwnd, hlslFile);
        else MessageBox(hwnd, hlslFile, L"Missing Shader File", MB_OK);
        return false;
    }

    // 2) Create shaders
    result = device->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(),
        vertexShaderBuffer->GetBufferSize(),
        NULL, &m_vertexShader);
    if (FAILED(result)) return false;

    result = device->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(),
        pixelShaderBuffer->GetBufferSize(),
        NULL, &m_pixelShader);
    if (FAILED(result)) return false;

    // 3) Input layout (POSITION, TEXCOORD, NORMAL) -> VS outputs extra TEXCOORDs but inputs 그대로
    D3D11_INPUT_ELEMENT_DESC polygonLayout[3] = {};
    polygonLayout[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
                         0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
    polygonLayout[1] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0,
                         D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 };
    polygonLayout[2] = { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
                         D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 };

    UINT numElements = _countof(polygonLayout);
    result = device->CreateInputLayout(polygonLayout, numElements,
        vertexShaderBuffer->GetBufferPointer(),
        vertexShaderBuffer->GetBufferSize(), &m_layout);
    if (FAILED(result)) return false;

    // Release shader blobs
    vertexShaderBuffer->Release(); vertexShaderBuffer = nullptr;
    pixelShaderBuffer->Release();  pixelShaderBuffer = nullptr;

    // 4) Sampler
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    result = device->CreateSamplerState(&samplerDesc, &m_sampleState);
    if (FAILED(result)) return false;

    // 5) Constant buffers
    D3D11_BUFFER_DESC desc = {};
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    // MatrixBuffer (VS slot 0)
    desc.ByteWidth = sizeof(MatrixBufferType);
    result = device->CreateBuffer(&desc, nullptr, &m_matrixBuffer);
    if (FAILED(result)) return false;

    // CameraBuffer (VS slot 1)
    desc.ByteWidth = sizeof(CameraBufferType);
    result = device->CreateBuffer(&desc, nullptr, &m_cameraBuffer);
    if (FAILED(result)) return false;

    // Dir LightBuffer (PS slot 0)
    desc.ByteWidth = sizeof(LightBufferType);
    result = device->CreateBuffer(&desc, nullptr, &m_lightBuffer);
    if (FAILED(result)) return false;

    // Point positions (VS slot 2)
    desc.ByteWidth = sizeof(PointLightPositionBufferType);
    result = device->CreateBuffer(&desc, nullptr, &m_pointPosBuffer);
    if (FAILED(result)) return false;

    // Point colors (PS slot 1)
    desc.ByteWidth = sizeof(PointLightColorBufferType);
    result = device->CreateBuffer(&desc, nullptr, &m_pointColorBuffer);
    if (FAILED(result)) return false;

    // Attenuation (PS slot 2)
    desc.ByteWidth = sizeof(AttenuationBufferType);
    result = device->CreateBuffer(&desc, nullptr, &m_attenuationBuffer);
    if (FAILED(result)) return false;

    // Toggle (PS slot 6)
    desc.ByteWidth = sizeof(ToggleBufferType);
    result = device->CreateBuffer(&desc, nullptr, &m_toggleBuffer);
    if (FAILED(result)) return false;

    // ShadowBuffer (VS & PS slot 7)
    desc.ByteWidth = sizeof(ShadowBufferType);
    result = device->CreateBuffer(&desc, nullptr, &m_shadowBuffer);
    if (FAILED(result)) return false;

    // FogBuffer (PS slot 8)
    desc.ByteWidth = sizeof(FogBufferType);
    result = device->CreateBuffer(&desc, nullptr, &m_fogBuffer);
    if (FAILED(result)) return false;

    return true;
}

void LightShaderClass::ShutdownShader()
{
    auto safeRelease = [](auto*& p) { if (p) { p->Release(); p = nullptr; } };

    safeRelease(m_fogBuffer);
    safeRelease(m_shadowBuffer);
    safeRelease(m_toggleBuffer);
    safeRelease(m_attenuationBuffer);
    safeRelease(m_pointColorBuffer);
    safeRelease(m_pointPosBuffer);

    safeRelease(m_cameraBuffer);
    safeRelease(m_lightBuffer);
    safeRelease(m_matrixBuffer);

    safeRelease(m_sampleState);
    safeRelease(m_layout);
    safeRelease(m_pixelShader);
    safeRelease(m_vertexShader);
}

void LightShaderClass::OutputShaderErrorMessage(ID3D10Blob* errorMessage, HWND hwnd, const WCHAR* shaderFilename)
{
    char* compileErrors = (char*)errorMessage->GetBufferPointer();
    unsigned long bufferSize = (unsigned long)errorMessage->GetBufferSize();

    std::ofstream fout("shader-error.txt", std::ios::out | std::ios::binary);
    fout.write(compileErrors, bufferSize);
    fout.close();

    errorMessage->Release(); errorMessage = nullptr;

    MessageBox(hwnd, L"Error compiling shader. Check shader-error.txt for message.", shaderFilename, MB_OK);
}

bool LightShaderClass::SetShaderParameters(ID3D11DeviceContext* deviceContext,
    XMMATRIX worldMatrix, XMMATRIX viewMatrix, XMMATRIX projectionMatrix,
    ID3D11ShaderResourceView* texture,
    XMFLOAT3 cameraPosition,
    XMFLOAT4 ambientColor,
    XMFLOAT4 diffuseColor,
    XMFLOAT3 lightDirection,
    XMFLOAT4 specularColor,
    float    specularPower)
{
    // 공통: 행렬 transpose
    worldMatrix = XMMatrixTranspose(worldMatrix);
    viewMatrix = XMMatrixTranspose(viewMatrix);
    projectionMatrix = XMMatrixTranspose(projectionMatrix);

    HRESULT hr;
    D3D11_MAPPED_SUBRESOURCE mapped;

    // VS slot 0: Matrix
    hr = deviceContext->Map(m_matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (FAILED(hr)) return false;
    {
        MatrixBufferType* p = (MatrixBufferType*)mapped.pData;
        p->world = worldMatrix;
        p->view = viewMatrix;
        p->projection = projectionMatrix;
    }
    deviceContext->Unmap(m_matrixBuffer, 0);
    deviceContext->VSSetConstantBuffers(0, 1, &m_matrixBuffer);

    // VS slot 1: Camera
    hr = deviceContext->Map(m_cameraBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (FAILED(hr)) return false;
    {
        CameraBufferType* p = (CameraBufferType*)mapped.pData;
        p->cameraPosition = cameraPosition;
        p->padding = 0.0f;
    }
    deviceContext->Unmap(m_cameraBuffer, 0);
    deviceContext->VSSetConstantBuffers(1, 1, &m_cameraBuffer);

    // PS slot 0: Dir Light
    hr = deviceContext->Map(m_lightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (FAILED(hr)) return false;
    {
        LightBufferType* p = (LightBufferType*)mapped.pData;
        p->ambientColor = ambientColor;
        p->diffuseColor = diffuseColor;
        p->lightDirection = lightDirection;
        p->specularColor = specularColor;
        p->specularPower = specularPower;
    }
    deviceContext->Unmap(m_lightBuffer, 0);
    deviceContext->PSSetConstantBuffers(0, 1, &m_lightBuffer);

    // VS slot 2: Point positions
    {
        PointLightPositionBufferType temp = {};
        for (int i = 0; i < NUM_LIGHTS; ++i) temp.lightPosition[i] = XMFLOAT4(0, 0, 0, 1);
        hr = deviceContext->Map(m_pointPosBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (FAILED(hr)) return false;
        memcpy(mapped.pData, &temp, sizeof(temp));
        deviceContext->Unmap(m_pointPosBuffer, 0);
        deviceContext->VSSetConstantBuffers(2, 1, &m_pointPosBuffer);
    }

    // PS slot 1: Point colors (all zero -> 영향 없음)
    {
        PointLightColorBufferType temp = {};
        for (int i = 0; i < NUM_LIGHTS; ++i) temp.pointDiffuse[i] = XMFLOAT4(0, 0, 0, 1);
        hr = deviceContext->Map(m_pointColorBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (FAILED(hr)) return false;
        memcpy(mapped.pData, &temp, sizeof(temp));
        deviceContext->Unmap(m_pointColorBuffer, 0);
        deviceContext->PSSetConstantBuffers(1, 1, &m_pointColorBuffer);
    }

    // PS slot 2: Attenuation (kc=1, kl=0, kq=0, scale=1)
    {
        AttenuationBufferType temp = { 1.0f, 0.0f, 0.0f, 1.0f };
        hr = deviceContext->Map(m_attenuationBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (FAILED(hr)) return false;
        memcpy(mapped.pData, &temp, sizeof(temp));
        deviceContext->Unmap(m_attenuationBuffer, 0);
        deviceContext->PSSetConstantBuffers(2, 1, &m_attenuationBuffer);
    }

    // PS slot 3: Toggle (ambient/diffuse/specular = on)
    {
        ToggleBufferType temp = { 1, 1, 1, 0 };
        hr = deviceContext->Map(m_toggleBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (FAILED(hr)) return false;
        memcpy(mapped.pData, &temp, sizeof(temp));
        deviceContext->Unmap(m_toggleBuffer, 0);
        deviceContext->PSSetConstantBuffers(3, 1, &m_toggleBuffer);
    }

    // Texture & Sampler
    deviceContext->PSSetShaderResources(0, 1, &texture);

    return true;
}

// ====================== Extended parameter path ======================
bool LightShaderClass::SetShaderParametersEx(ID3D11DeviceContext* deviceContext,
    XMMATRIX worldMatrix, XMMATRIX viewMatrix, XMMATRIX projectionMatrix,
    ID3D11ShaderResourceView* texture,
    // directional
    XMFLOAT3 cameraPosition,
    XMFLOAT4 ambientColor,
    XMFLOAT4 directionalDiffuse,
    XMFLOAT3 lightDirection,
    XMFLOAT4 specularColor,
    float    specularPower,
    // points
    const XMFLOAT4* pointPositions,
    const XMFLOAT4* pointDiffuse,
    int             pointCount,
    // attenuation & scale
    float kc, float kl, float kq,
    float pointIntensityScale,
    // toggles
    bool enableAmbient,
    bool enableDiffuse,
    bool enableSpecular,
    // Shadow Mapping
    ID3D11ShaderResourceView* shadowMapSRV,
    XMMATRIX lightViewMatrix,
    XMMATRIX lightProjMatrix,
    float shadowBias,
    float shadowIntensity,
    bool enableShadow,
    bool enablePCF,
    // Distance Fog (대기 거리 안개)
    XMFLOAT4 fogColor,
    float fogStart,
    float fogEnd,
    bool enableFog)
{
    worldMatrix = XMMatrixTranspose(worldMatrix);
    viewMatrix = XMMatrixTranspose(viewMatrix);
    projectionMatrix = XMMatrixTranspose(projectionMatrix);

    HRESULT hr;
    D3D11_MAPPED_SUBRESOURCE mapped;

    // VS slot 0: Matrix
    hr = deviceContext->Map(m_matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (FAILED(hr)) return false;
    {
        MatrixBufferType* p = (MatrixBufferType*)mapped.pData;
        p->world = worldMatrix;
        p->view = viewMatrix;
        p->projection = projectionMatrix;
    }
    deviceContext->Unmap(m_matrixBuffer, 0);
    deviceContext->VSSetConstantBuffers(0, 1, &m_matrixBuffer);

    // VS slot 1: Camera
    hr = deviceContext->Map(m_cameraBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (FAILED(hr)) return false;
    {
        CameraBufferType* p = (CameraBufferType*)mapped.pData;
        p->cameraPosition = cameraPosition;
        p->padding = 0.0f;
    }
    deviceContext->Unmap(m_cameraBuffer, 0);
    deviceContext->VSSetConstantBuffers(1, 1, &m_cameraBuffer);

    // PS slot 2: Dir Light
    hr = deviceContext->Map(m_lightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (FAILED(hr)) return false;
    {
        LightBufferType* p = (LightBufferType*)mapped.pData;
        p->ambientColor = ambientColor;
        p->diffuseColor = directionalDiffuse;
        p->lightDirection = lightDirection;
        p->specularColor = specularColor;
        p->specularPower = specularPower;
    }
    deviceContext->Unmap(m_lightBuffer, 0);
    deviceContext->PSSetConstantBuffers(2, 1, &m_lightBuffer);

    // VS slot 2 / PS slot 3: Point positions
    {
        PointLightPositionBufferType temp = {};
        int n = min(pointCount, NUM_LIGHTS);
        for (int i = 0; i < n; ++i) temp.lightPosition[i] = pointPositions[i];
        for (int i = n; i < NUM_LIGHTS; ++i) temp.lightPosition[i] = XMFLOAT4(0, 0, 0, 1);

        hr = deviceContext->Map(m_pointPosBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (FAILED(hr)) return false;
        memcpy(mapped.pData, &temp, sizeof(temp));
        deviceContext->Unmap(m_pointPosBuffer, 0);
        deviceContext->VSSetConstantBuffers(2, 1, &m_pointPosBuffer);
        deviceContext->PSSetConstantBuffers(3, 1, &m_pointPosBuffer);
    }

    // PS slot 4: Point colors
    {
        PointLightColorBufferType temp = {};
        int n = min(pointCount, NUM_LIGHTS);
        for (int i = 0; i < n; ++i) temp.pointDiffuse[i] = pointDiffuse[i];
        for (int i = n; i < NUM_LIGHTS; ++i) temp.pointDiffuse[i] = XMFLOAT4(0, 0, 0, 1);

        hr = deviceContext->Map(m_pointColorBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (FAILED(hr)) return false;
        memcpy(mapped.pData, &temp, sizeof(temp));
        deviceContext->Unmap(m_pointColorBuffer, 0);
        deviceContext->PSSetConstantBuffers(4, 1, &m_pointColorBuffer);
    }

    // PS slot 5: Attenuation
    {
        AttenuationBufferType temp = { kc, kl, kq, pointIntensityScale };
        hr = deviceContext->Map(m_attenuationBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (FAILED(hr)) return false;
        memcpy(mapped.pData, &temp, sizeof(temp));
        deviceContext->Unmap(m_attenuationBuffer, 0);
        deviceContext->PSSetConstantBuffers(5, 1, &m_attenuationBuffer);
    }

    // PS slot 6: Toggles
    {
        ToggleBufferType temp = { enableAmbient ? 1 : 0,
                                  enableDiffuse ? 1 : 0,
                                  enableSpecular ? 1 : 0,
                                  0 };
        hr = deviceContext->Map(m_toggleBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (FAILED(hr)) return false;
        memcpy(mapped.pData, &temp, sizeof(temp));
        deviceContext->Unmap(m_toggleBuffer, 0);
        deviceContext->PSSetConstantBuffers(6, 1, &m_toggleBuffer);
    }

    // VS & PS slot 7: Shadow Buffer
    if (m_shadowBuffer)
    {
        hr = deviceContext->Map(m_shadowBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (SUCCEEDED(hr))
        {
            ShadowBufferType* s = (ShadowBufferType*)mapped.pData;
            s->lightViewMatrix = XMMatrixTranspose(lightViewMatrix);
            s->lightProjectionMatrix = XMMatrixTranspose(lightProjMatrix);
            s->shadowBias = shadowBias;
            s->enableShadow = enableShadow ? 1 : 0;
            s->enablePCF = enablePCF ? 1 : 0;
            s->shadowIntensity = shadowIntensity;
            deviceContext->Unmap(m_shadowBuffer, 0);

            deviceContext->VSSetConstantBuffers(7, 1, &m_shadowBuffer);
            deviceContext->PSSetConstantBuffers(7, 1, &m_shadowBuffer);
        }
    }

    // PS slot 8: Fog Buffer (대기 거리 안개)
    if (m_fogBuffer)
    {
        hr = deviceContext->Map(m_fogBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (SUCCEEDED(hr))
        {
            FogBufferType* f = (FogBufferType*)mapped.pData;
            f->fogColor = fogColor;
            f->fogStart = fogStart;
            f->fogEnd = fogEnd;
            f->enableFog = enableFog ? 1 : 0;
            f->_fogPadding = 0.0f;
            deviceContext->Unmap(m_fogBuffer, 0);

            deviceContext->PSSetConstantBuffers(8, 1, &m_fogBuffer);
        }
    }

    // Texture (slot 0: Albedo, slot 3: Shadow Map)
    deviceContext->PSSetShaderResources(0, 1, &texture);
    if (shadowMapSRV)
    {
        deviceContext->PSSetShaderResources(3, 1, &shadowMapSRV);
    }

    return true;
}

void LightShaderClass::RenderShader(ID3D11DeviceContext* deviceContext, int indexCount)
{
    // IA Layout
    deviceContext->IASetInputLayout(m_layout);

    // Shaders
    deviceContext->VSSetShader(m_vertexShader, NULL, 0);
    deviceContext->PSSetShader(m_pixelShader, NULL, 0);

    // Sampler
    deviceContext->PSSetSamplers(0, 1, &m_sampleState);

    // Draw
    deviceContext->DrawIndexed(indexCount, 0, 0);
}
