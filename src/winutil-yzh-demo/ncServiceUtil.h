/***************************************************************************************************
ncServiceUtil.h:
	Copyright (c) Eisoo Software, Inc.(2004-2015), All rights reserved.

Purpose:
	Header file for service related utilities.

Author:
	Chaos

Creating Time:
	2015-03-06
***************************************************************************************************/

#ifndef __NC_SERVICE_UTILITIES_H__
#define __NC_SERVICE_UTILITIES_H__

#pragma once

class WINUTIL_DLLEXPORT ncServiceUtil
{
public:
	static DWORD GetServiceProcessId (LPCTSTR lpSvcName);
};

#endif // __NC_SERVICE_UTILITIES_H__
