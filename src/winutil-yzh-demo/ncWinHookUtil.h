/***************************************************************************************************
ncHookUtil.h:
Copyright (c) Eisoo Software, Inc.(2004-2017), All rights reserved.

Purpose:
Source file for winhook related utilities.

Author:
Chaos

Created Time:
***************************************************************************************************/

#ifndef __NC_HOOK_UTILITIES_H__
#define __NC_HOOK_UTILITIES_H__

#define HOOK_MODE_MAX								0
#define HOOK_MODE_MIN								1

#define DEALY_MODE_BEGIN							0
#define DEALY_MODE_WINDOW							1
#define DEALY_MODE_TIME								2
#define DEALY_MODE_END								3

#define LIMIT_FUN_NtResumeThread					0x1
#define LIMIT_FUN_NtCreateFile						0x2
#define LIMIT_FUN_CreateProcess						0x4
#define LIMIT_FUN_GetFileAttributesEx				0x8
#define LIMIT_FUN_SHCreateShellFolderView			0x10
#define LIMIT_FUN_NtQueryDirectoryFile				0x11
#define LIMIT_FUN_CoCreateInstance					0x12
#define LIMIT_FUN_SHFileOperation					0x14
#define LIMIT_FUN_StartDoc							0x18
#define LIMIT_FUN_StartPage							0x20
#define LIMIT_FUN_BitBlt							0x21

#pragma once

struct ncHookInfoHead
{
	unsigned long size;
	unsigned short mode;
	unsigned long excludePathSize;
	unsigned long excludeProcessSize;
	unsigned long includePathSize;
	unsigned long includeProcessSize;
	unsigned long delayPathSize;
	unsigned long delayProcessSize;
	unsigned long limitPathSize;
	unsigned long limitProcessSize;
};

struct ncHookInfoDelay
{
	tstring dest;
	unsigned long mode;
	tstring condtion;
};

struct ncHookInfoLimit
{
	tstring dest;
	unsigned long limit;
};

class WINUTIL_DLLEXPORT ncWinHookUtil
{
private:
	static int FunNameToNo (TCHAR* funName);
public:
	static HANDLE CreateHookInfo (LPCTSTR lpPath);

	static void GetInjecInfo (unsigned long& hookMode,
								vector<tstring>& excludePaths,
								vector<tstring>& excludeProcesses,
								vector<tstring>& includePaths,
								vector<tstring>& includeProcesses);
	static void GetDelayInfo (vector<ncHookInfoDelay>& delayPath, vector<ncHookInfoDelay>& delayProcess);
	static void GetLimitInfo (vector<ncHookInfoLimit>& limitPath, vector<ncHookInfoLimit>& limitProcess);
};

#endif // __NC_URL_UTILITIES_H__
