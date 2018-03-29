/***************************************************************************************************
ncAppUtil.cpp:
	Copyright (c) Eisoo Software, Inc.(2004-2015), All rights reserved.

Purpose:
	Source file for application related utilities.

Author:
	Chaos

Created Time:
	2015-05-25
***************************************************************************************************/

#include <abprec.h>
#include <atlbase.h>
#include "winutil.h"

#define KEY_AUTO_RUN		_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run")

// public static
void ncAppUtil::SetAutoRun (LPCTSTR lpAppName, LPCTSTR lpAppPath)
{
	if (lpAppName == NULL)
		return;

	CRegKey rk;
	if (rk.Create (HKEY_CURRENT_USER, KEY_AUTO_RUN) == ERROR_SUCCESS)
	{
		if (lpAppPath != NULL)
			rk.SetStringValue (lpAppName, lpAppPath);
		else
			rk.DeleteValue (lpAppName);
	}
}

// public static
BOOL ncAppUtil::GetAutoRun (LPCTSTR lpAppName)
{
	if (!lpAppName)
		return FALSE;

	CRegKey rk;
	TCHAR szValue[MAX_PATH];
	ULONG nChars = MAX_PATH;

	return (rk.Open (HKEY_CURRENT_USER, KEY_AUTO_RUN, KEY_READ) == ERROR_SUCCESS &&
			rk.QueryStringValue (lpAppName, szValue, &nChars) == ERROR_SUCCESS);
}
