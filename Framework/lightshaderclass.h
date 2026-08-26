////////////////////////////////////////////////////////////////////////////////
// Filename: lightshaderclass.h (UNIFIED: directional + point lights)
////////////////////////////////////////////////////////////////////////////////
#ifndef _LIGHTSHADERCLASS_H_
#define _LIGHTSHADERCLASS_H_

//////////////
// INCLUDES //
//////////////
#include <d3d11.h>
#include <directxmath.h>
#include <d3dcompiler.h>
#include <fstream>

using namespace std;
using namespace DirectX;

// HLSL과 동일하게 유지
#ifndef NUM_LIGHTS
#define NUM_LIGHTS 3
#endif

////////////////////////////////////////////////////////////////////////////////
// Class name: LightShaderClass
////////////////////////////////////////////////////////////////////////////////
class LightShaderClass
{
private:
    // HLSL: MatrixBuffer
    struct MatrixBufferType
    {
        XMMATRIX world;
        XMMATRIX view;
        XMMATRIX projection;
    };

    // HLSL: CameraBuffer
    struct CameraBufferType
    {
        XMFLOAT3 cameraPosition;
        float    padding; // 16-byte align
    };

    // HLSL: DirLightBuffer (Directional Phong)
    struct LightBufferType
    {
        XMFLOAT4 ambientColor;   // a
        XMFLOAT4 diffuseColor;   // d
        XMFLOAT3 lightDirection; // dir (toward scene)
        float    specularPower;  // shininess
        XMFLOAT4 specularColor;  // s
    };

    // HLSL: PointLightPositionBuffer
    struct PointLightPositionBufferType
    {
        XMFLOAT4 lightPosition[NUM_LIGHTS];
    };

    // HLSL: PointLightColorBuffer
    struct PointLightColorBufferType
    {
        XMFLOAT4 pointDiffuse[NUM_LIGHTS];  // rgba
    };

    // HLSL: AttenuationBuffer
    struct AttenuationBufferType
    {
        float kc;   // constant
        float kl;   // linear
        float kq;   // quadratic
        float pointIntensityScale; // 키 8/9로 조절
    };

    // HLSL: ToggleBuffer
    struct ToggleBufferType
    {
        int enableAmbient;   // 키 5
        int enableDiffuse;   // 키 6
        int enableSpecular;  // 키 7
        int _pad;            // 16-byte align
    };

public:
    LightShaderClass();
    LightShaderClass(const LightShaderClass&);
    ~LightShaderClass();

    bool Initialize(ID3D11Device*, HWND);
    void Shutdown();
    bool Render(ID3D11DeviceContext*, int,
        XMMATRIX, XMMATRIX, XMMATRIX,
        ID3D11ShaderResourceView*,
        XMFLOAT3 cameraPosition,
        XMFLOAT4 ambientColor,
        XMFLOAT4 diffuseColor,
        XMFLOAT3 lightDirection,
        XMFLOAT4 specularColor,
        float specularPower);


    bool RenderEx(ID3D11DeviceContext*, int,
        XMMATRIX, XMMATRIX, XMMATRIX,
        ID3D11ShaderResourceView*,
        // directional
        XMFLOAT3 cameraPosition,
        XMFLOAT4 ambientColor,
        XMFLOAT4 directionalDiffuse,
        XMFLOAT3 lightDirection,
        XMFLOAT4 specularColor,
        float    specularPower,
        // points
        const XMFLOAT4* pointPositions, // xyz pos, w ignored
        const XMFLOAT4* pointDiffuse,   // rgba
        int             pointCount,
        // attenuation & scale
        float kc, float kl, float kq,
        float pointIntensityScale,
        // toggles
        bool enableAmbient,
        bool enableDiffuse,
        bool enableSpecular);

private:
    bool InitializeShader(ID3D11Device*, HWND, const WCHAR* hlslFile);
    void ShutdownShader();
    void OutputShaderErrorMessage(ID3D10Blob*, HWND, const WCHAR*);

    // 기존 파라미터 셋업
    bool SetShaderParameters(ID3D11DeviceContext*,
        XMMATRIX, XMMATRIX, XMMATRIX,
        ID3D11ShaderResourceView*,
        XMFLOAT3 cameraPosition,
        XMFLOAT4 ambientColor,
        XMFLOAT4 diffuseColor,
        XMFLOAT3 lightDirection,
        XMFLOAT4 specularColor,
        float    specularPower);

    // 확장 파라미터 셋업
    bool SetShaderParametersEx(ID3D11DeviceContext*,
        XMMATRIX, XMMATRIX, XMMATRIX,
        ID3D11ShaderResourceView*,
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
        bool enableSpecular);

    void RenderShader(ID3D11DeviceContext*, int);

private:
    // Shaders & layout & sampler
    ID3D11VertexShader* m_vertexShader = nullptr;
    ID3D11PixelShader* m_pixelShader = nullptr;
    ID3D11InputLayout* m_layout = nullptr;
    ID3D11SamplerState* m_sampleState = nullptr;

    // Constant buffers
    ID3D11Buffer* m_matrixBuffer = nullptr; // MatrixBufferType
    ID3D11Buffer* m_cameraBuffer = nullptr; // CameraBufferType
    ID3D11Buffer* m_lightBuffer = nullptr; // LightBufferType (directional)

    // 추가된 버퍼들 (통합 셰이더용)
    ID3D11Buffer* m_pointPosBuffer = nullptr; // PointLightPositionBufferType
    ID3D11Buffer* m_pointColorBuffer = nullptr; // PointLightColorBufferType
    ID3D11Buffer* m_attenuationBuffer = nullptr; // AttenuationBufferType
    ID3D11Buffer* m_toggleBuffer = nullptr; // ToggleBufferType
};

#endif // _LIGHTSHADERCLASS_H_
