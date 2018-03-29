/***************************************************************************************************
ncFileUtil.cpp:
	Copyright (c) Eisoo Software, Inc.(2004-2015), All rights reserved.

Purpose:
	Source file for file related utilities.

Author:
	Chaos

Created Time:
	2015-02-27
***************************************************************************************************/

#include <abprec.h>
#include "winutil.h"

// public static
BOOL ncFileUtil::IsFileExists (LPCTSTR lpFilePath)
{
	if (lpFilePath == NULL)
		return FALSE;

	DWORD dwAttrib = GetFileAttributes (lpFilePath);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

// public static
BOOL ncFileUtil::IsEmptyFile (LPCTSTR lpFilePath)
{
	if (lpFilePath == NULL)
		return FALSE;

	WIN32_FILE_ATTRIBUTE_DATA fileInfo;

	if (!GetFileAttributesEx (lpFilePath, GetFileExInfoStandard, (void*)&fileInfo) ||
		fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ||
		fileInfo.nFileSizeLow != 0)
		return FALSE;

	return TRUE;
}

// public static
BOOL ncFileUtil::IsFileOpened (LPCTSTR lpFilePath)
{
	if (lpFilePath == NULL)
		return FALSE;

	ncScopedHandle hFile = CreateFile (lpFilePath,
		GENERIC_READ,
		0,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);

	return (hFile == INVALID_HANDLE_VALUE && GetLastError () == ERROR_SHARING_VIOLATION);
}

// public static
BOOL ncFileUtil::IsFileLocked (LPCTSTR lpFilePath, DWORD dwLockedAccess)
{
	if (lpFilePath == NULL)
		return FALSE;

	ncScopedHandle hFile = CreateFile (lpFilePath,
		dwLockedAccess,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);

	return (hFile == INVALID_HANDLE_VALUE && GetLastError () == ERROR_SHARING_VIOLATION);
}

// public static
BOOL ncFileUtil::WriteFile (LPCTSTR lpFilePath, LPCTSTR lpText, BOOL bAppend)
{
	if (lpFilePath == NULL || lpText == NULL)
		return FALSE;

	ncScopedHandle hFile = CreateFile (lpFilePath,
		GENERIC_WRITE,
		FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
		NULL,
		(bAppend ? OPEN_ALWAYS : CREATE_ALWAYS),
		FILE_ATTRIBUTE_NORMAL,
		0);

	if(hFile == INVALID_HANDLE_VALUE)
		return FALSE;

	if (bAppend && SetFilePointer (hFile, 0, 0, FILE_END) == INVALID_SET_FILE_POINTER)
		return FALSE;

	DWORD dwSize = (DWORD)_tcslen (lpText) * sizeof (TCHAR);
	DWORD hasWritten = 0;
	DWORD dwWritten;

	while (hasWritten < dwSize) {
		if (!::WriteFile (hFile, lpText + hasWritten, dwSize - hasWritten, &dwWritten, NULL))
			return FALSE;
		hasWritten += dwWritten;
	}

	return TRUE;
}

// public static
BOOL ncFileUtil::ReadFile (LPCTSTR lpFilePath, LPTSTR lpText, size_t length)
{
	if (lpFilePath == NULL || lpText == NULL)
		return FALSE;

	WINCMN_TRACE(_T("ReadFile lpFilePath: %s, lpText: %s"), lpFilePath, lpText);

	ncScopedHandle hFile = CreateFile (lpFilePath,
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);

	WINCMN_TRACE(_T("CreateFile hFile: %d"), hFile);

	if(hFile == INVALID_HANDLE_VALUE)
		return FALSE;

	DWORD dwSize = (DWORD)(length - 1) * sizeof (TCHAR);
	DWORD dwRead = 0;

	if (!::ReadFile (hFile, (LPVOID)lpText, dwSize, &dwRead, NULL))
		return FALSE;

	lpText[dwRead / sizeof (TCHAR)] = 0;

	WINCMN_TRACE(_T("ReadFile lpText: %s, dwSize: %d, dwRead: %d"), lpText, dwSize, dwRead);

	return TRUE;
}
