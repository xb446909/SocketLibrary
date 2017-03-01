
// SocketDebugDlg.h : ͷ�ļ�
//

#pragma once

#include "DlgTCPServer.h"
#include "DlgTCPClient.h"

// CSocketDebugDlg �Ի���
class CSocketDebugDlg : public CDialogEx
{
// ����
public:
	CSocketDebugDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SOCKETDEBUG_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

	CMFCTabCtrl m_tabCtrl;
	CDlgTCPServer m_dlgTcpServer;
	CDlgTCPClient m_dlgTcpClient;
};
