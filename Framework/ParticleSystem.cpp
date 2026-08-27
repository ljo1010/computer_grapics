#include "ParticleSystem.h"
#include "Event.h"
#include "EventBus.h"
#include <d3dcompiler.h>
#include <random>
#include <cmath>

#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;

ParticleSystem::ParticleSystem()
{
}

ParticleSystem::~ParticleSystem()
{
    Shutdown();
}

bool ParticleSystem::Initialize(ID3D11Device* device, HWND hwnd, const std::wstring& shaderFilename)
{
    if (!InitializeShaders(device, hwnd, shaderFilename)) return false;
    if (!InitializeBuffers(device)) return false;
    if (!InitializeRenderStates(device)) return false;

    // [옵저버 패턴] 이벤트 버스 구독
    EventBus::Get().Subscribe<AnimalFedEvent>([this](const AnimalFedEvent& e) {
        SpawnFeedParticles(e.position, e.isAllCompleted ? 32 : 24);
    });

    EventBus::Get().Subscribe<QuestResetEvent>([this](const QuestResetEvent& /*e*/) {
        Clear();
    });

    EventBus::Get().Subscribe<HayImpactEvent>([this](const HayImpactEvent& e) {
        SpawnHayImpact(e.position, 12);
    });

    return true;
}

void ParticleSystem::Shutdown()
{
    if (m_depthState) { m_depthState->Release(); m_depthState = nullptr; }
    if (m_blendState) { m_blendState->Release(); m_blendState = nullptr; }

    if (m_cameraBuffer) { m_cameraBuffer->Release(); m_cameraBuffer = nullptr; }
    if (m_matrixBuffer) { m_matrixBuffer->Release(); m_matrixBuffer = nullptr; }
    if (m_indexBuffer) { m_indexBuffer->Release(); m_indexBuffer = nullptr; }
    if (m_vertexBuffer) { m_vertexBuffer->Release(); m_vertexBuffer = nullptr; }

    if (m_layout) { m_layout->Release(); m_layout = nullptr; }
    if (m_pixelShader) { m_pixelShader->Release(); m_pixelShader = nullptr; }
    if (m_vertexShader) { m_vertexShader->Release(); m_vertexShader = nullptr; }

    m_particles.clear();
    EventBus::Get().Clear();
}

bool ParticleSystem::InitializeShaders(ID3D11Device* device, HWND hwnd, const std::wstring& shaderFilename)
{
    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* psBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;

    HRESULT hr = D3DCompileFromFile(
        shaderFilename.c_str(), nullptr, nullptr,
        "VS_main", "vs_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0,
        &vsBlob, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) { OutputDebugStringA((char*)errorBlob->GetBufferPointer()); errorBlob->Release(); }
        MessageBoxW(hwnd, L"Particle VS Compile Failed", L"Error", MB_OK);
        return false;
    }

    hr = D3DCompileFromFile(
        shaderFilename.c_str(), nullptr, nullptr,
        "PS_main", "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0,
        &psBlob, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) { OutputDebugStringA((char*)errorBlob->GetBufferPointer()); errorBlob->Release(); }
        MessageBoxW(hwnd, L"Particle PS Compile Failed", L"Error", MB_OK);
        vsBlob->Release();
        return false;
    }

    device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_vertexShader);
    device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_pixelShader);

    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "PSIZE",    0, DXGI_FORMAT_R32_FLOAT,          0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    device->CreateInputLayout(layout, _countof(layout), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_layout);

    vsBlob->Release();
    psBlob->Release();

    return true;
}

bool ParticleSystem::InitializeBuffers(ID3D11Device* device)
{
    // 동적 정점 버퍼 (최대 파티클 1000개 * 4개 정점)
    D3D11_BUFFER_DESC vbd = {};
    vbd.Usage = D3D11_USAGE_DYNAMIC;
    vbd.ByteWidth = sizeof(VertexType) * MAX_PARTICLES * 4;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    if (FAILED(device->CreateBuffer(&vbd, nullptr, &m_vertexBuffer))) return false;

    // 고정 인덱스 버퍼 (최대 파티클 1000개 * 6개 인덱스)
    std::vector<unsigned long> indices(MAX_PARTICLES * 6);
    for (int i = 0; i < MAX_PARTICLES; ++i)
    {
        int vBase = i * 4;
        int iBase = i * 6;
        indices[iBase + 0] = vBase + 0;
        indices[iBase + 1] = vBase + 1;
        indices[iBase + 2] = vBase + 2;
        indices[iBase + 3] = vBase + 0;
        indices[iBase + 4] = vBase + 2;
        indices[iBase + 5] = vBase + 3;
    }

    D3D11_BUFFER_DESC ibd = {};
    ibd.Usage = D3D11_USAGE_DEFAULT;
    ibd.ByteWidth = sizeof(unsigned long) * static_cast<UINT>(indices.size());
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA ibData = {};
    ibData.pSysMem = indices.data();
    if (FAILED(device->CreateBuffer(&ibd, &ibData, &m_indexBuffer))) return false;

    // MatrixBuffer (VS b0)
    D3D11_BUFFER_DESC mbd = {};
    mbd.Usage = D3D11_USAGE_DYNAMIC;
    mbd.ByteWidth = sizeof(MatrixBufferType);
    mbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    mbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    if (FAILED(device->CreateBuffer(&mbd, nullptr, &m_matrixBuffer))) return false;

    // CameraBuffer (VS b1)
    D3D11_BUFFER_DESC cbd = {};
    cbd.Usage = D3D11_USAGE_DYNAMIC;
    cbd.ByteWidth = sizeof(CameraBufferType);
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    if (FAILED(device->CreateBuffer(&cbd, nullptr, &m_cameraBuffer))) return false;

    return true;
}

bool ParticleSystem::InitializeRenderStates(ID3D11Device* device)
{
    // 가산 혼합 (Additive / Alpha Blending)
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE; // Additive glow
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    if (FAILED(device->CreateBlendState(&blendDesc, &m_blendState))) return false;

    // Depth Read-Only (반투명 파티클을 위해 깊이 쓰기 비활성화)
    D3D11_DEPTH_STENCIL_DESC depthDesc = {};
    depthDesc.DepthEnable = TRUE;
    depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; // No Depth Write
    depthDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

    if (FAILED(device->CreateDepthStencilState(&depthDesc, &m_depthState))) return false;

    return true;
}

void ParticleSystem::Update(float dt)
{
    for (size_t i = 0; i < m_particles.size(); )
    {
        Particle& p = m_particles[i];
        p.life -= dt;

        if (p.life <= 0.0f)
        {
            m_particles[i] = m_particles.back();
            m_particles.pop_back();
            continue;
        }

        // 물리 업데이트
        p.pos.x += p.vel.x * dt;
        p.pos.y += p.vel.y * dt;
        p.pos.z += p.vel.z * dt;
        p.vel.y -= p.gravity * dt;

        // 페이드 아웃
        float t = p.life / p.maxLife;
        p.color.w = (t * 1.5f > 1.0f) ? 1.0f : (t * 1.5f);

        ++i;
    }
}

void ParticleSystem::SpawnFeedParticles(const DirectX::XMFLOAT3& center, int count)
{
    static std::mt19937 rng(1337);
    std::uniform_real_distribution<float> distAngle(0.0f, XM_2PI);
    std::uniform_real_distribution<float> distSpd(1.5f, 4.2f);
    std::uniform_real_distribution<float> distUp(2.5f, 5.5f);
    std::uniform_real_distribution<float> distLife(0.8f, 1.4f);
    std::uniform_real_distribution<float> distColor(0.0f, 1.0f);

    for (int i = 0; i < count; ++i)
    {
        if (m_particles.size() >= MAX_PARTICLES) break;

        float angle = distAngle(rng);
        float spd = distSpd(rng);

        Particle p;
        p.pos = center;
        p.pos.y += 0.3f; // 약간 위쪽에서 생성

        p.vel.x = cosf(angle) * spd;
        p.vel.y = distUp(rng);
        p.vel.z = sinf(angle) * spd;

        float cPick = distColor(rng);
        if (cPick < 0.6f)
        {
            // 황금빛 별 (Golden Yellow)
            p.color = XMFLOAT4(1.0f, 0.88f, 0.25f, 1.0f);
            p.size = 0.55f;
        }
        else
        {
            // 행복한 분홍 하트 빛 (Pink Rose)
            p.color = XMFLOAT4(1.0f, 0.40f, 0.65f, 1.0f);
            p.size = 0.45f;
        }

        p.maxLife = distLife(rng);
        p.life = p.maxLife;
        p.gravity = 4.0f;

        m_particles.push_back(p);
    }
}

void ParticleSystem::SpawnHayImpact(const DirectX::XMFLOAT3& center, int count)
{
    static std::mt19937 rng(42);
    std::uniform_real_distribution<float> distAngle(0.0f, XM_2PI);
    std::uniform_real_distribution<float> distSpd(1.0f, 3.0f);
    std::uniform_real_distribution<float> distUp(1.0f, 3.5f);
    std::uniform_real_distribution<float> distLife(0.4f, 0.7f);

    for (int i = 0; i < count; ++i)
    {
        if (m_particles.size() >= MAX_PARTICLES) break;

        float angle = distAngle(rng);
        float spd = distSpd(rng);

        Particle p;
        p.pos = center;
        p.vel.x = cosf(angle) * spd;
        p.vel.y = distUp(rng);
        p.vel.z = sinf(angle) * spd;

        // 건초 노란색
        p.color = XMFLOAT4(0.95f, 0.85f, 0.35f, 1.0f);
        p.size = 0.35f;
        p.maxLife = distLife(rng);
        p.life = p.maxLife;
        p.gravity = 6.0f;

        m_particles.push_back(p);
    }
}

void ParticleSystem::Clear()
{
    m_particles.clear();
}

void ParticleSystem::Render(ID3D11DeviceContext* dc, const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& proj, const DirectX::XMFLOAT3& camPos)
{
    if (m_particles.empty() || !m_vertexBuffer || !m_indexBuffer) return;

    // 1. Dynamic Vertex Buffer 업데이트
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    if (FAILED(dc->Map(m_vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) return;

    VertexType* v = reinterpret_cast<VertexType*>(mapped.pData);
    int pCount = static_cast<int>(m_particles.size());

    for (int i = 0; i < pCount; ++i)
    {
        const Particle& p = m_particles[i];
        int base = i * 4;

        // Quad 4 vertices (UV 0~1)
        v[base + 0] = { p.pos, XMFLOAT2(0.0f, 0.0f), p.color, p.size };
        v[base + 1] = { p.pos, XMFLOAT2(1.0f, 0.0f), p.color, p.size };
        v[base + 2] = { p.pos, XMFLOAT2(1.0f, 1.0f), p.color, p.size };
        v[base + 3] = { p.pos, XMFLOAT2(0.0f, 1.0f), p.color, p.size };
    }

    dc->Unmap(m_vertexBuffer, 0);

    // 2. Matrix Buffer (b0)
    if (SUCCEEDED(dc->Map(m_matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        MatrixBufferType* mb = reinterpret_cast<MatrixBufferType*>(mapped.pData);
        mb->view = XMMatrixTranspose(view);
        mb->proj = XMMatrixTranspose(proj);
        dc->Unmap(m_matrixBuffer, 0);
        dc->VSSetConstantBuffers(0, 1, &m_matrixBuffer);
    }

    // 3. Camera Buffer (b1)
    if (SUCCEEDED(dc->Map(m_cameraBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        CameraBufferType* cb = reinterpret_cast<CameraBufferType*>(mapped.pData);
        cb->cameraPosition = camPos;
        cb->_pad = 0.0f;
        dc->Unmap(m_cameraBuffer, 0);
        dc->VSSetConstantBuffers(1, 1, &m_cameraBuffer);
    }

    // 4. Render States 설정
    float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    dc->OMSetBlendState(m_blendState, blendFactor, 0xffffffff);
    dc->OMSetDepthStencilState(m_depthState, 0);

    // 5. IA 바인딩 & Draw
    unsigned int stride = sizeof(VertexType);
    unsigned int offset = 0;
    dc->IASetInputLayout(m_layout);
    dc->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
    dc->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);
    dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    dc->VSSetShader(m_vertexShader, nullptr, 0);
    dc->PSSetShader(m_pixelShader, nullptr, 0);

    dc->DrawIndexed(pCount * 6, 0, 0);

    // 6. Render States 복원
    dc->OMSetBlendState(nullptr, blendFactor, 0xffffffff);
    dc->OMSetDepthStencilState(nullptr, 0);
}