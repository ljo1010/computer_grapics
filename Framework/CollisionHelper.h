#pragma once

#include <vector>
#include <DirectXMath.h>

using namespace DirectX;

struct AABB
{
    XMFLOAT3 min;  // world-space
    XMFLOAT3 max;  // world-space
};

struct BoundingSphereData
{
    XMFLOAT3 centerOffset; // 모델 로컬 좌표계 기준 중심
    float    radius;       // 로컬 좌표계 반지름
};

namespace Collision
{
    bool BoundingSphereCollision(
        const BoundingSphereData& firstSphere,
        const XMMATRIX& firstWorld,
        const BoundingSphereData& secondSphere,
        const XMMATRIX& secondWorld);

    bool BoundingBoxCollision(
        const AABB& firstBox,
        const AABB& secondBox);

    void CalculateAABB(
        const std::vector<XMFLOAT3>& boundingBoxVertsLocal,
        const XMMATRIX& world,
        AABB& outAabb);
}
