/***************************************************************************************************
ncOSUtil.cpp:
	Copyright (c) Eisoo Software, Inc.(2004-2015), All rights reserved.

Purpose:
	Source file for OS related utilities.

Author:
	Chaos

Created Time:
	2014-08-19
***************************************************************************************************/

#include <abprec.h>
#include <atlbase.h>
#include "winutil.h"

typedef NTSTATUS (WINAPI* RTLGETVERSION)(PRTL_OSVERSIONINFOEXW lpOsvi);

// public static
BOOL ncOSUtil::Is32BitOS ()
{
	SYSTEM_INFO si;
	GetNativeSystemInfo (&si);
	return si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL;
}

// public static
ncOSVersion ncOSUtil::GetOSVersion ()
{
	// RtlGetVersion is the kernel-mode equivalent of the user-mode GetVersionEx function in the Windows SDK.
	RTLGETVERSION fpRtlGetVersion = (RTLGETVERSION)GetProcAddress (GetModuleHandle (_T("ntdll.dll")), "RtlGetVersion");

	if (fpRtlGetVersion == NULL)
		return OSV_UNKNOWN;

	RTL_OSVERSIONINFOEXW osvi;
	osvi.dwOSVersionInfoSize = sizeof(osvi);
	NTSTATUS status = fpRtlGetVersion ((PRTL_OSVERSIONINFOEXW)&osvi);

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

// public static
BOOL ncOSUtil::IsUACEnabled ()
{
	if (GetOSVersion () < OSV_WINVISTA)
		return FALSE;

	const TCHAR* lpKey = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System");
	CRegKey rk;
	DWORD dwVal;

	return (rk.Open (HKEY_LOCAL_MACHINE, lpKey, KEY_READ) == ERROR_SUCCESS &&
			rk.QueryDWORDValue (_T("EnableLUA"), dwVal) == ERROR_SUCCESS &&
			dwVal != 0);
}
