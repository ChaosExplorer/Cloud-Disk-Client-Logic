/**
*	@Created Time : 2016-6-29
*	@Author : Chaos
**/

// BugReporterDlg.cpp : 实现文件
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

// CBugReporterDlg 对话框



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


// CBugReporterDlg 消息处理程序

BOOL CBugReporterDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	initMember();

	//pb = new ProcessBar;
	//pb->Create(DLG_PROCESS, this);

	checkClientTraceOnOff();
	UpdateClientTraceBtnStatus();
	checkServiceTraceOnOff();
	UpdateSvrTraceBtnStatus();

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CBugReporterDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CBugReporterDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

// 提交

void CBugReporterDlg::OnBnClickedOk()
{
	try 
	{
		if (!PathIsDirectory(outputDirPath))
			if (!CreateDirectory(outputDirPath, NULL)) {
				MessageBox(_T("创建目录失败，请检查用户权限"), MB_OK);
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


// 取消

void CBugReporterDlg::OnBnClickedCancel()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnCancel();
}


// 初始化路径等成员

void CBugReporterDlg::initMember()
{
	clientTraceOn = false;
	serviceTraceOn = false;

	try {
		//getInstallPath();
		
		TCHAR buff[MAX_PATH];
		GetCurrentDirectory(MAX_PATH, buff);
		installPath = buff;

		// 获取桌面文件夹、AppData路径，拼接路径
		TCHAR szPath[MAX_PATH];
		SHGetSpecialFolderPath(NULL, szPath, CSIDL_DESKTOP, FALSE);

		outputDirPath = szPath;
		outputDirPath.Append(_T("\\问题信息收集"));

		SHGetSpecialFolderPath(NULL, szPath, CSIDL_APPDATA, FALSE);
		configDirPath = szPath;
		configDirPath.Append(_T("\\AnyShare"));

		getCacheDirPath();

		dBDirPath = outputDirPath + _T("\\数据库文件");
		problemDirPath = outputDirPath + _T("\\问题附加文件");
		problemDesCribPath = outputDirPath + _T("\\问题描述.txt");
		traceLogDirPath = outputDirPath + _T("\\Trace日志");
		outConfigDirPath = outputDirPath + _T("\\客户端配置文件");
		envirPath = outputDirPath + _T("\\环境信息.txt");
		currentProcessPath = outputDirPath + _T("\\进程列表.txt");
		versionPath = outputDirPath + _T("\\版本信息.config");
		eventLogPath = outputDirPath + _T("\\系统日志.evtx");

		winUtilIniPath = installPath + _T("\\winutil.ini");
		cflConfigPath = installPath + _T("\\cfl.config");
		cflTraceStr = "\r\nEnableTrace=on\r\nTraceOutputLocation=file\r\nTraceOutputFile=.\\trace.log\r\nTraceType=sync\r\n";
		cflTraceStr2 = "EnableThreadSafe=off\r\nEnableTraceTime=on\r\nTraceModule=all";
	}
	catch (exception e) {
		MessageBox((LPCTSTR)e.what(), _T("Error Occured"), MB_OK);
	}

}

// 从注册表获取安装目录 
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
			MessageBox(_T("读取AnyShare注册表错误，请检查是否正确安装"), MB_OK);
			exit(0);
		}

		::RegCloseKey(hKey);
	}
	else {
		MessageBox(_T("读取AnyShare注册表错误，请检查是否正确安装"), MB_OK);
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

// 更新Trace按钮
void CBugReporterDlg::UpdateClientTraceBtnStatus()
{
	CButton *pBtn = (CButton *)GetDlgItem(ClientTraceBtn);

	if (clientTraceOn) {
		pBtn->SetWindowTextW(_T("关闭客户端日志"));
	}
	else {
		pBtn->SetWindowTextW(_T("开启客户端日志"));
	}

}
void CBugReporterDlg::UpdateSvrTraceBtnStatus()
{
	CButton *pBtn = (CButton *)GetDlgItem(ServiceTraceBtn);

	if (serviceTraceOn) {
		pBtn->SetWindowTextW(_T("关闭服务日志"));
	}
	else {
		pBtn->SetWindowTextW(_T("开启服务日志"));
	}
}


// 检查客户端Trace开启状态

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
		MessageBox(_T("cfl.config 读写错误，请检查用户权限"), MB_OK);
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
		MessageBox(_T("cfl.config 读写错误，请检查用户权限"), _T("Error Occured"), MB_OK);
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
		MessageBox(_T("cfl.config 读写错误，请检查用户权限"), _T("Error Occured"), MB_OK);
	}
}

// 检查服务Trace开启状态

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
		MessageBox(_T("winutil.ini 读写错误，请检查用户权限"), MB_OK);
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
		MessageBox(_T("winutil.ini 读写错误，请检查用户权限"), _T("Error Occured"), MB_OK);
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
		MessageBox(_T("winutil.ini 读写错误，请检查用户权限"), _T("Error Occured"), MB_OK);
	}
}


// 开启客户端日志

void CBugReporterDlg::OnBnClickedClienttracebtn()
{
	if (clientTraceOn) {
		TurnOffClientTrace ();

		MessageBox(_T("重启客户端生效"), _T("已关闭"), MB_OK);

	}
	else {
		TurnOnClientTrace ();

		MessageBox(_T("重启客户端生效"), _T("已开启"), MB_OK);
	}

	UpdateClientTraceBtnStatus();
}


// 开启服务日志

void CBugReporterDlg::OnBnClickedServicetracebtn()
{
	if (serviceTraceOn) {
		TurnOffSvrTrace ();

		if(IDOK == MessageBox(_T("重启AnyShare Service服务生效，确认现在重启？"), _T("已关闭"), MB_OKCANCEL))
		{
			ProgressBar* pb = new ProgressBar;
			pb->Create(DLG_PROCESS, this);
			pb->ShowWindow(SW_SHOW);
			this->EnableWindow(FALSE);

			if(ServiceTool::restartService(_T("AnyShare Service")))
				MessageBox(_T("服务已重启"), MB_OK);
			else
				MessageBox(_T("服务重启失败，请手动重启"), MB_OK);

			pb->CloseWindow();
			this->EnableWindow(TRUE);
		}
	}
	else {
		TurnOnSvrTrace ();

		if (IDOK == MessageBox(_T("重启AnyShare Service服务生效，确认现在重启？"), _T("已开启"), MB_OKCANCEL))
		{
			ProgressBar* pb = new ProgressBar;
			pb->Create(DLG_PROCESS, this);
			pb->ShowWindow(SW_SHOW);
			this->EnableWindow(FALSE);

			if(ServiceTool::restartService(_T("AnyShare Service")))
				MessageBox(_T("服务已重启"), MB_OK);
			else
				MessageBox(_T("服务重启失败，请手动重启"), MB_OK);

			pb->CloseWindow();
			this->EnableWindow(TRUE);
		}
	}

	UpdateSvrTraceBtnStatus();
}



// 写入问题描述

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
			MessageBox(_T("请输入问题标题"), _T("无效提交"), MB_OK);
			return false;
		}
		if (content.Trim().GetLength() == 0) {
			MessageBox(_T("请输入问题描述"), _T("无效提交"), MB_OK);
			return false;
		}

		string titleStr1,titleStr2, content1, content2;

		titleStr1 = "【问题标题】\r\n";
		content1 = "\r\n\r\n【问题描述】\r\n";

		titleStr2 = CString2str(title);
		content2 = CString2str(content);

		file.Write(titleStr1.c_str(), titleStr1.size());
		file.Write(titleStr2.c_str(), titleStr2.size());
		file.Write(content1.c_str(), content1.size());
		file.Write(content2.c_str(), content2.size());

		file.Flush();
		file.Close();

		// 拷贝附加文件
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

// 收集系统信息

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

//  收集系统日志

void CBugReporterDlg::collectEventLog()
{
	SysInfoTool sysTool;
	sysTool.exportEventLog(eventLogPath);
}

//  收集数据库文件

void CBugReporterDlg::collectDBFiles()
{
	CString dbPath = cacheDirPath + _T("\\.anyshare.cache");
	FileUtil::CopyDirectory(dbPath, dBDirPath);
}

//  收集AS文件

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

	// 配置文件
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

// 选择附加文件

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
					//从pc所指向的内存中解析出每个文件的名字,这里的fileName所占的内存不能和pc所占的内存发生冲突  
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
	// TODO: 在此添加专用代码和/或调用基类

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
