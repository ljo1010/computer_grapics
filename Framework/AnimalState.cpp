#include "AnimalState.h"
#include "AnimalQuestSystem.h"

// ============================================================================
// HungryState: 배고픔 상태
// ============================================================================
void HungryState::Enter(AnimalTarget& animal)
{
    animal.currentPos = animal.basePos;
    animal.hopOffset = 0.0f;
    animal.rotationOffset = 0.0f;
    m_breathTimer = 0.0f;
}

void HungryState::Update(AnimalTarget& animal, float dt)
{
    m_breathTimer += dt;
    // 배고픈 상태에서 살아 숨쉬는 듯한 미세한 상하 호흡 모션
    animal.currentPos.y = animal.basePos.y + sinf(m_breathTimer * 2.5f) * 0.04f;
    animal.hopOffset = 0.0f;
    animal.rotationOffset = 0.0f;
}

// ============================================================================
// HappyHopState: 먹이를 먹고 신나게 뜀뛰기하는 바운스 상태
// ============================================================================
void HappyHopState::Enter(AnimalTarget& animal)
{
    m_timer = m_duration;
}

void HappyHopState::Update(AnimalTarget& animal, float dt)
{
    m_timer -= dt;

    if (m_timer <= 0.0f)
    {
        // 뜀뛰기 시간이 끝나면 포만/만족 상태(SatisfiedState)로 전이
        animal.ChangeState(std::make_shared<SatisfiedState>());
        return;
    }

    // 통통 튀는 바운스 모션 (Sine wave)
    float bounce = fabsf(sinf(m_timer * 10.0f)) * 0.6f;
    animal.hopOffset = bounce;
    animal.currentPos.y = animal.basePos.y + bounce;
    animal.rotationOffset = sinf(m_timer * 12.0f) * 0.15f;
}

// ============================================================================
// SatisfiedState: 배부르고 만족한 평화로운 대기 상태
// ============================================================================
void SatisfiedState::Enter(AnimalTarget& animal)
{
    animal.currentPos = animal.basePos;
    animal.hopOffset = 0.0f;
    animal.rotationOffset = 0.0f;
    m_idleTimer = 0.0f;
}

void SatisfiedState::Update(AnimalTarget& animal, float dt)
{
    m_idleTimer += dt;
    // 배부른 상태에서 여유롭게 고개를 갸웃거리는 평화로운 모션
    animal.currentPos = animal.basePos;
    animal.hopOffset = 0.0f;
    animal.rotationOffset = sinf(m_idleTimer * 1.5f) * 0.05f;
}