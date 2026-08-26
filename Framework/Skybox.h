#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include "DDSTextureLoader.h"
using namespace DirectX;

class Skybox {
public:
    bool Init(ID3D11Device* dev, ID3D11DeviceContext* ctx, const wchar_t* ddsCubemapPath);
    void Update(const XMFLOAT3& camPos, const XMMATRIX& view, const XMMATRIX& proj);
    void Draw();

    void Release();

private:
    void createSphere(int lat, int lon);
    bool createStates();
    bool createShaders();
    bool createCB();

    // refs
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;

    // geometry
    ID3D11Buffer* vb = nullptr;
    ID3D11Buffer* ib = nullptr;
    UINT          indexCount = 0;

    // shaders
    ID3D11VertexShader* vs = nullptr;
    ID3D11PixelShader* ps = nullptr;
    ID3D11InputLayout* il = nullptr;

    // resources
    ID3D11ShaderResourceView* cubemapSRV = nullptr;
    ID3D11SamplerState* sampler = nullptr;

    // states
    ID3D11RasterizerState* rsCullNone = nullptr;
    ID3D11DepthStencilState* dsLessEqual = nullptr;

    // CB
    struct CBPerObject { XMMATRIX WVP; XMMATRIX World; } cbData{};
    ID3D11Buffer* cb = nullptr;

    // cached
    XMMATRIX world = XMMatrixIdentity();
    XMMATRIX view = XMMatrixIdentity();
    XMMATRIX proj = XMMatrixIdentity();
};
