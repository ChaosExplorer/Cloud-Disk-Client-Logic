/***************************************************************************************************
ncProcessUtil.cpp:
	Copyright (c) Eisoo Software, Inc.(2004-2015), All rights reserved.

Purpose:
	Source file for process related utilities.

Author:
	Chaos

Created Time:
	2015-03-06
***************************************************************************************************/

#include <abprec.h>
#include <atlbase.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <shlobj.h>
#include <shellapi.h>
#include "winutil.h"

typedef BOOL (WINAPI* ISWOW64PROCESS)(HANDLE, PBOOL);

// public static
BOOL ncProcessUtil::AdjustProcessPrivilege (HANDLE hProcess, LPCTSTR lpPrivName, BOOL bEnable)
{
	ncScopedHandle hToken = NULL;

	if (!OpenProcessToken (hProcess, TOKEN_ADJUST_PRIVILEGES, &hToken))
		return FALSE;

	LUID luid;
	if (!LookupPrivilegeValue (NULL, lpPrivName, &luid))
		return FALSE;

	TOKEN_PRIVILEGES tp;
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = (bEnable ? SE_PRIVILEGE_ENABLED : 0);

	return AdjustTokenPrivileges (hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
}

// public static
HANDLE ncProcessUtil::CreateUACElevatedProcess (LPCTSTR lpAppPath, LPCTSTR lpCmdLine)
{
	if (lpAppPath == NULL || lpCmdLine == NULL)
		return NULL;

	if (IsUserAnAdmin () || !ncOSUtil::IsUACEnabled ())
		return NULL;

	SHELLEXECUTEINFO sei = { 0 };
	sei.cbSize = sizeof(SHELLEXECUTEINFO);
	sei.fMask = SEE_MASK_FLAG_DDEWAIT | SEE_MASK_FLAG_NO_UI | SEE_MASK_NOCLOSEPROCESS;
	sei.lpVerb = _T("runas");
	sei.lpFile = lpAppPath;
	sei.lpParameters = lpCmdLine;
	sei.nShow = SW_SHOW;

	return (ShellExecuteEx (&sei) ? sei.hProcess : NULL);
}

// public static
BOOL ncProcessUtil::Is32BitProcess (HANDLE hProcess)
{
	if (ncOSUtil::Is32BitOS ())
		return TRUE;

	ISWOW64PROCESS fpIsWow64Process = 
		(ISWOW64PROCESS)GetProcAddress (GetModuleHandle (_T("kernel32")), "IsWow64Process");

	BOOL bIsWow64 = FALSE;

	if (fpIsWow64Process != NULL)
		fpIsWow64Process (hProcess, &bIsWow64);

	return bIsWow64;
}

// public static
tstring ncProcessUtil::GetCurrentProcessName ()
{
	TCHAR szProcName[MAX_PATH];
	if (GetModuleFileName (NULL, szProcName, MAX_PATH) == 0)
		return _T("");

	TCHAR* lpStr = _tcsrchr (szProcName, _T('\\'));
	return (++lpStr);
}

// public static
tstring ncProcessUtil::GetCurrentProcessDir ()
{
	TCHAR szProcName[MAX_PATH];
	if (GetModuleFileName (NULL, szProcName, MAX_PATH) == 0)
		return _T("");

	TCHAR* lpStr = _tcsrchr (szProcName, _T('\\'));
	lpStr++;
	* lpStr = _T('\0');
	return szProcName;
}

// public static
tstring ncProcessUtil::GetProcessName (HANDLE hProcess)
{
	TCHAR szProcName[MAX_PATH];
	if (GetProcessImageFileName (hProcess, szProcName, MAX_PATH) == 0)
		return _T("");

	TCHAR* lpStr = _tcsrchr (szProcName, _T('\\'));
	return (++lpStr);
}

// public static
void ncProcessUtil::GetProcessPathAndName (HANDLE hProcess, tstring &path, tstring &name)
{
	TCHAR tsFileDosPath[MAX_PATH];
	ZeroMemory(tsFileDosPath, sizeof(TCHAR)*(MAX_PATH));
	if (0 == GetProcessImageFileName(hProcess, tsFileDosPath, MAX_PATH))
	{
		path = _T("");
		name = _T("");
		return;
	}

	TCHAR* lpStr = _tcsrchr (tsFileDosPath, _T('\\'));
	name = ++lpStr;
	*lpStr = _T('\0');

	// 获取Logic Drive String长度
	UINT uiLen = GetLogicalDriveStrings(0, NULL);
	if (0 == uiLen) {
		return;
	}

	PTSTR pLogicDriveString = new TCHAR[uiLen + 1];
	ZeroMemory(pLogicDriveString, uiLen + 1);
	uiLen = GetLogicalDriveStrings(uiLen, pLogicDriveString);
	if (0 == uiLen) {
		delete[]pLogicDriveString;
		return;
	}

	TCHAR szDrive[3] = TEXT(" :");
	PTSTR pDosDriveName = new TCHAR[MAX_PATH];
	PTSTR pLogicIndex = pLogicDriveString;

	do {
		szDrive[0] = *pLogicIndex;
		uiLen = QueryDosDevice(szDrive, pDosDriveName, MAX_PATH);
		if (0 == uiLen)
		{
			if (ERROR_INSUFFICIENT_BUFFER != GetLastError())
			{
				break;
			}

			delete[]pDosDriveName;
			pDosDriveName = new TCHAR[uiLen + 1];
			uiLen = QueryDosDevice(szDrive, pDosDriveName, uiLen + 1);
			if (0 == uiLen)
			{
				break;
			}
		}

		uiLen = _tcslen(pDosDriveName);
		if (0 == _tcsnicmp(tsFileDosPath, pDosDriveName, uiLen))
		{
			path += szDrive;
			path += (tsFileDosPath + uiLen);
			break;
		}

		while (*pLogicIndex++);
	} while (*pLogicIndex);

	delete[]pLogicDriveString;
	delete[]pDosDriveName;
}

// public static
DWORD ncProcessUtil::GetProcessIdOfThread (HANDLE hThread)
{
	DWORD dwProcId = 0;

	NTQUERYINFORMATIONTHREAD fpNtQueryInformationThread = 
		(NTQUERYINFORMATIONTHREAD)GetProcAddress (GetModuleHandle (_T("ntdll")), "NtQueryInformationThread");

	if (fpNtQueryInformationThread == NULL)
		return 0;

	THREAD_BASIC_INFORMATION threadInfo;
	NTSTATUS status = fpNtQueryInformationThread (hThread, ThreadBasicInformation, &threadInfo, sizeof(THREAD_BASIC_INFORMATION), NULL);

	if (NT_SUCCESS (status))
	{
		dwProcId = (DWORD)threadInfo.ClientId.UniqueProcess;
	}

	return dwProcId;
}

// public static
DWORD ncProcessUtil::GetProcessIdByName (LPCTSTR lpProcName, DWORD dwSessionId/* = -1*/)
{
	DWORD dwProcId = 0;

	if (lpProcName == NULL)
		return 0;

	ncScopedHandle hSnapShot = CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0);
	if (hSnapShot == INVALID_HANDLE_VALUE)
		return 0;

	PROCESSENTRY32 pe32 = { sizeof (pe32) };
	if (Process32First (hSnapShot, &pe32))
	{
		do
		{
			if (_tcsicmp (pe32.szExeFile, lpProcName) == 0)
			{
				if (dwSessionId != -1)
				{
					DWORD dwSessId;
					if (!ProcessIdToSessionId (pe32.th32ProcessID, &dwSessId) || dwSessionId != dwSessId)
						continue;
				}

				dwProcId = pe32.th32ProcessID;
				break;
			}
		}
		while (Process32Next (hSnapShot, &pe32));
	}

	return dwProcId;
}

// public static
void ncProcessUtil::GetProcessChildren (DWORD dwProcId, vector<DWORD>& dwPids)
{
	dwPids.clear ();

	FILETIME ftCreate1, ftExit1, ftKernel1, ftUser1;
	FILETIME ftCreate2, ftExit2, ftKernel2, ftUser2;

	ncScopedHandle hProc = OpenProcess (MAXIMUM_ALLOWED, FALSE, dwProcId);
	GetProcessTimes (hProc, &ftCreate1, &ftExit1, &ftKernel1, &ftUser1);

	ncScopedHandle hSnapshot = CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE)
		return;

	PROCESSENTRY32 pe32 = { sizeof(pe32) };
	if (!Process32First (hSnapshot, &pe32))
		return;

	do
	{
		if (pe32.th32ParentProcessID  == dwProcId)
		{
			hProc = OpenProcess (MAXIMUM_ALLOWED, FALSE, pe32.th32ProcessID);
			GetProcessTimes (hProc, &ftCreate2, &ftExit2, &ftKernel2, &ftUser2);

			if (CompareFileTime (&ftCreate1, &ftCreate2) == -1)
				dwPids.push_back (pe32.th32ProcessID);
		}
	}
	while (Process32Next (hSnapshot, &pe32));
}

// public static
DWORD ncProcessUtil::GetProcessThreadCount (DWORD dwProcId)
{
	DWORD dwCount = 0;

	ncScopedHandle hSnapshot = CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0);
	if (hSnapshot != INVALID_HANDLE_VALUE)
	{
		PROCESSENTRY32 pe32 = { sizeof(pe32) };
		if (Process32First (hSnapshot, &pe32))
		{
			do
			{
				if (pe32.th32ProcessID == dwProcId)
				{
					dwCount = pe32.cntThreads;
					break;
				}
			}
			while (Process32Next (hSnapshot, &pe32));
		}
	}
	
	return dwCount;
}

// public static
HMODULE ncProcessUtil::GetProcessModuleHandle (HANDLE hProcess, LPCTSTR lpModName)
{
	HMODULE hMod = NULL;

	if (lpModName == NULL)
		return NULL;

	ncScopedHandle hSnapShot = CreateToolhelp32Snapshot (TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, GetProcessId(hProcess));
	if (hSnapShot != INVALID_HANDLE_VALUE)
	{
		MODULEENTRY32 me32 = { sizeof(MODULEENTRY32) };
		if (Module32First (hSnapShot, &me32))
		{
			do
			{
				if (_tcsicmp (lpModName, me32.szModule) == 0)
				{
					hMod = me32.hModule;
					break;
				}
			} while (Module32Next(hSnapShot, &me32));
		}
	}

	return hMod;
}

// public static
PVOID ncProcessUtil::GetProcessFuncAddress (HANDLE hProcess, LPCTSTR lpModName, LPCSTR lpFuncName)
{
	PVOID pModAddr = NULL;
	TCHAR szModPath[MAX_PATH];

	if (lpModName == NULL || lpFuncName == NULL)
		return NULL;

	ncScopedHandle hSnapShot = CreateToolhelp32Snapshot (TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, GetProcessId(hProcess));
	if (hSnapShot != INVALID_HANDLE_VALUE)
	{
		MODULEENTRY32 me32 = { sizeof(MODULEENTRY32) };
		if (Module32First (hSnapShot, &me32))
		{
			do
			{
				if (_tcsicmp (lpModName, me32.szModule) == 0)
				{
					pModAddr = (PVOID)me32.modBaseAddr;
					_tcscpy (szModPath, me32.szExePath);
					break;
				}
			} while (Module32Next (hSnapShot, &me32));
		}
	}

	if (pModAddr == NULL)
		return NULL;

	CT2CA lpModPath ((LPCTSTR)szModPath);
	DWORD dwFuncAddr = ncDebugUtil::GetImageFuncAddress (lpModPath, lpFuncName);

	if (dwFuncAddr == 0)
		return NULL;

	return (PVOID)((DWORD)pModAddr + dwFuncAddr);
}

// public static
PTOKEN_USER ncProcessUtil::GetProcessTokenUser (HANDLE hProcess)
{
	PTOKEN_USER pTokenUser = NULL;

	ncScopedHandle hToken = NULL;
	if (!OpenProcessToken (hProcess, TOKEN_READ, &hToken))
		return NULL;

	DWORD dwSize = 0;
	if (!GetTokenInformation (hToken, TokenUser, NULL, 0, &dwSize) && GetLastError () == ERROR_INSUFFICIENT_BUFFER)
	{
		pTokenUser = reinterpret_cast<PTOKEN_USER>(new BYTE[dwSize]);
		if (!GetTokenInformation (hToken, TokenUser, pTokenUser, dwSize, &dwSize))
		{
			delete[] pTokenUser;
			pTokenUser = NULL;
		}
	}

	return pTokenUser;
}

// public static
ncProcessFlags ncProcessUtil::GetProcessFlags (HANDLE hProcess)
{
	NTQUERYINFORMATIONPROCESS fpNtQueryInformationProcess = 
		(NTQUERYINFORMATIONPROCESS)GetProcAddress (GetModuleHandle (_T("ntdll")), "NtQueryInformationProcess");

	if (fpNtQueryInformationProcess == NULL)
		return NC_PF_UNKNOWN;

	PROCESS_BASIC_INFORMATION_EX pbi;
	NTSTATUS Status = fpNtQueryInformationProcess (hProcess, ProcessBasicInformation, &pbi, sizeof(pbi), NULL);

	if (!NT_SUCCESS (Status) || pbi.PebBaseAddress == NULL)
		return NC_PF_UNKNOWN;

	PEBEX peb;
	if (!ReadProcessMemory (hProcess, pbi.PebBaseAddress, &peb, sizeof(PEBEX), NULL))
		return NC_PF_UNKNOWN;

	if (peb.BeingDebugged)
		return NC_PF_DEBUGGED;

	if (peb.Ldr == NULL || peb.LoaderLock == NULL)
		return NC_PF_UNINITIALIZED;

	return NC_PF_NORMAL;
}
