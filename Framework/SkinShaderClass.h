// SkinShaderClass.h
#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>

class SkinShaderClass
{
public:
    SkinShaderClass();
    ~SkinShaderClass();

    bool Initialize(ID3D11Device* device, HWND hwnd);
    void Shutdown();

    // ½ºÅ°´× ¸Þ½¬ ·»´õ
    bool RenderMesh(
        ID3D11DeviceContext* dc,
        int indexCount,
        int startIndexLocation,
        const DirectX::XMMATRIX& world,
        const DirectX::XMMATRIX& view,
        const DirectX::XMMATRIX& proj,
        const std::vector<DirectX::XMMATRIX>& bones,
        ID3D11ShaderResourceView* texture);

private:
    bool SetShaderParameters(
        ID3D11DeviceContext* dc,
        const DirectX::XMMATRIX& world,
        const DirectX::XMMATRIX& view,
        const DirectX::XMMATRIX& proj,
        const std::vector<DirectX::XMMATRIX>& bones,
        ID3D11ShaderResourceView* texture);

    void RenderShader(
        ID3D11DeviceContext* dc,
        int indexCount,
        int startIndexLocation);

private:
    ID3D11VertexShader* m_vertexShader = nullptr;
    ID3D11PixelShader* m_pixelShader = nullptr;
    ID3D11InputLayout* m_layout = nullptr;
    ID3D11Buffer* m_matrixBuffer = nullptr; // b0
    ID3D11Buffer* m_boneBuffer = nullptr; // b1
    ID3D11SamplerState* m_sampleState = nullptr;
};
