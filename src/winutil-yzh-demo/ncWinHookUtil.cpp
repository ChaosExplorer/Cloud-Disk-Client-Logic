/***************************************************************************************************
ncHookUtil.cpp:
	Copyright (c) Eisoo Software, Inc.(2004-2017), All rights reserved.

Purpose:
	Source file for winhook related utilities.

Author:
	Chaos

Created Time:
	2017-03-22
***************************************************************************************************/

/***************************************************************************************************
ini结构

;全局设置。
[Global]
;键值：0 最大化HOOK；1 最小化HOOK。
Mode=0

;不HOOK名单。
[Exclude]

;键名以Path0 Path1 Path2 方式呈现。 键值：路径。
Path0=C:\test\

;键名以Process0 Process1 Process2 方式呈现。 键值：进程。
Process0=test.exe


;HOOK名单。
[Include]

;键名以Path0 Path1 Path2 方式呈现。 键值：路径。
Path0=C:\test\

;键名以Process0 Process1 Process2 方式呈现。 键值：进程。
Process0=test.exe

;延迟注入名单。
[Delay]

;键名以Path0 Path1 Path2 方式呈现 与PathMode,PathCondtion联合使用。 键值：路径。
Path0=C:\test\
;路径延迟方式，键名对应Path。 键值：1 固定窗口等待；2 固定时间等待。
PathMode0=2
;路径延迟条件，键名对应Path。 键值：对应等待窗口与等待时间。
PathCondtion0=1000

;键名以Process0 Process1 Process2 方式呈现 与ProcessMode,ProcessCondtion联合使用。 键值：进程。
Process0=MindCAD2D.exe
;进程延迟方式，键名对应Process。 键值：1 固定窗口等待；2 固定时间等待。
ProcessMode0=1
;进程延迟条件，键名对应Process。 键值：对应等待窗口与等待时间。
ProcessCondtion0=AlphaSplashScreen


;条件HOOK名单。
[Limit]

;键名以Path0 Path1 Path2 方式呈现 与PathFun联合使用。 键值：路径。
Path0=C:\test\
;路径函数限制，键名对应Path。 键值：需要HOOK的函数，以|隔开。
PathFun0=NtCreateFile|CreateProcess

;键名以Process0 Process1 Process2 方式呈现 与ProcessFun联合使用。 键值：进程。
Process0=test.exe
;路径函数限制，键名对应rocess。 键值：需要HOOK的函数，以|隔开。
ProcessFun0=NtCreateFile|CreateProcess
***************************************************************************************************/

#include <abprec.h>
#include <atlbase.h>
#include <atlstr.h>
#include "winutil.h"

// private static
int ncWinHookUtil::FunNameToNo (TCHAR* funName)
{
	int ret = 0;
	CString strFunName;
	TCHAR *buf = _tcstok(funName, _T("|"));
	while (buf) {
		strFunName = buf;
		if (strFunName == _T("NtResumeThread"))
			ret |= LIMIT_FUN_NtResumeThread;
		if (strFunName == _T("NtCreateFile"))
			ret |= LIMIT_FUN_NtCreateFile;
		if (strFunName == _T("CreateProcess"))
			ret |= LIMIT_FUN_CreateProcess;
		if (strFunName == _T("GetFileAttributesEx"))
			ret |= LIMIT_FUN_GetFileAttributesEx;
		if (strFunName == _T("SHCreateShellFolderView"))
			ret |= LIMIT_FUN_SHCreateShellFolderView;
		if (strFunName == _T("NtQueryDirectoryFile"))
			ret |= LIMIT_FUN_NtQueryDirectoryFile;
		if (strFunName == _T("CoCreateInstance"))
			ret |= LIMIT_FUN_CoCreateInstance;
		if (strFunName == _T("SHFileOperation"))
			ret |= LIMIT_FUN_SHFileOperation;
		if (strFunName == _T("StartDoc"))
			ret |= LIMIT_FUN_StartDoc;
		if (strFunName == _T("StartPage"))
			ret |= LIMIT_FUN_StartPage;
		if (strFunName == _T("BitBlt"))
			ret |= LIMIT_FUN_BitBlt;

		buf = _tcstok(NULL, _T("|"));
	}
	return ret;
}

// public static
HANDLE ncWinHookUtil::CreateHookInfo (LPCTSTR lpPath)
{
	// [Global]
	DWORD hookMode = GetPrivateProfileInt(_T("Global"), _T("Mode"), HOOK_MODE_MAX, lpPath);

	TCHAR buf[MAX_PATH];
	(hookMode == HOOK_MODE_MAX) ;
	CString strKeyName, strKeyValue;

	tstring excludePath;
	tstring excludeProcess;
	tstring includePath;
	tstring includeProcess;
	tstring delayPath;
	tstring delayProcess;
	tstring limitPath;
	tstring limitProcess;

	// [Exclude]
	int i = 0;
	while (true) {
		strKeyName.Format(_T("Path%d"), i);
		ZeroMemory(buf, sizeof(TCHAR)*MAX_PATH);
		GetPrivateProfileString(_T("Exclude"), strKeyName, _T("\0"), buf, MAX_PATH, lpPath);

		if (buf[0] == _T('\0'))
			break;

		if (i > 0)
			excludePath += _T('|');
		excludePath += buf;

		i++;
	}

	i = 0;
	while (true) {
		strKeyName.Format(_T("Process%d"), i);
		ZeroMemory(buf, sizeof(TCHAR)*MAX_PATH);
		GetPrivateProfileString(_T("Exclude"), strKeyName, _T("\0"), buf, MAX_PATH, lpPath);
		if (buf[0] == _T('\0'))
			break;

		if (i > 0)
			excludeProcess += _T('|');
		excludeProcess += buf;

		i++;
	}

	// [Include]
	i = 0;
	while (true) {
		strKeyName.Format(_T("Path%d"), i);
		ZeroMemory(buf, sizeof(TCHAR)*MAX_PATH);
		GetPrivateProfileString(_T("Include"), strKeyName, _T("\0"), buf, MAX_PATH, lpPath);

		if (buf[0] == _T('\0'))
			break;

		if (i > 0)
			includePath += _T('|');
		includePath += buf;

		i++;
	}

	i = 0;
	while (true) {
		strKeyName.Format(_T("Process%d"), i);
		ZeroMemory(buf, sizeof(TCHAR)*MAX_PATH);
		GetPrivateProfileString(_T("Include"), strKeyName, _T("\0"), buf, MAX_PATH, lpPath);
		if (buf[0] == _T('\0'))
			break;

		if (i > 0)
			includeProcess += _T('|');
		includeProcess += buf;

		i++;
	}

	// [Delay]
	i = 0;
	while (true) {
		strKeyValue = _T("");
		strKeyName.Format(_T("Path%d"), i);
		ZeroMemory(buf, sizeof(TCHAR)*MAX_PATH);
		GetPrivateProfileString(_T("Delay"), strKeyName, _T("\0"), buf, MAX_PATH, lpPath);

		if (buf[0] == _T('\0'))
			break;
		strKeyValue += buf;

		strKeyName.Format(_T("PathMode%d"), i);
		int delayMode = GetPrivateProfileInt(_T("Delay"), strKeyName, 0, lpPath);
		if (delayMode <= DEALY_MODE_BEGIN || delayMode >= DEALY_MODE_END)
			break;
		strKeyValue += _T(';');
		strKeyValue.Format(_T("%s%d"), strKeyValue, delayMode);

		strKeyName.Format(_T("PathCondtion%d"), i);
		if (delayMode == DEALY_MODE_WINDOW) {
			ZeroMemory(buf, sizeof(TCHAR)*MAX_PATH);
			GetPrivateProfileString(_T("Delay"), strKeyName, _T("\0"), buf, MAX_PATH, lpPath);
			if (buf[0] == _T('\0'))
				break;
			strKeyValue += _T(';');
			strKeyValue += buf;
		} 
		else {
			int waitTime = GetPrivateProfileInt(_T("Delay"), strKeyName, -1, lpPath);
			if (waitTime == -1)
				break;
			strKeyValue += _T(';');
			strKeyValue.Format(_T("%s%d"), strKeyValue, waitTime);
		}

		if (i > 0)
			delayPath += _T('|');
		delayPath += strKeyValue;

		i++;
	}

	i = 0;
	while (true) {
		strKeyValue = _T("");
		strKeyName.Format(_T("Process%d"), i);
		ZeroMemory(buf, sizeof(TCHAR)*MAX_PATH);
		GetPrivateProfileString(_T("Delay"), strKeyName, _T("\0"), buf, MAX_PATH, lpPath);
		if (buf[0] == _T('\0'))
			break;
		strKeyValue += buf;

		strKeyName.Format(_T("ProcessMode%d"), i);
		int delayMode = GetPrivateProfileInt(_T("Delay"), strKeyName, 0, lpPath);
		if (delayMode <= DEALY_MODE_BEGIN || delayMode >= DEALY_MODE_END)
			break;
		strKeyValue += _T(';');
		strKeyValue.Format(_T("%s%d"), strKeyValue, delayMode);

		strKeyName.Format(_T("ProcessCondtion%d"), i);
		if (delayMode == DEALY_MODE_WINDOW) {
			ZeroMemory(buf, sizeof(TCHAR)*MAX_PATH);
			GetPrivateProfileString(_T("Delay"), strKeyName, _T("\0"), buf, MAX_PATH, lpPath);
			if (buf[0] == _T('\0'))
				break;
			strKeyValue += _T(';');
			strKeyValue += buf;
		} 
		else {
			int waitTime = GetPrivateProfileInt(_T("Delay"), strKeyName, -1, lpPath);
			if (waitTime == -1)
				break;
			strKeyValue += _T(';');
			strKeyValue.Format(_T("%s%d"), strKeyValue, waitTime);
		}


		if (i > 0)
			delayProcess += _T('|');
		delayProcess += strKeyValue;

		i++;
	}

	// [Limit]
	i = 0;
	while (true) {
		strKeyValue = _T("");
		strKeyName.Format(_T("Path%d"), i);
		ZeroMemory(buf, sizeof(TCHAR)*MAX_PATH);
		GetPrivateProfileString(_T("Limit"), strKeyName, _T("\0"), buf, MAX_PATH, lpPath);

		if (buf[0] == _T('\0'))
			break;
		strKeyValue = buf;

		strKeyName.Format(_T("PathFun%d"), i);
		ZeroMemory(buf, sizeof(TCHAR)*MAX_PATH);
		GetPrivateProfileString(_T("Limit"), strKeyName, _T("\0"), buf, MAX_PATH, lpPath);
		if (buf[0] == _T('\0'))
			break;
		int funNo = FunNameToNo(buf);
		if (funNo == 0)
			break;
		strKeyValue += _T(';');
		strKeyValue.Format(_T("%s%d"), strKeyValue, funNo);

		if (i > 0)
			limitPath += _T('|');
		limitPath += strKeyValue;

		i++;
	}
	
	i = 0;
	while (true) {
		strKeyValue = _T("");
		strKeyName.Format(_T("Process%d"), i);
		ZeroMemory(buf, sizeof(TCHAR)*MAX_PATH);
		GetPrivateProfileString(_T("Limit"), strKeyName, _T("\0"), buf, MAX_PATH, lpPath);
		if (buf[0] == _T('\0'))
			break;
		strKeyValue += buf;

		strKeyName.Format(_T("ProcessFun%d"), i);
		ZeroMemory(buf, sizeof(TCHAR)*MAX_PATH);
		GetPrivateProfileString(_T("Limit"), strKeyName, _T("\0"), buf, MAX_PATH, lpPath);
		if (buf[0] == _T('\0'))
			break;
		int funNo = FunNameToNo(buf);
		if (funNo == 0)
			break;
		strKeyValue += _T(';');
		strKeyValue.Format(_T("%s%d"), strKeyValue, funNo);

		if (i > 0)
			limitProcess += _T('|');
		limitProcess += strKeyValue;

		i++;
	}

	ncHookInfoHead head;
	head.size = sizeof(ncHookInfoHead);
	head.mode = hookMode;
	head.size += head.excludePathSize = excludePath.length()*sizeof(TCHAR);
	head.size += head.excludeProcessSize = excludeProcess.length()*sizeof(TCHAR);
	head.size += head.includePathSize = includePath.length()*sizeof(TCHAR);
	head.size += head.includeProcessSize = includeProcess.length()*sizeof(TCHAR);
	head.size += head.delayPathSize = delayPath.length()*sizeof(TCHAR);
	head.size += head.delayProcessSize = delayProcess.length()*sizeof(TCHAR);
	head.size += head.limitPathSize = limitPath.length()*sizeof(TCHAR);
	head.size += head.limitProcessSize = limitProcess.length()*sizeof(TCHAR);

	PSECURITY_DESCRIPTOR pSec = (PSECURITY_DESCRIPTOR)LocalAlloc(LMEM_FIXED, SECURITY_DESCRIPTOR_MIN_LENGTH);
	if(!pSec) {
		return NULL;
	}
	if(!InitializeSecurityDescriptor(pSec, SECURITY_DESCRIPTOR_REVISION)) {
		LocalFree(pSec);
		return NULL;
	}
	if(!SetSecurityDescriptorDacl(pSec, TRUE, NULL, TRUE)) {
		LocalFree(pSec);
		return NULL;
	}
	SECURITY_ATTRIBUTES attr;
	attr.bInheritHandle = FALSE;
	attr.lpSecurityDescriptor = pSec;
	attr.nLength = sizeof(SECURITY_ATTRIBUTES);

	HANDLE hMapFile = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		&attr,
		PAGE_READWRITE,
		0,
		head.size,
		_T("Global\\AnyShareHookInfo"));

	if (hMapFile != NULL) {
		LPTSTR pBuf = (LPTSTR) MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, head.size);
		if (pBuf != NULL){
			int offset = 0;
			CopyMemory((PVOID)pBuf, (PVOID)&head, sizeof(ncHookInfoHead));
			offset += sizeof(ncHookInfoHead);
			CopyMemory((PVOID)(pBuf + offset), excludePath.c_str(), head.excludePathSize);
			offset += head.excludePathSize;
			CopyMemory((PVOID)(pBuf + offset), excludeProcess.c_str(), head.excludeProcessSize);
			offset += head.excludeProcessSize;
			CopyMemory((PVOID)(pBuf + offset), includePath.c_str(), head.includePathSize);
			offset += head.includePathSize;
			CopyMemory((PVOID)(pBuf + offset), includeProcess.c_str(), head.includeProcessSize);
			offset += head.includeProcessSize;
			CopyMemory((PVOID)(pBuf + offset), delayPath.c_str(), head.delayPathSize);
			offset += head.delayPathSize;
			CopyMemory((PVOID)(pBuf + offset), delayProcess.c_str(), head.delayProcessSize);
			offset += head.delayProcessSize;
			CopyMemory((PVOID)(pBuf + offset), limitPath.c_str(), head.limitPathSize);
			offset += head.limitPathSize;
			CopyMemory((PVOID)(pBuf + offset), limitProcess.c_str(), head.limitProcessSize);
			UnmapViewOfFile(pBuf);
		}
	}

	return hMapFile;
}

void ncWinHookUtil::GetInjecInfo (unsigned long& hookMode,
								  vector<tstring>& excludePaths,
								  vector<tstring>& excludeProcesses,
								  vector<tstring>& includePaths,
								  vector<tstring>& includeProcesses)
{
	hookMode = HOOK_MODE_MAX;
	// 固定的无需注入名单程序
	excludeProcesses.push_back(_T("sharetool.exe"));						// sharetool进程 避免产生注入死循环
	excludeProcesses.push_back(_T("sharesvc.exe"));							// sharesvc服务
	excludeProcesses.push_back(_T("x64Launcher.exe"));						// CS游戏软件的代理子进程 具体参看DAEG-12319 避免产生注入死循环
	excludeProcesses.push_back(_T("bipc.exe"));								// 浙江农信
	//excludeProcesses.push_back(_T("rundll32.exe"));							// dll载入进程(丹阳公安局IE) 时间很短就结束调用
	excludeProcesses.push_back(_T("QZDMTerm.exe"));							// 七州黑匣子
	excludeProcesses.push_back(_T("splwow64.exe"));							// 常德卷烟厂 打印程序
	excludeProcesses.push_back(_T("zhp.exe"));								// 中孚三合一
	excludeProcesses.push_back(_T("dwm.exe"));								// 桌面窗口管理
	excludeProcesses.push_back(_T("ScanFrm.exe"));							// 瑞星扫描
	excludeProcesses.push_back(_T("RsMain.exe"));							// 瑞星主程序
	excludeProcesses.push_back(_T("csh.exe"));								// 惠州国展genesis9.2c调用脚本程序
	excludeProcesses.push_back(_T("dxp.exe"));								// 惠州国展设计软件
	excludeProcesses.push_back(_T("igfxsrvc.exe"));							// intel显卡右键
	excludeProcesses.push_back(_T("DeviceDisplayObjectProvider.exe"));		// 打印机显示界面
	excludeProcesses.push_back(_T("runonce.exe"));							// 系统启动运行程序
	excludeProcesses.push_back(_T("HelpPane.exe"));							// 系统帮助程序
	excludeProcesses.push_back(_T("ShellExtEx.exe"));						// caxa cad 预览程序 自动触发下载卡住
	excludeProcesses.push_back(_T("AcroRd32Info.exe"));						// pdf 预览程序 自动触发下载卡住
	excludeProcesses.push_back(_T("sldShellExtServer.exe"));				// sw 预览程序 自动触发下载卡住
	excludeProcesses.push_back(_T("fontdrvhost.exe"));						// win10字体程序，注入了会产生奇怪影响
	excludeProcesses.push_back(_T("bybase.exe"));							// 四方冷链 打印
	excludeProcesses.push_back(_T("bypart.exe"));							// 四方冷链 打印
	excludeProcesses.push_back(_T("bywork.exe"));							// 四方冷链 打印
	excludeProcesses.push_back(_T("fppdis5.exe"));							// pdfFactoryPro 打印程序
	excludeProcesses.push_back(_T("ViGUARD.exe"));							// LANdesk程序（萧山农商银行冲突）

	// 固定的必须注入名单程序
	includeProcesses.push_back(_T("tray.exe"));								// tray需要注入，注入函数NtResumeThread
	includeProcesses.push_back(_T("explorer.exe"));							// explorer系统资源管理器必须注入
	includeProcesses.push_back(_T("svchost.exe"));							// svchost核心进程，注入函数NtResumeThread
	includeProcesses.push_back(_T("dllhost.exe"));							// dllhost图片浏览相关，注入函数NtCreateFile

	HANDLE hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, _T("Global\\AnyShareHookInfo"));
	if (hMapFile != NULL) {
		ncHookInfoHead head;
		LPCTSTR pBuf = (LPCTSTR)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(ncHookInfoHead));
		CopyMemory((PVOID)&head, (PVOID)pBuf, sizeof(ncHookInfoHead));
		UnmapViewOfFile(pBuf);

		hookMode = head.mode;

		pBuf = (LPCTSTR)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, head.size);

		TCHAR *subBuf, *splitBuf;
		unsigned long offset = 0;

 		offset += sizeof(ncHookInfoHead);
		subBuf = new TCHAR[head.excludePathSize/sizeof(TCHAR)+1];
		ZeroMemory(subBuf, head.excludePathSize+2);
		CopyMemory((PVOID)subBuf, (PVOID)(pBuf + offset), head.excludePathSize);
		splitBuf = _tcstok(subBuf, _T("|"));
		while (splitBuf) {
			excludePaths.push_back(splitBuf);
			splitBuf = _tcstok(NULL, _T("|"));
		}
		delete subBuf;

		offset += head.excludePathSize;
		subBuf = new TCHAR[head.excludeProcessSize/sizeof(TCHAR)+1];
		ZeroMemory(subBuf, head.excludeProcessSize+2);
		CopyMemory((PVOID)subBuf, (PVOID)(pBuf + offset), head.excludeProcessSize);
		splitBuf = _tcstok(subBuf, _T("|"));
		while (splitBuf) {
			excludeProcesses.push_back(splitBuf);
			splitBuf = _tcstok(NULL, _T("|"));
		}
		delete subBuf;

		offset += head.excludeProcessSize;
		subBuf = (TCHAR *)malloc(head.includePathSize+2);
		ZeroMemory(subBuf, head.includePathSize+2);
		CopyMemory((PVOID)subBuf, (PVOID)(pBuf + offset), head.includePathSize);
		splitBuf = _tcstok(subBuf, _T("|"));
		while (splitBuf) {
			includePaths.push_back(splitBuf);
			splitBuf = _tcstok(NULL, _T("|"));
		}
		free(subBuf);

		offset += head.includePathSize;
		subBuf = new TCHAR[head.includeProcessSize/sizeof(TCHAR)+1];
		ZeroMemory(subBuf, head.includeProcessSize+2);
		CopyMemory((PVOID)subBuf, (PVOID)(pBuf + offset), head.includeProcessSize);
		splitBuf = _tcstok(subBuf, _T("|"));
		while (splitBuf) {
			includeProcesses.push_back(splitBuf);
			splitBuf = _tcstok(NULL, _T("|"));
		}
		delete subBuf;

		UnmapViewOfFile(pBuf);

		CloseHandle(hMapFile);
	}
}

void ncWinHookUtil::GetDelayInfo (vector<ncHookInfoDelay>& delayPath, vector<ncHookInfoDelay>& delayProcess)
{
	// 固定延迟注入
	ncHookInfoDelay temp;
	// MindCAD2D程序，需要等待窗口AlphaSplashScreen结束后再注入(MindCAD)
	temp.dest = _T("MindCAD2D.exe");
	temp.mode = DEALY_MODE_WINDOW;
	temp.condtion = _T("AlphaSplashScreen");
	delayProcess.push_back(temp);

	// ZWCAD程序，因为无法获取载入窗口类，需要等待10秒结束后再注入，(中望CAD)
	temp.dest = _T("ZWCAD.exe");
	temp.mode = DEALY_MODE_TIME;
	temp.condtion = _T("10000");
	delayProcess.push_back(temp);

	// office三种常用软件延迟注入
	temp.dest = _T("WINWORD.EXE");
	temp.mode = DEALY_MODE_TIME;
	temp.condtion = _T("1000");
	delayProcess.push_back(temp);
	temp.dest = _T("EXCEL.EXE");
	temp.mode = DEALY_MODE_TIME;
	temp.condtion = _T("1000");
	delayProcess.push_back(temp);
	temp.dest = _T("POWERPNT.EXE");
	temp.mode = DEALY_MODE_TIME;
	temp.condtion = _T("1000");
	delayProcess.push_back(temp);

	// SLDWORKS程序，需要等待窗口splash结束后再注入(SolidWorks 2014)
	temp.dest = _T("SLDWORKS.exe");
	temp.mode = DEALY_MODE_WINDOW;
	temp.condtion = _T("splash");
	delayProcess.push_back(temp);

	HANDLE hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, _T("Global\\AnyShareHookInfo"));
	if (hMapFile != NULL) {
		ncHookInfoHead head;
		LPCTSTR pBuf = (LPCTSTR)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(ncHookInfoHead));
		CopyMemory((PVOID)&head, (PVOID)pBuf, sizeof(ncHookInfoHead));
		UnmapViewOfFile(pBuf);

		pBuf = (LPCTSTR)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, head.size);

		TCHAR *subBuf, *splitBuf;
		unsigned long offset = 0;

		offset += sizeof(ncHookInfoHead);
		offset += head.excludePathSize;
		offset += head.excludeProcessSize;
		offset += head.includePathSize;
		offset += head.includeProcessSize;
		subBuf = new TCHAR[head.delayPathSize/sizeof(TCHAR)+1];
		ZeroMemory(subBuf, head.delayPathSize+2);
		CopyMemory((PVOID)subBuf, (PVOID)(pBuf + offset), head.delayPathSize);
		splitBuf = _tcstok(subBuf, _T("|"));
		while (splitBuf) {
			TCHAR * pTemp = _tcstok(splitBuf, _T(";"));
			if (pTemp != NULL) {
				temp.dest = pTemp;
				pTemp = _tcstok(NULL, _T(";"));
				if (pTemp != NULL) {
					temp.mode = _tcstoul(pTemp, 0, 10);
					pTemp = _tcstok(NULL, _T(";"));
					if (pTemp != NULL) {
						temp.condtion = pTemp;
						delayPath.push_back(temp);
					}
				}
			}
			splitBuf = _tcstok(NULL, _T("|"));
		}
		delete subBuf;

		offset += head.delayPathSize;
		subBuf = new TCHAR[head.delayProcessSize/sizeof(TCHAR)+1];
		ZeroMemory(subBuf, head.delayProcessSize+2);
		CopyMemory((PVOID)subBuf, (PVOID)(pBuf + offset), head.delayProcessSize);
		splitBuf = _tcstok(subBuf, _T("|"));
		while (splitBuf) {
			TCHAR * pTemp = _tcstok(splitBuf, _T(";"));
			if (pTemp != NULL) {
				temp.dest = pTemp;
				pTemp = _tcstok(NULL, _T(";"));
				if (pTemp != NULL) {
					temp.mode = _tcstoul(pTemp, 0, 10);
					pTemp = _tcstok(NULL, _T(";"));
					if (pTemp != NULL) {
						temp.condtion = pTemp;
						delayProcess.push_back(temp);
					}
				}
			}
			splitBuf = _tcstok(NULL, _T("|"));
		}
		delete subBuf;

		UnmapViewOfFile(pBuf);

		CloseHandle(hMapFile);
	}
}

void ncWinHookUtil::GetLimitInfo (vector<ncHookInfoLimit>& limitPath, vector<ncHookInfoLimit>& limitProcess)
{
	// 固定HOOK函数限制
	// 常德卷烟厂 和+ 客户端 不对CreateProcess进行HOOK
	ncHookInfoLimit temp;
	temp.dest = _T("mansion.exe");
	temp.limit = LIMIT_FUN_CreateProcess;

	HANDLE hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, _T("Global\\AnyShareHookInfo"));
	if (hMapFile != NULL) {
		ncHookInfoHead head;
		LPCTSTR pBuf = (LPCTSTR)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(ncHookInfoHead));
		CopyMemory((PVOID)&head, (PVOID)pBuf, sizeof(ncHookInfoHead));
		UnmapViewOfFile(pBuf);

		pBuf = (LPCTSTR)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, head.size);

		TCHAR *subBuf, *splitBuf;
		unsigned long offset = 0;

		offset += sizeof(ncHookInfoHead);
		offset += head.excludePathSize;
		offset += head.excludeProcessSize;
		offset += head.includePathSize;
		offset += head.includeProcessSize;
		offset += head.delayPathSize;
		offset += head.delayProcessSize;
		subBuf = new TCHAR[head.limitPathSize/sizeof(TCHAR)+1];
		ZeroMemory(subBuf, head.limitPathSize+2);
		CopyMemory((PVOID)subBuf, (PVOID)(pBuf + offset), head.limitPathSize);
		splitBuf = _tcstok(subBuf, _T("|"));
		while (splitBuf) {
			ncHookInfoLimit temp;
			TCHAR * pTemp = _tcstok(splitBuf, _T(";"));
			if (pTemp != NULL) {
				temp.dest = pTemp;
				pTemp = _tcstok(NULL, _T(";"));
				if (pTemp != NULL) {
					temp.limit = _tcstoul(pTemp, 0, 10);
					limitPath.push_back(temp);
				}
			}
			splitBuf = _tcstok(NULL, _T("|"));
		}
		delete subBuf;

		offset += head.limitPathSize;
		subBuf = new TCHAR[head.limitProcessSize/sizeof(TCHAR)+1];
		ZeroMemory(subBuf, head.limitProcessSize+2);
		CopyMemory((PVOID)subBuf, (PVOID)(pBuf + offset), head.limitProcessSize);
		splitBuf = _tcstok(subBuf, _T("|"));
		while (splitBuf) {
			ncHookInfoLimit temp;
			TCHAR * pTemp = _tcstok(splitBuf, _T(";"));
			if (pTemp != NULL) {
				temp.dest = pTemp;
				pTemp = _tcstok(NULL, _T(";"));
				if (pTemp != NULL) {
					temp.limit = _tcstoul(pTemp, 0, 10);
					limitProcess.push_back(temp);
				}
			}
			splitBuf = _tcstok(NULL, _T("|"));
		}
		delete subBuf;

		UnmapViewOfFile(pBuf);

		CloseHandle(hMapFile);
	}
}



