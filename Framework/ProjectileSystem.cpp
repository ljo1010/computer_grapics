////////////////////////////////////////////////////////////////////////////////
// Filename: ProjectileSystem.cpp
////////////////////////////////////////////////////////////////////////////////
#include "ProjectileSystem.h"
#include <algorithm>

using namespace DirectX;

ProjectileSystem::ProjectileSystem()
{
}

ProjectileSystem::~ProjectileSystem()
{
}

void ProjectileSystem::Spawn(const XMFLOAT3& camPos, const XMVECTOR& forward)
{
    HayProjectile proj;

    // Normalize forward direction
    XMFLOAT3 f;
    XMStoreFloat3(&f, XMVector3Normalize(forward));

    // Spawn position: slightly in front of camera
    proj.pos = XMFLOAT3(
        camPos.x + f.x * 1.0f,
        camPos.y + 0.2f,
        camPos.z + f.z * 1.0f
    );

    // Velocity
    proj.vel = XMFLOAT3(
        f.x * m_speed,
        f.y * m_speed,
        f.z * m_speed
    );

    m_projectiles.push_back(proj);
}

void ProjectileSystem::Update(float dt, const XMFLOAT3& pigPos, float pigRadius, bool& outPigHit)
{
    outPigHit = false;

    if (dt <= 0.0f || m_projectiles.empty())
        return;

    m_projectiles.erase(
        std::remove_if(
            m_projectiles.begin(), m_projectiles.end(),
            [this, dt, &pigPos, pigRadius, &outPigHit](HayProjectile& p)
            {
                // Update position
                p.pos.x += p.vel.x * dt;
                p.pos.y += p.vel.y * dt;
                p.pos.z += p.vel.z * dt;

                // Check pig collision
                if (CheckPigCollision(p.pos, pigPos, pigRadius))
                {
                    outPigHit = true;
                    return true;    // Remove this projectile
                }

                // Remove if too far
                float distSq = p.pos.x * p.pos.x + p.pos.y * p.pos.y + p.pos.z * p.pos.z;
                return distSq > m_maxDist * m_maxDist;
            }
        ),
        m_projectiles.end()
    );
}

void ProjectileSystem::UpdateMotionOnly(float dt)
{
    if (dt <= 0.0f || m_projectiles.empty())
        return;

    m_projectiles.erase(
        std::remove_if(
            m_projectiles.begin(), m_projectiles.end(),
            [this, dt](HayProjectile& p)
            {
                p.pos.x += p.vel.x * dt;
                p.pos.y += p.vel.y * dt;
                p.pos.z += p.vel.z * dt;

                float distSq = p.pos.x * p.pos.x + p.pos.y * p.pos.y + p.pos.z * p.pos.z;
                return distSq > m_maxDist * m_maxDist;
            }
        ),
        m_projectiles.end()
    );
}

bool ProjectileSystem::CheckPigCollision(const XMFLOAT3& projPos,
                                          const XMFLOAT3& pigPos,
                                          float pigRadius)
{
    // Pig center is slightly above ground
    XMFLOAT3 pigCenter(pigPos.x, pigPos.y + 0.5f, pigPos.z);

    float dx = projPos.x - pigCenter.x;
    float dz = projPos.z - pigCenter.z;
    float distSq = dx * dx + dz * dz;

    return distSq < pigRadius * pigRadius;
}
