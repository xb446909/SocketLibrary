#pragma once


// CDlgTCPServer �Ի���

class CDlgTCPServer : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgTCPServer)

public:
	CDlgTCPServer(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CDlgTCPServer();

// �Ի�������
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_TCP_SERVER };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()
};
