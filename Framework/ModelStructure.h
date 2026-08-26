//==================================================================
//		## �𵨿� �ʿ��� ����ü ���� ## 
//==================================================================
#pragma once
#include "pch.h"

//���� ����
struct Vertex
{
	vector<XMFLOAT3> position;
	vector<XMFLOAT4> color;
	vector<XMFLOAT2> uv;
	vector<XMFLOAT3> normal;
	vector<XMFLOAT3> tangent;
	vector<XMFLOAT3> bitangent;
	vector<XMUINT4>	 boneidx;
	vector<XMFLOAT4> weight;
};

//���� ����
struct Material
{
	wstring name = L"";

	Texture* diffuseMap = NULL;
	Texture* alphaMap = NULL;
	Texture* normalMap = NULL;
	Texture* sepcularMap = NULL;
	Texture* lightMap = NULL;
};


//�⺻ �޽�, �ε��� ������ ����.
struct BaseMesh {
	wstring	name = L"None";		//Object Name Info

	UINT start = 0;			//Object Start Idx or Vert Info
	UINT count = 0;			//Object Idx or Vert Count Info

	UINT startVert = 0;			//�޽��� ���� ������ ������ ���° �������� ǥ��.

	bool isDraw = true;			//check to Render

};


//������Ʈ�� ��ġ ������ ���� ���
struct NodeInfo {
	int			depth = 0;

	wstring		name = L"none";
	NodeInfo* parent = NULL;

	//TM info
	XMMATRIX localTM = XMMatrixIdentity();
	XMMATRIX worldTM = XMMatrixIdentity();

	NodeInfo(NodeInfo* parent, wstring& name, XMMATRIX& tm, int depth = 0)
		: parent(parent), name(name), depth(depth), localTM(tm), worldTM(tm)
	{
		//�θ��尡 ������ ���� ��� ����
		if (parent)
			this->worldTM = localTM * parent->worldTM;
	}
};


//Offset Info
struct BoneInfo {
	NodeInfo* linkNode = NULL;
	DirectX::XMMATRIX matOffset = DirectX::XMMatrixIdentity();

	// �� �߰�: ��Ű�׿� ���� �ε���
	int skinIndex = -1;
};


//���� ������ ���� �޽�
struct HierarchyMesh : public BaseMesh {
	int matIdx = -1;
	NodeInfo* linkNode = NULL;
	vector<BoneInfo> boneList;

	HierarchyMesh(wstring name, int start, int count, int matIdx, int startVert)
	{
		if (name != L"")
			this->name = name;

		this->start = start, this->count = count, this->matIdx = matIdx, this->startVert = startVert;
	};
};

//���� ���� ����Ʈ
struct VertexDataBuffer {
	ID3D11Buffer* positionBuffer = NULL;
	ID3D11Buffer* colorBuffer = NULL;
	ID3D11Buffer* texcoordBuffer = NULL;
	ID3D11Buffer* normalBuffer = NULL;
	ID3D11Buffer* tangentBuffer = NULL;
	ID3D11Buffer* bitangentBuffer = NULL;
	ID3D11Buffer* boneIdBuffer = NULL;
	ID3D11Buffer* weightBuffer = NULL;
};

