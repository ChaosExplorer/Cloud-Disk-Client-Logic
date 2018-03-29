/**
*	@Created Time : 2016-6-29
*	@Author : Chaos
**/

#include "stdafx.h"
#include "SysInfoTool.h"
#include <windows.h>
#include <stdio.h>
#include "FileUtil.h"

#pragma comment(lib, "Advapi32.lib")

typedef LONG (_stdcall* RTLGETVERSION)(PRTL_OSVERSIONINFOEXW lpOsvi);

SysInfoTool::SysInfoTool()
{
}


SysInfoTool::~SysInfoTool()
{
}

string SysInfoTool::getSystemInfo()
{
	string result;

	result.append("操作系统版本：  ");
	result.append(OSString[GetOSVersion()]);

	result.append("\r\n");
	
	result.append("操作系统位数：  ");
	result.append(Is32BitOS() ? "32" : "64");

	result.append("\r\n");

	result.append("当前用户：  ");
	TCHAR  lpBuffer[100];
	DWORD size = 100;
	LPDWORD lpnSize = &size;
	GetUserName(lpBuffer, lpnSize);
	CString temp = lpBuffer;
	result.append(FileUtil::CString2str(temp));
	result.append("\r\n");

	result.append("\r\n 环境变量 :  \r\n\r\n");
	char *pathvar;
	pathvar = getenv("PATH");

	result.append(pathvar);

	return result;
}

void SysInfoTool::exportEventLog(const CString& outputPath)
{
	try
	{
		CFileFind finder;
		if (finder.FindFile(outputPath))
			DeleteFile(outputPath);
		finder.Close();

		/*
		#include <winevt.h>
		
		CTime time = GetCurrentTime();
		int year = time.GetYear();
		int month = time.GetMonth();
		int day = time.GetDay();

		char temp[100];
		sprintf_s(temp, "Event//System[@TimeCreated SystemTime > date('%d-%d-%d')]", year, month, day -3);

		CString queryStr = temp;
	
		LPWSTR pPath = _T("Application");

		if (!EvtExportLog(NULL, pPath, NULL, outputPath, EvtExportLogChannelPath))
			wprintf(_T("EvtExportLog failed for initial export with %lu.\n"), GetLastError());

		*/

		LPWSTR pPath = _T("Application");
		
		HANDLE h = OpenEventLog (NULL, pPath);

		if (!BackupEventLog(h, outputPath))
			wprintf(_T("EvtExportLog failed for initial export with %lu.\n"), GetLastError());

	}
	catch (...) {
		wprintf(_T("EvtExportLog failed for initial export with %lu.\n"), GetLastError());
	}
}

// public static
BOOL SysInfoTool::Is32BitOS ()
{
	SYSTEM_INFO si;
	GetNativeSystemInfo (&si);
	return si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL;
}

// public static
ncOSVersion SysInfoTool::GetOSVersion ()
{
	// RtlGetVersion is the kernel-mode equivalent of the user-mode GetVersionEx function in the Windows SDK.
	RTLGETVERSION fpRtlGetVersion = (RTLGETVERSION)GetProcAddress (GetModuleHandle (_T("ntdll.dll")), "RtlGetVersion");

	if (fpRtlGetVersion == NULL)
		return OSV_UNKNOWN;

	RTL_OSVERSIONINFOEXW osvi;
	osvi.dwOSVersionInfoSize = sizeof(osvi);
	LONG status = fpRtlGetVersion ((PRTL_OSVERSIONINFOEXW)&osvi);

	if (osvi.dwPlatformId != VER_PLATFORM_WIN32_NT)
		return OSV_UNKNOWN;

	switch (osvi.dwMajorVersion)
	{
	case 5:
		if (osvi.dwMinorVersion == 1)
			return OSV_WINXP;
		else if (osvi.dwMinorVersion == 2)
			return OSV_WIN2003;
	case 6:
		if (osvi.dwMinorVersion == 0)
			return (osvi.wProductType == VER_NT_WORKSTATION ? OSV_WINVISTA : OSV_WIN2008);
		else if (osvi.dwMinorVersion == 1)
			return (osvi.wProductType == VER_NT_WORKSTATION ? OSV_WIN7 : OSV_WIN2008_R2);
		else if (osvi.dwMinorVersion == 2)
			return (osvi.wProductType == VER_NT_WORKSTATION ? OSV_WIN8 : OSV_WIN2012);
		else if (osvi.dwMinorVersion == 3)
			return (osvi.wProductType == VER_NT_WORKSTATION ? OSV_WIN8_1 : OSV_WIN2012_R2);
	case 10:
		if (osvi.dwMinorVersion == 0)
			return (osvi.wProductType == VER_NT_WORKSTATION ? OSV_WIN10 : OSV_UNKNOWN);
	}

	return OSV_UNKNOWN;
}