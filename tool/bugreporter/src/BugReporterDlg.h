/**
*	@Created Time : 2016-6-29
*	@Author : Chaos
**/

// BugReporterDlg.h : 头文件
//

#pragma once

#include<vector>
using namespace std;

// CBugReporterDlg 对话框
class CBugReporterDlg : public CDialogEx
{
// 构造
public:
	CBugReporterDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_BUGREPORTER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedClienttracebtn();
	afx_msg void OnBnClickedServicetracebtn();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedChoosefilebtn();

private:
	void initMember();
	void getInstallPath();
	void getCacheDirPath();

	void UpdateClientTraceBtnStatus();
	void UpdateSvrTraceBtnStatus();

	bool checkClientTraceOnOff();
	void TurnOnClientTrace();
	void TurnOffClientTrace();

	bool checkServiceTraceOnOff();
	void TurnOnSvrTrace();
	void TurnOffSvrTrace();

	bool writeProblemDescr();

	void collectSysInfo();

	void collectEventLog ();

	void collectDBFiles();

	void collectASFiles();

public:
	string CString2str (CString& tchar);

private:

	CString installPath;

	CString cacheDirPath;

	CString outputDirPath;

	CString dBDirPath;
	
	CString configDirPath;

	CString outConfigDirPath;

	CString problemDirPath;

	CString problemDesCribPath;

	CString traceLogDirPath;

	CString envirPath;

	CString currentProcessPath;

	CString versionPath;

	CString eventLogPath;

	CString winUtilIniPath;

	CString cflConfigPath;

	string cflTraceStr;

	string cflTraceStr2;

	bool clientTraceOn;

	bool serviceTraceOn;

	vector<CString> chooseFiles;

public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
//	afx_msg void OnClose();
	afx_msg void OnDestroy();
};
