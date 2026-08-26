////////////////////////////////////////////////////////////////////////////////
// Filename: DepthShaderClass.h
// 설명: 광원 시점 깊이 기록용 셰이더 클래스
////////////////////////////////////////////////////////////////////////////////
#ifndef _DEPTHSHADERCLASS_H_
#define _DEPTHSHADERCLASS_H_

#include <d3d11.h>
#include <directxmath.h>
#include <d3dcompiler.h>
#include "AlignedAllocationPolicy.h"

using namespace DirectX;

class DepthShaderClass : public AlignedAllocationPolicy<16>
{
private:
    struct MatrixBufferType
    {
        XMMATRIX world;
        XMMATRIX lightView;
        XMMATRIX lightProjection;
    };

public:
    DepthShaderClass();
    ~DepthShaderClass();

    bool Initialize(ID3D11Device* device, HWND hwnd);
    void Shutdown();
    bool Render(ID3D11DeviceContext* deviceContext, int indexCount,
                XMMATRIX worldMatrix, XMMATRIX lightViewMatrix, XMMATRIX lightProjectionMatrix);

private:
    bool InitializeShader(ID3D11Device* device, HWND hwnd, const WCHAR* vsFilename, const WCHAR* psFilename);
    void ShutdownShader();
    bool SetShaderParameters(ID3D11DeviceContext* deviceContext,
                             XMMATRIX worldMatrix, XMMATRIX lightViewMatrix, XMMATRIX lightProjectionMatrix);
    void RenderShader(ID3D11DeviceContext* deviceContext, int indexCount);

private:
    ID3D11VertexShader* m_vertexShader;
    ID3D11PixelShader* m_pixelShader;
    ID3D11InputLayout* m_layout;
    ID3D11Buffer* m_matrixBuffer;
};

#endif
