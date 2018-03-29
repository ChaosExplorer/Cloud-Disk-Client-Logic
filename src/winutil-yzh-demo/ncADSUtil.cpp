/***************************************************************************************************
ncADSUtil.cpp:
	Copyright (c) Eisoo Software, Inc.(2004-2015), All rights reserved.

Purpose:
	Source file for alternate data stream related utilities¡£

Author:
	Chaos

Created Time:
	2015-03-09
***************************************************************************************************/

#include <abprec.h>
#include <shlwapi.h>
#include "winutil.h"

#define LONGPATH_PREFIX			_T("\\\\?\\")
#define MAX_ADS_PATH			512

// public static
BOOL ncADSUtil::SetADS (LPCTSTR lpFilePath, LPCTSTR lpADSName, LPCTSTR lpValue)
{
	if (lpFilePath == NULL || lpADSName == NULL || lpValue == NULL)
		return FALSE;

	if (!PathFileExists (lpFilePath))
	{
		::SetLastError (ERROR_FILE_NOT_FOUND);
		return FALSE;
	}

	TCHAR szPath[MAX_ADS_PATH];
	MakeADSPath (szPath, lpFilePath, lpADSName);

	return ncFileUtil::WriteFile (szPath, lpValue, FALSE);
}

// public static
BOOL ncADSUtil::SetADS (LPCTSTR lpFilePath, LPCTSTR lpADSName, int64 nValue)
{
	if (lpFilePath == NULL || lpADSName == NULL)
		return FALSE;

	if (!PathFileExists (lpFilePath))
	{
		::SetLastError (ERROR_FILE_NOT_FOUND);
		return FALSE;
	}

	TCHAR szPath[MAX_ADS_PATH];
	MakeADSPath (szPath, lpFilePath, lpADSName);

	TCHAR szValue[100];
	_i64tot (nValue, szValue, 10);

	return ncFileUtil::WriteFile (szPath, szValue, FALSE);
}

// public static
BOOL ncADSUtil::GetADS (LPCTSTR lpFilePath, LPCTSTR lpADSName, LPTSTR lpValue, size_t length)
{
	if (lpFilePath == NULL || lpADSName == NULL || lpValue == NULL)
		return FALSE;

	TCHAR szPath[MAX_ADS_PATH];
	MakeADSPath (szPath, lpFilePath, lpADSName);

	return ncFileUtil::ReadFile (szPath, lpValue, length);
}

// public static
BOOL ncADSUtil::GetADS (LPCTSTR lpFilePath, LPCTSTR lpADSName, int64& nValue)
{
	if (lpFilePath == NULL || lpADSName == NULL)
		return FALSE;

	WINCMN_TRACE(_T("GetADS: lpFilePath: %s, lpADSName: %s"), lpFilePath, lpADSName);

	TCHAR szPath[MAX_ADS_PATH];
	MakeADSPath (szPath, lpFilePath, lpADSName);

	WINCMN_TRACE(_T("MakeADSPath: szPath: %s"), szPath);

	TCHAR szValue[100] = {};
	BOOL result = ncFileUtil::ReadFile (szPath, szValue, 100);

	WINCMN_TRACE(_T("ReadFile: szValue: %s, result: %d"), szValue, result);

	if (result)
		nValue = _ttoi64 (szValue);

	return result;
}

// public static
BOOL ncADSUtil::CheckADS (LPCTSTR lpFilePath, LPCTSTR lpADSName)
{
	if (lpFilePath == NULL || lpADSName == NULL)
		return FALSE;

	TCHAR szPath[MAX_ADS_PATH];
	MakeADSPath (szPath, lpFilePath, lpADSName);

	return ncFileUtil::IsFileExists (szPath);
}

// public static
BOOL ncADSUtil::RemoveADS (LPCTSTR lpFilePath, LPCTSTR lpADSName)
{
	if (lpFilePath == NULL || lpADSName == NULL)
		return FALSE;

	TCHAR szPath[MAX_ADS_PATH];
	MakeADSPath (szPath, lpFilePath, lpADSName);

	return ::DeleteFile (szPath);
}

// protected static
void ncADSUtil::MakeADSPath (LPTSTR lpADSPath, LPCTSTR lpFilePath, LPCTSTR lpADSName)
{
	if (_tcslen (lpFilePath) + _tcslen (lpADSName) < MAX_PATH ||
		_tcsncmp (lpFilePath, LONGPATH_PREFIX, _tcslen (LONGPATH_PREFIX)) == 0)
		::_sntprintf_s (lpADSPath, MAX_ADS_PATH, _TRUNCATE, _T("%s:%s"), lpFilePath, lpADSName);
	else
		::_sntprintf_s (lpADSPath, MAX_ADS_PATH, _TRUNCATE, _T("%s%s:%s"), LONGPATH_PREFIX, lpFilePath, lpADSName);
}
