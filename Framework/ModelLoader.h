// ModelLoader.h
#pragma once

#include <vector>
#include <string>
#include <unordered_map>

#include <assimp/scene.h>

#include "ModelStructure.h"
#include "SkinModel.h"

class ModelLoader
{
public:
    static SkinModel* LoadModel(CString path, UINT flag, ID3D11Device* device);
    static void       LoadAnimation(CString path, SkinModel* model, UINT flag);

private:
    static void ProcessNode(
        aiNode* aiNodeInfo,
        SkinModel* skModel,
        NodeInfo* parent = nullptr,
        int depth = 0);

    static void ProcessMesh(
        aiMesh* mesh,
        Vertex& vertices,
        std::vector<unsigned long>& indices,
        std::vector<HierarchyMesh*>& meshList);

    static void ProcessMaterial(
        const aiScene* pScene,
        std::vector<Material>& matList,
        CString directoryPath);

    static void ProcessSkin(
        aiMesh* aiMesh,
        HierarchyMesh* mesh,
        Vertex& vertices,
        std::vector<unsigned long>& indices,
        SkinModel* skModel,
        std::unordered_map<std::wstring, int>& boneIndexMap,
        int& nextBoneIndex);

    static void ProcessAnimation(
        const aiScene* pScene,
        SkinModel* skModel);

    static NodeInfo* FindNode(aiString name, std::vector<NodeInfo*> nodeList);
};
