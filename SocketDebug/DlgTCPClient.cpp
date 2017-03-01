// DlgTCPClient.cpp : 实现文件
//

#include "stdafx.h"
#include "SocketDebug.h"
#include "DlgTCPClient.h"
#include "afxdialogex.h"
#include "..\SocketLibrary\SocketLibrary.h"

#include <sstream>

using namespace std;


// CDlgTCPClient 对话框

#ifdef _DEBUG
#pragma comment(lib, "..\\Debug\\SocketLibrary.lib")
#else
#pragma comment(lib, "..\\Release\\SocketLibrary.lib")
#endif


DWORD WINAPI TcpClientRecvThread(__in  LPVOID lpParameter);

HANDLE g_hTcpClientEvent;
char g_recvBuf[TCP_CLIENT_RECV_BUF_SIZE] = { 0 };


IMPLEMENT_DYNAMIC(CDlgTCPClient, CDialogEx)

CDlgTCPClient::CDlgTCPClient(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_DIALOG_TCP_CLIENT, pParent)
{
	g_hTcpClientEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
}

CDlgTCPClient::~CDlgTCPClient()
{
	CloseHandle(g_hTcpClientEvent);
}

void CDlgTCPClient::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CDlgTCPClient, CDialogEx)
	ON_BN_CLICKED(IDC_BTN_CONNECT, &CDlgTCPClient::OnBnClickedBtnConnect)
	ON_BN_CLICKED(IDC_BTN_SEND, &CDlgTCPClient::OnBnClickedBtnSend)
	ON_WM_PAINT()
	ON_MESSAGE(UM_TCP_CLIENT_SOCK_CLOSE, &CDlgTCPClient::OnServerClosed)
	ON_WM_DESTROY()
	ON_NOTIFY(NM_CLICK, IDC_LINK_CLEAR, &CDlgTCPClient::OnNMClickLinkClear)
END_MESSAGE_MAP()


// CDlgTCPClient 消息处理程序


void CDlgTCPClient::DrawConnect(CPaintDC & dc)
{
	CPen pen(PS_SOLID, 1, RGB(255, 0, 0));
	CBrush brush(RGB(255, 0, 0));
	CBrush* pOldBrush = dc.SelectObject(&brush);
	CPen* pOldPen = dc.SelectObject(&pen);
	dc.Ellipse(540, 15, 560, 35);
	dc.SelectObject(pOldBrush);

	int r1 = 15;
	int r2 = 22;
	double pi = 3.14159265354;

	for (int i = 0; i < 8; i++)
	{
		double rad = i * pi / 4;
		dc.MoveTo(r1 * sin(rad) + 550, r1 * cos(rad) + 25);
		dc.LineTo(r2 * sin(rad) + 550, r2 * cos(rad) + 25);
	}
	dc.SelectObject(pOldPen);
}

void CDlgTCPClient::DrawDisconnect(CPaintDC & dc)
{
	CBrush brush(RGB(0, 0, 0));
	CBrush* pOldBrush = dc.SelectObject(&brush);
	dc.Ellipse(540, 15, 560, 35);
	dc.SelectObject(pOldBrush);
}

LRESULT CDlgTCPClient::OnServerClosed(WPARAM wParam, LPARAM lParam)
{
	UninitSocket(TCP_CLIENT_ID);
	Invalidate();
	WaitForSingleObject(m_hRecvThread, INFINITE);
	CloseHandle(m_hRecvThread);
	OnConnect(FALSE);
	ResetEvent(g_hTcpClientEvent);
	return 0;
}


void CDlgTCPClient::OnBnClickedBtnConnect()
{
	// TODO: 在此添加控件通知处理程序代码

	char strIpAddr[64] = { 0 };
	char strPort[64] = { 0 };
	char strApp[128] = { 0 };
	sprintf(strApp, "TCP Client%d", TCP_CLIENT_ID);

	GetDlgItemTextA(GetSafeHwnd(), IDC_IPADDRESS_SERVER, strIpAddr, 64);
	GetDlgItemTextA(GetSafeHwnd(), IDC_EDIT_PORT, strPort, 64);
	WritePrivateProfileStringA(strApp, "Server IP Address", strIpAddr, INI_PATH);
	WritePrivateProfileStringA(strApp, "Server Port", strPort, INI_PATH);

	if (m_bConnect)
	{
		UninitSocket(TCP_CLIENT_ID);
	}
	else
	{
		InitSocket(TCP_CLIENT_ID, TCP_CLIENT, INI_PATH);
		if (TCPConnect(TCP_CLIENT_ID, GetDlgItemInt(IDC_EDIT_TIMEOUT)) == SOCK_SUCCESS)
		{
			OnConnect(TRUE);
			m_hRecvThread = CreateThread(NULL, 0, TcpClientRecvThread, GetSafeHwnd(), 0, NULL);
			WaitForSingleObject(g_hTcpClientEvent, INFINITE);
		}
		else
			MessageBox(_T("连接失败！"));
	}
}


void CDlgTCPClient::OnBnClickedBtnSend()
{
	// TODO: 在此添加控件通知处理程序代码
	CString str;
	GetDlgItemText(IDC_EDIT_SEND, str);
	TCPSend(TCP_CLIENT_ID, CT2A(str));
}


BOOL CDlgTCPClient::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化

	char strIpAddr[64] = { 0 };
	char strPort[64] = { 0 };
	char strApp[128] = { 0 };
	sprintf(strApp, "TCP Client%d", TCP_CLIENT_ID);

	GetPrivateProfileStringA(strApp, "Server IP Address", "127.0.0.1", strIpAddr, 63, INI_PATH);
	WritePrivateProfileStringA(strApp, "Server IP Address", strIpAddr, INI_PATH);
	GetPrivateProfileStringA(strApp, "Server Port", "10000", strPort, 63, INI_PATH);
	WritePrivateProfileStringA(strApp, "Server Port", strPort, INI_PATH);

	SetDlgItemText(IDC_IPADDRESS_SERVER, CA2T(strIpAddr));
	SetDlgItemText(IDC_EDIT_PORT, CA2T(strPort));
	SetDlgItemText(IDC_EDIT_TIMEOUT, _T("0"));

	OnConnect(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}

void CDlgTCPClient::OnConnect(BOOL bConnect)
{
	m_bConnect = bConnect;
	if (bConnect)
		SetDlgItemText(IDC_BTN_CONNECT, _T("断开"));
	else
		SetDlgItemText(IDC_BTN_CONNECT, _T("连接"));
	Invalidate();
}


void CDlgTCPClient::OnPaint()
{
	CPaintDC dc(this); // device context for painting
					   // TODO: 在此处添加消息处理程序代码
					   // 不为绘图消息调用 CDialogEx::OnPaint()
	if (m_bConnect)
		DrawConnect(dc);
	else
		DrawDisconnect(dc);
	
}

DWORD WINAPI TcpClientRecvThread(LPVOID lpParameter)
{
	SetEvent(g_hTcpClientEvent);

	HWND hWnd = (HWND)lpParameter;
	HWND hEditWnd = GetDlgItem(hWnd, IDC_EDIT_RECV);

	while (true)
	{
		if (WaitForSingleObject(g_hTcpClientEvent, 0) != WAIT_OBJECT_0)
			break;

		if (TCPRecv(TCP_CLIENT_ID, g_recvBuf, TCP_CLIENT_RECV_BUF_SIZE, INT_MAX) != SOCK_SUCCESS)
		{
			PostMessage(hWnd, UM_TCP_CLIENT_SOCK_CLOSE, 0, 0);
			return 0;
		}
		else
		{
			SendMessage(hEditWnd, EM_SETSEL, -1, -1);
			SendMessage(hEditWnd, EM_REPLACESEL, 0, (LPARAM)(LPCTSTR)CA2T(g_recvBuf));
		}
	}
	return 0;
}


void CDlgTCPClient::OnDestroy()
{
	CDialogEx::OnDestroy();

	// TODO: 在此处添加消息处理程序代码
	OnConnect(FALSE);
	UninitSocket(TCP_CLIENT_ID);
	ResetEvent(g_hTcpClientEvent);
	WaitForSingleObject(m_hRecvThread, INFINITE);
}



void CDlgTCPClient::OnNMClickLinkClear(NMHDR *pNMHDR, LRESULT *pResult)
{
	// TODO: 在此添加控件通知处理程序代码
	SetDlgItemText(IDC_EDIT_RECV, _T(""));
	*pResult = 0;
}
