////////////////////////////////////////////////////////////////////////////////
// Filename: cameraclass.cpp
////////////////////////////////////////////////////////////////////////////////
#include "cameraclass.h"


CameraClass::CameraClass()
{
	m_position.x = 0.0f;
	m_position.y = 0.0f;
	m_position.z = 0.0f;

	m_rotation.x = 0.0f;
	m_rotation.y = 0.0f;
	m_rotation.z = 0.0f;

	m_camPosition = XMVectorSet(0.0f, 0.0f, -10.0f, 0.0f);
	m_camTarget = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	m_camUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	m_yaw = 0.0f;
	m_pitch = 0.0f;

}


CameraClass::CameraClass(const CameraClass& other)
{
}
CameraClass::~CameraClass()
{
}


void CameraClass::SetPosition(float x, float y, float z)
{
	m_position.x = x;
	m_position.y = y;
	m_position.z = z;
	m_camPosition = XMVectorSet(x, y, z, 1.0f);
}


void CameraClass::SetRotation(float x, float y, float z)
{
	m_rotation.x = x;
	m_rotation.y = y;
	m_rotation.z = z;

	m_pitch = x * 0.0174533f;
	m_yaw = y * 0.0174533f;
}


XMFLOAT3 CameraClass::GetPosition()
{
	return m_position;
}


XMFLOAT3 CameraClass::GetRotation()
{
	return m_rotation;
}

//1인칭 이동 코드
void CameraClass::Move(float leftRight, float forward)
{
	// 방향 벡터 계산
	XMVECTOR forwardVec = XMVectorSet(
		sinf(m_yaw),
		0.0f,
		cosf(m_yaw),
		0.0f
	);
	XMVECTOR rightVec = XMVector3Cross(m_camUp, forwardVec);
	forwardVec = XMVector3Normalize(forwardVec);
	rightVec = XMVector3Normalize(rightVec);

	// 이동 적용
	m_camPosition += leftRight * rightVec;
	m_camPosition += forward * forwardVec;

	// float3 위치값도 업데이트
	XMStoreFloat3(&m_position, m_camPosition);
}
//마우스 회전
void CameraClass::Rotate(float yawDelta, float pitchDelta)
{
	m_yaw += yawDelta;
	m_pitch += pitchDelta;

	// pitch 범위 제한 (수직 회전 제한)
	if (m_pitch > XM_PIDIV2 - 0.01f) m_pitch = XM_PIDIV2 - 0.01f;
	if (m_pitch < -XM_PIDIV2 + 0.01f) m_pitch = -XM_PIDIV2 + 0.01f;

	// float3 회전값도 업데이트
	m_rotation = { m_pitch * 57.2958f, m_yaw * 57.2958f, 0.0f };
}

//view 행렬 갱신
void CameraClass::Update()
{
	// yaw → pitch 순서로 회전
	XMMATRIX rotY = XMMatrixRotationY(m_yaw);
	XMMATRIX rotX = XMMatrixRotationX(m_pitch);
	XMMATRIX rotationMatrix = rotY * rotX;

	XMVECTOR forward = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	forward = XMVector3TransformCoord(forward, rotationMatrix);
	forward = XMVector3Normalize(forward);

	m_camTarget = m_camPosition + forward;

	XMVECTOR upVec = XMVector3TransformCoord(XMVectorSet(0, 1, 0, 0), rotationMatrix);
	m_viewMatrix = XMMatrixLookAtLH(m_camPosition, m_camTarget, upVec);
}



//legacy code update cam
void CameraClass::Render()
{
	XMVECTOR up, position, lookAt;
	float yaw, pitch, roll;
	XMMATRIX rotationMatrix;

	// Setup the vector that points upwards.
	up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	// Setup the position of the camera in the world.
	position = XMLoadFloat3(&m_position);

	// Setup where the camera is looking by default.
	lookAt = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

	// Set the yaw (Y axis), pitch (X axis), and roll (Z axis) rotations in radians.
	pitch = m_rotation.x * 0.0174532925f;
	yaw   = m_rotation.y * 0.0174532925f;
	roll  = m_rotation.z * 0.0174532925f;

	// Create the rotation matrix from the yaw, pitch, and roll values.
	rotationMatrix = XMMatrixRotationRollPitchYaw(pitch, yaw, roll);

	// Transform the lookAt and up vector by the rotation matrix so the view is correctly rotated at the origin.
	lookAt = XMVector3TransformCoord(lookAt, rotationMatrix);
	up = XMVector3TransformCoord(up, rotationMatrix);

	// Translate the rotated camera position to the location of the viewer.
	lookAt = position + lookAt;

	// Finally create the view matrix from the three updated vectors.
	m_viewMatrix = XMMatrixLookAtLH(position, lookAt, up);

	return;
}


void CameraClass::GetViewMatrix(XMMATRIX& viewMatrix)
{
	viewMatrix = m_viewMatrix;
}