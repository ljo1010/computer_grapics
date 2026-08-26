////////////////////////////////////////////////////////////////////////////////
// Filename: ProjectileSystem.h
// Description: Manages hay projectile spawning, updating, and collision
////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <vector>
#include <DirectXMath.h>

struct HayProjectile
{
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT3 vel;
};

class ProjectileSystem
{
public:
    ProjectileSystem();
    ~ProjectileSystem();

    // Spawn a projectile from camera position in forward direction
    void Spawn(const DirectX::XMFLOAT3& camPos, const DirectX::XMVECTOR& forward);

    // Update all projectiles, check collision with pig
    // Returns true if pig was hit
    void Update(float dt, const DirectX::XMFLOAT3& pigPos, float pigRadius, bool& outPigHit);

    // Update positions and lifetime only (collision handled by AnimalQuestSystem)
    void UpdateMotionOnly(float dt);

    // Get all active projectiles (for rendering)
    const std::vector<HayProjectile>& GetProjectiles() const { return m_projectiles; }
    std::vector<HayProjectile>& GetProjectilesRef() { return m_projectiles; }

    // Ammo management
    bool HasAmmo() const { return m_hasAmmo; }
    void SetHasAmmo(bool has) { m_hasAmmo = has; }

    // Clear all projectiles
    void Clear() { m_projectiles.clear(); }

private:
    bool CheckPigCollision(const DirectX::XMFLOAT3& projPos,
                           const DirectX::XMFLOAT3& pigPos,
                           float pigRadius);

private:
    std::vector<HayProjectile> m_projectiles;
    bool m_hasAmmo = false;
    float m_speed = 10.0f;
    float m_maxDist = 200.0f;
};
