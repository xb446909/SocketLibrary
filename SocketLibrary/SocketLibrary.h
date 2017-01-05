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

typedef int(*RecvCallback)(int, sockaddr_in, int, char*);

typedef int(__stdcall *fInitSocket)(int, int, const char*, RecvCallback);
typedef void(__stdcall *fUninitSocket)(int);
typedef int(__stdcall *fTCPConnect)(int, int);
typedef int(__stdcall *fTCPSend)(int, sockaddr_in addr, char*);//server给指定的client发消息时，client的地址为addr
typedef int(__stdcall *fTCPRecv)(int, char*, int, int);
