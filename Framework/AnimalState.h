#pragma once

#include <DirectXMath.h>
#include <memory>
#include <string>
#include <cmath>

struct AnimalTarget;

// ============================================================================
// IAnimalState: 동물 상태 추상 인터페이스 (State Pattern / FSM)
// ============================================================================
class IAnimalState
{
public:
    virtual ~IAnimalState() = default;

    virtual void Enter(AnimalTarget& animal) {}
    virtual void Update(AnimalTarget& animal, float dt) = 0;
    virtual void Exit(AnimalTarget& animal) {}

    virtual bool CanEat() const { return false; }
    virtual bool IsFed() const { return false; }
    virtual const char* GetStateName() const = 0;
};

// ============================================================================
// SatisfiedState: 배부르고 만족한 평화로운 대기 상태
// ============================================================================
class SatisfiedState : public IAnimalState
{
public:
    void Enter(AnimalTarget& animal) override;
    void Update(AnimalTarget& animal, float dt) override;
    bool CanEat() const override { return false; }
    bool IsFed() const override { return true; }
    const char* GetStateName() const override { return "만족/행복 (Satisfied)"; }

private:
    float m_idleTimer = 0.0f;
};

// ============================================================================
// HungryState: 배고픔 상태 (먹이를 먹을 수 있음)
// ============================================================================
class HungryState : public IAnimalState
{
public:
    void Enter(AnimalTarget& animal) override;
    void Update(AnimalTarget& animal, float dt) override;
    bool CanEat() const override { return true; }
    bool IsFed() const override { return false; }
    const char* GetStateName() const override { return "배고픔 (Hungry)"; }

private:
    float m_breathTimer = 0.0f;
};

// ============================================================================
// HappyHopState: 먹이를 먹고 신나게 뜀뛰기하는 바운스 상태
// ============================================================================
class HappyHopState : public IAnimalState
{
public:
    explicit HappyHopState(float duration = 2.5f)
        : m_duration(duration), m_timer(duration)
    {
    }

    void Enter(AnimalTarget& animal) override;
    void Update(AnimalTarget& animal, float dt) override;
    bool CanEat() const override { return false; }
    bool IsFed() const override { return true; }
    const char* GetStateName() const override { return "신나게 뜀뛰기 (Happy Hop)"; }

private:
    float m_duration;
    float m_timer;
};