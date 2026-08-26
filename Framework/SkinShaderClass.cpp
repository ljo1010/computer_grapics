#include "SkinShaderClass.h"

#include <Windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#include <fstream>
#include <dxgiformat.h>
#ifndef DXGI_FORMAT_R32G32B32A_FLOAT
#define DXGI_FORMAT_R32G32B32A_FLOAT ((DXGI_FORMAT)2)
#endif

#ifndef DXGI_FORMAT_R32G32B32A_UINT
#define DXGI_FORMAT_R32G32B32A_UINT  ((DXGI_FORMAT)3)
#endif
using namespace DirectX;
using std::vector;

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
static const UINT MAX_BONES = 120;

struct MatrixBufferType
{
    XMMATRIX world;
    XMMATRIX view;
    XMMATRIX projection;
};

struct BoneBufferType
{
    XMMATRIX boneMatrices[MAX_BONES];
};

SkinShaderClass::SkinShaderClass() {}
SkinShaderClass::~SkinShaderClass()
{
    Shutdown();
}

bool SkinShaderClass::Initialize(ID3D11Device* device, HWND hwnd)
{
    HRESULT result;
    ID3D10Blob* vsBuffer = nullptr;
    ID3D10Blob* psBuffer = nullptr;
    ID3D10Blob* errorMsg = nullptr;

    // VS
    result = D3DCompileFromFile(
        L"./data/skinshader.hlsl",
        nullptr, nullptr,
        "VS", "vs_5_0",
        D3D10_SHADER_ENABLE_STRICTNESS, 0,
        &vsBuffer, &errorMsg);

    if (FAILED(result))
    {
        if (errorMsg)
        {
            std::ofstream fout("skinshader-vs-error.txt",
                std::ios::binary);
            fout.write(
                (const char*)errorMsg->GetBufferPointer(),
                errorMsg->GetBufferSize());
            fout.close();
            errorMsg->Release();
        }
        MessageBox(hwnd, L"skinshader VS compile error",
            L"Error", MB_OK);
        return false;
    }

    // PS
    result = D3DCompileFromFile(
        L"./data/skinshader.hlsl",
        nullptr, nullptr,
        "PS", "ps_5_0",
        D3D10_SHADER_ENABLE_STRICTNESS, 0,
        &psBuffer, &errorMsg);

    if (FAILED(result))
    {
        if (errorMsg)
        {
            std::ofstream fout("skinshader-ps-error.txt",
                std::ios::binary);
            fout.write(
                (const char*)errorMsg->GetBufferPointer(),
                errorMsg->GetBufferSize());
            fout.close();
            errorMsg->Release();
        }
        if (vsBuffer) vsBuffer->Release();
        MessageBox(hwnd, L"skinshader PS compile error",
            L"Error", MB_OK);
        return false;
    }

    // VS/PS ��ü
    result = device->CreateVertexShader(
        vsBuffer->GetBufferPointer(),
        vsBuffer->GetBufferSize(),
        nullptr,
        &m_vertexShader);
    if (FAILED(result))
    {
        vsBuffer->Release();
        psBuffer->Release();
        return false;
    }

    result = device->CreatePixelShader(
        psBuffer->GetBufferPointer(),
        psBuffer->GetBufferSize(),
        nullptr,
        &m_pixelShader);
    if (FAILED(result))
    {
        vsBuffer->Release();
        psBuffer->Release();
        return false;
    }

    // Input Layout
    D3D11_INPUT_ELEMENT_DESC layoutDesc[4] = {};

    // POSITION
    layoutDesc[0].SemanticName = "POSITION";
    layoutDesc[0].SemanticIndex = 0;
    layoutDesc[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    layoutDesc[0].InputSlot = 0;
    layoutDesc[0].AlignedByteOffset = 0;
    layoutDesc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    layoutDesc[0].InstanceDataStepRate = 0;

    // TEXCOORD
    layoutDesc[1].SemanticName = "TEXCOORD";
    layoutDesc[1].SemanticIndex = 0;
    layoutDesc[1].Format = DXGI_FORMAT_R32G32_FLOAT;
    layoutDesc[1].InputSlot = 0;
    layoutDesc[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
    layoutDesc[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    layoutDesc[1].InstanceDataStepRate = 0;

    // BONEID (uint4)
    layoutDesc[2].SemanticName = "BONEID";
    layoutDesc[2].SemanticIndex = 0;
    layoutDesc[2].Format = DXGI_FORMAT_R32G32B32A_UINT;
    layoutDesc[2].InputSlot = 0;
    layoutDesc[2].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
    layoutDesc[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    layoutDesc[2].InstanceDataStepRate = 0;

    // WEIGHT (float4)
    layoutDesc[3].SemanticName = "WEIGHT";
    layoutDesc[3].SemanticIndex = 0;
    layoutDesc[3].Format = DXGI_FORMAT_R32G32B32A_FLOAT;
    layoutDesc[3].InputSlot = 0;
    layoutDesc[3].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
    layoutDesc[3].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    layoutDesc[3].InstanceDataStepRate = 0;

    UINT numElements = 4;

    result = device->CreateInputLayout(
        layoutDesc,
        numElements,
        vsBuffer->GetBufferPointer(),
        vsBuffer->GetBufferSize(),
        &m_layout);

    vsBuffer->Release();
    psBuffer->Release();

    if (FAILED(result))
        return false;

    // Matrix buffer (b0)
    D3D11_BUFFER_DESC mbDesc = {};
    mbDesc.Usage = D3D11_USAGE_DYNAMIC;
    mbDesc.ByteWidth = sizeof(MatrixBufferType);
    mbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    mbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    result = device->CreateBuffer(&mbDesc, nullptr, &m_matrixBuffer);
    if (FAILED(result))
        return false;

    // Bone buffer (b1)
    D3D11_BUFFER_DESC bbDesc = {};
    bbDesc.Usage = D3D11_USAGE_DYNAMIC;
    bbDesc.ByteWidth = sizeof(BoneBufferType);
    bbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    result = device->CreateBuffer(&bbDesc, nullptr, &m_boneBuffer);
    if (FAILED(result))
        return false;

    // Sampler
    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

    result = device->CreateSamplerState(&sampDesc, &m_sampleState);
    if (FAILED(result))
        return false;

    return true;
}

void SkinShaderClass::Shutdown()
{
    if (m_sampleState) { m_sampleState->Release();  m_sampleState = nullptr; }
    if (m_boneBuffer) { m_boneBuffer->Release();   m_boneBuffer = nullptr; }
    if (m_matrixBuffer) { m_matrixBuffer->Release(); m_matrixBuffer = nullptr; }
    if (m_layout) { m_layout->Release();       m_layout = nullptr; }
    if (m_pixelShader) { m_pixelShader->Release();  m_pixelShader = nullptr; }
    if (m_vertexShader) { m_vertexShader->Release(); m_vertexShader = nullptr; }
}

bool SkinShaderClass::SetShaderParameters(
    ID3D11DeviceContext* dc,
    const XMMATRIX& world,
    const XMMATRIX& view,
    const XMMATRIX& proj,
    const std::vector<XMMATRIX>& bones,
    ID3D11ShaderResourceView* texture)
{
    HRESULT result;
    D3D11_MAPPED_SUBRESOURCE mapped;
    MatrixBufferType* mPtr;
    BoneBufferType* bPtr;

    // ��� ��ġ
    XMMATRIX w = XMMatrixTranspose(world);
    XMMATRIX v = XMMatrixTranspose(view);
    XMMATRIX p = XMMatrixTranspose(proj);

    // b0: matrix
    result = dc->Map(m_matrixBuffer, 0,
        D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (FAILED(result)) return false;

    mPtr = (MatrixBufferType*)mapped.pData;
    mPtr->world = w;
    mPtr->view = v;
    mPtr->projection = p;

    dc->Unmap(m_matrixBuffer, 0);
    dc->VSSetConstantBuffers(0, 1, &m_matrixBuffer);

    // ==== [2] ���⼭ bones ����� �α� �߰� =============================
    size_t boneCount = bones.size();
    char buf[256];
    sprintf_s(buf, "[SkinShader] bones.size() = %zu\n", boneCount);
    OutputDebugStringA(buf);

    for (size_t i = 0; i < min(boneCount, (size_t)3); ++i)
    {
        XMFLOAT4X4 m;
        XMStoreFloat4x4(&m, bones[i]);
        sprintf_s(buf,
            "[SkinShader] bone %zu: %.3f %.3f %.3f %.3f | %.3f %.3f %.3f %.3f\n",
            i, m._11, m._12, m._13, m._14,
            m._21, m._22, m._23, m._24);
        OutputDebugStringA(buf);
    }

    // b1: bones
    result = dc->Map(m_boneBuffer, 0,
        D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (FAILED(result)) return false;

    bPtr = (BoneBufferType*)mapped.pData;

    if (boneCount > MAX_BONES) boneCount = MAX_BONES;

    for (size_t i = 0; i < boneCount; ++i)
        bPtr->boneMatrices[i] = XMMatrixTranspose(bones[i]);

    for (size_t i = boneCount; i < MAX_BONES; ++i)
        bPtr->boneMatrices[i] = XMMatrixIdentity();

    dc->Unmap(m_boneBuffer, 0);
    dc->VSSetConstantBuffers(1, 1, &m_boneBuffer);

    // �ؽ�ó & ���÷�
    if (texture)
        dc->PSSetShaderResources(0, 1, &texture);
    dc->PSSetSamplers(0, 1, &m_sampleState);

    return true;
}

void SkinShaderClass::RenderShader(
    ID3D11DeviceContext* dc,
    int indexCount,
    int startIndexLocation)
{
    dc->IASetInputLayout(m_layout);
    dc->VSSetShader(m_vertexShader, nullptr, 0);
    dc->PSSetShader(m_pixelShader, nullptr, 0);

    dc->DrawIndexed(indexCount, startIndexLocation, 0);
}

bool SkinShaderClass::RenderMesh(
    ID3D11DeviceContext* dc,
    int indexCount,
    int startIndexLocation,
    const XMMATRIX& world,
    const XMMATRIX& view,
    const XMMATRIX& proj,
    const std::vector<XMMATRIX>& bones,
    ID3D11ShaderResourceView* texture)
{
    if (!SetShaderParameters(dc, world, view, proj, bones, texture))
        return false;

    RenderShader(dc, indexCount, startIndexLocation);
    return true;
}
