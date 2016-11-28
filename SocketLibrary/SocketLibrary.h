#pragma once

#include <WinSock2.h>

#define TCP_SERVER		0
#define TCP_CLIENT		1
#define UDP_SERVER		2
#define UDP_CLIENT		3

#define RECV_ERROR		0
#define RECV_CLOSE		1
#define RECV_DATA		2
#define RECV_SOCKET		3

#define SOCK_SUCCESS	0
#define SOCK_ERROR		-1
#define SOCK_TIMEOUT	-2

typedef int (*RecvCallback)(int nRecvType, sockaddr_in addrClient, int nSize, char* pBuffer);

typedef int (__stdcall *fInitSocket)(int nID, int nType, const char* szIniPath, RecvCallback pCallback);
typedef void (__stdcall*fUninitSocket)(int nID);
typedef int (__stdcall *fTCPConnect)(int nID, int nTimeoutMs);
typedef int (__stdcall *fTCPSend)(int nID, char* szSendBuf);
typedef int (__stdcall *fTCPRecv)(int nID, char* szRecvBuf, int nBufLen, int nTimeoutMs);

#ifndef SOCKETLIBRARY_EXPORTS
__declspec(dllimport) int __stdcall InitSocket(int nID, int nType, const char* szIniPath = NULL, RecvCallback pCallback = NULL);
__declspec(dllimport) void __stdcall UninitSocket(int nID);
__declspec(dllimport) int __stdcall TCPConnect(int nID, int nTimeoutMs);
__declspec(dllimport) int __stdcall TCPSend(int nID, char* szSendBuf);
__declspec(dllimport) int __stdcall TCPRecv(int nID, char* szRecvBuf, int nBufLen, int nTimeoutMs);
#endif

