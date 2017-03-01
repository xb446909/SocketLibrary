#pragma once


// CDlgTCPClient 对话框

#define		TCP_CLIENT_ID						1
#define		TCP_CLIENT_RECV_BUF_SIZE			(10 * 1024 * 1024)
#define		UM_TCP_CLIENT_SOCK_CLOSE			(WM_USER + 1)

class CDlgTCPClient : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgTCPClient)

public:
	CDlgTCPClient(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CDlgTCPClient();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_TCP_CLIENT };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

	void DrawConnect(CPaintDC& dc);
	void DrawDisconnect(CPaintDC& dc);
	afx_msg LRESULT OnServerClosed(WPARAM wParam, LPARAM lParam);

	HANDLE m_hRecvThread;
	BOOL m_bConnect;
public:
	afx_msg void OnBnClickedBtnConnect();
	afx_msg void OnBnClickedBtnSend();
	virtual BOOL OnInitDialog();


	void OnConnect(BOOL bConnect);
	afx_msg void OnPaint();
	afx_msg void OnDestroy();
	afx_msg void OnNMClickLinkClear(NMHDR *pNMHDR, LRESULT *pResult);
};
