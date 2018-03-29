/**
*	@Created Time : 2016-6-29
*	@Author : Chaos
**/

// BugReporterDlg.h : ͷ�ļ�
//

#pragma once

#include<vector>
using namespace std;

// CBugReporterDlg �Ի���
class CBugReporterDlg : public CDialogEx
{
// ����
public:
	CBugReporterDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_BUGREPORTER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
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
