/***************************************************************************************************
ncOSUtil.h:
	Copyright (c) Eisoo Software, Inc.(2004-2015), All rights reserved.

Purpose:
	Header file for OS related utilities.

Author:
	Chaos

Created Time:
	2015-03-06
***************************************************************************************************/

#ifndef __NC_OS_UTILITIES_H__
#define __NC_OS_UTILITIES_H__

#pragma once

// ����ϵͳ�汾���塣
enum ncOSVersion
{
	OSV_UNKNOWN,
	OSV_WINXP,
	OSV_WIN2003,
	OSV_WINVISTA,
	OSV_WIN2008,
	OSV_WIN7,
	OSV_WIN2008_R2,
	OSV_WIN8,
	OSV_WIN2012,
	OSV_WIN8_1,
	OSV_WIN2012_R2,
	OSV_WIN10,
};

class WINUTIL_DLLEXPORT ncOSUtil
{
public:
	// ������ϵͳ�Ƿ�Ϊ32λ��
	static BOOL Is32BitOS ();

	// ��ȡ����ϵͳ�汾��
	static ncOSVersion GetOSVersion ();

	// ����Ƿ�Ϊ XP ϵͳ���塣
	static BOOL IsWinXPFamily ()
	{
		ncOSVersion osv = GetOSVersion ();
		return (osv >= OSV_WINXP && osv < OSV_WINVISTA);
	}

	// ��鵱ǰϵͳ�Ƿ�����UAC��
	static BOOL IsUACEnabled ();
};

#endif // __NC_OS_UTILITIES_H__
