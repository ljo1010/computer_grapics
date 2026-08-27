#pragma once

#include <DirectXMath.h>
#include <d3d11.h>

// ============================================================================
// RenderContext: 렌더 패스들 간에 공유되는 프레임 렌더링 문맥 데이터
// ============================================================================
struct RenderContext
{
    ID3D11DeviceContext* deviceContext = nullptr;

    DirectX::XMMATRIX worldMatrix;
    DirectX::XMMATRIX viewMatrix;
    DirectX::XMMATRIX projectionMatrix;
    DirectX::XMMATRIX orthoMatrix;

    DirectX::XMMATRIX lightViewMatrix;
    DirectX::XMMATRIX lightProjectionMatrix;

    DirectX::XMFLOAT3 cameraPosition;

    ID3D11ShaderResourceView* shadowMapSRV = nullptr;
    float shadowBias = 0.0008f;
    float shadowIntensity = 0.65f;
    bool enableShadow = true;
    bool enablePCF = true;
    bool wireframe = false;
};