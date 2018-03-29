/***************************************************************************************************
ncShellDisk.cpp:
	Copyright (c) Eisoo Software, Inc.(2004-2015), All rights reserved.

Purpose:
	Source file for class ncShellDisk.

Author:
	Chaos

Created Time:
	2015-05-25
***************************************************************************************************/

#include <abprec.h>
#include <atlbase.h>
#include <shlwapi.h>
#include "winutil.h"

#define KEY_NAMESPACE					\
	_T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\MyComputer\\Namespace\\%s")

// public ctor
ncShellDisk::ncShellDisk (LPCTSTR lpClsid)
	: _clsid (lpClsid)
	, _bReg (FALSE)
{
	CRegKey rk;
	TCHAR szKey[BUFSIZE];
	TCHAR szValue[BUFSIZE];

	//
	// 检查盘符对象是否已注册。
	//
	_sntprintf_s (szKey, BUFSIZE, _TRUNCATE, KEY_NAMESPACE, lpClsid);
	if (rk.Open (HKEY_CURRENT_USER, szKey, KEY_READ) == ERROR_SUCCESS)
	{
		_bReg = TRUE;
		rk.Close ();
	}

	if (_bReg)
	{
		//
		// 获取盘符对象对应的目标路径。
		//
		_sntprintf_s (szKey, BUFSIZE, _TRUNCATE, _T("Software\\Classes\\CLSID\\%s\\Instance\\InitPropertyBag"), lpClsid);
		if (rk.Open (HKEY_CURRENT_USER, szKey, KEY_READ) == ERROR_SUCCESS)
		{
			ULONG nChars = BUFSIZE;
			if (rk.QueryStringValue (_T("Target"), szValue, &nChars) == ERROR_SUCCESS)
				_target.assign (szValue);
			rk.Close ();
		}

		//
		// 获取盘符对象对应的打开命令。
		//
		_sntprintf_s (szKey, BUFSIZE, _TRUNCATE, _T("Software\\Classes\\CLSID\\%s\\Shell\\Open\\Command"), lpClsid);
		if (rk.Open (HKEY_CURRENT_USER, szKey, KEY_READ) == ERROR_SUCCESS)
		{
			ULONG nChars = BUFSIZE;
			if (rk.QueryStringValue (NULL, szValue, &nChars) == ERROR_SUCCESS)
				_openCmd.assign (szValue);
			rk.Close ();
		}
	}
}

// public
void ncShellDisk::Register (LPCTSTR lpName,
							LPCTSTR lpDesc,
							LPCTSTR lpDefIcon,
							LPCTSTR lpPshName,
							LPCTSTR lpPshClsid,
							LPCTSTR lpTarget/* = NULL*/,
							LPCTSTR lpOpenCmd/* = NULL*/)
{
	if (lpName == NULL)
		return;

	CRegKey rk;
	TCHAR szKey[BUFSIZE];

	_sntprintf_s (szKey, BUFSIZE, _TRUNCATE, _T("Software\\Classes\\CLSID\\%s"), _clsid.c_str ());
	if (rk.Create (HKEY_CURRENT_USER, szKey) == ERROR_SUCCESS)
	{
		//
		// 创建盘符对象对应的注册表键值。
		//
		rk.SetStringValue (NULL, lpName);

		if (lpDesc != NULL)
			rk.SetStringValue (_T("InfoTip"), lpDesc);

		if (lpDefIcon != NULL)
			rk.SetKeyValue (_T("DefaultIcon"), lpDefIcon);

		rk.SetKeyValue (_T("InprocServer32"), _T("shdocvw.dll"));
		rk.SetKeyValue (_T("InprocServer32"), _T("Apartment"), _T("ThreadingModel"));

		rk.SetKeyValue (_T("Instance"), _T("{0afaced1-e828-11d1-9187-b532f1e9575d}"), _T("CLSID"));

		if (lpPshName != NULL && lpPshClsid != NULL)
		{
			TCHAR szKeyPsh[BUFSIZE];
			_sntprintf_s (szKeyPsh, BUFSIZE, _TRUNCATE, _T("shellex\\PropertySheetHandlers\\%s"), lpPshName);
			rk.SetKeyValue (szKeyPsh, lpPshClsid);
		}

		CRegKey rksf;
		if (rksf.Create (rk.m_hKey, _T("ShellFolder")) == ERROR_SUCCESS)
		{
			rksf.SetDWORDValue (_T("Attributes"), 0xf8801148);
			rksf.SetStringValue (_T("wantsFORPARSING"), _T(""));
			rksf.SetStringValue (_T("PinToNameSpaceTree"), _T(""));
			rksf.SetStringValue (_T("QueryForOverlay"), _T(""));
			rksf.Close ();
		}

		//
		// 拷贝生成32位注册表键值，以支持32位程序中的盘符。
		//
		CRegKey rk32;
		if (rk32.Create (HKEY_CURRENT_USER, szKey, REG_NONE, REG_OPTION_NON_VOLATILE, KEY_WRITE|KEY_WOW64_32KEY) == ERROR_SUCCESS)
		{
			SHCopyKey (rk.m_hKey, NULL, rk32.m_hKey, NULL);
			rk32.Close ();
		}

		rk.Close ();
	}

	//
	// 设置盘符对象的目标位置和打开命令。
	//
	SetTarget (lpTarget);
	SetOpenCmd (lpOpenCmd);

	//
	// 将盘符对象注册到【我的电脑】下面。
	//
	_sntprintf_s (szKey, BUFSIZE, _TRUNCATE, KEY_NAMESPACE, _clsid.c_str ());
	if (rk.Create (HKEY_CURRENT_USER, szKey) == ERROR_SUCCESS)
	{
		_bReg = TRUE;
		rk.Close ();
	}
}

// public
void ncShellDisk::Unregister ()
{
	if (!_bReg)
		return;

	TCHAR szKey[BUFSIZE];
	_sntprintf_s (szKey, BUFSIZE, _TRUNCATE, KEY_NAMESPACE, _clsid.c_str ());
	SHDeleteKey (HKEY_CURRENT_USER, szKey);

	_sntprintf_s (szKey, BUFSIZE, _TRUNCATE, _T("Software\\Classes\\CLSID\\%s"), _clsid.c_str ());
	SHDeleteKey (HKEY_CURRENT_USER, szKey);

	_sntprintf_s (szKey, BUFSIZE, _TRUNCATE, _T("Software\\Classes\\Wow6432Node\\CLSID\\%s"), _clsid.c_str ());
	SHDeleteKey (HKEY_CURRENT_USER, szKey);

	_target.clear ();
	_openCmd.clear ();
	_bReg = FALSE;
}

// public
void ncShellDisk::SetTarget (LPCTSTR lpTarget)
{
	if (lpTarget == NULL || _tcsicmp (lpTarget, _target.c_str ()) == 0)
		return;

	CRegKey rk;
	TCHAR szKey[BUFSIZE];

	_sntprintf_s (szKey, BUFSIZE, _TRUNCATE, _T("Software\\Classes\\CLSID\\%s"), _clsid.c_str ());
	if (rk.Open (HKEY_CURRENT_USER, szKey) == ERROR_SUCCESS)
	{
		rk.SetKeyValue (_T("Instance\\InitPropertyBag"), lpTarget, _T("Target"));
		rk.Close ();
	}

	if (rk.Open (HKEY_CURRENT_USER, szKey, KEY_WRITE|KEY_WOW64_32KEY) == ERROR_SUCCESS)
	{
		rk.SetKeyValue (_T("Instance\\InitPropertyBag"), lpTarget, _T("Target"));
		rk.Close ();
	}

	_target.assign (lpTarget);
}

// public
void ncShellDisk::SetOpenCmd (LPCTSTR lpOpenCmd)
{
	TCHAR szKey[BUFSIZE];

	if (lpOpenCmd == NULL)
	{
		_sntprintf_s (szKey, BUFSIZE, _TRUNCATE, _T("Software\\Classes\\CLSID\\%s\\Shell"), _clsid.c_str ());
		SHDeleteKey (HKEY_CURRENT_USER, szKey);

		_sntprintf_s (szKey, BUFSIZE, _TRUNCATE, _T("Software\\Classes\\Wow6432Node\\CLSID\\%s\\Shell"), _clsid.c_str ());
		SHDeleteKey (HKEY_CURRENT_USER, szKey);

		_openCmd.clear ();
	}
	else if (_tcsicmp (lpOpenCmd, _openCmd.c_str ()) != 0)
	{
		CRegKey rk;

		_sntprintf_s (szKey, BUFSIZE, _TRUNCATE, _T("Software\\Classes\\CLSID\\%s"), _clsid.c_str ());
		if (rk.Open (HKEY_CURRENT_USER, szKey) == ERROR_SUCCESS)
		{
			rk.SetKeyValue (_T("Shell\\Open\\Command"), lpOpenCmd);
			rk.Close ();
		}

		if (rk.Open (HKEY_CURRENT_USER, szKey, KEY_WRITE|KEY_WOW64_32KEY) == ERROR_SUCCESS)
		{
			rk.SetKeyValue (_T("Shell\\Open\\Command"), lpOpenCmd);
			rk.Close ();
		}

		_openCmd.assign (lpOpenCmd);
	}
}
