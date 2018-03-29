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
ini�ṹ

;ȫ�����á�
[Global]
;��ֵ��0 ���HOOK��1 ��С��HOOK��
Mode=0

;��HOOK������
[Exclude]

;������Path0 Path1 Path2 ��ʽ���֡� ��ֵ��·����
Path0=C:\test\

;������Process0 Process1 Process2 ��ʽ���֡� ��ֵ�����̡�
Process0=test.exe


;HOOK������
[Include]

;������Path0 Path1 Path2 ��ʽ���֡� ��ֵ��·����
Path0=C:\test\

;������Process0 Process1 Process2 ��ʽ���֡� ��ֵ�����̡�
Process0=test.exe

;�ӳ�ע��������
[Delay]

;������Path0 Path1 Path2 ��ʽ���� ��PathMode,PathCondtion����ʹ�á� ��ֵ��·����
Path0=C:\test\
;·���ӳٷ�ʽ��������ӦPath�� ��ֵ��1 �̶����ڵȴ���2 �̶�ʱ��ȴ���
PathMode0=2
;·���ӳ�������������ӦPath�� ��ֵ����Ӧ�ȴ�������ȴ�ʱ�䡣
PathCondtion0=1000

;������Process0 Process1 Process2 ��ʽ���� ��ProcessMode,ProcessCondtion����ʹ�á� ��ֵ�����̡�
Process0=MindCAD2D.exe
;�����ӳٷ�ʽ��������ӦProcess�� ��ֵ��1 �̶����ڵȴ���2 �̶�ʱ��ȴ���
ProcessMode0=1
;�����ӳ�������������ӦProcess�� ��ֵ����Ӧ�ȴ�������ȴ�ʱ�䡣
ProcessCondtion0=AlphaSplashScreen


;����HOOK������
[Limit]

;������Path0 Path1 Path2 ��ʽ���� ��PathFun����ʹ�á� ��ֵ��·����
Path0=C:\test\
;·���������ƣ�������ӦPath�� ��ֵ����ҪHOOK�ĺ�������|������
PathFun0=NtCreateFile|CreateProcess

;������Process0 Process1 Process2 ��ʽ���� ��ProcessFun����ʹ�á� ��ֵ�����̡�
Process0=test.exe
;·���������ƣ�������Ӧrocess�� ��ֵ����ҪHOOK�ĺ�������|������
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
	// �̶�������ע����������
	excludeProcesses.push_back(_T("sharetool.exe"));						// sharetool���� �������ע����ѭ��
	excludeProcesses.push_back(_T("sharesvc.exe"));							// sharesvc����
	excludeProcesses.push_back(_T("x64Launcher.exe"));						// CS��Ϸ����Ĵ����ӽ��� ����ο�DAEG-12319 �������ע����ѭ��
	excludeProcesses.push_back(_T("bipc.exe"));								// �㽭ũ��
	//excludeProcesses.push_back(_T("rundll32.exe"));							// dll�������(����������IE) ʱ��ܶ̾ͽ�������
	excludeProcesses.push_back(_T("QZDMTerm.exe"));							// ���ݺ�ϻ��
	excludeProcesses.push_back(_T("splwow64.exe"));							// ���¾��̳� ��ӡ����
	excludeProcesses.push_back(_T("zhp.exe"));								// ��������һ
	excludeProcesses.push_back(_T("dwm.exe"));								// ���洰�ڹ���
	excludeProcesses.push_back(_T("ScanFrm.exe"));							// ����ɨ��
	excludeProcesses.push_back(_T("RsMain.exe"));							// ����������
	excludeProcesses.push_back(_T("csh.exe"));								// ���ݹ�չgenesis9.2c���ýű�����
	excludeProcesses.push_back(_T("dxp.exe"));								// ���ݹ�չ������
	excludeProcesses.push_back(_T("igfxsrvc.exe"));							// intel�Կ��Ҽ�
	excludeProcesses.push_back(_T("DeviceDisplayObjectProvider.exe"));		// ��ӡ����ʾ����
	excludeProcesses.push_back(_T("runonce.exe"));							// ϵͳ�������г���
	excludeProcesses.push_back(_T("HelpPane.exe"));							// ϵͳ��������
	excludeProcesses.push_back(_T("ShellExtEx.exe"));						// caxa cad Ԥ������ �Զ��������ؿ�ס
	excludeProcesses.push_back(_T("AcroRd32Info.exe"));						// pdf Ԥ������ �Զ��������ؿ�ס
	excludeProcesses.push_back(_T("sldShellExtServer.exe"));				// sw Ԥ������ �Զ��������ؿ�ס
	excludeProcesses.push_back(_T("fontdrvhost.exe"));						// win10�������ע���˻�������Ӱ��
	excludeProcesses.push_back(_T("bybase.exe"));							// �ķ����� ��ӡ
	excludeProcesses.push_back(_T("bypart.exe"));							// �ķ����� ��ӡ
	excludeProcesses.push_back(_T("bywork.exe"));							// �ķ����� ��ӡ
	excludeProcesses.push_back(_T("fppdis5.exe"));							// pdfFactoryPro ��ӡ����
	excludeProcesses.push_back(_T("ViGUARD.exe"));							// LANdesk������ɽũ�����г�ͻ��

	// �̶��ı���ע����������
	includeProcesses.push_back(_T("tray.exe"));								// tray��Ҫע�룬ע�뺯��NtResumeThread
	includeProcesses.push_back(_T("explorer.exe"));							// explorerϵͳ��Դ����������ע��
	includeProcesses.push_back(_T("svchost.exe"));							// svchost���Ľ��̣�ע�뺯��NtResumeThread
	includeProcesses.push_back(_T("dllhost.exe"));							// dllhostͼƬ�����أ�ע�뺯��NtCreateFile

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
	// �̶��ӳ�ע��
	ncHookInfoDelay temp;
	// MindCAD2D������Ҫ�ȴ�����AlphaSplashScreen��������ע��(MindCAD)
	temp.dest = _T("MindCAD2D.exe");
	temp.mode = DEALY_MODE_WINDOW;
	temp.condtion = _T("AlphaSplashScreen");
	delayProcess.push_back(temp);

	// ZWCAD������Ϊ�޷���ȡ���봰���࣬��Ҫ�ȴ�10���������ע�룬(����CAD)
	temp.dest = _T("ZWCAD.exe");
	temp.mode = DEALY_MODE_TIME;
	temp.condtion = _T("10000");
	delayProcess.push_back(temp);

	// office���ֳ�������ӳ�ע��
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

	// SLDWORKS������Ҫ�ȴ�����splash��������ע��(SolidWorks 2014)
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
	// �̶�HOOK��������
	// ���¾��̳� ��+ �ͻ��� ����CreateProcess����HOOK
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



