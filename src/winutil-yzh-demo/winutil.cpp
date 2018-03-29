/***************************************************************************************************
winutil.cpp:
	Copyright (c) Eisoo Software, Inc.(2004-2015), All rights reserved.

Purpose:
	Source file for winutil library.

Author:
	Chaos

Creating Time:
	2015-02-25
***************************************************************************************************/

#include <abprec.h>
#include <atlbase.h>
#include <time.h>
#include "winutil.h"

//
// ��¼ϵͳ��־��
//
void ncWriteEventLog (WORD wEventType, LPCTSTR lpFmt, ...)
{
	static HANDLE hEventLog = RegisterEventSource (NULL, WINUTIL_APP_NAME);
	if (hEventLog == NULL)
		return;

	va_list ap;
	va_start (ap, lpFmt);

	TCHAR szEventMsg[512];
	_vsntprintf_s (szEventMsg, 512, _TRUNCATE, lpFmt, ap);
	va_end (ap);

	LPCTSTR lpEventMsg[1];
	lpEventMsg[0] = szEventMsg;
	ReportEvent (hEventLog, wEventType, 0, 0, NULL, 1, 0, lpEventMsg, NULL);
}

//
// ����Ƿ���Ҫ��¼��־��
//    1.������־��¼�������ļ�����ڵ�ǰģ�����ڵ�Ŀ¼�¡�
//    2.ֻ��ģ���ʼ��ʱ��ȡһ�����ã�����޷�������ʱ��̬�������á�
//
bool ncNeedTrace (LPCTSTR lpModuleName)
{
	static TCHAR szModules[100];
	static bool bInit = false;

	if (!bInit)
	{
		bInit = true;
		tstring configPath = ncDebugUtil::GetCurrentImageDirPath ();
		configPath.append (_T("\\"));
		configPath.append (WINUTIL_CONFIG_FILENAME);
		GetPrivateProfileString (_T("Trace"), _T("modules"), NULL, szModules, 100, configPath.c_str ());
	}

	return (szModules[0] != 0 && _tcsstr (szModules, lpModuleName) != NULL);
}

//
// ����־��Ϣд���ļ���
//    1.��־�ļ�����ڵ�ǰģ�����ڵ�Ŀ¼��Ҳ������װĿ¼���¡�
//
void ncWriteTrace (LPCSTR lpFile, int nLine, LPCTSTR lpFmt, ...)
{
	static ncThreadMutexLock mutex;
	ncScopedLock<ncThreadMutexLock> lock (&mutex);

	static tstring tracePath;
	if (tracePath.empty ())
	{
		tracePath = ncDebugUtil::GetCurrentImageDirPath ();
		tracePath.append (_T("\\"));
		tracePath.append (WINUTIL_TRACE_FILENAME);
	}

	static tstring procName = ncProcessUtil::GetCurrentProcessName ();

	va_list ap;
	va_start (ap, lpFmt);
	int len = _vsctprintf (lpFmt, ap);
	TCHAR* szMsg = new TCHAR[len + 1];
	_vsntprintf_s (szMsg, len + 1, _TRUNCATE, lpFmt, ap);
	va_end (ap);

	CA2CT szFile (lpFile);
	TCHAR* lpFileName = _tcsrchr (szFile, _T('\\'));
	lpFileName++;

	time_t now;
	time (&now);

	//
	// ��־��ʽ��time process file:line msg
	//
	TCHAR* szLog = new TCHAR[1024];
	_sntprintf_s (szLog,
				  1024,
				  _TRUNCATE,
				  _T("%s\t%s(%d)\t%s:%d\t%s\r\n"),
				  _tctime (&now),
				  procName.c_str (),
				  GetCurrentProcessId (),
				  lpFileName,
				  nLine,
				  szMsg);

	ncFileUtil::WriteFile (tracePath.c_str (), szLog, TRUE);

	delete[] szMsg;
	delete[] szLog;
}
