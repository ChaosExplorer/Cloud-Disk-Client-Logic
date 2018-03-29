/**
*	@Created Time : 2016-6-29
*	@Author : Chaos
**/

#pragma once


// ProgressBar 对话框

class ProgressBar : public CDialogEx
{
	DECLARE_DYNAMIC(ProgressBar)

public:
	ProgressBar(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~ProgressBar();

	virtual void OnFinalRelease();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG1 };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
};
