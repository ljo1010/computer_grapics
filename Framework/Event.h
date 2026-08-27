#pragma once

#include <DirectXMath.h>
#include <string>

// 동물 먹이주기 성공 이벤트
struct AnimalFedEvent
{
    DirectX::XMFLOAT3 position;
    int animalType;
    std::string animalName;
    bool isAllCompleted; // 4마리 모두 완료 여부
};

// 건초 투사체 발사 이벤트
struct HayThrownEvent
{
};

// 건초 탄약 충돌 이벤트 (지형 또는 프롭 충돌)
struct HayImpactEvent
{
    DirectX::XMFLOAT3 position;
};

// 퀘스트 리셋 이벤트
struct QuestResetEvent
{
};