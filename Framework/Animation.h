//====================================================================================
//		## Animation ## (�ִϸ��̼ǰ� ���õ� ����ü�� Ŭ����)
//====================================================================================
#pragma once

enum class PLAY_STATE {
	PLAY, STOP
};

//Ű ������ ����
struct KeyFrame {
	float		timePos = 0.f;
	XMFLOAT3	trans = XMFLOAT3(0.f, 0.f, 0.f);
	XMFLOAT4	rotation = XMFLOAT4(0.f, 0.f, 0.f, 0.f);
	XMFLOAT3	scale = XMFLOAT3(0.f, 0.f, 0.f);
};

//���� ����� �ִϸ��̼� ����
struct AniNode {
	vector<KeyFrame>	keyFrame;							//Ű������ ����Ʈ
	wstring				name = L"none";					//����� �� �̸�

	XMMATRIX			aniTM = XMMatrixIdentity();		//�ִϸ��̼� ���, �����ӿ� ���� ������Ʈ�� ��� ������ ����
};


//�ִϸ��̼� Ŭ����
class Animation
{
	//*************************************** ��� �Լ� ************************************************//
public:
	Animation();
	~Animation();

	//Animation Play Function
	void				Play() { _playState = PLAY_STATE::PLAY; }
	void				Pause() { _playState = PLAY_STATE::STOP; }
	void				Stop() {
		_playState = PLAY_STATE::STOP;
		_playTime = 0.f;
	}

	//�ݺ� ��� ���� üũ
	bool				IsRepeat() { return _isRepeat; }
	bool isPlaying() { return _playState == PLAY_STATE::PLAY ? true : false; }
	bool isStop() { return _playState == PLAY_STATE::STOP ? true : false; }

	//Set Function
	void				SetTickPerSecond(float tps) { _tickPerSecond = tps; }		//ƽ �� ����
	void				SetDuration(float duration) { _duration = duration; }		//����ӵ� ����
	void				SetLastFrame(float time) { _lastTime = time; }		//������ ������ ����
	void				SetName(wstring name) { _name = name; }		//�ִϸ��̼� �̸�
	void				TurnRepeat() { _isRepeat = !_isRepeat; }		//�ݺ���� ��ȯ
	void SetRepeat(bool flag);

	//Get Function
	vector<AniNode>& GetAniNodeList() { return _aniNode; }		//�ִϸ��̼� ��� ����Ʈ ��ȯ
	XMMATRIX			GetAniTM(wstring nodeName, XMMATRIX& tm);												//��忡 �´� �ִϸ��̼� ����� ��ȯ
	wstring				GetName() { return _name; }		//�ִϸ��̼� �̸� ��ȯ

	//Update AniTM
	void				UpdateAnimation(float t);																//�ִϸ��̼��� ������Ʈ�Ѵ�.


	//*************************************** ��� ���� ***********************************************//
private:
	vector<AniNode>		_aniNode;										//�ִϸ��̼� ��� ����Ʈ

	wstring				_name = L"none";						//�̸�
	float				_tickPerSecond = 0.f;							//tick ����
	float				_duration = 0.f;							//�ӵ�
	float				_playTime = 0.f;							//���� ������
	float				_lastTime = 0.f;							//������ ������

	bool				_isRepeat = false;							//��� �Ϸ�� �ݺ� ��� �÷���
	PLAY_STATE			_playState = PLAY_STATE::STOP;				//��� ����
};
