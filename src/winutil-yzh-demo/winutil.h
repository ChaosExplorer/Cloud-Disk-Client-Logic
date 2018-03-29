/***************************************************************************************************
winutil.h:
	Copyright (c) Eisoo Software, Inc.(2004-2015), All rights reserved.

Purpose:
	Header file for winutil library.

Author:
	Chaos

Creating Time:
	2015-02-25
***************************************************************************************************/

#ifndef __WINDOWS_UTILITIES_H__
#define __WINDOWS_UTILITIES_H__

#pragma once

////////////////////////////////////////////////////////////////////////////////////////////////////
// __MAKE_WINUTIL_DLL__
//
#ifdef __MAKE_WINUTIL_DLL__
	#define WINUTIL_CDLLEXPORT					extern "C" __declspec(dllexport)
	#define WINUTIL_DLLEXPORT					__declspec(dllexport)
#elif defined(__USE_WINUTIL_DLL__)
	#define WINUTIL_CDLLEXPORT					extern "C" __declspec(dllimport)
	#define WINUTIL_DLLEXPORT					__declspec(dllimport)
#else
	#define WINUTIL_CDLLEXPORT
	#define WINUTIL_DLLEXPORT
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
// WinUtil Definitions
//

#include <string>
typedef std::basic_string<TCHAR, std::char_traits<TCHAR>, std::allocator<TCHAR>> tstring;

////////////////////////////////////////////////////////////////////////////////////////////////////
// WinUtil Event Log
//

WINUTIL_CDLLEXPORT void ncWriteEventLog (WORD wEventType, LPCTSTR lpFmt, ...);

////////////////////////////////////////////////////////////////////////////////////////////////////
// WinUtil Trace
//

#define WINUTIL_CONFIG_FILENAME		_T("winutil.ini")
#define WINUTIL_TRACE_FILENAME		_T("winutil.log")

WINUTIL_CDLLEXPORT bool ncNeedTrace (LPCTSTR lpModuleName);
WINUTIL_CDLLEXPORT void ncWriteTrace (LPCSTR lpFile, int nLine, LPCTSTR lpFmt, ...);

#define WINUTIL_TRACE(module, ...)												\
	do {																		\
		if (ncNeedTrace (module))												\
			ncWriteTrace (__FILE__, __LINE__, __VA_ARGS__);						\
	} while (false)

#define WINCMN_TRACE(...)			WINUTIL_TRACE(_T("winutil"), __VA_ARGS__)

////////////////////////////////////////////////////////////////////////////////////////////////////
// WinUtil Headers
//

#include "ntdef.h"
#include "ncOSUtil.h"
#include "ncNetUtil.h"
#include "ncHandleUtil.h"
#include "ncLockUtil.h"
#include "ncDebugUtil.h"
#include "ncAppUtil.h"
#include "ncServiceUtil.h"
#include "ncProcessUtil.h"
#include "ncVolumeUtil.h"
#include "ncDirUtil.h"
#include "ncFileUtil.h"
#include "ncPathUtil.h"
#include "ncUrlUtil.h"
#include "ncADSUtil.h"
#include "ncSharedMemory.h"
#include "ncShellDisk.h"
#include "ncShellUtil.h"
#include "ncCursorUtil.h"
#include "ncWinHookUtil.h"
#include "winipc/winipc.h"
#include "usedef.h"

#endif // __WINDOWS_UTILITIES_H__
