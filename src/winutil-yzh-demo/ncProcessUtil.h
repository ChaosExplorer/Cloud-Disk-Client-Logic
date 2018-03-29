/***************************************************************************************************
ncProcessUtil.h:
	Copyright (c) Eisoo Software, Inc.(2004-2015), All rights reserved.

Purpose:
	Header file for process related utilities.

Author:
	Chaos

Created Time:
	2015-03-06
***************************************************************************************************/

#ifndef __NC_PROCESS_UTILITIES_H__
#define __NC_PROCESS_UTILITIES_H__

#pragma once

// 进程状态标识
enum ncProcessFlags
{
	NC_PF_UNKNOWN =				0x00000000,			// 未知状态
	NC_PF_NORMAL =				0x00000001,			// 常规状态
	NC_PF_DEBUGGED =			0x00000002,			// 调试状态
	NC_PF_UNINITIALIZED =		0x00000004,			// 未初始化状态
};

class WINUTIL_DLLEXPORT ncProcessUtil
{
public:
	static BOOL AdjustProcessPrivilege (HANDLE hProcess, LPCTSTR lpPrivName, BOOL bEnable);

	static HANDLE CreateUACElevatedProcess (LPCTSTR lpAppPath, LPCTSTR lpCmdLine);

	static BOOL Is32BitProcess (HANDLE hProcess);

	static tstring GetCurrentProcessName ();

	static tstring GetCurrentProcessDir ();

	static tstring GetProcessName (HANDLE hProcess);

	static void GetProcessPathAndName (HANDLE hProcess, tstring &path, tstring &name);

	static DWORD GetProcessIdOfThread (HANDLE hThread);

	static DWORD GetProcessIdByName (LPCTSTR lpProcName, DWORD dwSessionId = -1);

	static void GetProcessChildren (DWORD dwProcId, vector<DWORD>& dwPids);

	static DWORD GetProcessThreadCount (DWORD dwProcId);

	static HMODULE GetProcessModuleHandle (HANDLE hProcess, LPCTSTR lpModName);

	static PVOID GetProcessFuncAddress (HANDLE hProcess, LPCTSTR lpModName, LPCSTR lpFuncName);

	static PTOKEN_USER GetProcessTokenUser (HANDLE hProcess);

	static ncProcessFlags GetProcessFlags (HANDLE hProcess);
};

#endif // __NC_PROCESS_UTILITIES_H__
