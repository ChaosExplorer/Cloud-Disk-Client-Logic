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
	 * ע��/��ע�������̷���
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
	 * ���lpTarget��ԭtargetһ�£����ظ����á�
	 */
	void SetTarget (LPCTSTR lpTarget);

	const tstring& GetTarget () { return _target; }

	/**
	 * ���lpOpenCmdΪNULL�����Ƴ�������
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
