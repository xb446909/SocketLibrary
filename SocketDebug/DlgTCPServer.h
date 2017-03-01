#pragma once


// CDlgTCPServer 对话框

class CDlgTCPServer : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgTCPServer)

public:
	CDlgTCPServer(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CDlgTCPServer();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_TCP_SERVER };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
};
