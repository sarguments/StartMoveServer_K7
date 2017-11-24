#include "stdafx.h"

#include "hoxy_Header.h"
#include "Start_Protocol.h"

// define
#define SERVERPORT 3000
#define WIDTH 80
#define HEIGHT 23
#define MAX_ARRNUM 50 // 50 -> 60

// 스타 정보 저장 구조체 배열
st_StartInfo g_startInfoArr[MAX_ARRNUM];

// listenSock
SOCKET g_listenSock = INVALID_SOCKET;

// 고유 번호
static int g_uniqueNum = 100;

// 사이즈만큼 받기
int recvn(SOCKET sock, char* buf, int size);

// 범위 체크
bool AreaCheck(int x, int y);

bool SendPacketUnicast(int idxParam, char* packet, int size);
bool SendPacketBroadcast(char* packet, int size, int idxParam = -1); // idxParam 은 예외

bool NetInit(void);
bool NetProc(void);

bool NewUserProc(void);
bool PacketRecvProc(FD_SET* setParam, int countNum);
bool CloseProc(int idxParam);

bool Render(void);

// main
int main()
{
	NetInit();

	while (1)
	{
		if (!NetProc())
		{
			CCmdStart::CmdDebugText(L"NetProc", false);
			break;
		}

		Render();
	}

	return 0;
}

// recvn
int recvn(SOCKET sock, char * buf, int size)
{
	int left = size;
	char* nowPtr = buf;

	while (left > 0)
	{
		int retval = recv(sock, nowPtr, left, 0);
		if (retval < 0)
		{
			CCmdStart::CmdDebugText(L"recvn()", false);
			return -1;
		}

		left -= retval;
		nowPtr += retval;
	}

	return size - left;
}

// 렌더링
bool Render(void)
{
	char groundArr[HEIGHT][WIDTH] = { 0, };

	memset(groundArr, ' ', sizeof(groundArr));

	for (int i = 0; i < HEIGHT; i++)
	{
		groundArr[i][WIDTH - 1] = '\0';
	}

	for (int i = 0; i < MAX_ARRNUM; i++)
	{
		if (g_startInfoArr[i]._inUse == false)
		{
			continue;
		}

		int x = g_startInfoArr[i]._X;
		int y = g_startInfoArr[i]._Y;

		groundArr[y][x] = '*';
	}

	// 지우고 그린다
	system("cls");

	for (int i = 0; i < HEIGHT; i++)
	{
		printf("%s\n", groundArr[i]);
	}

	return true;
}

// 범위체크
bool AreaCheck(int x, int y)
{
	if (x < 0 || x > WIDTH - 2 || y < 0 || y > HEIGHT - 1)
	{
		return false;
	}

	return true;
}

// 유니캐스트
bool SendPacketUnicast(int idxParam, char * packet, int size)
{
	for (int i = 0; i < MAX_ARRNUM; i++)
	{
		if (g_startInfoArr[i]._inUse == false ||
			i != idxParam)
		{
			continue;
		}

		int ret_send = send(g_startInfoArr[i]._sock, packet, size, 0);
		if (ret_send == SOCKET_ERROR)
		{
			CCmdStart::CmdDebugText(L"listen, socket()", false);
			return false;
		}

		wcout << L"Unicast send : " << ret_send << endl;

		return true;
	}

	return false;
}

// 브로드캐스트
bool SendPacketBroadcast(char * packet, int size, int idxParam)
{
	// TODO : 템플릿화?

	for (int i = 0; i < MAX_ARRNUM; i++)
	{
		// TODO : 여기도 전체 다 도나?
		if (
			g_startInfoArr[i]._inUse == false ||
			(idxParam != -1 &&	i == idxParam)
			)
		{
			continue;
		}

		if (idxParam == -1)
		{
			wcout << L"idxParam is : " << -1 << endl;
		}
		
		int ret_send = send(g_startInfoArr[i]._sock, packet, size, 0);
		if (ret_send == SOCKET_ERROR)
		{
			CCmdStart::CmdDebugText(L"listen, socket()", false);
			return false;
		}

		wcout << L"Broadcast send : " << ret_send << endl;
	}

	return true;
}

//  초기화
bool NetInit(void)
{
	// wsaStart
	CSockUtill::WSAStart();

	// listenSocket
	g_listenSock = socket(AF_INET, SOCK_STREAM, 0);
	if (g_listenSock == SOCKET_ERROR)
	{
		CCmdStart::CmdDebugText(L"listen, socket()", false);
		return false;
	}
	else
	{
		CCmdStart::CmdDebugText(L"listen, socket()", true);
	}

	// bind
	SOCKADDR_IN listenAddr;
	listenAddr.sin_family = AF_INET;
	listenAddr.sin_port = htons(SERVERPORT);
	listenAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	int ret_bind = bind(g_listenSock, (SOCKADDR*)&listenAddr, sizeof(SOCKADDR));
	if (ret_bind == SOCKET_ERROR)
	{
		CCmdStart::CmdDebugText(L"bind()", false);
		return false;
	}
	else
	{
		CCmdStart::CmdDebugText(L"bind()", true);
	}

	// 구조체 sock 초기화
	for (int i = 0; i < MAX_ARRNUM; i++)
	{
		// invalid socket
		g_startInfoArr[i]._inUse = false;
		g_startInfoArr[i]._sock = INVALID_SOCKET;
	}

	// 마지막으로 listen
	int ret_listen = listen(g_listenSock, SOMAXCONN);
	if (ret_listen == SOCKET_ERROR)
	{
		CCmdStart::CmdDebugText(L"listen()", false);
		return false;
	}
	else
	{
		CCmdStart::CmdDebugText(L"listen()", true);
	}

	return true;
}

// 네트워크 처리
bool NetProc(void)
{
	while (1)
	{
		FD_SET rset;
		FD_ZERO(&rset);
		// listenSock 리드셋에 등록
		FD_SET(g_listenSock, &rset);

		// 기타 소켓 리드셋에 등록..
		for (int i = 0; i < MAX_ARRNUM; i++)
		{
			if (g_startInfoArr[i]._inUse == false)
			{
				continue;
			}

			// TODO : 여기는 끝가지 도는게 맞나?
			FD_SET(g_startInfoArr[i]._sock, &rset);
		}

		// select
		// timeval 을 NULL로 해서 블록
		int ret_select = select(0, &rset, NULL, NULL, NULL);
		if (ret_select == SOCKET_ERROR)
		{
			CCmdStart::CmdDebugText(L"select", false);
			return false;
		}
		else if (ret_select == 0)
		{
			// select 반응 없는 경우
			wcout << L"ret_select : " << ret_select << endl;
			break;
		}

		if (FD_ISSET(g_listenSock, &rset) != 0)
		{
			// 신규접속
			NewUserProc();
			--ret_select;
		}

		if (ret_select == 0)
		{
			// 새로운 접속 처리 후 읽을게 없을 경우
			break;
		}

		// 받은 패킷 처리
		PacketRecvProc(&rset, ret_select);
	}

	return true;
}

// 신규 접속자 처리
bool NewUserProc(void)
{
	int infoIdx = -1;

	for (int i = 0; i < MAX_ARRNUM; i++)
	{
		if (g_startInfoArr[i]._inUse == true)
		{
			continue;
		}

		infoIdx = i;

		SOCKADDR_IN clientAddr;
		int addrSize = sizeof(clientAddr);
		ZeroMemory(&clientAddr, addrSize);

		// accept
		SOCKET NewClientSock = accept(g_listenSock, (SOCKADDR*)&clientAddr, &addrSize);
		if (NewClientSock == INVALID_SOCKET)
		{
			CCmdStart::CmdDebugText(L"accept g_listenSock", false);
			return false;
		}

		WCHAR ipText[20] = { 0, };
		InetNtop(AF_INET, &clientAddr.sin_addr.s_addr, ipText, 20);
		wcout << L"New Client IP Adress is : " << ipText << endl;

		// TODO : 나중에 IP도 저장
		// TODO : 전역 접근 하는 코드 나중에 다시 수정

		g_startInfoArr[infoIdx]._inUse = true;
		g_startInfoArr[infoIdx]._sock = NewClientSock;
		g_startInfoArr[infoIdx]._ID = g_uniqueNum;
		++g_uniqueNum;
		g_startInfoArr[infoIdx]._X = rand() % WIDTH;
		g_startInfoArr[infoIdx]._Y = rand() % HEIGHT;

		break;
	}

	// 1. st_IdAlloc 패킷 생성
	st_IdAlloc idPacket;
	idPacket.ID = g_uniqueNum;
	idPacket.Type = e_Type::ID_ALLOC;

	// 해당 클라에게 유니캐스팅
	SendPacketUnicast(infoIdx, (char*)&idPacket, sizeof(st_IdAlloc));

	for (int i = 0; i < MAX_ARRNUM; i++)
	{


		// 2. st_StarMake 패킷 생성
		st_StarMake makePacket;
		makePacket.ID = idPacket.ID;
		makePacket.Type = e_Type::STAR_MAKE;
		makePacket.X = g_startInfoArr[infoIdx]._X;
		makePacket.Y = g_startInfoArr[infoIdx]._Y;

		///////////////////////////////////////////////
		// 모든 클라에게 브로드캐스팅
		///////////////////////////////////////////////
		SendPacketBroadcast((char*)&makePacket, sizeof(st_StarMake));

		return true;
	}

	//  TODO : 빈 세션 검색.. 없으면 closesocket?
	return false;
}

// 클라에게 받은 패킷 처리
bool PacketRecvProc(FD_SET* setParam, int countNum)
{
	int isSetCount = countNum;

	for (int i = 0; i < MAX_ARRNUM; i++)
	{
		// 일단 다 돌긴 도는데
		// ret_select 만큼 ISSET 반응하면 break;
		if (FD_ISSET(g_startInfoArr[i]._sock, setParam) == 0)
		{
			continue;
		}

		char buf[16] = { 0, };
		int recvRet = recvn(g_startInfoArr[i]._sock, buf, 16);
		wcout << L"recved : " << recvRet << endl;

		if (recvRet < 0)
		{
			return false;
		}

		///////////////////////////////////////////////
		// 해당 클라 제외한 다른 클라에게 이동 패킷 브로드캐스팅
		///////////////////////////////////////////////
		st_Packet* packetPtr = (st_Packet*)buf;
		if (packetPtr->Type == e_Type::STAR_MOVE)
		{
			st_StarMove movePacket;
			movePacket.ID = g_startInfoArr[i]._ID;
			movePacket.Type = e_Type::STAR_MOVE;
			movePacket.X = g_startInfoArr[i]._X;
			movePacket.Y = g_startInfoArr[i]._Y;

			SendPacketBroadcast((char*)&movePacket, sizeof(st_StarMove), i);
		}

		--isSetCount;

		if (isSetCount == 0)
		{
			return true;
		}
	}
	// TODO : close socket 처리? 다 돌았는데 소켓이 없거나 0을 받은 경우?
	// TODO : delete 해야함

	return false;
}