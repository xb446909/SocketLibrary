// SocketLibrary.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include <utility>
#include <map>
#include <vector>
#include "SocketLibrary.h"

using namespace std;

#pragma comment(lib, "Ws2_32.lib")

#define  UM_QUIT	(WM_USER + 1)

#define INT_DATABUFFER_SIZE (1024)

typedef struct _tagSocketParameter
{
	SOCKET ConnectSocket;
	SOCKET AcceptSocket;
	int nType;
	BOOL bOK;
	HANDLE hEvent;
	DWORD dwThreadID;
	RecvCallback pCallback;
	char szDataBuff[INT_DATABUFFER_SIZE];
	char szIniPath[MAX_PATH];
}SocketParameter, *pSocketParameter;

typedef struct _tagRecvSocket
{
	SOCKET      AcceptSocket;
	sockaddr_in addr;

	bool operator==(const _tagRecvSocket& src)
	{
		return ((AcceptSocket == src.AcceptSocket) &&
			(addr.sin_addr.S_un.S_addr == src.addr.sin_addr.S_un.S_addr) &&
			(addr.sin_family == src.addr.sin_family) &&
			(addr.sin_port == src.addr.sin_port));
	}
}RecvSocket, *pRecvSocket;

map<int, pSocketParameter> g_socketMap;
vector<RecvSocket> g_vecRecvSocket;
HANDLE g_mutex;

BOOL BindSocket(int nID, int nType);
DWORD WINAPI TCPListenReceiveThread(LPVOID lpParam);
DWORD WINAPI UDPReceiveThread(LPVOID lpParam);
void DeleteAddr(sockaddr_in addr);
bool CompareAddr(sockaddr_in addr1, sockaddr_in addr2);

bool CompareAddr(sockaddr_in addr1, sockaddr_in addr2)
{
	return ((addr1.sin_addr.S_un.S_addr == addr2.sin_addr.S_un.S_addr) &&
		(addr1.sin_family == addr2.sin_family) &&
		(addr1.sin_port == addr2.sin_port));
}

void DeleteAddr(sockaddr_in addr)
{
	for (size_t i = 0; i < g_vecRecvSocket.size(); i++)
	{
		if (CompareAddr(addr, g_vecRecvSocket[i].addr)) g_vecRecvSocket.erase(g_vecRecvSocket.begin() + i);
	}
}

pSocketParameter FindSockParam(int nID)
{
	auto it = g_socketMap.find(nID);
	if (it == g_socketMap.end())
	{
		return nullptr;
	}
	return it->second;
}

void UninitSocket(int nID)
{
	WaitForSingleObject(g_mutex, INFINITE);

	pSocketParameter pSockParam;
	pSockParam = FindSockParam(nID);
	if (pSockParam == nullptr)
	{
		return;
	}
	if (pSockParam->nType == TCP_SERVER)
	{
		PostThreadMessage(pSockParam->dwThreadID, UM_QUIT, 0, 0);
	}
	closesocket(pSockParam->ConnectSocket);
	WSACleanup();
	pSockParam->bOK = FALSE;

	delete pSockParam;
	g_socketMap.erase(nID);

	ReleaseMutex(g_mutex);
}

int InitSocket(int nID, int nType, const char* szIniPath, RecvCallback pCallback)
{
	int iResult;
	WSADATA wsaData;
	//是否已经有绑定的socket，若已经初始化则先卸载
	pSocketParameter pSockParam = FindSockParam(nID);
	g_mutex = CreateMutex(NULL, FALSE, NULL);
	if (pSockParam != nullptr)
	{
		UninitSocket(nID);
	}
	pSockParam = new SocketParameter;

	//----------------------
	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR) {
		fstream fs(".\\SocketErrorLog.txt", ios::out | ios::in | ios::app);
		fs << "Initialize socket error: " << WSAGetLastError() << endl;
		return SOCK_ERROR;
	}
	switch (nType)
	{
	case TCP_SERVER:
	case TCP_CLIENT:
		pSockParam->ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);//(S1)
		break;
	case UDP_SERVER:
	case UDP_CLIENT:
		pSockParam->ConnectSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		break;
	default:
		WSACleanup();
		fstream fs(".\\SocketErrorLog.txt", ios::out | ios::in | ios::app);
		fs << "Socket type is error" << endl;

		return SOCK_ERROR;
	}

	if (pSockParam->ConnectSocket == INVALID_SOCKET)
	{
		fstream fs(".\\SocketErrorLog.txt", ios::out | ios::in | ios::app);
		fs << "Create socket error: " << WSAGetLastError() << endl;
		WSACleanup();
		return SOCK_ERROR;
	}

	if (szIniPath != NULL)
	{
		strcpy(pSockParam->szIniPath, szIniPath);
	}
	else
	{
		strcpy(pSockParam->szIniPath, ".\\SocketConfig.ini");
	}

	pSockParam->pCallback = pCallback;
	pSockParam->nType = nType;
	g_socketMap.insert(pair<int, pSocketParameter>(nID, pSockParam));

	if (nType == TCP_SERVER || nType == UDP_SERVER)
	{
		BOOL bOptVal = TRUE;
		if (nType == TCP_SERVER)
			setsockopt(pSockParam->ConnectSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&bOptVal, sizeof(bOptVal));
		else
			setsockopt(pSockParam->ConnectSocket, SOL_SOCKET, SO_BROADCAST, (char *)&bOptVal, sizeof(BOOL));

		if (BindSocket(nID, nType) == FALSE) return SOCK_ERROR; //(S2, S3)
		pSockParam->bOK = TRUE;
	}

	return SOCK_SUCCESS;
}

int TCPConnect(int nID, int nTimeoutMs)
{
	pSocketParameter pSockParam = FindSockParam(nID);
	if (pSockParam == nullptr)
	{
		return SOCK_ERROR;
	}
	if (pSockParam->nType != TCP_CLIENT)
	{
		fstream fs(".\\SocketErrorLog.txt", ios::out | ios::in | ios::app);
		fs << "The socket type is incorrect" << endl;
		return SOCK_ERROR;
	}
	char strIpAddr[64] = { 0 };
	char strPort[64] = { 0 };
	char strApp[128] = { 0 };
	sprintf(strApp, "TCP Client%d", nID);

	GetPrivateProfileStringA(strApp, "Server IP Address", "127.0.0.1", strIpAddr, 63, pSockParam->szIniPath);
	WritePrivateProfileStringA(strApp, "Server IP Address", strIpAddr, pSockParam->szIniPath);
	GetPrivateProfileStringA(strApp, "Server Port", "10000", strPort, 63, pSockParam->szIniPath);
	WritePrivateProfileStringA(strApp, "Server Port", strPort, pSockParam->szIniPath);

	struct sockaddr_in clientService;

	//----------------------
	// The sockaddr_in structure specifies the address family,
	// IP address, and port of the server to be connected to.
	clientService.sin_family = AF_INET;
	clientService.sin_addr.s_addr = inet_addr(strIpAddr);
	clientService.sin_port = htons(atoi(strPort));

	u_long iMode = 1;
	ioctlsocket(pSockParam->ConnectSocket, FIONBIO, &iMode);

	//----------------------
	// Connect to server.
	connect(pSockParam->ConnectSocket, (SOCKADDR*)&clientService, sizeof(clientService));

	fd_set r;
	FD_ZERO(&r);
	FD_SET(pSockParam->ConnectSocket, &r);
	struct timeval timeout;
	timeout.tv_sec = nTimeoutMs / 1000;
	timeout.tv_usec = (nTimeoutMs % 1000) * 1000;
	int ret = select(0, 0, &r, 0, &timeout);

	if (ret <= 0)
	{
		fstream fs(".\\SocketErrorLog.txt", ios::out | ios::in | ios::app);
		fs << "Connect server timeout" << endl;
		closesocket(pSockParam->ConnectSocket);
		WSACleanup();
		return SOCK_TIMEOUT;
	}
	pSockParam->bOK = TRUE;
	return SOCK_SUCCESS;
}

int TCPSend(int nID, char* szDstIP, int nDstPort, char* szSendBuf)
{
	pSocketParameter pSockParam = FindSockParam(nID);
	if (pSockParam == nullptr)
	{
		return SOCK_ERROR;
	}
	if (!pSockParam->bOK)
	{
		fstream fs(".\\SocketErrorLog.txt", ios::out | ios::in | ios::app);
		fs << "Send error, socket is closed, first time" << endl;
		TCPConnect(nID, 1000);
		if (!pSockParam->bOK)
		{
			fs << "Send error, socket is closed, second time" << endl;
			return SOCK_ERROR;
		}
	}
	int iResult, res;
	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));

	switch (pSockParam->nType)
	{
	case TCP_SERVER:
		if (szDstIP && strlen(szDstIP) > 0)
		{
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = inet_addr(szDstIP);
			addr.sin_port = htons(nDstPort);
			SOCKET AcceptSocket;
			res = SOCK_ERROR;
			for (size_t i = 0; i < g_vecRecvSocket.size(); i++)
			{
				if (CompareAddr(addr, g_vecRecvSocket[i].addr))
				{
					AcceptSocket = g_vecRecvSocket[i].AcceptSocket;
					res = 0;
				}
			}
			if (res == SOCK_ERROR)
				return SOCK_ERROR;
			iResult = send(AcceptSocket, szSendBuf, strlen(szSendBuf) + 1, 0);
		}
		break;

	case TCP_CLIENT:
		iResult = send(pSockParam->ConnectSocket, szSendBuf, strlen(szSendBuf) + 1, 0);
		break;

	default:
		break;
	}


	if (iResult == SOCKET_ERROR)
	{
		UninitSocket(nID);
		fstream fs(".\\SocketErrorLog.txt", ios::out | ios::in | ios::app);
		fs << "Send error! error code:" << WSAGetLastError() << endl;
		return SOCK_ERROR;
	}
	return SOCK_SUCCESS;
}

int UDPSend(int nID, char* szDstIP, int nDstPort, char* szSendBuf)
{
	pSocketParameter pSockParam = FindSockParam(nID);
	if (pSockParam == nullptr)
	{
		return SOCK_ERROR;
	}

	int iResult, res;
	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	if (szDstIP && strlen(szDstIP) > 0)
	{
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = inet_addr(szDstIP);
		addr.sin_port = htons(nDstPort);

		switch (pSockParam->nType)
		{
		case UDP_CLIENT:
		case UDP_SERVER:
			iResult = sendto(pSockParam->ConnectSocket, szSendBuf, strlen(szSendBuf) + 1, 0, (SOCKADDR *)&addr, sizeof(SOCKADDR));
			break;
		default:
			break;
		}
	}
	else
	{
		fstream fs(".\\SocketErrorLog.txt", ios::out | ios::in | ios::app);
		fs << "Send error! error code: input address error!"<< endl;
		return SOCK_ERROR;
	}

	if (iResult == SOCKET_ERROR)
	{
		UninitSocket(nID);
		fstream fs(".\\SocketErrorLog.txt", ios::out | ios::in | ios::app);
		fs << "Send error! error code:" << WSAGetLastError() << endl;
		return SOCK_ERROR;
	}
	return SOCK_SUCCESS;
}

int TCPRecv(int nID, char* szRecvBuf, int nBufLen, int nTimeoutMs)
{
	pSocketParameter pSockParam = FindSockParam(nID);
	if (pSockParam == nullptr)
	{
		return SOCK_ERROR;
	}
	fd_set r;
	FD_ZERO(&r);
	//	recv(pSockParam->ConnectSocket, szRecvBuf, nBufLen, 0);
	FD_SET(pSockParam->ConnectSocket, &r);

	struct timeval timeout;
	timeout.tv_sec = nTimeoutMs / 1000;
	timeout.tv_usec = (nTimeoutMs % 1000) * 1000;
	int ret = select(0, &r, 0, 0, &timeout);

	if (ret <= 0)
	{
		fstream fs(".\\SocketErrorLog.txt", ios::out | ios::in | ios::app);
		fs << "Receive timeout!" << endl;
		return SOCK_TIMEOUT;
	}
	else
	{
		if (pSockParam->nType == TCP_CLIENT)
		{
			recv(pSockParam->ConnectSocket, szRecvBuf, nBufLen, 0);
		}
	}
	return SOCK_SUCCESS;
}

int UDPRecv(int nID, char* szRecvBuf, int nBufLen, int nTimeoutMs)
{
	pSocketParameter pSockParam = FindSockParam(nID);
	if (pSockParam == nullptr)
	{
		return SOCK_ERROR;
	}
	fd_set r;
	FD_ZERO(&r);
    FD_SET(pSockParam->ConnectSocket, &r);

	struct timeval timeout;
	timeout.tv_sec = nTimeoutMs / 1000;
	timeout.tv_usec = (nTimeoutMs % 1000) * 1000;
	int ret = select(0, &r, 0, 0, &timeout);

	if (ret <= 0)
	{
		fstream fs(".\\SocketErrorLog.txt", ios::out | ios::in | ios::app);
		fs << "Receive timeout!" << endl;
		return SOCK_TIMEOUT;
	}
	else
	{
		if (pSockParam->nType == UDP_CLIENT)
		{
			sockaddr_in addrAccept;
			int nAcceptLen = sizeof(addrAccept);
			memset(&addrAccept, 0, sizeof(addrAccept));
			memset(szRecvBuf, 0, INT_DATABUFFER_SIZE);
			recvfrom(pSockParam->ConnectSocket, szRecvBuf, INT_DATABUFFER_SIZE, 0, (SOCKADDR*)&addrAccept, &nAcceptLen);
		}
	}
	return SOCK_SUCCESS;
}

BOOL BindSocket(int nID, int nType)
{
	pSocketParameter pSockParam = FindSockParam(nID);
	if (pSockParam == nullptr)
	{
		return FALSE;
	}

	char strPort[64] = { 0 };
	char strIp[64] = { 0 };
	char strApp[128] = { 0 };
	if (nType == TCP_SERVER)
		sprintf(strApp, "TCP Server%d", nID);
	else
		sprintf(strApp, "UDP Server%d", nID);

	GetPrivateProfileStringA(strApp, "Server Ip", "127.0.0.1", strIp, 63, pSockParam->szIniPath);
	WritePrivateProfileStringA(strApp, "Server Ip", strIp, pSockParam->szIniPath);
	GetPrivateProfileStringA(strApp, "Server Port", "10000", strPort, 63, pSockParam->szIniPath);
	WritePrivateProfileStringA(strApp, "Server Port", strPort, pSockParam->szIniPath);

	struct sockaddr_in clientService;

	//----------------------
	// The sockaddr_in structure specifies the address family,
	// IP address, and port of the server to be connected to.
	clientService.sin_family = AF_INET;
	clientService.sin_addr.s_addr = inet_addr(strIp);
	clientService.sin_port = htons(atoi(strPort));

	//----------------------
	// Bind the socket.//(S2)
	if (bind(pSockParam->ConnectSocket, (SOCKADDR*)&clientService, sizeof(clientService)) == SOCKET_ERROR)
	{
		fstream fs(".\\SocketErrorLog.txt", ios::out | ios::in | ios::app);
		fs << "Bind socket error: " << WSAGetLastError() << endl;
		UninitSocket(nID);
		return FALSE;
	}
	if (nType == TCP_SERVER)
	{
		HANDLE hThread = CreateThread(NULL, 0, TCPListenReceiveThread, (LPVOID)nID, 0, &pSockParam->dwThreadID);//(S3)
	}
	else
	{
		HANDLE hThread = CreateThread(NULL, 0, UDPReceiveThread, (LPVOID)nID, 0, &pSockParam->dwThreadID);//(S3)
	}
	WaitForSingleObject(pSockParam->hEvent, INFINITE);
	ResetEvent(pSockParam->hEvent);

	return TRUE;
}

DWORD WINAPI UDPReceiveThread(LPVOID lpParam)
{
	sockaddr_in addrAccept;
	int nAcceptLen = sizeof(addrAccept);
	int nRecvRes;
	int nID = (int)lpParam;
	pSocketParameter pSockParam = FindSockParam(nID);
	if (pSockParam == nullptr)
	{
		return FALSE;
	}
	MSG msg;
	PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);
	SetEvent(pSockParam->hEvent);

	while (1)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == UM_QUIT)
			{
				UninitSocket(nID);
				return SOCK_SUCCESS;
			}
		}
		memset(&addrAccept, 0, sizeof(addrAccept));
		memset(pSockParam->szDataBuff, 0, INT_DATABUFFER_SIZE);

		nRecvRes = recvfrom(pSockParam->ConnectSocket, pSockParam->szDataBuff, 100, 0, (sockaddr *)&addrAccept, &nAcceptLen);
		if (nRecvRes == SOCK_ERROR)
		{
			if (pSockParam->pCallback != NULL)
				pSockParam->pCallback(RECV_ERROR, addrAccept, 0, NULL);
			continue;
		}

		if (0 < nRecvRes)
		{
			//打印接收的数据 
			if (pSockParam->pCallback != NULL)
				pSockParam->pCallback(RECV_DATA, addrAccept, nRecvRes, pSockParam->szDataBuff);
		}
	}
	return SOCK_SUCCESS;
}

DWORD WINAPI TCPListenReceiveThread(LPVOID lpParam)
{
	int nID = (int)lpParam;
	pSocketParameter pSockParam = FindSockParam(nID);
	if (pSockParam == nullptr)
	{
		return FALSE;
	}

	MSG msg;
	PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);
	SetEvent(pSockParam->hEvent);

	fd_set fd;
	FD_ZERO(&fd);
	FD_SET(pSockParam->ConnectSocket, &fd);

	int iResult;
	int iRecvSize;
	sockaddr_in addrAccept, addrTemp;
	int iAcceptLen = sizeof(addrAccept);
	int iTempLen = sizeof(addrTemp);

	if (listen(pSockParam->ConnectSocket, SOMAXCONN) == SOCKET_ERROR)//(S3)
	{
		UninitSocket(nID);
		return SOCK_ERROR;
	}

	RecvSocket RecvS;
	while (1)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == UM_QUIT)
			{
				UninitSocket(nID);
				return SOCK_SUCCESS;
			}
		}
		fd_set fdOld = fd;
		struct timeval timeout;
		timeout.tv_sec = 0;
		timeout.tv_usec = 50000;
		iResult = select(0, &fdOld, NULL, NULL, &timeout);
		if (0 < iResult)
		{
			for (size_t i = 0; i < fd.fd_count; i++)
			{
				if (FD_ISSET(fd.fd_array[i], &fdOld))
				{
					//如果socket是服务器，则接收连接  
					if (fd.fd_array[i] == pSockParam->ConnectSocket)
					{
						memset(&addrAccept, 0, sizeof(addrAccept));
						pSockParam->AcceptSocket = accept(pSockParam->ConnectSocket, (sockaddr *)&addrAccept, &iAcceptLen);//(S4)

						if (INVALID_SOCKET != pSockParam->AcceptSocket)
						{
							if (pSockParam->pCallback != NULL)
								pSockParam->pCallback(RECV_SOCKET, addrAccept, 0, NULL);
							FD_SET(pSockParam->AcceptSocket, &fd);

							RecvS.AcceptSocket = pSockParam->AcceptSocket;
							memcpy(&RecvS.addr, &addrAccept, sizeof(sockaddr_in));
							g_vecRecvSocket.push_back(RecvS);
						}
					}
					else //非服务器,接收数据(因为fd是读数据集)    
					{
						memset(pSockParam->szDataBuff, 0, INT_DATABUFFER_SIZE);
						iRecvSize = recv(fd.fd_array[i], pSockParam->szDataBuff, INT_DATABUFFER_SIZE, 0);
						memset(&addrTemp, 0, sizeof(addrTemp));
						iTempLen = sizeof(addrTemp);
						getpeername(fd.fd_array[i], (sockaddr *)&addrTemp, &iTempLen);

						if (SOCKET_ERROR == iRecvSize)
						{
							if (pSockParam->pCallback != NULL)
								pSockParam->pCallback(RECV_ERROR, addrTemp, 0, NULL);
							closesocket(fd.fd_array[i]);
							FD_CLR(fd.fd_array[i], &fd);
							i--;
							continue;
						}

						if (0 == iRecvSize)
						{
							//客户socket关闭    
							if (pSockParam->pCallback != NULL)
								pSockParam->pCallback(RECV_CLOSE, addrTemp, 0, NULL);
							closesocket(fd.fd_array[i]);
							FD_CLR(fd.fd_array[i], &fd);
							DeleteAddr(addrTemp);
							i--;
						}

						if (0 < iRecvSize)
						{
							//打印接收的数据    
							if (pSockParam->pCallback != NULL)
								pSockParam->pCallback(RECV_DATA, addrTemp, iRecvSize, pSockParam->szDataBuff);
						}
					}
				}
			}
		}
		else if (SOCKET_ERROR == iResult)
		{
			WSACleanup();
			fstream fs(".\\SocketErrorLog.txt", ios::out | ios::in | ios::app);
			fs << "Failed to select socket, error: " << WSAGetLastError() << endl;
			Sleep(100);
		}
	}
	return SOCK_SUCCESS;
}