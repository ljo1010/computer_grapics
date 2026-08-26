////////////////////////////////////////////////////////////////////////////////
// Filename: AnimalQuestSystem.cpp
////////////////////////////////////////////////////////////////////////////////
#include "AnimalQuestSystem.h"
#include "ParticleSystem.h"
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
    horse.isFed = false;
    horse.hopTimer = 0.0f;
    horse.hopOffset = 0.0f;
    horse.rotationOffset = 0.0f;
    m_animals.push_back(horse);

    // 2. 돼지 (Pig - FBX Index 3)
    AnimalTarget pig{};
    pig.type = AnimalTarget::PIG;
    pig.name = u8"돼지 (Pig)";
    pig.fbxIndex = 3;
    pig.basePos = XMFLOAT3(-4.0f, 0.0f, 8.0f);
    pig.currentPos = pig.basePos;
    pig.collisionRadius = 1.6f;
    pig.isFed = false;
    pig.hopTimer = 0.0f;
    pig.hopOffset = 0.0f;
    pig.rotationOffset = 0.0f;
    m_animals.push_back(pig);

    // 3. 닭 (Chicken - FBX Index 4)
    AnimalTarget chicken{};
    chicken.type = AnimalTarget::CHICKEN;
    chicken.name = u8"닭 (Chicken)";
    chicken.fbxIndex = 4;
    chicken.basePos = XMFLOAT3(3.0f, 0.0f, 6.0f);
    chicken.currentPos = chicken.basePos;
    chicken.collisionRadius = 1.0f;
    chicken.isFed = false;
    chicken.hopTimer = 0.0f;
    chicken.hopOffset = 0.0f;
    chicken.rotationOffset = 0.0f;
    m_animals.push_back(chicken);

    // 4. 염소 (Goat - FBX Index 5)
    AnimalTarget goat{};
    goat.type = AnimalTarget::GOAT;
    goat.name = u8"염소 (Goat)";
    goat.fbxIndex = 5;
    goat.basePos = XMFLOAT3(0.0f, 0.2f, 10.0f);
    goat.currentPos = goat.basePos;
    goat.collisionRadius = 1.3f;
    goat.isFed = false;
    goat.hopTimer = 0.0f;
    goat.hopOffset = 0.0f;
    goat.rotationOffset = 0.0f;
    m_animals.push_back(goat);

    // 상태 초기화 (총 4마리: 말, 돼지, 닭, 염소)
    m_fedCount = 0;
    m_isCompleted = false;
    m_ammoCount = 1;
    m_hayAvailable = true;
    m_hayRespawnTimer = 0.0f;
    m_completeCelebrationTimer = 0.0f;
}

void AnimalQuestSystem::Update(float dt, const DirectX::XMFLOAT3& playerPos, std::vector<HayProjectile>& projs, ParticleSystem* particleSys)
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

    // 2. 동물들의 뜀뛰기(Hop) 및 축하 반응 애니메이션 업데이트
    for (auto& a : m_animals)
    {
        if (a.hopTimer > 0.0f)
        {
            a.hopTimer -= dt;
            if (a.hopTimer < 0.0f) a.hopTimer = 0.0f;

            // 통통 튀는 바운스 모션 (Sine wave)
            float bounce = fabsf(sinf(a.hopTimer * 10.0f)) * 0.6f;
            a.hopOffset = bounce;
            a.currentPos.y = a.basePos.y + bounce;
            a.rotationOffset = sinf(a.hopTimer * 12.0f) * 0.15f;
        }
        else
        {
            a.hopOffset = 0.0f;
            a.currentPos.y = a.basePos.y;
            a.rotationOffset = 0.0f;
        }
    }

    // 3. 건초 발사체와 동물 충돌 검사
    for (int pIdx = (int)projs.size() - 1; pIdx >= 0; --pIdx)
    {
        auto& pr = projs[pIdx];
        bool projHit = false;

        for (auto& a : m_animals)
        {
            if (a.isFed) continue; // 이미 먹은 동물은 패스

            float dx = pr.pos.x - a.basePos.x;
            float dy = pr.pos.y - (a.basePos.y + 0.5f);
            float dz = pr.pos.z - a.basePos.z;
            float distSq = dx * dx + dy * dy + dz * dz;

            if (distSq < (a.collisionRadius * a.collisionRadius))
            {
                // 먹이 적중!
                a.isFed = true;
                a.hopTimer = 2.0f; // 2초간 신나게 뜀뛰기
                m_fedCount++;
                projHit = true;

                // 머리 위 반짝이는 파티클 팝업!
                if (particleSys)
                {
                    particleSys->SpawnFeedParticles(a.basePos, 24);
                }
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
        // 미션 완료 시 모든 동물이 일제히 축하 뜀뛰기 및 대량 파티클 분출!
        for (auto& a : m_animals)
        {
            a.hopTimer = 3.5f;
            if (particleSys)
            {
                particleSys->SpawnFeedParticles(a.basePos, 32);
            }
        }
    }
}

// 플레이어가 E키로 가까운 동물에게 직접 먹이를 줌
bool AnimalQuestSystem::TryFeedNearAnimal(const DirectX::XMFLOAT3& playerPos, std::string& outFedAnimalName, ParticleSystem* particleSys)
{
    if (m_ammoCount <= 0) return false;

    const float FEED_INTERACT_RADIUS = 2.8f;

    for (auto& a : m_animals)
    {
        if (a.isFed) continue;

        float dx = playerPos.x - a.basePos.x;
        float dy = playerPos.y - a.basePos.y;
        float dz = playerPos.z - a.basePos.z;
        float distSq = dx * dx + dy * dy + dz * dz;

        if (distSq < (FEED_INTERACT_RADIUS * FEED_INTERACT_RADIUS))
        {
            m_ammoCount--;
            a.isFed = true;
            a.hopTimer = 2.5f;
            m_fedCount++;
            outFedAnimalName = a.name;

            // 반짝이는 파티클 방출!
            if (particleSys)
            {
                particleSys->SpawnFeedParticles(a.basePos, 28);
            }

            if (m_fedCount >= (int)m_animals.size())
            {
                m_isCompleted = true;
                for (auto& other : m_animals)
                {
                    other.hopTimer = 3.5f;
                    if (particleSys)
                    {
                        particleSys->SpawnFeedParticles(other.basePos, 32);
                    }
                }
            }
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
        a.isFed = false;
        a.hopTimer = 0.0f;
        a.hopOffset = 0.0f;
        a.rotationOffset = 0.0f;
        a.currentPos = a.basePos;
    }

    m_fedCount = 0;
    m_isCompleted = false;
    m_ammoCount = 1;
    m_hayAvailable = true;
    m_hayRespawnTimer = 0.0f;
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
        if (a.isFed) continue;

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