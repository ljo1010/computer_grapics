// RMModel.cpp
#include "pch.h"
#include "RMModel.h"
#include "SkinModel.h"
#include "ModelLoader.h"
#include "d3dclass.h"
#include <assimp/postprocess.h>   // aiProcess_ 플래그들
#include <atlstr.h>               // CString

// D3DClass 전역 포인터가 있다면 (이름은 프로젝트에 맞게 수정)
extern D3DClass* g_D3D;

SkinModel* RMModel::loadResource(std::wstring fileName, void* param)
{
    unsigned int flag = 0;

    if (!param) {
        flag = aiProcess_ConvertToLeftHanded
            | aiProcess_GenNormals
            | aiProcess_CalcTangentSpace;
    }
    else {
        flag = *reinterpret_cast<unsigned int*>(param);
    }

    ID3D11Device* device = (g_D3D ? g_D3D->GetDevice() : nullptr);
    if (!device) {
        std::ofstream log("assimp_error.log", std::ios::app);
        log << "RMModel::loadResource - device is null\n";
        return nullptr;
    }

    CString path(fileName.c_str());

    ModelLoader loader;
    return loader.LoadModel(path, flag, device);   // ← 여기 3인자 버전 호출 OK
}



// 모델 삭제
void RMModel::releaseResource(SkinModel* data)
{
    delete data;
}
