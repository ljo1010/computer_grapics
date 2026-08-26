////////////////////////////////////////////////////////////////////////////////
// Filename: PlayerController.cpp
////////////////////////////////////////////////////////////////////////////////
#include "PlayerController.h"
#include "cameraclass.h"

using namespace DirectX;

PlayerController::PlayerController()
{
}

PlayerController::~PlayerController()
{
}

void PlayerController::Initialize(CameraClass* camera)
{
    m_camera = camera;

    m_position = XMFLOAT3(0.0f, 1.7f, -5.0f);
    m_yaw = 0.0f;
    m_pitch = 0.0f;
    m_yawSens = 0.0025f;
    m_pitchSens = 0.0025f;
    m_invertY = false;

    m_moveForward = m_moveRight = m_moveUp = 0.0f;
    m_moveDt = 0.0f;
    m_moveSpeed = 5.0f;
    m_cameraRadius = 0.5f;

    if (m_camera)
    {
        m_camera->SetPosition(m_position.x, m_position.y, m_position.z);
        m_camera->SetRotation(0.0f, 0.0f, 0.0f);
    }
}

void PlayerController::Update(int mouseDX, int mouseDY)
{
    // Apply mouse rotation
    const float invY = m_invertY ? -1.0f : 1.0f;
    m_yaw += mouseDX * m_yawSens;
    m_pitch += mouseDY * m_pitchSens * invY;

    // Clamp pitch
    const float kPitchMax = 1.55f;
    if (m_pitch > kPitchMax) m_pitch = kPitchMax;
    if (m_pitch < -kPitchMax) m_pitch = -kPitchMax;

    // Wrap yaw
    if (m_yaw > XM_PI) m_yaw -= XM_2PI;
    if (m_yaw < -XM_PI) m_yaw += XM_2PI;

    // Calculate movement vectors
    XMFLOAT3 oldPos = m_position;
    XMVECTOR pos = XMLoadFloat3(&m_position);

    const float dt = m_moveDt;
    const float sy = sinf(m_yaw);
    const float cy = cosf(m_yaw);

    XMVECTOR forward = XMVectorSet(sy, 0.0f, cy, 0.0f);
    XMVECTOR right = XMVectorSet(sinf(m_yaw + XM_PIDIV2), 0.0f,
                                  cosf(m_yaw + XM_PIDIV2), 0.0f);
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMVECTOR move =
        XMVectorScale(forward, m_moveForward * m_moveSpeed * dt) +
        XMVectorScale(right, m_moveRight * m_moveSpeed * dt) +
        XMVectorScale(up, m_moveUp * m_moveSpeed * dt);

    XMVECTOR newPosV = XMVectorAdd(pos, move);
    XMFLOAT3 newPos;
    XMStoreFloat3(&newPos, newPosV);

    // Check fence collision
    if (!CheckFenceCollision(newPos))
    {
        m_position = newPos;
    }
    else
    {
        m_position = oldPos;
    }

    // Update camera
    if (m_camera)
    {
        const float RAD2DEG = 57.2957795f;
        m_camera->SetRotation(m_pitch * RAD2DEG, m_yaw * RAD2DEG, 0.0f);
        m_camera->SetPosition(m_position.x, m_position.y, m_position.z);
    }
}

void PlayerController::SetMovement(float forward, float right, float up, float dt)
{
    // Normalize movement
    float len2 = forward * forward + right * right + up * up;
    if (len2 > 1e-6f)
    {
        float inv = 1.0f / sqrtf(len2);
        forward *= inv;
        right *= inv;
        up *= inv;
    }

    m_moveForward = forward;
    m_moveRight = right;
    m_moveUp = up;
    m_moveDt = dt;
}

void PlayerController::SetSensitivity(float yaw, float pitch)
{
    m_yawSens = yaw;
    m_pitchSens = pitch;
}

void PlayerController::SetFenceColliders(const std::vector<FenceCollider>& colliders)
{
    m_fenceColliders = colliders;
}

XMVECTOR PlayerController::GetForward() const
{
    const float sy = sinf(m_yaw);
    const float cy = cosf(m_yaw);
    return XMVectorSet(sy, 0.0f, cy, 0.0f);
}

bool PlayerController::CheckFarmerCollision(const XMFLOAT3& farmerPos, float farmerRadius)
{
    XMFLOAT3 farmerCenter(
        farmerPos.x,
        farmerPos.y + 1.0f,
        farmerPos.z
    );

    XMVECTOR vCam = XMLoadFloat3(&m_position);
    XMVECTOR vFarmer = XMLoadFloat3(&farmerCenter);

    float distSq = XMVectorGetX(XMVector3LengthSq(vCam - vFarmer));
    float sumR = farmerRadius + m_cameraRadius;

    return (distSq < sumR * sumR);
}

bool PlayerController::CheckFenceCollision(const XMFLOAT3& pos)
{
    if (m_fenceColliders.empty())
        return false;

    for (const auto& col : m_fenceColliders)
    {
        float dx = pos.x - col.center.x;
        float dz = pos.z - col.center.z;

        float distSq = dx * dx + dz * dz;
        float sumR = m_cameraRadius + col.radius;

        if (distSq < sumR * sumR)
            return true;
    }

    return false;
}
