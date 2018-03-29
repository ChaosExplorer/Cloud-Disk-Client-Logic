/***************************************************************************************************
ncDebugUtil.h:
	Copyright (c) Eisoo Software, Inc.(2004-2015), All rights reserved.

Purpose:
	Header file for debug related utilities.

Author:
	Chaos

Creating Time:
	2015-03-08
***************************************************************************************************/

#ifndef __NC_DEBUG_UTILITIES_H__
#define __NC_DEBUG_UTILITIES_H__

#pragma once

class WINUTIL_DLLEXPORT ncDebugUtil
{
public:
	static tstring GetCurrentImageDirPath ();

	static DWORD GetImageFuncAddress (LPCSTR lpImagePath, LPCSTR lpFuncName);
};

#endif // __NC_DEBUG_UTILITIES_H__
