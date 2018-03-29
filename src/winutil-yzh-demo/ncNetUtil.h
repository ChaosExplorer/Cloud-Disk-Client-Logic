/***************************************************************************************************
ncNetUtil.h:
	Copyright (c) Eisoo Software, Inc.(2004-2015), All rights reserved.

Purpose:
	Header file for network related utilities.

Author:
	Chaos

Creating Time:
	2015-07-15
***************************************************************************************************/

#ifndef __NC_NETWORK_UTILITIES_H__
#define __NC_NETWORK_UTILITIES_H__

#pragma once

class WINUTIL_DLLEXPORT ncNetUtil
{
public:
	static tstring GetMacAddress (TCHAR separator = _T(''));
};

#endif // __NC_NETWORK_UTILITIES_H__
