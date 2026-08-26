#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>
#include <string>

class ParticleSystem
{
public:
    struct Particle
    {
        DirectX::XMFLOAT3 pos;
        DirectX::XMFLOAT3 vel;
        DirectX::XMFLOAT4 color;
        float size;
        float life;
        float maxLife;
        float gravity;
    };

    struct VertexType
    {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT2 tex;
        DirectX::XMFLOAT4 color;
        float size;
    };

    struct MatrixBufferType
    {
        DirectX::XMMATRIX view;
        DirectX::XMMATRIX proj;
    };

    struct CameraBufferType
    {
        DirectX::XMFLOAT3 cameraPosition;
        float _pad;
    };

public:
    ParticleSystem();
    ~ParticleSystem();

    bool Initialize(ID3D11Device* device, HWND hwnd, const std::wstring& shaderFilename);
    void Shutdown();
    void Update(float dt);
    void Render(ID3D11DeviceContext* dc, const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& proj, const DirectX::XMFLOAT3& camPos);

    // 파티클 생성 함수
    void SpawnFeedParticles(const DirectX::XMFLOAT3& center, int count = 24);
    void SpawnHayImpact(const DirectX::XMFLOAT3& center, int count = 12);
    void Clear();

    int GetActiveCount() const { return static_cast<int>(m_particles.size()); }

private:
    bool InitializeBuffers(ID3D11Device* device);
    bool InitializeShaders(ID3D11Device* device, HWND hwnd, const std::wstring& shaderFilename);
    bool InitializeRenderStates(ID3D11Device* device);

private:
    static const int MAX_PARTICLES = 1000;

    std::vector<Particle> m_particles;

    ID3D11VertexShader* m_vertexShader = nullptr;
    ID3D11PixelShader* m_pixelShader = nullptr;
    ID3D11InputLayout* m_layout = nullptr;

    ID3D11Buffer* m_vertexBuffer = nullptr;
    ID3D11Buffer* m_indexBuffer = nullptr;
    ID3D11Buffer* m_matrixBuffer = nullptr;
    ID3D11Buffer* m_cameraBuffer = nullptr;

    ID3D11BlendState* m_blendState = nullptr;
    ID3D11DepthStencilState* m_depthState = nullptr;
};