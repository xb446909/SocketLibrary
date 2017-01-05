#pragma once

#include <WinSock2.h>

#define	USE_STATIC		0	//0：使用动态库，1：使用静态库

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

typedef int(*RecvCallback)(int, sockaddr_in, int, char*);

#if USE_STATIC
typedef int(*fInitSocket)(int, int, const char*, RecvCallback);
typedef void(*fUninitSocket)(int);
typedef int(*fTCPConnect)(int, int);
typedef int(*fTCPSend)(int, sockaddr_in addr, char*);//server给指定的client发消息时，client的地址为addr
typedef int(*fTCPRecv)(int, char*, int, int);

#ifndef SOCKETLIBRARY_EXPORTS
extern "C" __declspec(dllimport) int InitSocket(int nID, int nType, const char* szIniPath = NULL, RecvCallback pCallback = NULL);
extern "C" __declspec(dllimport) void UninitSocket(int nID);
extern "C" __declspec(dllimport) int TCPConnect(int nID, int nTimeoutMs);
extern "C" __declspec(dllimport) int TCPSend(int nID, sockaddr_in addr, char* szSendBuf);
extern "C" __declspec(dllimport) int TCPRecv(int nID, char* szRecvBuf, int nBufLen, int nTimeoutMs);
#else
extern "C" __declspec(dllexport) int InitSocket(int nID, int nType, const char* szIniPath = NULL, RecvCallback pCallback = NULL);
extern "C" __declspec(dllexport) void UninitSocket(int nID);
extern "C" __declspec(dllexport) int TCPConnect(int nID, int nTimeoutMs);
extern "C" __declspec(dllexport) int TCPSend(int nID, sockaddr_in addr, char* szSendBuf);
extern "C" __declspec(dllexport) int TCPRecv(int nID, char* szRecvBuf, int nBufLen, int nTimeoutMs);
#endif
#else
typedef int(__stdcall *fInitSocket)(int, int, const char*, RecvCallback);
typedef void(__stdcall *fUninitSocket)(int);
typedef int(__stdcall *fTCPConnect)(int, int);
typedef int(__stdcall *fTCPSend)(int, sockaddr_in addr, char*);//server给指定的client发消息时，client的地址为addr
typedef int(__stdcall *fTCPRecv)(int, char*, int, int);
#endif






