/**
*	@Created Time : 2016-6-29
*	@Author : Chaos
**/

#pragma once


// ProgressBar �Ի���

class ProgressBar : public CDialogEx
{
	DECLARE_DYNAMIC(ProgressBar)

public:
	ProgressBar(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~ProgressBar();

	virtual void OnFinalRelease();

// �Ի�������
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG1 };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
};
