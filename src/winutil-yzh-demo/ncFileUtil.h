/***************************************************************************************************
ncFileUtil.h:
	Copyright (c) Eisoo Software, Inc.(2004-2015), All rights reserved.

Purpose:
	Header file for file related utilities.

Author:
	Chaos

Created Time:
	2015-02-27
***************************************************************************************************/

#ifndef __NC_FILE_UTILITIES_H__
#define __NC_FILE_UTILITIES_H__

#pragma once

class WINUTIL_DLLEXPORT ncFileUtil
{
public:
	static BOOL IsFileExists (LPCTSTR lpFilePath);

	static BOOL IsEmptyFile (LPCTSTR lpFilePath);

	static BOOL IsFileOpened (LPCTSTR lpFilePath);

	static BOOL IsFileLocked (LPCTSTR lpFilePath, DWORD dwLockedAccess);

	static BOOL WriteFile (LPCTSTR lpFilePath, LPCTSTR lpText, BOOL bAppend);

	static BOOL ReadFile (LPCTSTR lpFilePath, LPTSTR lpText, size_t length);
};

#endif // __NC_FILE_UTILITIES_H__
