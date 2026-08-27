////////////////////////////////////////////////////////////////////////////////
// Filename: AnimalQuestSystem.cpp
////////////////////////////////////////////////////////////////////////////////
#include "AnimalQuestSystem.h"
#include "Event.h"
#include "EventBus.h"
#include <cmath>
#include <algorithm>

using namespace DirectX;

AnimalQuestSystem::AnimalQuestSystem()
{
}

AnimalQuestSystem::~AnimalQuestSystem()
{
}

void AnimalQuestSystem::Initialize()
{
    m_animals.clear();

    // 1. 말 (Horse - FBX Index 2)
    AnimalTarget horse{};
    horse.type = AnimalTarget::HORSE;
    horse.name = u8"말 (Horse)";
    horse.fbxIndex = 2;
    horse.basePos = XMFLOAT3(5.0f, 0.0f, 15.0f);
    horse.currentPos = horse.basePos;
    horse.collisionRadius = 2.0f;
    horse.ChangeState(std::make_shared<HungryState>());
    m_animals.push_back(horse);

    // 2. 돼지 (Pig - FBX Index 3)
    AnimalTarget pig{};
    pig.type = AnimalTarget::PIG;
    pig.name = u8"돼지 (Pig)";
    pig.fbxIndex = 3;
    pig.basePos = XMFLOAT3(-4.0f, 0.0f, 8.0f);
    pig.currentPos = pig.basePos;
    pig.collisionRadius = 1.6f;
    pig.ChangeState(std::make_shared<HungryState>());
    m_animals.push_back(pig);

    // 3. 닭 (Chicken - FBX Index 4)
    AnimalTarget chicken{};
    chicken.type = AnimalTarget::CHICKEN;
    chicken.name = u8"닭 (Chicken)";
    chicken.fbxIndex = 4;
    chicken.basePos = XMFLOAT3(3.0f, 0.0f, 6.0f);
    chicken.currentPos = chicken.basePos;
    chicken.collisionRadius = 1.0f;
    chicken.ChangeState(std::make_shared<HungryState>());
    m_animals.push_back(chicken);

    // 4. 염소 (Goat - FBX Index 5)
    AnimalTarget goat{};
    goat.type = AnimalTarget::GOAT;
    goat.name = u8"염소 (Goat)";
    goat.fbxIndex = 5;
    goat.basePos = XMFLOAT3(0.0f, 0.2f, 10.0f);
    goat.currentPos = goat.basePos;
    goat.collisionRadius = 1.3f;
    goat.ChangeState(std::make_shared<HungryState>());
    m_animals.push_back(goat);

    // 상태 초기화 (총 4마리: 말, 돼지, 닭, 염소)
    m_fedCount = 0;
    m_isCompleted = false;
    m_ammoCount = 1;
    m_hayAvailable = true;
    m_hayRespawnTimer = 0.0f;
    m_completeCelebrationTimer = 0.0f;
}

void AnimalQuestSystem::Update(float dt, const DirectX::XMFLOAT3& playerPos, std::vector<HayProjectile>& projs)
{
    // 1. 건초 리스폰 타이머 갱신
    if (!m_hayAvailable)
    {
        m_hayRespawnTimer -= dt;
        if (m_hayRespawnTimer <= 0.0f)
        {
            m_hayAvailable = true;
            m_hayRespawnTimer = 0.0f;
        }
    }

    // 2. [상태 패턴] 동물들의 상태 머신(FSM) 및 애니메이션 모션 업데이트
    for (auto& a : m_animals)
    {
        a.UpdateState(dt);
    }

    // 3. 건초 발사체와 동물 충돌 검사
    for (int pIdx = (int)projs.size() - 1; pIdx >= 0; --pIdx)
    {
        auto& pr = projs[pIdx];
        bool projHit = false;

        for (auto& a : m_animals)
        {
            // 현재 상태에서 먹이를 먹을 수 없는 경우(이미 포만/뜀뛰기 상태) 패스
            if (!a.CanEat()) continue;

            float dx = pr.pos.x - a.basePos.x;
            float dy = pr.pos.y - (a.basePos.y + 0.5f);
            float dz = pr.pos.z - a.basePos.z;
            float distSq = dx * dx + dy * dy + dz * dz;

            if (distSq < (a.collisionRadius * a.collisionRadius))
            {
                // [상태 패턴] 먹이 적중: 신나게 뜀뛰기 상태(HappyHopState)로 전이
                a.ChangeState(std::make_shared<HappyHopState>(2.0f));
                m_fedCount++;
                projHit = true;

                // [옵저버 패턴] 이벤트 발행: 동물 먹이 적중
                bool isCompleted = (m_fedCount >= (int)m_animals.size());
                EventBus::Get().Publish(AnimalFedEvent{ a.basePos, static_cast<int>(a.type), a.name, isCompleted });
                break;
            }
        }

        if (projHit)
        {
            projs.erase(projs.begin() + pIdx);
        }
    }

    // 4. 미션 완료 검사
    if (m_fedCount >= (int)m_animals.size() && !m_isCompleted)
    {
        m_isCompleted = true;
        // [상태 패턴] 미션 완료 시 모든 동물이 일제히 축하 뜀뛰기 상태로 전이
        for (auto& a : m_animals)
        {
            a.ChangeState(std::make_shared<HappyHopState>(3.5f));
        }
    }
}

// 플레이어가 E키로 가까운 동물에게 직접 먹이를 줌
bool AnimalQuestSystem::TryFeedNearAnimal(const DirectX::XMFLOAT3& playerPos, std::string& outFedAnimalName)
{
    if (m_ammoCount <= 0) return false;

    const float FEED_INTERACT_RADIUS = 2.8f;

    for (auto& a : m_animals)
    {
        // 현재 상태에서 먹이를 먹을 수 없는 경우(이미 포만/뜀뛰기 상태) 패스
        if (!a.CanEat()) continue;

        float dx = playerPos.x - a.basePos.x;
        float dy = playerPos.y - a.basePos.y;
        float dz = playerPos.z - a.basePos.z;
        float distSq = dx * dx + dy * dy + dz * dz;

        if (distSq < (FEED_INTERACT_RADIUS * FEED_INTERACT_RADIUS))
        {
            m_ammoCount--;
            // [상태 패턴] 직접 먹이기 성공: 신나게 뜀뛰기 상태(HappyHopState)로 전이
            a.ChangeState(std::make_shared<HappyHopState>(2.5f));
            m_fedCount++;
            outFedAnimalName = a.name;

            if (m_fedCount >= (int)m_animals.size())
            {
                m_isCompleted = true;
                for (auto& other : m_animals)
                {
                    other.ChangeState(std::make_shared<HappyHopState>(3.5f));
                }
            }

            // [옵저버 패턴] 이벤트 발행: 먹이주기 성공
            EventBus::Get().Publish(AnimalFedEvent{ a.basePos, static_cast<int>(a.type), a.name, m_isCompleted });
            return true;
        }
    }

    return false;
}

// 건초 더미에서 건초 줍기
bool AnimalQuestSystem::TryPickupHay(const DirectX::XMFLOAT3& playerPos)
{
    if (!m_hayAvailable) return false;
    if (m_ammoCount >= m_maxAmmo) return false;

    float dx = playerPos.x - m_hayPos.x;
    float dy = playerPos.y - m_hayPos.y;
    float dz = playerPos.z - m_hayPos.z;
    float distSq = dx * dx + dy * dy + dz * dz;
    const float PICKUP_RADIUS = 2.2f;

    if (distSq < (PICKUP_RADIUS * PICKUP_RADIUS))
    {
        m_ammoCount = (m_ammoCount + 1 > m_maxAmmo) ? m_maxAmmo : (m_ammoCount + 1);
        m_hayAvailable = false;
        m_hayRespawnTimer = HAY_RESPAWN_TIME;
        return true;
    }

    return false;
}

bool AnimalQuestSystem::ConsumeAmmo()
{
    if (m_ammoCount > 0)
    {
        m_ammoCount--;
        return true;
    }
    return false;
}

void AnimalQuestSystem::ResetQuest()
{
    for (auto& a : m_animals)
    {
        // [상태 패턴] 배고픔 상태(HungryState)로 초기화
        a.ChangeState(std::make_shared<HungryState>());
    }

    m_fedCount = 0;
    m_isCompleted = false;
    m_ammoCount = 1;
    m_hayAvailable = true;
    m_hayRespawnTimer = 0.0f;

    // [옵저버 패턴] 이벤트 발행: 퀘스트 리셋
    EventBus::Get().Publish(QuestResetEvent{});
}

const AnimalTarget* AnimalQuestSystem::GetAnimalByType(AnimalTarget::Type type) const
{
    for (const auto& a : m_animals)
    {
        if (a.type == type) return &a;
    }
    return nullptr;
}

bool AnimalQuestSystem::GetNearestAnimalPrompt(const DirectX::XMFLOAT3& playerPos, std::string& outPrompt)
{
    float nearestDist = 999.0f;
    const AnimalTarget* nearestAnimal = nullptr;

    for (const auto& a : m_animals)
    {
        // 이미 배부르거나 먹이를 먹을 수 없는 상태면 프롬프트에서 제외
        if (!a.CanEat()) continue;

        float dx = playerPos.x - a.basePos.x;
        float dy = playerPos.y - a.basePos.y;
        float dz = playerPos.z - a.basePos.z;
        float dist = sqrtf(dx * dx + dy * dy + dz * dz);

        if (dist < nearestDist)
        {
            nearestDist = dist;
            nearestAnimal = &a;
        }
    }

    if (nearestAnimal && nearestDist < 3.0f)
    {
        if (m_ammoCount > 0)
        {
            outPrompt = u8"[E 키] " + nearestAnimal->name + u8"에게 건초 먹이기";
        }
        else
        {
            outPrompt = nearestAnimal->name + u8"이(가) 배고파합니다! (건초 더미에서 짚을 획득하세요)";
        }
        return true;
    }

    return false;
}