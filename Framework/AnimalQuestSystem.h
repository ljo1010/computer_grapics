////////////////////////////////////////////////////////////////////////////////
// Filename: AnimalQuestSystem.h
// Description: 동물 먹이주기 퀘스트 및 상태 머신, 건초 탄약/리스폰 관리자
////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <vector>
#include <string>
#include <DirectXMath.h>
#include "ProjectileSystem.h"

// 동물 종류 및 상태 구조체
struct AnimalTarget
{
    enum Type { HORSE, PIG, CHICKEN, GOAT };

    Type type;
    std::string name;       // 동물 이름 (말, 돼지, 닭, 염소)
    size_t fbxIndex;        // FBX 씬 인덱스 (Horse: 2, Pig: 3, Chicken: 4, Goat: 5)
    DirectX::XMFLOAT3 basePos;    // 원본 위치
    DirectX::XMFLOAT3 currentPos; // 뜀뛰기 애니메이션이 반영된 실시간 위치
    float collisionRadius;  // 건초 피격 및 인터랙션 반경
    bool isFed;             // 먹이를 먹었는지 여부 (true: 행복, false: 배고픔)
    float hopTimer;         // 먹이 먹었을 때 신나게 뛰는 타이머
    float hopOffset;        // Y축 점프 높이
    float rotationOffset;   // 회전 각도 (뜀뛰기 시 좌우 흔들림)
};

class AnimalQuestSystem
{
public:
    AnimalQuestSystem();
    ~AnimalQuestSystem();

    void Initialize();
    void Update(float dt, const DirectX::XMFLOAT3& playerPos, std::vector<HayProjectile>& projs);

    // 플레이어 직접 상호작용 (E키로 가까이 있는 동물에게 먹이 주기)
    bool TryFeedNearAnimal(const DirectX::XMFLOAT3& playerPos, std::string& outFedAnimalName);

    // 건초 줍기 처리
    bool TryPickupHay(const DirectX::XMFLOAT3& playerPos);

    // 건초 던지기 (탄약 1개 소모)
    bool CanShoot() const { return m_ammoCount > 0; }
    bool ConsumeAmmo();

    // 퀘스트 재시작 (R키)
    void ResetQuest();

    // Getter
    int GetAmmo() const { return m_ammoCount; }
    int GetMaxAmmo() const { return m_maxAmmo; }
    int GetFedCount() const { return m_fedCount; }
    int GetTotalCount() const { return (int)m_animals.size(); }
    bool IsCompleted() const { return m_isCompleted; }
    bool IsHayAvailable() const { return m_hayAvailable; }
    DirectX::XMFLOAT3 GetHayPosition() const { return m_hayPos; }

    const std::vector<AnimalTarget>& GetAnimals() const { return m_animals; }
    const AnimalTarget* GetAnimalByType(AnimalTarget::Type type) const;

    // 가장 가까운 동물과의 거리 및 힌트 텍스트
    bool GetNearestAnimalPrompt(const DirectX::XMFLOAT3& playerPos, std::string& outPrompt);

private:
    std::vector<AnimalTarget> m_animals;

    // 건초 탄약 및 리스폰
    int m_ammoCount = 1;         // 시작 시 1개 기본 지급
    int m_maxAmmo = 5;           // 최대 5개 보유 가능
    DirectX::XMFLOAT3 m_hayPos = DirectX::XMFLOAT3(-7.0f, 0.0f, 18.0f);
    bool m_hayAvailable = true;
    float m_hayRespawnTimer = 0.0f;
    const float HAY_RESPAWN_TIME = 2.5f; // 2.5초 후 건초 리스폰

    // 퀘스트 진행 상태
    int m_fedCount = 0;
    bool m_isCompleted = false;
    float m_completeCelebrationTimer = 0.0f;
};