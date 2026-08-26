////////////////////////////////////////////////////////////////////////////////
// Filename: lightshaderclass.h (UNIFIED: directional + point lights + shadow)
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
        float    padding;
    };

    // HLSL: DirLightBuffer (Directional Phong)
    struct LightBufferType
    {
        XMFLOAT4 ambientColor;
        XMFLOAT4 diffuseColor;
        XMFLOAT3 lightDirection;
        float    specularPower;
        XMFLOAT4 specularColor;
    };

    // HLSL: PointLightPositionBuffer
    struct PointLightPositionBufferType
    {
        XMFLOAT4 lightPosition[NUM_LIGHTS];
    };

    // HLSL: PointLightColorBuffer
    struct PointLightColorBufferType
    {
        XMFLOAT4 pointDiffuse[NUM_LIGHTS];
    };

    // HLSL: AttenuationBuffer
    struct AttenuationBufferType
    {
        float kc;
        float kl;
        float kq;
        float pointIntensityScale;
    };

    // HLSL: ToggleBuffer
    struct ToggleBufferType
    {
        int enableAmbient;
        int enableDiffuse;
        int enableSpecular;
        int _pad;
    };

    // HLSL: ShadowBuffer (실시간 섀도우 매핑)
    struct ShadowBufferType
    {
        XMMATRIX lightViewMatrix;
        XMMATRIX lightProjectionMatrix;
        float shadowBias;
        int enableShadow;
        int enablePCF;
        float shadowIntensity;
    };

    // HLSL: FogBuffer (거리 기반 대기 안개)
    struct FogBufferType
    {
        XMFLOAT4 fogColor;
        float fogStart;
        float fogEnd;
        int enableFog;
        float _fogPadding;
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
        // Shadow Mapping (그림자)
        ID3D11ShaderResourceView* shadowMapSRV = nullptr,
        XMMATRIX lightViewMatrix = XMMatrixIdentity(),
        XMMATRIX lightProjMatrix = XMMatrixIdentity(),
        float shadowBias = 0.0015f,
        float shadowIntensity = 0.85f,
        bool enableShadow = true,
        bool enablePCF = true,
        // Distance Fog (대기 거리 안개)
        XMFLOAT4 fogColor = XMFLOAT4(0.72f, 0.76f, 0.85f, 1.0f),
        float fogStart = 15.0f,
        float fogEnd = 75.0f,
        bool enableFog = true);

private:
    bool InitializeShader(ID3D11Device*, HWND, const WCHAR* hlslFile);
    void ShutdownShader();
    void OutputShaderErrorMessage(ID3D10Blob*, HWND, const WCHAR*);

    bool SetShaderParameters(ID3D11DeviceContext*,
        XMMATRIX, XMMATRIX, XMMATRIX,
        ID3D11ShaderResourceView*,
        XMFLOAT3 cameraPosition,
        XMFLOAT4 ambientColor,
        XMFLOAT4 diffuseColor,
        XMFLOAT3 lightDirection,
        XMFLOAT4 specularColor,
        float    specularPower);

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
        bool enableSpecular,
        // Shadow Mapping (그림자)
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
        bool enableFog);

    void RenderShader(ID3D11DeviceContext*, int);

private:
    ID3D11VertexShader* m_vertexShader = nullptr;
    ID3D11PixelShader* m_pixelShader = nullptr;
    ID3D11InputLayout* m_layout = nullptr;
    ID3D11SamplerState* m_sampleState = nullptr;

    ID3D11Buffer* m_matrixBuffer = nullptr;
    ID3D11Buffer* m_cameraBuffer = nullptr;
    ID3D11Buffer* m_lightBuffer = nullptr;

    ID3D11Buffer* m_pointPosBuffer = nullptr;
    ID3D11Buffer* m_pointColorBuffer = nullptr;
    ID3D11Buffer* m_attenuationBuffer = nullptr;
    ID3D11Buffer* m_toggleBuffer = nullptr;
    ID3D11Buffer* m_shadowBuffer = nullptr;
    ID3D11Buffer* m_fogBuffer = nullptr;
};

#endif // _LIGHTSHADERCLASS_H_