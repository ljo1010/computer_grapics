// RMModel.h
#pragma once

#include <string>

class SkinModel;

// 단순 리소스 매니저 래퍼 (static 함수만 사용하는 버전)
class RMModel
{
public:
    static SkinModel* loadResource(std::wstring fileName, void* param);
    static void       releaseResource(SkinModel* data);
};
