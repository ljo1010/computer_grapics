#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>

#include "textureclass.h"

// Assimp 헤더
#include "include/assimp/Importer.hpp"
#include "include/assimp/scene.h"
#include "include/assimp/postprocess.h"

#pragma comment(lib, "lib/assimp-vc140-mt.lib")

using namespace DirectX;

class FbxModelClass
{
public:
    FbxModelClass();
    ~FbxModelClass();

    bool Initialize(ID3D11Device* device,
        const wchar_t* fbxFilename,
        const wchar_t* textureFilename);
    void Shutdown();

    // 기존: 단일 모델 렌더
    void Render(ID3D11DeviceContext* deviceContext);

    // ====== 인스턴싱용 추가 API ======
    bool InitializeInstanceBuffer(
        ID3D11Device* device,
        const std::vector<XMFLOAT3>& instancePositions);

    void RenderInstanced(ID3D11DeviceContext* deviceContext);

    int  GetIndexCount() const { return m_indexCount; }
    int  GetInstanceCount() const { return m_instanceCount; }

    ID3D11ShaderResourceView* GetTexture()
    {
        return m_Texture ? m_Texture->GetTexture() : nullptr;
    }

private:
    struct VertexType
    {
        XMFLOAT3 position;
        XMFLOAT2 tex;
        XMFLOAT3 normal;
    };

    struct InstanceType
    {
        XMFLOAT3 instancePos;
    };

    bool InitializeBuffers(ID3D11Device* device, const aiMesh* mesh);
    void ShutdownBuffers();
    void RenderBuffers(ID3D11DeviceContext* deviceContext);

    void RenderInstanceBuffers(ID3D11DeviceContext* deviceContext);

    bool LoadTexture(ID3D11Device* device, const wchar_t* filename);
    void ReleaseTexture();

private:
    ID3D11Buffer* m_vertexBuffer = nullptr;
    ID3D11Buffer* m_indexBuffer = nullptr;
    int           m_vertexCount = 0;
    int           m_indexCount = 0;

    // 인스턴싱용
    ID3D11Buffer* m_instanceBuffer = nullptr;
    int           m_instanceCount = 0;

    TextureClass* m_Texture = nullptr;
};
