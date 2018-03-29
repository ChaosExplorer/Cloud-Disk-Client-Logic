/***************************************************************************************************
ncShellDisk.h:
	Copyright (c) Eisoo Software, Inc.(2004-2015), All rights reserved.

Purpose:
	Header file for class ncShellDisk.

Author:
	Chaos

Created Time:
	2015-05-25
***************************************************************************************************/

#ifndef __NC_SHELL_DISK_H__
#define __NC_SHELL_DISK_H__

#pragma once

class WINUTIL_DLLEXPORT ncShellDisk
{
public:
	ncShellDisk (LPCTSTR lpClsid);

public:
	BOOL IsRegistered () { return _bReg; }

	/**
	 * 注册/解注册虚拟盘符。
	 */
	void Register (LPCTSTR lpName,
				   LPCTSTR lpDesc,
				   LPCTSTR lpDefIcon,
				   LPCTSTR lpPshName,
				   LPCTSTR lpPshClsid,
				   LPCTSTR lpTarget = NULL,
				   LPCTSTR lpOpenCmd = NULL);

	void Unregister ();

	/**
	 * 如果lpTarget与原target一致，则不重复设置。
	 */
	void SetTarget (LPCTSTR lpTarget);

	const tstring& GetTarget () { return _target; }

	/**
	 * 如果lpOpenCmd为NULL，则移除其打开命令。
	 */
	void SetOpenCmd (LPCTSTR lpOpenCmd);

	const tstring& GetOpenCmd () { return _openCmd; }

private:
	static const int	BUFSIZE = 512;

	BOOL				_bReg;
	tstring				_clsid;
	tstring				_target;
	tstring				_openCmd;
};

#endif // __NC_SHELL_DISK_H__
