// SocketLibrary.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include <utility>
#include <map>
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

map<int, pSocketParameter> g_socketMap;

BOOL BindSocket(int nID);
DWORD WINAPI ListenReceiveThread( LPVOID lpParam );

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
	pSocketParameter pSockParam;
	pSockParam = FindSockParam(nID);
	if (pSockParam == nullptr )
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
}

int InitSocket(int nID, int nType, const char* szIniPath, RecvCallback pCallback)
{
	int iResult;
	WSADATA wsaData;

	pSocketParameter pSockParam = FindSockParam(nID);
	if (pSockParam != nullptr)
	{
		UninitSocket(nID);
	}

	pSockParam = new SocketParameter;

	//----------------------
	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != NO_ERROR) {
		fstream fs(".\\ErrorLog.txt", ios::out | ios::in | ios::app);
		fs << "Initialize socket error: " << WSAGetLastError() << endl;
		return SOCK_ERROR;
	}

	switch (nType)
	{
	case TCP_SERVER:
	case TCP_CLIENT:
		pSockParam->ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		break;
	case UDP_SERVER:
	case UDP_CLIENT:
		pSockParam->ConnectSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		break;
	default:
		WSACleanup();
		fstream fs(".\\ErrorLog.txt", ios::out | ios::in | ios::app);
		fs << "Socket type is error" << endl;
		return SOCK_ERROR;
	}

	if (pSockParam->ConnectSocket == INVALID_SOCKET)
	{
		fstream fs(".\\ErrorLog.txt", ios::out | ios::in | ios::app);
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

	if (nType == TCP_SERVER)
	{
		BOOL bOptVal = TRUE;
		setsockopt(pSockParam->ConnectSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&bOptVal, sizeof(bOptVal));
		if (BindSocket(nID) == FALSE) return SOCK_ERROR;
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
		fstream fs(".\\ErrorLog.txt", ios::out | ios::in | ios::app);
		fs << "The socket type is incorrect" << endl;
		return SOCK_ERROR;
	}
	char strIpAddr[64] = {0};
	char strPort[64] = {0};
	char strApp[128] = {0};
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
	clientService.sin_addr.s_addr = inet_addr( strIpAddr );
	clientService.sin_port = htons(atoi(strPort));

	u_long iMode = 1;
	ioctlsocket(pSockParam->ConnectSocket, FIONBIO, &iMode);

	//----------------------
	// Connect to server.
	connect(pSockParam->ConnectSocket, (SOCKADDR*) &clientService, sizeof(clientService) );

	fd_set r;
	FD_ZERO(&r);
	FD_SET(pSockParam->ConnectSocket, &r);
	struct timeval timeout;
	timeout.tv_sec = nTimeoutMs / 1000;
	timeout.tv_usec = (nTimeoutMs % 1000) * 1000;
	int ret = select(0, 0, &r, 0, &timeout);

	if (ret <= 0)
	{
		fstream fs(".\\ErrorLog.txt", ios::out | ios::in | ios::app);
		fs << "Connect server timeout" << endl;
		closesocket(pSockParam->ConnectSocket);
		WSACleanup();
		return SOCK_TIMEOUT;
	}
	pSockParam->bOK = TRUE;
	return SOCK_SUCCESS;
}

int TCPSend(int nID, char* szSendBuf)
{
	pSocketParameter pSockParam = FindSockParam(nID);
	if (pSockParam == nullptr)
	{
		return SOCK_ERROR;
	}
	if (!pSockParam->bOK)
	{
		fstream fs(".\\ErrorLog.txt", ios::out | ios::in | ios::app);
		fs << "Send error, socket is closed, first time" << endl;
		TCPConnect(nID, 1000);
		if (!pSockParam->bOK)
		{
			fs << "Send error, socket is closed, second time" << endl;
			return SOCK_ERROR;
		}
	}
	int iResult;
	if (pSockParam->nType == TCP_SERVER)
		iResult = send(pSockParam->AcceptSocket, szSendBuf, strlen(szSendBuf), 0);
	else
		iResult = send(pSockParam->ConnectSocket, szSendBuf, strlen(szSendBuf), 0);
	if (iResult == SOCKET_ERROR) 
	{
		UninitSocket(nID);
		fstream fs(".\\ErrorLog.txt", ios::out | ios::in | ios::app);
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
	recv(pSockParam->ConnectSocket, szRecvBuf, nBufLen, 0);

	fd_set r;
	FD_ZERO(&r);
	FD_SET(pSockParam->ConnectSocket, &r);
	struct timeval timeout;
	timeout.tv_sec = nTimeoutMs / 1000;
	timeout.tv_usec = (nTimeoutMs % 1000) * 1000;
	int ret = select(0, &r, 0, 0, &timeout);

	if (ret <= 0)
	{
		fstream fs(".\\ErrorLog.txt", ios::out | ios::in | ios::app);
		fs << "Receive timeout!" << endl;
		return SOCK_TIMEOUT;
	}
	else
	{
		recv(pSockParam->ConnectSocket, szRecvBuf, nBufLen, 0);
	}
	return SOCK_SUCCESS;
}


BOOL BindSocket(int nID)
{
	pSocketParameter pSockParam = FindSockParam(nID);
	if (pSockParam == nullptr)
	{
		return FALSE;
	}

	char strPort[64] = {0};
	char strIp[64] = {0};
	char strApp[128] = {0};
	sprintf(strApp, "TCP Server%d", nID);

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
	// Bind the socket.
	if (bind(pSockParam->ConnectSocket,
		(SOCKADDR*) &clientService, 
		sizeof(clientService)) == SOCKET_ERROR) {
			fstream fs(".\\ErrorLog.txt", ios::out | ios::in | ios::app);
			fs << "Bind socket error: " << WSAGetLastError() << endl;
			UninitSocket(nID);
			return FALSE;
	}
	HANDLE hThread = CreateThread(NULL, 0, ListenReceiveThread, (LPVOID)nID, 0, &pSockParam->dwThreadID);
	WaitForSingleObject(pSockParam->hEvent, INFINITE);
	ResetEvent(pSockParam->hEvent);

	return TRUE;
}


DWORD WINAPI ListenReceiveThread( LPVOID lpParam )
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

	if (listen(pSockParam->ConnectSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		UninitSocket(nID);
		return SOCK_ERROR;
	}


	while(1)
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
			for(int i = 0;i < fd.fd_count; i++)
			{    
				if (FD_ISSET(fd.fd_array[i], &fdOld))
				{    
					//如果socket是服务器，则接收连接    
					if (fd.fd_array[i] == pSockParam->ConnectSocket)
					{    
						memset(&addrAccept, 0, sizeof(addrAccept)); 
						pSockParam->AcceptSocket = accept(pSockParam->ConnectSocket, (sockaddr *)&addrAccept, &iAcceptLen);
						if (INVALID_SOCKET != pSockParam->AcceptSocket)
						{
							if(pSockParam->pCallback != NULL)
								pSockParam->pCallback(RECV_SOCKET, addrAccept, 0, NULL);
							FD_SET(pSockParam->AcceptSocket, &fd);
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
							FD_CLR(fd.fd_array[i],&fd);    
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
			fstream fs(".\\ErrorLog.txt", ios::out | ios::in | ios::app);
			fs << "Failed to select socket, error: " << WSAGetLastError() << endl;
			Sleep(100);    
		}
	}
	return SOCK_SUCCESS;
}