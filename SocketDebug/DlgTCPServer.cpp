// DlgTCPServer.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "SocketDebug.h"
#include "DlgTCPServer.h"
#include "afxdialogex.h"


// CDlgTCPServer �Ի���

IMPLEMENT_DYNAMIC(CDlgTCPServer, CDialogEx)

CDlgTCPServer::CDlgTCPServer(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_DIALOG_TCP_SERVER, pParent)
{

}

CDlgTCPServer::~CDlgTCPServer()
{
}

void CDlgTCPServer::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CDlgTCPServer, CDialogEx)
END_MESSAGE_MAP()


// CDlgTCPServer ��Ϣ�������
