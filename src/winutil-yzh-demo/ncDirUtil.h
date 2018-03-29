/***************************************************************************************************
ncDirUtil.h:
	Copyright (c) Eisoo Software, Inc.(2004-2015), All rights reserved.

Purpose:
	Header file for directory related utilities.

Author:
	Chaos

Created Time:
	2015-03-06
***************************************************************************************************/

#ifndef __NC_DIRECTORY_UTILITIES_H__
#define __NC_DIRECTORY_UTILITIES_H__

#pragma once

class WINUTIL_DLLEXPORT ncDirUtil
{
public:
	// ����Ƿ�Ϊ"."��".."Ŀ¼��
	static BOOL IsDotsDir (LPCTSTR lpFileName)
	{
		return (_tcsncmp(lpFileName, _T("."), 2) == 0 || _tcsncmp(lpFileName, _T(".."), 3) == 0);
	}

	// �о�Ŀ¼�µ��������ļ�����Ŀ¼��
	static BOOL ListAll (LPCTSTR lpDirPath, vector<tstring>& files, vector<tstring>& dirs, BOOL bRecurse);

	// �о�Ŀ¼�µ��������ļ���
	static BOOL ListFiles (LPCTSTR lpDirPath, vector<tstring>& files, BOOL bRecurse);

	// �о�Ŀ¼�µ�������Ŀ¼��
	static BOOL ListDirs (LPCTSTR lpDirPath, vector<tstring>& dirs, BOOL bRecurse);

	// ��ȡĿ¼��С��Ϣ��
	static BOOL GetDirSize (LPCTSTR lpDirPath, INT64& nSize, DWORD& dwFileCount, DWORD& dwDirCount, BOOL bRecurse);

protected:
	enum ncListMask
	{
		NC_LM_FILE = 0x01,
		NC_LM_DIR = 0x02,
		NC_LM_ALL = NC_LM_FILE | NC_LM_DIR,
	};

	static BOOL ListSubs (LPCTSTR lpDirPath, vector<tstring>& files, vector<tstring>& dirs, BOOL bRecurse, ncListMask mask);
};

#endif // __NC_DIRECTORY_UTILITIES_H__
