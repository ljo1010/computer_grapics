////////////////////////////////////////////////////////////////////////////////
// Filename: cameraclass.h
////////////////////////////////////////////////////////////////////////////////
#ifndef _CAMERACLASS_H_
#define _CAMERACLASS_H_


//////////////
// INCLUDES //
//////////////
#include <directxmath.h>

#include "AlignedAllocationPolicy.h"

using namespace DirectX;

////////////////////////////////////////////////////////////////////////////////
// Class name: CameraClass
////////////////////////////////////////////////////////////////////////////////
class CameraClass : public AlignedAllocationPolicy<16>
{
public:
	CameraClass();
	CameraClass(const CameraClass&);
	~CameraClass();

	void SetPosition(float, float, float);
	void SetRotation(float, float, float);

	XMFLOAT3 GetPosition();
	XMFLOAT3 GetRotation();

	void Render();
	void GetViewMatrix(XMMATRIX&);

	//1인칭 카메라 움직임
	void Move(float leftRight, float forward);
	void Rotate(float yawDelta, float pitchDelta);
	void Update();  // 카메라 방향 업데이트

private:
	XMFLOAT3 m_position;
	XMFLOAT3 m_rotation;
	XMMATRIX m_viewMatrix;
	//추가해야할 멤버변수
	XMVECTOR m_camPosition;
	XMVECTOR m_camTarget;
	XMVECTOR m_camUp;
	XMVECTOR m_camForward;
	XMVECTOR m_camRight;

	float m_yaw, m_pitch;
};

#endif