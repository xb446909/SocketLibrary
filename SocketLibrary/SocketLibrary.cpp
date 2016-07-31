// SocketLibrary.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include "SocketLibrary.h"

using namespace std;

#pragma comment(lib, "Ws2_32.lib")

#define  UM_QUIT	(WM_USER + 1)

#define INT_DATABUFFER_SIZE (10 * 1024 * 1024)

SOCKET g_ConnectSocket;
int g_nType;
BOOL g_bOK;
HANDLE g_hEvent;
DWORD g_dwThreadID;
RecvCallback g_pCallback = NULL;
char g_szDataBuff[INT_DATABUFFER_SIZE];
char g_szIniPath[MAX_PATH];


BOOL BindSocket();
DWORD WINAPI ListenReceiveThread( LPVOID lpParam );

void _stdcall UninitSocket()
{
	if (g_nType == TCP_SERVER)
	{
		PostThreadMessage(g_dwThreadID, UM_QUIT, 0, 0);
	}
	closesocket(g_ConnectSocket);
	WSACleanup();
	g_bOK = FALSE;
}

BOOL _stdcall InitSocket(int nType, const char* szIniPath)
{
	int iResult;
	WSADATA wsaData;

	UninitSocket();

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
		g_ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		break;
	case UDP_SERVER:
	case UDP_CLIENT:
		g_ConnectSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		break;
	default:
		WSACleanup();
		fstream fs(".\\ErrorLog.txt", ios::out | ios::in | ios::app);
		fs << "Socket type is error" << endl;
		return FALSE;
	}

	if (g_ConnectSocket == INVALID_SOCKET) 
	{
		fstream fs(".\\ErrorLog.txt", ios::out | ios::in | ios::app);
		fs << "Create socket error: " << WSAGetLastError() << endl;
		WSACleanup();
		return FALSE;
	}

	if (nType == TCP_SERVER)
	{
		BOOL bOptVal = TRUE;
		setsockopt(g_ConnectSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&bOptVal, sizeof(bOptVal));
		if (BindSocket() == FALSE) return FALSE;
	}

	if (szIniPath != NULL)
	{
		strcpy(g_szIniPath, szIniPath);
	}
	else
	{
		strcpy(g_szIniPath, "..\\SocketConfig.ini");
	}
	g_nType = nType;
	return TRUE;
}

BOOL _stdcall TCPConnect(int nTimeoutMs)
{
	if (g_nType != TCP_CLIENT)
	{
		fstream fs(".\\ErrorLog.txt", ios::out | ios::in | ios::app);
		fs << "The socket type is incorrect" << endl;
		return FALSE;
	}
	char strIpAddr[64] = {0};
	char strPort[64] = {0};

	GetPrivateProfileStringA("TCP Client", "Server IP Address", "127.0.0.1", strIpAddr, 63, g_szIniPath);
	WritePrivateProfileStringA("TCP Client", "Server IP Address", strIpAddr, g_szIniPath);
	GetPrivateProfileStringA("TCP Client", "Server Port", "10000", strPort, 63, g_szIniPath);
	WritePrivateProfileStringA("TCP Client", "Server Port", strPort, g_szIniPath);

	struct sockaddr_in clientService; 

	//----------------------
	// The sockaddr_in structure specifies the address family,
	// IP address, and port of the server to be connected to.
	clientService.sin_family = AF_INET;
	clientService.sin_addr.s_addr = inet_addr( strIpAddr );
	clientService.sin_port = htons(atoi(strPort));

	u_long iMode = 1;
	ioctlsocket(g_ConnectSocket, FIONBIO, &iMode);

	//----------------------
	// Connect to server.
	connect( g_ConnectSocket, (SOCKADDR*) &clientService, sizeof(clientService) );

	fd_set r;
	FD_ZERO(&r);
	FD_SET(g_ConnectSocket, &r);
	struct timeval timeout;
	timeout.tv_sec = nTimeoutMs / 1000;
	timeout.tv_usec = (nTimeoutMs % 1000) * 1000;
	int ret = select(0, 0, &r, 0, &timeout);

	if (ret <= 0)
	{
		fstream fs(".\\ErrorLog.txt", ios::out | ios::in | ios::app);
		fs << "Connect server timeout" << endl;
		closesocket(g_ConnectSocket);
		WSACleanup();
		return FALSE;
	}
	g_bOK = TRUE;
	return TRUE;
}

BOOL _stdcall TCPSend(char* szSendBuf)
{
	if (!g_bOK)	
	{
		fstream fs(".\\ErrorLog.txt", ios::out | ios::in | ios::app);
		fs << "Send error, socket is closed" << endl;
		return FALSE;
	}
	int iResult = send(g_ConnectSocket, szSendBuf, strlen(szSendBuf), 0);
	if (iResult == SOCKET_ERROR) 
	{
		UninitSocket();
		fstream fs(".\\ErrorLog.txt", ios::out | ios::in | ios::app);
		fs << "Send error! error code:" << WSAGetLastError() << endl;
		return FALSE;
	}
	return TRUE;
}

BOOL _stdcall TCPRecv(char* szRecvBuf, int nBufLen, int nTimeoutMs)
{
	recv(g_ConnectSocket, szRecvBuf, nBufLen, 0);

	fd_set r;
	FD_ZERO(&r);
	FD_SET(g_ConnectSocket, &r);
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
		recv(g_ConnectSocket, szRecvBuf, nBufLen, 0);
	}
	return TRUE;
}

BOOL _stdcall SetRecvCallback(RecvCallback pCallback)
{
	g_pCallback = pCallback;
	return TRUE;
}


BOOL BindSocket()
{
	char strPort[64] = {0};

	GetPrivateProfileStringA("TCP Server", "Server Port", "10000", strPort, 63, g_szIniPath);
	WritePrivateProfileStringA("TCP Server", "Server Port", strPort, g_szIniPath);

	struct sockaddr_in clientService; 

	//----------------------
	// The sockaddr_in structure specifies the address family,
	// IP address, and port of the server to be connected to.
	clientService.sin_family = AF_INET;
	clientService.sin_addr.s_addr = INADDR_ANY;
	clientService.sin_port = htons(atoi(strPort));

	//----------------------
	// Bind the socket.
	if (bind( g_ConnectSocket, 
		(SOCKADDR*) &clientService, 
		sizeof(clientService)) == SOCKET_ERROR) {
			fstream fs(".\\ErrorLog.txt", ios::out | ios::in | ios::app);
			fs << "Bind socket error: " << WSAGetLastError() << endl;
			UninitSocket();
			return FALSE;
	}
	HANDLE hThread = CreateThread(NULL, 0, ListenReceiveThread, NULL, 0, &g_dwThreadID);
	WaitForSingleObject(g_hEvent, INFINITE);
	ResetEvent(g_hEvent);

	return TRUE;
}


DWORD WINAPI ListenReceiveThread( LPVOID lpParam )
{
	MSG msg;
	PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);
	SetEvent(g_hEvent);

	fd_set fd;    
	FD_ZERO(&fd);    
	FD_SET(g_ConnectSocket, &fd);

	int iResult;
	int iRecvSize;
	sockaddr_in addrAccept, addrTemp;
	int iAcceptLen = sizeof(addrAccept);
	int iTempLen = sizeof(addrTemp);
	SOCKET sockAccept;

	if (listen(g_ConnectSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		UninitSocket();
		return 0;
	}


	while(1)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == UM_QUIT)
			{
				UninitSocket();
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
					if (fd.fd_array[i] == g_ConnectSocket)    
					{    
						memset(&addrAccept, 0, sizeof(addrAccept)); 
						sockAccept = accept(g_ConnectSocket, (sockaddr *)&addrAccept, &iAcceptLen); 
						if (INVALID_SOCKET != sockAccept)
						{
							if(g_pCallback != NULL)
								g_pCallback(RECV_SOCKET, addrAccept, 0, NULL);
							FD_SET(sockAccept, &fd);
						}    
					}    
					else //非服务器,接收数据(因为fd是读数据集)    
					{    
						memset(g_szDataBuff, 0, INT_DATABUFFER_SIZE); 
						iRecvSize = recv(fd.fd_array[i], g_szDataBuff, INT_DATABUFFER_SIZE, 0);    
						memset(&addrTemp, 0, sizeof(addrTemp));
						iTempLen = sizeof(addrTemp);    
						getpeername(fd.fd_array[i], (sockaddr *)&addrTemp, &iTempLen);

						if (SOCKET_ERROR == iRecvSize)    
						{
							if (g_pCallback != NULL)
								g_pCallback(RECV_ERROR, addrTemp, 0, NULL);
							closesocket(fd.fd_array[i]);
							FD_CLR(fd.fd_array[i],&fd);    
							i--;    
							continue;    
						}    

						if (0 == iRecvSize)    
						{    
							//客户socket关闭    
							if (g_pCallback != NULL)
								g_pCallback(RECV_CLOSE, addrTemp, 0, NULL);
							closesocket(fd.fd_array[i]);    
							FD_CLR(fd.fd_array[i], &fd);    
							i--;        
						}    

						if (0 < iRecvSize)    
						{    
							//打印接收的数据    
							if (g_pCallback != NULL)
								g_pCallback(RECV_DATA, addrTemp, iRecvSize, g_szDataBuff);
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