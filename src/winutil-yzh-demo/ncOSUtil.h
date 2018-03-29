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

// 操作系统版本定义。
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
	// 检查操作系统是否为32位。
	static BOOL Is32BitOS ();

	// 获取操作系统版本。
	static ncOSVersion GetOSVersion ();

	// 检查是否为 XP 系统家族。
	static BOOL IsWinXPFamily ()
	{
		ncOSVersion osv = GetOSVersion ();
		return (osv >= OSV_WINXP && osv < OSV_WINVISTA);
	}

	// 检查当前系统是否开启了UAC。
	static BOOL IsUACEnabled ();
};

#endif // __NC_OS_UTILITIES_H__
