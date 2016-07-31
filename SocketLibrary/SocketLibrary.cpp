// SocketLibrary.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include <winsock2.h>
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
	int nType;
	BOOL bOK;
	HANDLE hEvent;
	DWORD dwThreadID;
	RecvCallback pCallback;
	char szDataBuff[INT_DATABUFFER_SIZE];
	char szIniPath[MAX_PATH];
}SocketParameter, *pSocketParameter;

map<int, SocketParameter> g_socketMap;

BOOL BindSocket(int nID);
DWORD WINAPI ListenReceiveThread( LPVOID lpParam );

BOOL FindSockParam(int nID, SocketParameter& sockParam)
{
	auto it = g_socketMap.find(nID);
	if (it == g_socketMap.end())
	{
		return FALSE;
	}
	sockParam = it->second;
	return TRUE;
}

void _stdcall UninitSocket(int nID)
{
	SocketParameter sockParam;
	if (!FindSockParam(nID, sockParam))
	{
		return;
	}
	if (sockParam.nType == TCP_SERVER)
	{
		PostThreadMessage(sockParam.dwThreadID, UM_QUIT, 0, 0);
	}
	closesocket(sockParam.ConnectSocket);
	WSACleanup();
	sockParam.bOK = FALSE;
}

BOOL _stdcall InitSocket(int nID, int nType, const char* szIniPath, RecvCallback pCallback)
{
	int iResult;
	WSADATA wsaData;

	SocketParameter sockParam;
	if (FindSockParam(nID, sockParam))
	{
		UninitSocket(nID);
		g_socketMap.erase(nID);
	}

	//----------------------
	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != NO_ERROR) {
		fstream fs(".\\ErrorLog.txt", ios::out | ios::in | ios::app);
		fs << "Initialize socket error: " << WSAGetLastError() << endl;
		return FALSE;
	}

	switch (nType)
	{
	case TCP_SERVER:
	case TCP_CLIENT:
		sockParam.ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		break;
	case UDP_SERVER:
	case UDP_CLIENT:
		sockParam.ConnectSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		break;
	default:
		WSACleanup();
		fstream fs(".\\ErrorLog.txt", ios::out | ios::in | ios::app);
		fs << "Socket type is error" << endl;
		return FALSE;
	}

	if (sockParam.ConnectSocket == INVALID_SOCKET)
	{
		fstream fs(".\\ErrorLog.txt", ios::out | ios::in | ios::app);
		fs << "Create socket error: " << WSAGetLastError() << endl;
		WSACleanup();
		return FALSE;
	}

	if (szIniPath != NULL)
	{
		strcpy(sockParam.szIniPath, szIniPath);
	}
	else
	{
		strcpy(sockParam.szIniPath, ".\\SocketConfig.ini");
	}
	sockParam.pCallback = pCallback;
	sockParam.nType = nType;
	g_socketMap.insert(pair<int, SocketParameter>(nID, sockParam));

	if (nType == TCP_SERVER)
	{
		BOOL bOptVal = TRUE;
		setsockopt(sockParam.ConnectSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&bOptVal, sizeof(bOptVal));
		if (BindSocket(nID) == FALSE) return FALSE;
	}

	return TRUE;
}

BOOL _stdcall TCPConnect(int nID, int nTimeoutMs)
{
	SocketParameter sockParam;
	if (!FindSockParam(nID, sockParam))
	{
		return FALSE;
	}
	if (sockParam.nType != TCP_CLIENT)
	{
		fstream fs(".\\ErrorLog.txt", ios::out | ios::in | ios::app);
		fs << "The socket type is incorrect" << endl;
		return FALSE;
	}
	char strIpAddr[64] = {0};
	char strPort[64] = {0};

	GetPrivateProfileStringA("TCP Client", "Server IP Address", "127.0.0.1", strIpAddr, 63, sockParam.szIniPath);
	WritePrivateProfileStringA("TCP Client", "Server IP Address", strIpAddr, sockParam.szIniPath);
	GetPrivateProfileStringA("TCP Client", "Server Port", "10000", strPort, 63, sockParam.szIniPath);
	WritePrivateProfileStringA("TCP Client", "Server Port", strPort, sockParam.szIniPath);

	struct sockaddr_in clientService; 

	//----------------------
	// The sockaddr_in structure specifies the address family,
	// IP address, and port of the server to be connected to.
	clientService.sin_family = AF_INET;
	clientService.sin_addr.s_addr = inet_addr( strIpAddr );
	clientService.sin_port = htons(atoi(strPort));

	u_long iMode = 1;
	ioctlsocket(sockParam.ConnectSocket, FIONBIO, &iMode);

	//----------------------
	// Connect to server.
	connect(sockParam.ConnectSocket, (SOCKADDR*) &clientService, sizeof(clientService) );

	fd_set r;
	FD_ZERO(&r);
	FD_SET(sockParam.ConnectSocket, &r);
	struct timeval timeout;
	timeout.tv_sec = nTimeoutMs / 1000;
	timeout.tv_usec = (nTimeoutMs % 1000) * 1000;
	int ret = select(0, 0, &r, 0, &timeout);

	if (ret <= 0)
	{
		fstream fs(".\\ErrorLog.txt", ios::out | ios::in | ios::app);
		fs << "Connect server timeout" << endl;
		closesocket(sockParam.ConnectSocket);
		WSACleanup();
		return FALSE;
	}
	sockParam.bOK = TRUE;
	return TRUE;
}

BOOL _stdcall TCPSend(int nID, char* szSendBuf)
{
	SocketParameter sockParam;
	if (!FindSockParam(nID, sockParam))
	{
		return FALSE;
	}
	if (!sockParam.bOK)
	{
		fstream fs(".\\ErrorLog.txt", ios::out | ios::in | ios::app);
		fs << "Send error, socket is closed" << endl;
		return FALSE;
	}
	int iResult = send(sockParam.ConnectSocket, szSendBuf, strlen(szSendBuf), 0);
	if (iResult == SOCKET_ERROR) 
	{
		UninitSocket(nID);
		fstream fs(".\\ErrorLog.txt", ios::out | ios::in | ios::app);
		fs << "Send error! error code:" << WSAGetLastError() << endl;
		return FALSE;
	}
	return TRUE;
}

BOOL _stdcall TCPRecv(int nID, char* szRecvBuf, int nBufLen, int nTimeoutMs)
{
	SocketParameter sockParam;
	if (!FindSockParam(nID, sockParam))
	{
		return FALSE;
	}
	recv(sockParam.ConnectSocket, szRecvBuf, nBufLen, 0);

	fd_set r;
	FD_ZERO(&r);
	FD_SET(sockParam.ConnectSocket, &r);
	struct timeval timeout;
	timeout.tv_sec = nTimeoutMs / 1000;
	timeout.tv_usec = (nTimeoutMs % 1000) * 1000;
	int ret = select(0, &r, 0, 0, &timeout);

	if (ret <= 0)
	{
		fstream fs(".\\ErrorLog.txt", ios::out | ios::in | ios::app);
		fs << "Receive timeout!" << endl;
		return FALSE;
	}
	else
	{
		recv(sockParam.ConnectSocket, szRecvBuf, nBufLen, 0);
	}
	return TRUE;
}


BOOL BindSocket(int nID)
{
	SocketParameter sockParam;
	if (!FindSockParam(nID, sockParam))
	{
		return FALSE;
	}

	char strPort[64] = {0};

	GetPrivateProfileStringA("TCP Server", "Server Port", "10000", strPort, 63, sockParam.szIniPath);
	WritePrivateProfileStringA("TCP Server", "Server Port", strPort, sockParam.szIniPath);

	struct sockaddr_in clientService; 

	//----------------------
	// The sockaddr_in structure specifies the address family,
	// IP address, and port of the server to be connected to.
	clientService.sin_family = AF_INET;
	clientService.sin_addr.s_addr = INADDR_ANY;
	clientService.sin_port = htons(atoi(strPort));

	//----------------------
	// Bind the socket.
	if (bind(sockParam.ConnectSocket,
		(SOCKADDR*) &clientService, 
		sizeof(clientService)) == SOCKET_ERROR) {
			fstream fs(".\\ErrorLog.txt", ios::out | ios::in | ios::app);
			fs << "Bind socket error: " << WSAGetLastError() << endl;
			UninitSocket(nID);
			return FALSE;
	}
	HANDLE hThread = CreateThread(NULL, 0, ListenReceiveThread, (LPVOID)nID, 0, &sockParam.dwThreadID);
	WaitForSingleObject(sockParam.hEvent, INFINITE);
	ResetEvent(sockParam.hEvent);

	return TRUE;
}


DWORD WINAPI ListenReceiveThread( LPVOID lpParam )
{
	int nID = (int)lpParam;
	SocketParameter sockParam;
	if (!FindSockParam(nID, sockParam))
	{
		return FALSE;
	}

	MSG msg;
	PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);
	SetEvent(sockParam.hEvent);

	fd_set fd;    
	FD_ZERO(&fd);    
	FD_SET(sockParam.ConnectSocket, &fd);

	int iResult;
	int iRecvSize;
	sockaddr_in addrAccept, addrTemp;
	int iAcceptLen = sizeof(addrAccept);
	int iTempLen = sizeof(addrTemp);
	SOCKET sockAccept;

	if (listen(sockParam.ConnectSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		UninitSocket(nID);
		return 0;
	}


	while(1)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == UM_QUIT)
			{
				UninitSocket(nID);
				return 0;
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
					if (fd.fd_array[i] == sockParam.ConnectSocket)
					{    
						memset(&addrAccept, 0, sizeof(addrAccept)); 
						sockAccept = accept(sockParam.ConnectSocket, (sockaddr *)&addrAccept, &iAcceptLen);
						if (INVALID_SOCKET != sockAccept)
						{
							if(sockParam.pCallback != NULL)
								sockParam.pCallback(RECV_SOCKET, addrAccept, 0, NULL);
							FD_SET(sockAccept, &fd);
						}    
					}    
					else //非服务器,接收数据(因为fd是读数据集)    
					{    
						memset(sockParam.szDataBuff, 0, INT_DATABUFFER_SIZE);
						iRecvSize = recv(fd.fd_array[i], sockParam.szDataBuff, INT_DATABUFFER_SIZE, 0);
						memset(&addrTemp, 0, sizeof(addrTemp));
						iTempLen = sizeof(addrTemp);    
						getpeername(fd.fd_array[i], (sockaddr *)&addrTemp, &iTempLen);

						if (SOCKET_ERROR == iRecvSize)    
						{
							if (sockParam.pCallback != NULL)
								sockParam.pCallback(RECV_ERROR, addrTemp, 0, NULL);
							closesocket(fd.fd_array[i]);
							FD_CLR(fd.fd_array[i],&fd);    
							i--;    
							continue;    
						}    

						if (0 == iRecvSize)    
						{    
							//客户socket关闭    
							if (sockParam.pCallback != NULL)
								sockParam.pCallback(RECV_CLOSE, addrTemp, 0, NULL);
							closesocket(fd.fd_array[i]);    
							FD_CLR(fd.fd_array[i], &fd);    
							i--;        
						}    

						if (0 < iRecvSize)    
						{    
							//打印接收的数据    
							if (sockParam.pCallback != NULL)
								sockParam.pCallback(RECV_DATA, addrTemp, iRecvSize, sockParam.szDataBuff);
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
	return 0;
}