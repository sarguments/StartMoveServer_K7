#pragma once

// Ŭ��� �ٸ�
struct st_StartInfo
{
	// INADDR_IN
	bool _inUse;
	SOCKET _sock;
	int _ID;
	int _X;
	int _Y;
};

enum e_Type
{
	ID_ALLOC = 0,
	STAR_MAKE,
	STAR_DEL,
	STAR_MOVE,
	NONE
};

//��Ŷ�������� - ��Ŷ�� 16����Ʈ ����
struct st_Packet
{
	int Type;
	int ID;
	int X;
	int Y;
};

//ID�Ҵ�(0)	Type(4Byte) | ID(4Byte) | �Ⱦ�(4Byte) | �Ⱦ�(4Byte)
struct st_IdAlloc
{
	int Type;
	int ID;
	int notUse1 = UINT_MAX;
	int notUse2 = UINT_MAX;
};

//������(1)	Type(4Byte) | ID(4Byte) | X(4Byte) | Y(4Byte)
struct st_StarMake
{
	int Type;
	int ID;
	int X;
	int Y;
};

//������(2)	Type(4Byte) | ID(4Byte) | �Ⱦ�(4Byte) | �Ⱦ�(4Byte)
struct st_StartDelete
{
	int Type;
	int ID;
	int notUse1 = UINT_MAX;
	int notUse2 = UINT_MAX;
};

//�̵�(3)		Type(4Byte) | ID(4Byte) | X(4Byte) | Y(4Byte)
struct st_StarMove
{
	int Type;
	int ID;
	int X;
	int Y;
};

//0 ~2�� ��Ŷ�� ����->Ŭ����Ŷ �̸�, 3�� ��Ŷ��  Ŭ�� <->����  �ֹ� ��Ŷ��.
