#include "Skybox.h"
#include <vector>
#include <cstdint>
#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")
using namespace DirectX;

// ======== Skybox HLSL (내장 버전) ========
static const char* SKY_VS = R"(
cbuffer CBPerObject : register(b0) { matrix WVP; matrix World; };
struct VSIn  { float3 pos : POSITION; };
struct VSOut { float4 posH : SV_POSITION; float3 dir : TEXCOORD0; };
VSOut SKYMAP_VS(VSIn i) {
    VSOut o;
    float4 wpos = mul(float4(i.pos, 1), World);
    o.posH = mul(wpos, WVP);
    o.posH.z = o.posH.w;      // 항상 가장 뒤에
    o.dir = i.pos;            // 방향 벡터 전달
    return o;
})";

static const char* SKY_PS = R"(
TextureCube skyCube : register(t0);
SamplerState samp   : register(s0);
struct PSIn { float4 posH : SV_POSITION; float3 dir : TEXCOORD0; };
float4 SKYMAP_PS(PSIn i) : SV_Target {
    return skyCube.Sample(samp, normalize(i.dir));
})";
// =========================================

bool Skybox::Init(ID3D11Device* dev, ID3D11DeviceContext* ctx, const wchar_t* ddsCubemapPath)
{
    device = dev;
    context = ctx;

    
    createSphere(100, 100);

    if (!createShaders()) return false;

    D3D11_SAMPLER_DESC sd{};
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
    if (FAILED(device->CreateSamplerState(&sd, &sampler))) return false;

    // 큐브맵 로드 (CubeMap 형식)
    if (FAILED(CreateDDSTextureFromFile(device, ddsCubemapPath, nullptr, &cubemapSRV)))
        return false;

    if (!createStates()) return false;
    if (!createCB()) return false;
    return true;
}

void Skybox::Update(const XMFLOAT3& /*camPos*/, const XMMATRIX& v, const XMMATRIX& p)
{
    // 1) 월드: 스케일만
    const float S = 50.f;
    world = XMMatrixScaling(S, S, S);

    // 2) 뷰: translation 제거
    XMMATRIX viewNoTrans = v;
    viewNoTrans.r[3] = XMVectorSet(0, 0, 0, 1);

    // 3) 상수버퍼 갱신
    XMMATRIX WVP = world * viewNoTrans * p;
    cbData.WVP = XMMatrixTranspose(WVP);
    cbData.World = XMMatrixTranspose(world);
    context->UpdateSubresource(cb, 0, nullptr, &cbData, 0, 0);
}



void Skybox::Draw()
{
    UINT stride = sizeof(XMFLOAT3), offset = 0;
    context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
    context->IASetIndexBuffer(ib, DXGI_FORMAT_R32_UINT, 0);
    context->IASetInputLayout(il);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // ← 추가

    context->OMSetDepthStencilState(dsLessEqual, 0);
    context->RSSetState(rsCullNone);

    context->VSSetShader(vs, nullptr, 0);
    context->VSSetConstantBuffers(0, 1, &cb);
    context->PSSetShader(ps, nullptr, 0);
    context->PSSetShaderResources(0, 1, &cubemapSRV);
    context->PSSetSamplers(0, 1, &sampler);

    context->DrawIndexed(indexCount, 0, 0);
}


void Skybox::Release()
{
    if (vb) vb->Release();
    if (ib) ib->Release();
    if (il) il->Release();
    if (vs) vs->Release();
    if (ps) ps->Release();
    if (cb) cb->Release();
    if (cubemapSRV) cubemapSRV->Release();
    if (sampler) sampler->Release();
    if (rsCullNone) rsCullNone->Release();
    if (dsLessEqual) dsLessEqual->Release();
}

void Skybox::createSphere(int lat, int lon)
{
    const int numVerts = ((lat - 2) * lon) + 2;
    const int numFaces = ((lat - 3) * lon * 2) + (lon * 2);
    indexCount = numFaces * 3;

    std::vector<XMFLOAT3> verts(numVerts);
    verts[0] = XMFLOAT3(0, 0, 1);
    int v = 1;
    for (int i = 0; i < lat - 2; i++) {
        float pitch = (i + 1) * (XM_PI / (lat - 1));
        for (int j = 0; j < lon; j++) {
            float yaw = j * (XM_2PI / lon);
            XMFLOAT3 p{
                sinf(pitch) * sinf(yaw),
                -cosf(pitch),  // 내부 보기
                sinf(pitch) * cosf(yaw)
            };
            verts[v++] = p;
        }
    }
    verts[numVerts - 1] = XMFLOAT3(0, 0, -1);

    std::vector<uint32_t> idx(indexCount);
    int k = 0;
    for (int l = 0; l < lon - 1; l++) { idx[k++] = 0; idx[k++] = l + 1; idx[k++] = l + 2; }
    idx[k++] = 0; idx[k++] = lon; idx[k++] = 1;

    for (int i = 0; i < lat - 3; i++) {
        for (int j = 0; j < lon - 1; j++) {
            uint32_t a = i * lon + j + 1, b = a + 1, c = (i + 1) * lon + j + 1, d = c + 1;
            idx[k++] = a; idx[k++] = b; idx[k++] = c;
            idx[k++] = c; idx[k++] = b; idx[k++] = d;
        }
        uint32_t a = i * lon + lon, b = i * lon + 1, c = (i + 1) * lon + lon, d = (i + 1) * lon + 1;
        idx[k++] = a; idx[k++] = b; idx[k++] = c;
        idx[k++] = c; idx[k++] = b; idx[k++] = d;
    }
    for (int l = 0; l < lon - 1; l++) {
        idx[k++] = numVerts - 1; idx[k++] = (numVerts - 1) - (l + 1); idx[k++] = (numVerts - 1) - (l + 2);
    }
    idx[k++] = numVerts - 1; idx[k++] = numVerts - 1 - lon; idx[k++] = numVerts - 2;

    D3D11_BUFFER_DESC vbd{}; vbd.Usage = D3D11_USAGE_DEFAULT; vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.ByteWidth = UINT(sizeof(XMFLOAT3) * verts.size());
    D3D11_SUBRESOURCE_DATA vinit{ verts.data(), 0, 0 };
    device->CreateBuffer(&vbd, &vinit, &vb);

    D3D11_BUFFER_DESC ibd{}; ibd.Usage = D3D11_USAGE_DEFAULT; ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.ByteWidth = UINT(sizeof(uint32_t) * idx.size());
    D3D11_SUBRESOURCE_DATA iinit{ idx.data(), 0, 0 };
    device->CreateBuffer(&ibd, &iinit, &ib);
}

bool Skybox::createStates() {
    D3D11_RASTERIZER_DESC rs{}; rs.FillMode = D3D11_FILL_SOLID; rs.CullMode = D3D11_CULL_NONE;
    if (FAILED(device->CreateRasterizerState(&rs, &rsCullNone))) return false;

    D3D11_DEPTH_STENCIL_DESC ds{};
    ds.DepthEnable = TRUE;
    ds.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;   // ← 기록 금지
    ds.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;        // ← <=
    if (FAILED(device->CreateDepthStencilState(&ds, &dsLessEqual))) return false;
    return true;
}



bool Skybox::createShaders()
{
    ID3DBlob* vsb = nullptr, * psb = nullptr, * err = nullptr;
    if (FAILED(D3DCompile(SKY_VS, strlen(SKY_VS), nullptr, nullptr, nullptr, "SKYMAP_VS", "vs_5_0", 0, 0, &vsb, &err)))
        return false;
    if (FAILED(D3DCompile(SKY_PS, strlen(SKY_PS), nullptr, nullptr, nullptr, "SKYMAP_PS", "ps_5_0", 0, 0, &psb, &err)))
        return false;

    device->CreateVertexShader(vsb->GetBufferPointer(), vsb->GetBufferSize(), nullptr, &vs);
    device->CreatePixelShader(psb->GetBufferPointer(), psb->GetBufferSize(), nullptr, &ps);

    D3D11_INPUT_ELEMENT_DESC desc[] = {
        { "POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0 }
    };
    device->CreateInputLayout(desc, 1, vsb->GetBufferPointer(), vsb->GetBufferSize(), &il);

    vsb->Release(); psb->Release();
    return true;
}

bool Skybox::createCB()
{
    D3D11_BUFFER_DESC bd{}; bd.Usage = D3D11_USAGE_DEFAULT; bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.ByteWidth = sizeof(CBPerObject);
    return SUCCEEDED(device->CreateBuffer(&bd, nullptr, &cb));
}
