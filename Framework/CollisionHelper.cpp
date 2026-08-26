#include "CollisionHelper.h"
#include <cfloat>

using namespace DirectX;

namespace Collision
{
    bool BoundingSphereCollision(
        const BoundingSphereData& firstSphere,
        const XMMATRIX& firstWorld,
        const BoundingSphereData& secondSphere,
        const XMMATRIX& secondWorld)
    {
        // 로컬 중심 -> 월드 중심
        XMVECTOR c1Local = XMLoadFloat3(&firstSphere.centerOffset);
        XMVECTOR c2Local = XMLoadFloat3(&secondSphere.centerOffset);

        XMVECTOR c1World = XMVector3TransformCoord(c1Local, firstWorld);
        XMVECTOR c2World = XMVector3TransformCoord(c2Local, secondWorld);

        XMVECTOR diff = c1World - c2World;
        float    distSq = XMVectorGetX(XMVector3LengthSq(diff));
        float    rSum = firstSphere.radius + secondSphere.radius;

        return distSq <= rSum * rSum;
    }

    bool BoundingBoxCollision(
        const AABB& firstBox,
        const AABB& secondBox)
    {
        // PDF에서 설명한 AABB 교차 판정 그대로
        if (firstBox.max.x < secondBox.min.x) return false; // first는 왼쪽
        if (firstBox.min.x > secondBox.max.x) return false; // first는 오른쪽

        if (firstBox.max.y < secondBox.min.y) return false; // 아래
        if (firstBox.min.y > secondBox.max.y) return false; // 위

        if (firstBox.max.z < secondBox.min.z) return false; // 앞
        if (firstBox.min.z > secondBox.max.z) return false; // 뒤

        return true; // 세 축 모두 겹침 → 충돌
    }

    void CalculateAABB(
        const std::vector<XMFLOAT3>& boundingBoxVertsLocal,
        const XMMATRIX& world,
        AABB& outAabb)
    {
        XMFLOAT3 minV(FLT_MAX, FLT_MAX, FLT_MAX);
        XMFLOAT3 maxV(-FLT_MAX, -FLT_MAX, -FLT_MAX);

        for (size_t i = 0; i < boundingBoxVertsLocal.size(); ++i)
        {
            XMVECTOR vLocal = XMLoadFloat3(&boundingBoxVertsLocal[i]);
            XMVECTOR vWorld = XMVector3TransformCoord(vLocal, world);

            XMFLOAT3 p;
            XMStoreFloat3(&p, vWorld);

            minV.x = std::min(minV.x, p.x);
            minV.y = std::min(minV.y, p.y);
            minV.z = std::min(minV.z, p.z);

            maxV.x = std::max(maxV.x, p.x);
            maxV.y = std::max(maxV.y, p.y);
            maxV.z = std::max(maxV.z, p.z);
        }

        outAabb.min = minV;
        outAabb.max = maxV;
    }
}
