#pragma once

#define TCP_SERVER		0
#define TCP_CLIENT		1
#define UDP_SERVER		2
#define UDP_CLIENT		3

#define RECV_ERROR		0
#define RECV_CLOSE		1
#define RECV_DATA		2
#define RECV_SOCKET		3

typedef int (*RecvCallback)(int, sockaddr_in, int, char*);

#ifndef SOCKETLIBRARY_EXPORTS
__declspec(dllimport) BOOL _stdcall InitSocket(int nID, int nType, const char* szIniPath = NULL, RecvCallback pCallback = NULL);
__declspec(dllimport) void _stdcall UninitSocket(int nID);
__declspec(dllimport) BOOL _stdcall TCPConnect(int nID, int nTimeoutMs);
__declspec(dllimport) BOOL _stdcall TCPSend(int nID, char* szSendBuf);
__declspec(dllimport) BOOL _stdcall TCPRecv(int nID, char* szRecvBuf, int nBufLen, int nTimeoutMs);
#endif

