// ModelLoader.cpp
#include "ModelLoader.h"

#include <windows.h>
#include <fstream>
#include <string>
#include <algorithm>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <DirectXMath.h>

#include "SkinModel.h"
#include "ModelStructure.h"   // Vertex, HierarchyMesh, NodeInfo, Material, BoneInfo, AniNode, KeyFrame
#include "Animation.h"

// MYUTIL / RM_TEXTURE �� �� ������Ʈ���� ���� ��ƿ/���ҽ� ��ũ�� ���
#include "MYUtil.h"
#include "RMTexture.h"   // ����: RM_TEXTURE �� ����� ��� (�̸��� �� ������Ʈ�� �����)
#include "d3dclass.h"
#include <unordered_map>

using namespace DirectX;
using std::vector;
using std::wstring;
using std::string;
extern D3DClass* g_D3D;
extern std::string CStringToUtf8(const CString&);

// CString -> UTF-8 std::string
static std::string CStringToUtf8(const CString& w)
{
    int wlen = w.GetLength();
    if (wlen == 0) return std::string();

    int size = ::WideCharToMultiByte(
        CP_UTF8, 0,
        w, wlen,
        nullptr, 0,
        nullptr, nullptr
    );

    std::string ret(size, 0);
    ::WideCharToMultiByte(
        CP_UTF8, 0,
        w, wlen,
        &ret[0], size,
        nullptr, nullptr
    );

    return ret;
}

// ====================================================================
//  LoadModel(CString path, UINT flag)
//  - ���(ModelLoader.h) ����� ��Ȯ�� ��ġ�ؾ� ��
// ====================================================================
SkinModel* ModelLoader::LoadModel(CString path, UINT flag, ID3D11Device* device)
{
    // 0) ����̽� üũ
    if (!device)
    {
        std::ofstream log("assimp_error.log", std::ios::app);
        log << "[LoadModel] device is null. path = "
            << CStringToUtf8(path) << "\n";
        return nullptr;
    }

    Assimp::Importer importer;

    // 1) ��θ� UTF-8�� ��ȯ�ؼ� Assimp�� ����
    std::string strPath = CStringToUtf8(path);

    const aiScene* pScene = importer.ReadFile(strPath, flag);
    if (!pScene)
    {
        std::ofstream log("assimp_error.log");
        log << "[LoadModel] ReadFile failed. path = " << strPath << "\n";
        log << "Assimp error = " << importer.GetErrorString() << "\n";
        return nullptr;
    }

    // 2) ������ ���� �����̳�
    Vertex                    vertices;
    std::vector<unsigned long> indices;
    SkinModel* model = new SkinModel;

    // �� �̸�
    std::wstring modelName = MYUTIL::ConvertToWString(path);
    model->SetName(modelName);

    // 3) �޽� ����
    for (UINT i = 0; i < pScene->mNumMeshes; ++i)
        ProcessMesh(pScene->mMeshes[i], vertices, indices, model->GetMeshList());

    // 4) ����
    ProcessMaterial(pScene, model->GetMaterialList(), MYUTIL::GetDirectoryPath(path));

    // 5) ����(��)
    ProcessNode(pScene->mRootNode, model);
    std::sort(
        model->GetNodeList().begin(),
        model->GetNodeList().end(),
        [](const NodeInfo* a, const NodeInfo* b) { return a->depth < b->depth; }
    );

    std::unordered_map<std::wstring, int> boneIndexMap;
    int nextBoneIndex = 0;

    for (UINT i = 0; i < pScene->mNumMeshes; ++i)
    {
        aiMesh* aimesh = pScene->mMeshes[i];
        if (!aimesh->HasBones())
            continue;

        HierarchyMesh* mesh = (HierarchyMesh*)model->GetMeshList()[i];
        ProcessSkin(aimesh, mesh, vertices, indices, model,
            boneIndexMap, nextBoneIndex);
    }

    // �ִϸ��̼�
    if (pScene->HasAnimations())
        ProcessAnimation(pScene, model);

    // �� ���⼭ �ݵ�� ȣ��
    if (!model->CreateModel(device, vertices, indices))
    {
        std::ofstream log("assimp_error.log", std::ios::app);
        log << "CreateModel failed: " << strPath << "\n";
        log << "vtx = " << vertices.position.size()
            << ", idx = " << indices.size() << "\n";
        delete model;
        return nullptr;
    }

    model->UpdateMeshByMaterial();
    return model;
}



// ====================================================================
//  LoadAnimation
// ====================================================================
void ModelLoader::LoadAnimation(CString path, SkinModel* model, UINT flag)
{
    Assimp::Importer importer;
    std::string strPath = CStringToUtf8(path);

    const aiScene* pScene = importer.ReadFile(strPath, flag);
    if (!pScene) return;

    if (pScene->HasAnimations())
        ProcessAnimation(pScene, model);
}

// ====================================================================
//  ProcessNode
// ====================================================================
void ModelLoader::ProcessNode(aiNode* aiNodeInfo,
    SkinModel* skModel,
    NodeInfo* parent,
    int depth)
{
    // �������� ���� ���� �� �߰�
    CString tmp = (CString)aiNodeInfo->mName.C_Str();
    wstring nodeName = MYUTIL::ConvertToWString(tmp);
    XMMATRIX tm = XMMatrixTranspose(XMMATRIX(aiNodeInfo->mTransformation[0]));

    NodeInfo* node = new NodeInfo(parent, nodeName, tm, depth);
    skModel->GetNodeList().emplace_back(node);

    // �޽��� ����� ���̸� ��������
    if (aiNodeInfo->mNumMeshes > 0)
    {
        HierarchyMesh* hiMesh = (HierarchyMesh*)skModel->GetMeshList()[aiNodeInfo->mMeshes[0]];
        hiMesh->linkNode = node;
    }

    // ���� ��� Ž��
    for (UINT i = 0; i < aiNodeInfo->mNumChildren; i++)
    {
        ProcessNode(aiNodeInfo->mChildren[i], skModel, node, depth + 1);
    }
}

// ====================================================================
//  ProcessMesh
// ====================================================================
void ModelLoader::ProcessMesh(aiMesh* mesh,
    Vertex& vertices,
    std::vector<unsigned long>& indices,
    std::vector<HierarchyMesh*>& meshList)
{
    UINT startIdx = (UINT)indices.size();
    UINT startVert = (UINT)vertices.position.size();

    // ���� ���� ����
    for (UINT i = 0; i < mesh->mNumVertices; i++)
    {
        XMFLOAT3 position(0, 0, 0);
        XMFLOAT3 normal(0, 1, 0);
        XMFLOAT3 bitangent(0, 0, 0);
        XMFLOAT3 tangent(0, 0, 0);
        XMFLOAT2 uv(0, 0);

        // Position
        if (mesh->mVertices)
            memcpy_s(&position, sizeof(position),
                &mesh->mVertices[i], sizeof(mesh->mVertices[i]));

        // Normal
        if (mesh->mNormals)
            memcpy_s(&normal, sizeof(normal),
                &mesh->mNormals[i], sizeof(mesh->mNormals[i]));

        // Bitangent
        if (mesh->mBitangents)
            memcpy_s(&bitangent, sizeof(bitangent),
                &mesh->mBitangents[i], sizeof(mesh->mBitangents[i]));

        // Tangent
        if (mesh->mTangents)
            memcpy_s(&tangent, sizeof(tangent),
                &mesh->mTangents[i], sizeof(mesh->mTangents[i]));

        // UV
        if (mesh->mTextureCoords[0])
        {
            uv.x = (float)mesh->mTextureCoords[0][i].x;
            uv.y = (float)mesh->mTextureCoords[0][i].y;
        }

        // �׻� ���� ������ push
        vertices.position.emplace_back(position);
        vertices.normal.emplace_back(normal);
        vertices.bitangent.emplace_back(bitangent);
        vertices.tangent.emplace_back(tangent);
        vertices.uv.emplace_back(uv);
    }

    // �ε��� ����
    for (UINT i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for (UINT j = 0; j < face.mNumIndices; j++)
        {
            indices.emplace_back(face.mIndices[j] + startVert);
        }
    }

    // �޽� ����
    std::wstring meshName = (std::wstring)((CString)mesh->mName.C_Str()).TrimLeft();
    meshList.emplace_back(new HierarchyMesh(
        meshName,
        startIdx,
        (int)indices.size() - startIdx,
        mesh->mMaterialIndex,
        startVert));
}

// ====================================================================
//  ProcessMaterial
// ====================================================================
void ModelLoader::ProcessMaterial(
    const aiScene* pScene,
    std::vector<Material>& matList,
    CString directoryPath)
{
    for (unsigned int i = 0; i < pScene->mNumMaterials; i++)
    {
        Material newMat;

        if (pScene->mMaterials[i] != nullptr)
        {
            aiString texture_path[3];

            // Diffuse
            pScene->mMaterials[i]->GetTexture(aiTextureType_DIFFUSE, 0, &texture_path[0], NULL, NULL, NULL, NULL, NULL);
            CString tmp = (CString)texture_path[0].C_Str();
            wstring texPath = MYUTIL::ConvertToWString(directoryPath) + MYUTIL::ConvertToWString(tmp);
            newMat.diffuseMap = RM_TEXTURE.AddResource(texPath);

            // Alpha
            pScene->mMaterials[i]->GetTexture(aiTextureType_OPACITY, 0, &texture_path[1], NULL, NULL, NULL, NULL, NULL);
            texPath = directoryPath + MYUTIL::getFileName((CString)texture_path[1].C_Str());
            newMat.alphaMap = RM_TEXTURE.AddResource(texPath);

            // Normal
            pScene->mMaterials[i]->GetTexture(aiTextureType_HEIGHT, 0, &texture_path[2], NULL, NULL, NULL, NULL, NULL);
            texPath = directoryPath + MYUTIL::getFileName((CString)texture_path[2].C_Str());
            newMat.normalMap = RM_TEXTURE.AddResource(texPath);
        }

        matList.emplace_back(newMat);
    }
}

// ====================================================================
//  ProcessSkin
// ====================================================================
void ModelLoader::ProcessSkin(aiMesh* aiMesh,
    HierarchyMesh* mesh,
    Vertex& vertices,
    std::vector<unsigned long>& indices,
    SkinModel* skModel,
    std::unordered_map<std::wstring, int>& boneIndexMap,
    int& nextBoneIndex)
{
    auto  node = skModel->GetNodeList();
    auto& listBoneId = vertices.boneidx;
    auto& listWeight = vertices.weight;

    // ���� ������ŭ ��/����Ʈ �迭 Ȯ��
    if (listBoneId.size() < vertices.position.size())
    {
        listBoneId.resize(vertices.position.size(), XMUINT4(0, 0, 0, 0));
        listWeight.resize(vertices.position.size(), XMFLOAT4(0.f, 0.f, 0.f, 0.f));
    }

    // ��Ű�� ���� ����
    for (UINT i = 0; i < aiMesh->mNumBones; i++)
    {
        aiBone* aibone = aiMesh->mBones[i];

        // �̸��� wstring ����
        CString tmp = (CString)aibone->mName.C_Str();
        std::wstring boneName = MYUTIL::ConvertToWString(tmp);

        // �� �۷ι� �� �ε��� ����
        int skinIndex = -1;
        auto it = boneIndexMap.find(boneName);
        if (it == boneIndexMap.end())
        {
            skinIndex = nextBoneIndex++;
            boneIndexMap[boneName] = skinIndex;
        }
        else
        {
            skinIndex = it->second;
        }

        // ������/��� ����
        BoneInfo bone;
        bone.matOffset = XMMatrixTranspose(XMMATRIX(aibone->mOffsetMatrix[0]));
        bone.linkNode = FindNode(aibone->mName, node);
        bone.skinIndex = skinIndex;                // �� ���⿡ ����
        mesh->boneList.emplace_back(bone);

        // ����ġ ����
        for (UINT j = 0; j < aibone->mNumWeights; j++)
        {
            UINT  vertId = aibone->mWeights[j].mVertexId + mesh->startVert;
            float weight = aibone->mWeights[j].mWeight;

            XMUINT4& bi = listBoneId[vertId];
            XMFLOAT4& bw = listWeight[vertId];

            if (bw.x == 0.0f)
            {
                bi.x = skinIndex;
                bw.x = weight;
            }
            else if (bw.y == 0.0f)
            {
                bi.y = skinIndex;
                bw.y = weight;
            }
            else if (bw.z == 0.0f)
            {
                bi.z = skinIndex;
                bw.z = weight;
            }
            else if (bw.w == 0.0f)
            {
                bi.w = skinIndex;
                bw.w = weight;
            }
        }
    }
}

// ====================================================================
//  ProcessAnimation
// ====================================================================
void ModelLoader::ProcessAnimation(const aiScene* pScene, SkinModel* skModel)
{
    for (UINT i = 0; i < pScene->mNumAnimations; i++)
    {
        auto aiAni = pScene->mAnimations[i];

        float lastTime = 0.f;
        Animation aniInfo;
        aniInfo.SetDuration((float)aiAni->mDuration);
        aniInfo.SetTickPerSecond((float)aiAni->mTicksPerSecond);

        CString aniName = (CString)aiAni->mName.C_Str();
        aniName = aniName.TrimLeft();

        if (aniName != L"")
            aniInfo.SetName((wstring)aniName);

        // ����� ��� ��ŭ...
        for (UINT j = 0; j < aiAni->mNumChannels; j++)
        {
            auto aiAniNode = aiAni->mChannels[j];

            AniNode aniNodeInfo;
            CString tmp = (CString)aiAniNode->mNodeName.C_Str();
            aniNodeInfo.name = MYUTIL::ConvertToWString(tmp);

            UINT keyCnt = max(aiAniNode->mNumPositionKeys, aiAniNode->mNumRotationKeys);
            keyCnt = max(keyCnt, aiAniNode->mNumScalingKeys);

            XMFLOAT3 translation = XMFLOAT3(0.f, 0.f, 0.f);
            XMFLOAT3 scale = XMFLOAT3(0.f, 0.f, 0.f);
            XMFLOAT4 rotation = XMFLOAT4(0.f, 0.f, 0.f, 0.f);
            float    time = 0.f;

            for (UINT k = 0; k < keyCnt; k++)
            {
                if (aiAniNode->mNumPositionKeys > k)
                {
                    auto posKey = aiAniNode->mPositionKeys[k];
                    memcpy_s(&translation, sizeof(translation), &posKey.mValue, sizeof(posKey.mValue));
                    time = (float)aiAniNode->mPositionKeys[k].mTime;
                }

                if (aiAniNode->mNumRotationKeys > k)
                {
                    auto rotKey = aiAniNode->mRotationKeys[k];
                    rotation = XMFLOAT4(
                        rotKey.mValue.x,
                        rotKey.mValue.y,
                        rotKey.mValue.z,
                        rotKey.mValue.w);
                    time = (float)aiAniNode->mRotationKeys[k].mTime;
                }

                if (aiAniNode->mNumScalingKeys > k)
                {
                    auto scaleKey = aiAniNode->mScalingKeys[k];
                    memcpy_s(&scale, sizeof(scale), &scaleKey.mValue, sizeof(scaleKey.mValue));
                    time = (float)aiAniNode->mScalingKeys[k].mTime;
                }

                aniNodeInfo.keyFrame.emplace_back(
                    KeyFrame{ time, translation, rotation, scale });
            }

            lastTime = max(aniNodeInfo.keyFrame.back().timePos, lastTime);
            aniInfo.GetAniNodeList().emplace_back(aniNodeInfo);
        }

        aniInfo.SetLastFrame(lastTime);
        skModel->GetAnimationList().emplace_back(aniInfo);
    }
}

// ====================================================================
//  FindNode
// ====================================================================
NodeInfo* ModelLoader::FindNode(aiString name, std::vector<NodeInfo*> nodeList)
{
    NodeInfo* findNode = nullptr;
    CString tmp = (CString)name.C_Str();
    wstring bName = MYUTIL::ConvertToWString(tmp);

    auto nodeInfo = std::find_if(
        nodeList.begin(),
        nodeList.end(),
        [bName](const NodeInfo* a)->bool { return a->name == bName; });

    if (nodeInfo != nodeList.end())
        findNode = *nodeInfo;

    return findNode;
}
