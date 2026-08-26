#include "pch.h"
#include "Animation.h"

Animation::Animation()
{
}

Animation::~Animation()
{
}


void Animation::UpdateAnimation(float t)
{
	//Calc Frame Cnt
	if (_playState == PLAY_STATE::PLAY)
		_playTime += fmod(_tickPerSecond * t, _duration);

	//������ �ð� �ʱ�ȭ
	if (_playTime > _lastTime) {
		_playTime = 0.f;

		if (!_isRepeat)
			_playState = PLAY_STATE::STOP;
	}



	//���� ��ȯ ������ TM����� ����� ���� ���Ǵ� ���� ����
	XMVECTOR Z = XMVectorSet(0.f, 0.f, 0.f, 1.f);

	//AniNode Update
	for (auto& node : _aniNode) {
		//���� ������ �ð��� ù ��° Ű ������ �ð����� ���� ���
		if (node.keyFrame.front().timePos >= _playTime) {
			XMVECTOR T = XMLoadFloat3(&node.keyFrame.front().trans);
			XMVECTOR S = XMLoadFloat3(&node.keyFrame.front().scale);
			XMVECTOR R = XMLoadFloat4(&node.keyFrame.front().rotation);

			node.aniTM = XMMatrixAffineTransformation(S, Z, R, T);
		}
		//���� ������ �ð��� ������ Ű ������ �ð����� ���� ���
		else if (node.keyFrame.back().timePos <= _playTime) {
			XMVECTOR T = XMLoadFloat3(&node.keyFrame.back().trans);
			XMVECTOR S = XMLoadFloat3(&node.keyFrame.back().scale);
			XMVECTOR R = XMLoadFloat4(&node.keyFrame.back().rotation);

			node.aniTM = XMMatrixAffineTransformation(S, Z, R, T);
		}
		else {
			for (UINT i = 0; i < node.keyFrame.size() - 1; i++) {
				auto key = node.keyFrame;
				if (_playTime >= key[i].timePos && _playTime <= key[i + 1].timePos) {
					float lerpRate = (_playTime - key[i].timePos) / (key[i + 1].timePos - key[i].timePos);

					XMVECTOR s1 = XMLoadFloat3(&key[i].scale);
					XMVECTOR s2 = XMLoadFloat3(&key[i + 1].scale);

					XMVECTOR t1 = XMLoadFloat3(&key[i].trans);
					XMVECTOR t2 = XMLoadFloat3(&key[i + 1].trans);

					XMVECTOR r1 = XMLoadFloat4(&key[i].rotation);
					XMVECTOR r2 = XMLoadFloat4(&key[i + 1].rotation);

					//�� ������ ���� ���� ����
					XMVECTOR T = XMVectorLerp(t1, t2, lerpRate);
					XMVECTOR S = XMVectorLerp(s1, s2, lerpRate);
					XMVECTOR R = XMQuaternionSlerp(r1, r2, lerpRate);

					node.aniTM = XMMatrixAffineTransformation(S, Z, R, T);
					break;
				}
			}
		}
	}
}

void Animation::SetRepeat(bool flag)
{
	_isRepeat = flag;
}

//��� ������ �´� ANI ��� ���� ��ȯ
XMMATRIX Animation::GetAniTM(wstring nodeName, XMMATRIX& tm)
{
	XMMATRIX aniTM = tm;

	//��ġ�ϴ� ��� Ž��
	auto res = std::find_if(_aniNode.begin(), _aniNode.end(), [&](AniNode aniNode) {	return nodeName == aniNode.name; });

	//�ִ� ��� ��ȯ, ������ ���� �������(�������) �״�� ��ȯ
	//�ִϸ��̼� ����� ����� ���� ������ �����ϰ� �ֱ� ������ ����� ���� ����� �ȴ�.
	if (res != _aniNode.end())
		tm = res->aniTM;

	return aniTM;
}
