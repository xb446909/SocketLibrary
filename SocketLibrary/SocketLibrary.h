#pragma once

#include <WinSock2.h>
#define USE_STATIC      0
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
#define SOCK_CLOSED		-3

typedef int(*RecvCallback)(int, sockaddr_in, int, char*);

typedef int  (__stdcall *fInitSocket)(int, int, const char*, RecvCallback);
typedef void (__stdcall *fUninitSocket)(int);
typedef int  (__stdcall *fTCPConnect)(int, int);
typedef int  (__stdcall *fTCPSend)(int, char*, char* szDstIP, int nDstPort);//server给指定的client发消息时，
                                                                            //client的地址为addr   
																			//client发送给sever时，IP和port为空
typedef int  (__stdcall *fUDPSend)(int, char*, char* szDstIP, int nDstPort);//server给指定的client发消息时，client的地址为addr 
typedef int  (__stdcall *fTCPRecv)(int, char*, int, int);
typedef int  (__stdcall *fUDPRecv)(int, char*, int, int);

#ifndef USE_STATIC
#pragma comment(lib, "..//Debug//SocketLibrary.lib")
#endif

#ifndef SOCKETLIBRARY_EXPORTS
extern "C" __declspec(dllimport) int  __stdcall InitSocket(int nID, int nType, const char* szIniPath = NULL, RecvCallback pCallback = NULL);
extern "C" __declspec(dllimport) void __stdcall UninitSocket(int nID);
extern "C" __declspec(dllimport) int  __stdcall TCPConnect(int nID, int nTimeoutMs);
extern "C" __declspec(dllimport) int  __stdcall TCPSend(int nID, char* szSendBuf, char* szDstIP = NULL, int nDstPort = 0);
extern "C" __declspec(dllimport) int  __stdcall UDPSend(int nID, char* szSendBuf, char* szDstIP, int nDstPort);
extern "C" __declspec(dllimport) int  __stdcall TCPRecv(int nID, char* szRecvBuf, int nBufLen, int nTimeoutMs);
extern "C" __declspec(dllimport) int  __stdcall UDPRecv(int nID, char* szRecvBuf, int nBufLen, int nTimeoutMs);
#else								 
extern "C" __declspec(dllexport) int __stdcall InitSocket(int nID, int nType, const char* szIniPath = NULL, RecvCallback pCallback = NULL);
extern "C" __declspec(dllexport) void __stdcall UninitSocket(int nID);
extern "C" __declspec(dllexport) int __stdcall TCPConnect(int nID, int nTimeoutMs);
extern "C" __declspec(dllexport) int __stdcall TCPSend(int nID, char* szSendBuf, char* szDstIP = NULL, int nDstPort = 0);
extern "C" __declspec(dllexport) int __stdcall UDPSend(int nID, char* szSendBuf, char* szDstIP, int nDstPort);
extern "C" __declspec(dllexport) int __stdcall TCPRecv(int nID, char* szRecvBuf, int nBufLen, int nTimeoutMs);
extern "C" __declspec(dllexport) int __stdcall UDPRecv(int nID, char* szRecvBuf, int nBufLen, int nTimeoutMs);
#endif

