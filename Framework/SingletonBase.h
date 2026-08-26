#pragma once
//=============================================================
//	## singleTonBase ## (�̱��� ����)
//=============================================================
#include <memory>
using namespace std;

template<class T>
class SingletonBase
{
protected:
	//�̱��� �ν��Ͻ� ����
	static unique_ptr<T> _instance;

	SingletonBase() {}
	~SingletonBase() {}
public:
	static T& GetSingleton(void);
};

//�̱��� �ʱ�ȭ
template<class T>
unique_ptr<T> SingletonBase<T>::_instance;

//�̱��� ��������
template<class T>
inline T& SingletonBase<T>::GetSingleton(void)
{
	//�̱����� ������ ���� �����ϱ� �� �̱��� ����
	if (!_instance) _instance = make_unique<T>();
	return *(_instance.get());
}

