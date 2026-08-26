// SkinModel.cpp
#include "SkinModel.h"
#include <algorithm>
#include <cassert>
#include <unordered_map>

using namespace DirectX;

SkinModel::SkinModel()
    : _playAniIdx(-1)
    , _aniTime(0.f)
    , m_Texture(nullptr)
{
    // �ִ� �� ����(���̴� �ʰ� ����� ��)
    _boneMatrices.resize(120, XMMatrixIdentity());
}

SkinModel::~SkinModel()
{
    Release();
}

void SkinModel::Release()
{
    if (m_vertexBuffer)
    {
        m_vertexBuffer->Release();
        m_vertexBuffer = nullptr;
    }
    if (m_indexBuffer)
    {
        m_indexBuffer->Release();
        m_indexBuffer = nullptr;
    }

    for (auto* n : _nodeList)
        delete n;
    _nodeList.clear();

    for (auto* m : _meshList)
        delete m;
    _meshList.clear();

    _materialList.clear();
    _aniList.clear();
    _meshByMaterial.clear();

    if (m_Texture)
    {
        m_Texture->Shutdown();
        delete m_Texture;
        m_Texture = nullptr;
    }
}

bool SkinModel::CreateModel(
    ID3D11Device* device,
    const Vertex& srcVertices,
    const std::vector<unsigned long>& indices)
{
    if (!device) return false;
    if (srcVertices.position.empty() || indices.empty()) return false;

    const size_t vCount = srcVertices.position.size();

    // ���� Vertex�� const �̹Ƿ� ���� ���纻���� ������ ����
    Vertex v = srcVertices;

    auto ensureSize3 = [&](std::vector<XMFLOAT3>& a)
        {
            if (a.size() < vCount) a.resize(vCount, XMFLOAT3(0, 0, 0));
        };
    auto ensureSize2 = [&](std::vector<XMFLOAT2>& a)
        {
            if (a.size() < vCount) a.resize(vCount, XMFLOAT2(0, 0));
        };
    auto ensureSize4 = [&](std::vector<XMFLOAT4>& a)
        {
            if (a.size() < vCount) a.resize(vCount, XMFLOAT4(0, 0, 0, 0));
        };
    auto ensureSize4U = [&](std::vector<XMUINT4>& a)
        {
            if (a.size() < vCount) a.resize(vCount, XMUINT4(0, 0, 0, 0));
        };

    ensureSize3(v.normal);
    ensureSize3(v.tangent);
    ensureSize3(v.bitangent);
    ensureSize2(v.uv);
    ensureSize4U(v.boneidx);
    ensureSize4(v.weight);

    std::vector<SkinnedVertexType> vertices(vCount);
    const UINT MAX_BONE = static_cast<UINT>(_boneMatrices.size() - 1);

    for (size_t i = 0; i < vCount; ++i)
    {
        XMFLOAT4& w = v.weight[i];
        XMUINT4& bi = v.boneidx[i];

        float sumW = w.x + w.y + w.z + w.w;

        if (sumW <= 1e-4f)
        {
            // ��Ų �����Ͱ� ���� ���� ����
            bi = XMUINT4(0, 0, 0, 0);
            w = XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f);
        }
        else
        {
            auto clampIdx = [MAX_BONE](UINT& idx, float& ww)
                {
                    if (idx > MAX_BONE)
                    {
                        idx = 0;
                        ww = 0.0f;
                    }
                };
            clampIdx(bi.x, w.x);
            clampIdx(bi.y, w.y);
            clampIdx(bi.z, w.z);
            clampIdx(bi.w, w.w);

            auto killSmall = [](float& ww, UINT& idx)
                {
                    if (ww < 1e-3f)
                    {
                        ww = 0.0f;
                        idx = 0;
                    }
                };
            killSmall(w.x, bi.x);
            killSmall(w.y, bi.y);
            killSmall(w.z, bi.z);
            killSmall(w.w, bi.w);

            float s = w.x + w.y + w.z + w.w;
            if (s <= 1e-4f)
            {
                bi = XMUINT4(0, 0, 0, 0);
                w = XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f);
            }
            else
            {
                float inv = 1.0f / s;
                w.x *= inv; w.y *= inv; w.z *= inv; w.w *= inv;
            }
        }

        vertices[i].position = v.position[i];
        vertices[i].tex = v.uv[i];
        vertices[i].boneIdx = bi;
        vertices[i].weight = w;
    }

    if (m_vertexBuffer)
    {
        m_vertexBuffer->Release();
        m_vertexBuffer = nullptr;
    }
    if (m_indexBuffer)
    {
        m_indexBuffer->Release();
        m_indexBuffer = nullptr;
    }

    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.Usage = D3D11_USAGE_DEFAULT;
    vbDesc.ByteWidth = static_cast<UINT>(sizeof(SkinnedVertexType) * vertices.size());
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vbData = {};
    vbData.pSysMem = vertices.data();

    HRESULT hr = device->CreateBuffer(&vbDesc, &vbData, &m_vertexBuffer);
    if (FAILED(hr))
        return false;

    D3D11_BUFFER_DESC ibDesc = {};
    ibDesc.Usage = D3D11_USAGE_DEFAULT;
    ibDesc.ByteWidth = static_cast<UINT>(sizeof(unsigned long) * indices.size());
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA ibData = {};
    ibData.pSysMem = indices.data();

    hr = device->CreateBuffer(&ibDesc, &ibData, &m_indexBuffer);
    if (FAILED(hr))
        return false;

    m_indexCount = static_cast<UINT>(indices.size());

    return true;
}

bool SkinModel::LoadTexture(ID3D11Device* device, const WCHAR* filename)
{
    if (m_Texture)
    {
        m_Texture->Shutdown();
        delete m_Texture;
        m_Texture = nullptr;
    }

    m_Texture = new TextureClass;
    if (!m_Texture) return false;

    bool result = m_Texture->Initialize(device, filename);
    if (!result)
    {
        delete m_Texture;
        m_Texture = nullptr;
        return false;
    }

    return true;
}

ID3D11ShaderResourceView* SkinModel::GetTexture()
{
    if (!m_Texture) return nullptr;
    return m_Texture->GetTexture();
}

bool SkinModel::LoadFromFbx(
    ID3D11Device* device,
    const WCHAR* /*modelFile*/,
    const WCHAR* texFile)
{
    if (!texFile) return true;
    return LoadTexture(device, texFile);
}

bool SkinModel::InitializeSkinBuffer(ID3D11Device* /*device*/)
{
    return true;
}

void SkinModel::PlayAni(int idx)
{
    if (idx >= 0 && idx < static_cast<int>(_aniList.size()))
    {
        _playAniIdx = idx;
        _aniList[idx].Stop();
        _aniList[idx].SetRepeat(true);
        _aniList[idx].Play();
    }
    else
    {
        _playAniIdx = -1;
    }
}

void SkinModel::StopAni()
{
    if (_playAniIdx >= 0 && _playAniIdx < static_cast<int>(_aniList.size()))
        _aniList[_playAniIdx].Stop();
    _playAniIdx = -1;
}

void SkinModel::PauseAni()
{
    if (_playAniIdx >= 0 && _playAniIdx < static_cast<int>(_aniList.size()))
        _aniList[_playAniIdx].Pause();
}

void SkinModel::Update(float dt)
{
    if (_playAniIdx >= 0 && _playAniIdx < static_cast<int>(_aniList.size()))
    {
        Animation& ani = _aniList[_playAniIdx];
        if (ani.isPlaying())
            ani.UpdateAnimation(dt);
    }

    // �� ȣ��
    UpdateNodeTM(dt);
}


// SkinModel.cpp - UpdateNodeTM()
void SkinModel::UpdateNodeTM(float /*dt*/)
{
    std::fill(_boneMatrices.begin(), _boneMatrices.end(), XMMatrixIdentity());

    if (_playAniIdx < 0 || _playAniIdx >= static_cast<int>(_aniList.size()))
        return;

    Animation& ani = _aniList[_playAniIdx];
    auto& aniNodes = ani.GetAniNodeList();

    // 1단계: 노드 월드 TM 업데이트
    for (NodeInfo* node : _nodeList)
    {
        if (!node) continue;

        XMMATRIX localTM = node->localTM;

        for (auto& an : aniNodes)
        {
            if (an.name == node->name)
            {
                localTM = an.aniTM;
                break;
            }
        }

        if (node->parent)
            node->worldTM = localTM * node->parent->worldTM;
        else
            node->worldTM = localTM;
    }

    // 2단계: 본 행렬 계산
    XMMATRIX scaleFix = XMMatrixScaling(_boneScaleFix, _boneScaleFix, _boneScaleFix);

    for (HierarchyMesh* mesh : _meshList)
    {
        if (!mesh) continue;

        for (const BoneInfo& bone : mesh->boneList)
        {
            NodeInfo* node = bone.linkNode;
            if (!node) continue;

            int boneIndex = bone.skinIndex;
            if (boneIndex < 0 || boneIndex >= static_cast<int>(_boneMatrices.size()))
                continue;

            XMMATRIX finalTM = bone.matOffset * node->worldTM * scaleFix;
            _boneMatrices[boneIndex] = finalTM;
        }
    }
}





void SkinModel::UpdateMeshByMaterial()
{
    _meshByMaterial.clear();
}

void SkinModel::Render(ID3D11DeviceContext* deviceContext)
{
    UINT stride = sizeof(SkinnedVertexType);
    UINT offset = 0;

    deviceContext->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
    deviceContext->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void SkinModel::RenderSkinned(
    ID3D11DeviceContext* dc,
    SkinShaderClass* skinShader,
    const XMMATRIX& world,
    const XMMATRIX& view,
    const XMMATRIX& proj,
    ID3D11ShaderResourceView* diffuseTex)
{
    if (!dc || !skinShader) return;
    if (!m_vertexBuffer || !m_indexBuffer) return;

    UINT stride = sizeof(SkinnedVertexType);
    UINT offset = 0;

    dc->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
    dc->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);
    dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    if (diffuseTex)
        dc->PSSetShaderResources(0, 1, &diffuseTex);

    skinShader->RenderMesh(
        dc,
        m_indexCount,
        0,
        world,
        view,
        proj,
        _boneMatrices,
        GetTexture()
    );
}
