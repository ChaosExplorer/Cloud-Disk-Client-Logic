/**
*	@Created Time : 2016-6-29
*	@Author : Chaos
**/

// BugReporterDlg.cpp : ʵ���ļ�
//
#include "stdafx.h"
#include "BugReporter.h"
#include "BugReporterDlg.h"
#include "afxdialogex.h"
#include "SysInfoTool.h"
#include "ServiceTool.h"
#include "Windows.h"
#include "Shlobj.h"
#include <iostream>
#include <fstream>
#include "ProgressBar.h"
#include "FileUtil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CBugReporterDlg �Ի���



CBugReporterDlg::CBugReporterDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_BUGREPORTER_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CBugReporterDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CBugReporterDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDOK, &CBugReporterDlg::OnBnClickedOk)
	ON_BN_CLICKED(ClientTraceBtn, &CBugReporterDlg::OnBnClickedClienttracebtn)
	ON_BN_CLICKED(ServiceTraceBtn, &CBugReporterDlg::OnBnClickedServicetracebtn)
	ON_BN_CLICKED(IDCANCEL, &CBugReporterDlg::OnBnClickedCancel)
	ON_BN_CLICKED(ChooseFileBtn, &CBugReporterDlg::OnBnClickedChoosefilebtn)
//	ON_WM_CLOSE()
	ON_WM_DESTROY()
END_MESSAGE_MAP()


// CBugReporterDlg ��Ϣ�������

BOOL CBugReporterDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ���ô˶Ի����ͼ�ꡣ  ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	initMember();

	//pb = new ProcessBar;
	//pb->Create(DLG_PROCESS, this);

	checkClientTraceOnOff();
	UpdateClientTraceBtnStatus();
	checkServiceTraceOnOff();
	UpdateSvrTraceBtnStatus();

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ  ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CBugReporterDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CBugReporterDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

// �ύ

void CBugReporterDlg::OnBnClickedOk()
{
	try 
	{
		if (!PathIsDirectory(outputDirPath))
			if (!CreateDirectory(outputDirPath, NULL)) {
				MessageBox(_T("����Ŀ¼ʧ�ܣ������û�Ȩ��"), MB_OK);
				return;
			}

		if(!writeProblemDescr())
			return;

		collectSysInfo();

		collectASFiles();

		collectDBFiles();

		this->EnableWindow(FALSE);
		ProgressBar* pb = new ProgressBar;
		pb->Create(DLG_PROCESS, this);
		pb->ShowWindow(SW_SHOW);


		collectEventLog();

		pb->CloseWindow();
		this->EnableWindow(TRUE);

		ShellExecute (NULL, _T("open"), outputDirPath, NULL, NULL, SW_SHOWNORMAL);
	}
	catch (exception e) {
		MessageBox((LPCTSTR)e.what(), _T("Error Occured"), MB_OK);
	}

	CDialogEx::OnOK();
}


// ȡ��

void CBugReporterDlg::OnBnClickedCancel()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	CDialogEx::OnCancel();
}


// ��ʼ��·���ȳ�Ա

void CBugReporterDlg::initMember()
{
	clientTraceOn = false;
	serviceTraceOn = false;

	try {
		//getInstallPath();
		
		TCHAR buff[MAX_PATH];
		GetCurrentDirectory(MAX_PATH, buff);
		installPath = buff;

		// ��ȡ�����ļ��С�AppData·����ƴ��·��
		TCHAR szPath[MAX_PATH];
		SHGetSpecialFolderPath(NULL, szPath, CSIDL_DESKTOP, FALSE);

		outputDirPath = szPath;
		outputDirPath.Append(_T("\\������Ϣ�ռ�"));

		SHGetSpecialFolderPath(NULL, szPath, CSIDL_APPDATA, FALSE);
		configDirPath = szPath;
		configDirPath.Append(_T("\\AnyShare"));

		getCacheDirPath();

		dBDirPath = outputDirPath + _T("\\���ݿ��ļ�");
		problemDirPath = outputDirPath + _T("\\���⸽���ļ�");
		problemDesCribPath = outputDirPath + _T("\\��������.txt");
		traceLogDirPath = outputDirPath + _T("\\Trace��־");
		outConfigDirPath = outputDirPath + _T("\\�ͻ��������ļ�");
		envirPath = outputDirPath + _T("\\������Ϣ.txt");
		currentProcessPath = outputDirPath + _T("\\�����б�.txt");
		versionPath = outputDirPath + _T("\\�汾��Ϣ.config");
		eventLogPath = outputDirPath + _T("\\ϵͳ��־.evtx");

		winUtilIniPath = installPath + _T("\\winutil.ini");
		cflConfigPath = installPath + _T("\\cfl.config");
		cflTraceStr = "\r\nEnableTrace=on\r\nTraceOutputLocation=file\r\nTraceOutputFile=.\\trace.log\r\nTraceType=sync\r\n";
		cflTraceStr2 = "EnableThreadSafe=off\r\nEnableTraceTime=on\r\nTraceModule=all";
	}
	catch (exception e) {
		MessageBox((LPCTSTR)e.what(), _T("Error Occured"), MB_OK);
	}

}

// ��ע����ȡ��װĿ¼ 
void CBugReporterDlg::getInstallPath()
{
	HKEY hKey;
	LPCTSTR data_set = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\eisoo_anyshare3.5_Setup_AppId_is1");

	HRESULT ret = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, data_set, 0, KEY_READ, &hKey);

	if (ERROR_SUCCESS == ret) {
		char dwValue[256];
		DWORD dwSzType = REG_SZ;
		DWORD dwSize = sizeof(dwValue);

		if (ERROR_SUCCESS == ::RegQueryValueEx(hKey, _T("InstallLocation"), 0, &dwSzType, (LPBYTE)&dwValue, &dwSize)) {
			installPath = dwValue;
		}
		else {
			MessageBox(_T("��ȡAnyShareע�����������Ƿ���ȷ��װ"), MB_OK);
			exit(0);
		}

		::RegCloseKey(hKey);
	}
	else {
		MessageBox(_T("��ȡAnyShareע�����������Ƿ���ȷ��װ"), MB_OK);
		exit(0);
	}
}

void CBugReporterDlg::getCacheDirPath()
{
	CString str1 = configDirPath + _T("\\common.ini");
	CString str2 = configDirPath + _T("\\tray.ini");

	CString lastserver, lastuser;
	TCHAR buf[512];
	GetPrivateProfileString(_T("Global"), _T("lastserver"), _T(""), buf, 512, str1);
	lastserver = buf;
	GetPrivateProfileString(lastserver, _T("lastuser"), _T(""), buf, 512, str1);
	lastuser = buf;
	GetPrivateProfileString(lastuser, _T("cachepath"), _T(""), buf, 512, str2);
	cacheDirPath = buf;
}

// ����Trace��ť
void CBugReporterDlg::UpdateClientTraceBtnStatus()
{
	CButton *pBtn = (CButton *)GetDlgItem(ClientTraceBtn);

	if (clientTraceOn) {
		pBtn->SetWindowTextW(_T("�رտͻ�����־"));
	}
	else {
		pBtn->SetWindowTextW(_T("�����ͻ�����־"));
	}

}
void CBugReporterDlg::UpdateSvrTraceBtnStatus()
{
	CButton *pBtn = (CButton *)GetDlgItem(ServiceTraceBtn);

	if (serviceTraceOn) {
		pBtn->SetWindowTextW(_T("�رշ�����־"));
	}
	else {
		pBtn->SetWindowTextW(_T("����������־"));
	}
}


// ���ͻ���Trace����״̬

bool CBugReporterDlg::checkClientTraceOnOff()
{
	try
	{
		CFileFind finder;

		if (!finder.FindFile(cflConfigPath))
		{
			MessageBox(_T("cfl.config file does not exist"), MB_OK);
			return false;
		}
		else {
			char str[1024];
			CFile file(cflConfigPath, CFile::modeRead);

			int count = (int)file.GetLength();
			file.Read(str, count);

			CString buff = str;
			if (buff.Find(_T("EnableTrace=on")) != -1)
				clientTraceOn = true;

			file.Close();
		}
	}
	catch (...)
	{
		MessageBox(_T("cfl.config ��д���������û�Ȩ��"), MB_OK);
	} 
			
	return clientTraceOn;
}

void CBugReporterDlg::TurnOnClientTrace()
{
	try {
		CFile file;
		file.Open(cflConfigPath, CFile::modeWrite);

		file.SeekToEnd();

		file.Write(cflTraceStr.c_str(), cflTraceStr.size());
		file.Write(cflTraceStr2.c_str(), cflTraceStr2.size());

		file.Flush();

		file.Close();

		clientTraceOn = true;
	}
	catch (...) {
		MessageBox(_T("cfl.config ��д���������û�Ȩ��"), _T("Error Occured"), MB_OK);
	}
}

void CBugReporterDlg::TurnOffClientTrace()
{
	try {
		CFile file(cflConfigPath, CFile::modeRead);
		char str[1024];

		int count = (int)file.GetLength();
		file.Read(str, count);

		string buff = str;
		int index = buff.find("\r\nEnableTrace=on");
		buff = buff.substr(0,index);

		file.Close();

		file.Open(cflConfigPath, CFile::modeWrite | CFile::modeCreate);

		file.Write(buff.c_str(), buff.length());
		file.Flush();

		file.Close();

		clientTraceOn = false;
	}
	catch (...) {
		MessageBox(_T("cfl.config ��д���������û�Ȩ��"), _T("Error Occured"), MB_OK);
	}
}

// ������Trace����״̬

bool CBugReporterDlg::checkServiceTraceOnOff()
{
	try{

		CFileFind finder;

		if (!finder.FindFile(winUtilIniPath))
		{
			return false;
		}

		TCHAR buf[256];
		GetPrivateProfileString(_T("Trace"), _T("modules"), _T(""), buf, 512, winUtilIniPath);
		CString temp = buf;

		if (temp.Compare(_T("winhook")) == 0)
			serviceTraceOn = true;
	}
	catch (...)
	{
		MessageBox(_T("winutil.ini ��д���������û�Ȩ��"), MB_OK);
	} 
	return serviceTraceOn;
}

void CBugReporterDlg::TurnOnSvrTrace()
{
	try {
		CFile file;
		file.Open(winUtilIniPath, CFile::modeCreate | CFile::modeWrite);
		file.Close();

		WritePrivateProfileString(_T("Trace"), _T("modules"), _T("winhook"),winUtilIniPath);

		serviceTraceOn = true;
	}
	catch (...) {
		MessageBox(_T("winutil.ini ��д���������û�Ȩ��"), _T("Error Occured"), MB_OK);
	}
}

void CBugReporterDlg::TurnOffSvrTrace()
{
	try {
		CFile file(winUtilIniPath, CFile::modeWrite);
		file.SetLength(0);

		file.Close();

		serviceTraceOn = false;
	}
	catch (...) {
		MessageBox(_T("winutil.ini ��д���������û�Ȩ��"), _T("Error Occured"), MB_OK);
	}
}


// �����ͻ�����־

void CBugReporterDlg::OnBnClickedClienttracebtn()
{
	if (clientTraceOn) {
		TurnOffClientTrace ();

		MessageBox(_T("�����ͻ�����Ч"), _T("�ѹر�"), MB_OK);

	}
	else {
		TurnOnClientTrace ();

		MessageBox(_T("�����ͻ�����Ч"), _T("�ѿ���"), MB_OK);
	}

	UpdateClientTraceBtnStatus();
}


// ����������־

void CBugReporterDlg::OnBnClickedServicetracebtn()
{
	if (serviceTraceOn) {
		TurnOffSvrTrace ();

		if(IDOK == MessageBox(_T("����AnyShare Service������Ч��ȷ������������"), _T("�ѹر�"), MB_OKCANCEL))
		{
			ProgressBar* pb = new ProgressBar;
			pb->Create(DLG_PROCESS, this);
			pb->ShowWindow(SW_SHOW);
			this->EnableWindow(FALSE);

			if(ServiceTool::restartService(_T("AnyShare Service")))
				MessageBox(_T("����������"), MB_OK);
			else
				MessageBox(_T("��������ʧ�ܣ����ֶ�����"), MB_OK);

			pb->CloseWindow();
			this->EnableWindow(TRUE);
		}
	}
	else {
		TurnOnSvrTrace ();

		if (IDOK == MessageBox(_T("����AnyShare Service������Ч��ȷ������������"), _T("�ѿ���"), MB_OKCANCEL))
		{
			ProgressBar* pb = new ProgressBar;
			pb->Create(DLG_PROCESS, this);
			pb->ShowWindow(SW_SHOW);
			this->EnableWindow(FALSE);

			if(ServiceTool::restartService(_T("AnyShare Service")))
				MessageBox(_T("����������"), MB_OK);
			else
				MessageBox(_T("��������ʧ�ܣ����ֶ�����"), MB_OK);

			pb->CloseWindow();
			this->EnableWindow(TRUE);
		}
	}

	UpdateSvrTraceBtnStatus();
}



// д����������

bool CBugReporterDlg::writeProblemDescr()
{
	try
	{
		CFile file;
		file.Open(problemDesCribPath, CFile::modeCreate | CFile::modeWrite);

		CString title, content;

		GetDlgItem(ProblemTitle)->GetWindowText(title);
		GetDlgItem(ProblemContent)->GetWindowText(content);

		if (title.Trim().GetLength() == 0) {
			MessageBox(_T("�������������"), _T("��Ч�ύ"), MB_OK);
			return false;
		}
		if (content.Trim().GetLength() == 0) {
			MessageBox(_T("��������������"), _T("��Ч�ύ"), MB_OK);
			return false;
		}

		string titleStr1,titleStr2, content1, content2;

		titleStr1 = "��������⡿\r\n";
		content1 = "\r\n\r\n������������\r\n";

		titleStr2 = CString2str(title);
		content2 = CString2str(content);

		file.Write(titleStr1.c_str(), titleStr1.size());
		file.Write(titleStr2.c_str(), titleStr2.size());
		file.Write(content1.c_str(), content1.size());
		file.Write(content2.c_str(), content2.size());

		file.Flush();
		file.Close();

		// ���������ļ�
		if (!PathIsDirectory(problemDirPath))
			CreateDirectory(problemDirPath, NULL);

		vector<CString>::iterator it = chooseFiles.begin();
		CString tempPath, srcPath ,fileName;
		int index = 0;
		CFileFind finder;
		for (; it != chooseFiles.end(); ++it)
		{
			srcPath = *it;
			if (finder.FindFile(*it)) {
				do
				{
					index =srcPath.Find(_T("\\"));
					srcPath = srcPath.Right(srcPath.GetLength() - index -1);

				}while (index != -1);

				fileName = srcPath;

				tempPath = problemDirPath + _T("\\") +fileName;
			}

			CopyFile(*it, tempPath, FALSE);
		}

	}
	catch (...)
	{
		return false;
	}

	return true;
}

// �ռ�ϵͳ��Ϣ

void CBugReporterDlg::collectSysInfo()
{
	try
	{
		SysInfoTool sysTool;
		CFile file(envirPath, CFile::modeCreate | CFile::modeWrite);

		string result = sysTool.getSystemInfo();
		file.Write(result.c_str(), result.size());

		file.Flush();
		file.Close();
		

		CFile file2;
		file2.Open(currentProcessPath, CFile::modeCreate | CFile::modeWrite);
		file2.Close();

		char str[100];
		sprintf_s(str, "tasklist > %s", CString2str(currentProcessPath).c_str());
		string param = str;
		system(param.c_str());
	}
	catch (...)
	{
	}
}

//  �ռ�ϵͳ��־

void CBugReporterDlg::collectEventLog()
{
	SysInfoTool sysTool;
	sysTool.exportEventLog(eventLogPath);
}

//  �ռ����ݿ��ļ�

void CBugReporterDlg::collectDBFiles()
{
	CString dbPath = cacheDirPath + _T("\\.anyshare.cache");
	FileUtil::CopyDirectory(dbPath, dBDirPath);
}

//  �ռ�AS�ļ�

void CBugReporterDlg::collectASFiles()
{
	CString verPath = installPath + _T("\\version.config");
	CString hookLog =  installPath + _T("\\winutil.log");
	CString traceLog =  installPath + _T("\\trace.log");

	if (PathFileExists(verPath))
		CopyFile(verPath, versionPath, FALSE);

	// Trace Log
	if (!PathIsDirectory(traceLogDirPath))
			CreateDirectory(traceLogDirPath, NULL);

	if (PathFileExists(hookLog))
		CopyFile(hookLog, traceLogDirPath+ _T("\\winutil.log"), FALSE);

	if (PathFileExists(traceLog))
		CopyFile(traceLog, traceLogDirPath+ _T("\\trace.log"), FALSE);

	// �����ļ�
	if (!PathIsDirectory(outConfigDirPath))
			CreateDirectory(outConfigDirPath, NULL);

	FileUtil::CopyDirectory(configDirPath, outConfigDirPath);
}

string CBugReporterDlg::CString2str(CString& tchar)
{
	 int iLen = WideCharToMultiByte(CP_ACP, 0, tchar, -1, NULL, 0, NULL, NULL);

	 char* chRtn =new char[iLen*sizeof(char)];

	 WideCharToMultiByte(CP_ACP, 0, tchar, -1, chRtn, iLen, NULL, NULL);

	string str(chRtn);

	return str;
}

// ѡ�񸽼��ļ�

void CBugReporterDlg::OnBnClickedChoosefilebtn()
{
	try {
		CFileDialog openDlg(TRUE,
			NULL,
			NULL,
			OFN_ALLOWMULTISELECT);

		const int nMaxFiles = 100;
		const int nMaxPathBuffer = (nMaxFiles * (MAX_PATH + 1)) + 1;
		LPWSTR pc = (LPWSTR)malloc(nMaxPathBuffer * sizeof(WCHAR));
		if (pc)
		{
			openDlg.GetOFN().lpstrFile = pc;
			openDlg.GetOFN().lpstrFile[0] = NULL;
			openDlg.m_ofn.nMaxFile = nMaxPathBuffer;

			if (openDlg.DoModal() == IDOK)
			{
				POSITION pos = openDlg.GetStartPosition();
				while (pos)
				{
					//��pc��ָ����ڴ��н�����ÿ���ļ�������,�����fileName��ռ���ڴ治�ܺ�pc��ռ���ڴ淢����ͻ  
					CString fileName = openDlg.GetNextPathName(pos);

					if (find(chooseFiles.begin(), chooseFiles.end(), fileName) == chooseFiles.end())
						chooseFiles.push_back(fileName);
				}
			}
			free(pc);
		}
	}
	catch (...)
	{
		MessageBox(_T("Choose file dialog create failed."), MB_OK);
	}

	// Update ChoosedFiles
	CStatic *pStatic = (CStatic *) GetDlgItem(ChoosedFilesStc);
	CString tempStr;

	if (chooseFiles.size() > 0) {
		vector<CString>::iterator it = chooseFiles.begin();
		for (; it != chooseFiles.end() - 1; ++it) {
			tempStr.Append(*it);
			tempStr.Append(_T(" ; "));
		}
		tempStr.Append(*it);
	}

	pStatic->SetWindowTextW(tempStr);
}


void CBugReporterDlg::OnOK()
{
	// TODO: �ڴ����ר�ô����/����û���

	//CDialogEx::OnOK();
}


BOOL CBugReporterDlg::PreTranslateMessage(MSG* pMsg)
{
	if(pMsg->message==WM_KEYDOWN && pMsg->wParam==VK_RETURN) 
	{
		if (!(GetDlgItem(ProblemContent) == GetFocus()))
			return true;
	} 

	return CDialogEx::PreTranslateMessage(pMsg);
}

void CBugReporterDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	if (clientTraceOn)
		TurnOffClientTrace();
	if (serviceTraceOn)
		TurnOffSvrTrace();
}
