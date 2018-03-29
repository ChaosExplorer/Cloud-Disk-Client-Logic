/***************************************************************************************************
ncPathUtil.h:
	Copyright (c) Eisoo Software, Inc.(2004-2015), All rights reserved.

Purpose:
	Header file for path related utilities.

Author:
	Chaos

Created Time:
	2015-11-24
***************************************************************************************************/

#ifndef __NC_PATH_UTILITIES_H__
#define __NC_PATH_UTILITIES_H__

#pragma once

class WINUTIL_DLLEXPORT ncPathUtil
{
public:
	// 检查路径1是否为路径2的父路径。
	static BOOL IsParentPath (LPCTSTR lpPath1, LPCTSTR lpPath2, BOOL bIncludeSelf = FALSE);
};

#endif // __NC_PATH_UTILITIES_H__
