////////////////////////////////////////////////////////////////////////////////
// Filename: PlayerController.h
// Description: Manages camera movement, rotation, and collision detection
////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <DirectXMath.h>
#include <vector>

class CameraClass;

struct FenceCollider
{
    DirectX::XMFLOAT3 center;
    float radius;
};

class PlayerController
{
public:
    PlayerController();
    ~PlayerController();

    void Initialize(CameraClass* camera);
    void Update(int mouseDX, int mouseDY);

    // Movement input
    void SetMovement(float forward, float right, float up, float dt);
    void SetSensitivity(float yaw, float pitch);
    void SetInvertY(bool invert) { m_invertY = invert; }

    // Collision setup
    void SetFenceColliders(const std::vector<FenceCollider>& colliders);

    // Getters
    DirectX::XMFLOAT3 GetPosition() const { return m_position; }
    DirectX::XMVECTOR GetForward() const;
    float GetYaw() const { return m_yaw; }
    float GetPitch() const { return m_pitch; }
    float GetCameraRadius() const { return m_cameraRadius; }

    // Collision checks
    bool CheckFarmerCollision(const DirectX::XMFLOAT3& farmerPos, float farmerRadius);

private:
    bool CheckFenceCollision(const DirectX::XMFLOAT3& pos);

private:
    CameraClass* m_camera = nullptr;

    // Position & Rotation
    DirectX::XMFLOAT3 m_position = { 0.0f, 1.7f, -5.0f };
    float m_yaw = 0.0f;
    float m_pitch = 0.0f;

    // Sensitivity
    float m_yawSens = 0.0025f;
    float m_pitchSens = 0.0025f;
    bool m_invertY = false;

    // Movement
    float m_moveForward = 0.0f;
    float m_moveRight = 0.0f;
    float m_moveUp = 0.0f;
    float m_moveDt = 0.0f;
    float m_moveSpeed = 5.0f;

    // Collision
    float m_cameraRadius = 0.5f;
    std::vector<FenceCollider> m_fenceColliders;
};
