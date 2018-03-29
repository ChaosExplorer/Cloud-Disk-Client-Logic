/***************************************************************************************************
ncDirUtil.cpp:
	Copyright (c) Eisoo Software, Inc.(2004-2015), All rights reserved.

Purpose:
	Source file for directory related utilities.

Author:
	Chaos

Created Time:
	2015-03-06
***************************************************************************************************/

#include <abprec.h>
#include <shlwapi.h>
#include "winutil.h"

// public static
BOOL ncDirUtil::ListAll (LPCTSTR lpDirPath, vector<tstring>& files, vector<tstring>& dirs, BOOL bRecurse)
{
	if (lpDirPath == NULL)
		return FALSE;

	files.clear ();
	dirs.clear ();

	return ListSubs (lpDirPath, files, dirs, bRecurse, NC_LM_ALL);
}

// public static
BOOL ncDirUtil::ListFiles (LPCTSTR lpDirPath, vector<tstring>& files, BOOL bRecurse)
{
	if (lpDirPath == NULL)
		return FALSE;

	vector<tstring> dirs;
	files.clear ();

	return ListSubs (lpDirPath, files, dirs, bRecurse, NC_LM_FILE);
}

// public static
BOOL ncDirUtil::ListDirs (LPCTSTR lpDirPath, vector<tstring>& dirs, BOOL bRecurse)
{
	if (lpDirPath == NULL)
		return FALSE;

	vector<tstring> files;
	dirs.clear ();

	return ListSubs (lpDirPath, files, dirs, bRecurse, NC_LM_DIR);
}

// protected static
BOOL ncDirUtil::ListSubs (LPCTSTR lpDirPath, vector<tstring>& files, vector<tstring>& dirs, BOOL bRecurse, ncListMask mask)
{
	TCHAR szDirSpec[MAX_PATH];
	_sntprintf_s (szDirSpec, MAX_PATH, _TRUNCATE, _T("%s\\*"), lpDirPath);

	WIN32_FIND_DATA fd;
	TCHAR szFileName[MAX_PATH];

	HANDLE hFind = FindFirstFile (szDirSpec, &fd);
	if (hFind == INVALID_HANDLE_VALUE)
		return FALSE;

	do {
		if (ncDirUtil::IsDotsDir (fd.cFileName))
			continue;

		PathCombine (szFileName, lpDirPath, fd.cFileName);

		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if (mask & NC_LM_DIR)
				dirs.push_back (szFileName);

			if (bRecurse && !ListSubs (szFileName, files, dirs, TRUE, mask)) {
				FindClose (hFind);
				return FALSE;
			}
		}
		else if (mask & NC_LM_FILE) {
			files.push_back (szFileName);
		}
	} while (FindNextFile (hFind, &fd));

	FindClose (hFind);
	return TRUE;
}

// public static
BOOL ncDirUtil::GetDirSize (LPCTSTR lpDirPath, INT64& nSize, DWORD& dwFileCount, DWORD& dwDirCount, BOOL bRecurse)
{
	if (lpDirPath == NULL)
		return FALSE;

	nSize = 0;
	dwFileCount = 0;
	dwDirCount = 0;

	TCHAR szDirSpec[MAX_PATH];
	_sntprintf_s (szDirSpec, MAX_PATH, _TRUNCATE, _T("%s\\*"), lpDirPath);

	WIN32_FIND_DATA fd;
	TCHAR szFileName[MAX_PATH];

	HANDLE hFind = FindFirstFile (szDirSpec, &fd);
	if (hFind == INVALID_HANDLE_VALUE)
		return FALSE;

	do {
		if (ncDirUtil::IsDotsDir (fd.cFileName))
			continue;

		PathCombine (szFileName, lpDirPath, fd.cFileName);

		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			++dwDirCount;

			if (bRecurse) {
				INT64 nSubSize;
				DWORD dwSubFileCount, dwSubDirCount;

				if (GetDirSize (szFileName, nSubSize, dwSubFileCount, dwSubDirCount, TRUE)) {
					nSize += nSubSize;
					dwFileCount += dwSubFileCount;
					dwDirCount += dwSubDirCount;
				}
				else {
					FindClose (hFind);
					return FALSE;
				}
			}
		}
		else {
			++dwFileCount;
			nSize += fd.nFileSizeHigh * ((INT64)MAXDWORD + 1) + fd.nFileSizeLow;
		}
	} while (FindNextFile (hFind, &fd));

	FindClose (hFind);
	return TRUE;
}
