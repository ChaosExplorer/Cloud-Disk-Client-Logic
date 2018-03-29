/***************************************************************************************************
ncADSUtil.h:
	Copyright (c) Eisoo Software, Inc.(2004-2015), All rights reserved.

Purpose:
	Header file for alternate data stream related utilities¡£

Author:
	Chaos

Created Time:
	2015-03-06
***************************************************************************************************/

#ifndef __NC_ADS_UTILITIES_H__
#define __NC_ADS_UTILITIES_H__

#pragma once

class WINUTIL_DLLEXPORT ncADSUtil
{
public:
	static BOOL SetADS (LPCTSTR lpFilePath, LPCTSTR lpADSName, LPCTSTR lpValue);

	static BOOL SetADS (LPCTSTR lpFilePath, LPCTSTR lpADSName, int64 nValue);

	static BOOL GetADS (LPCTSTR lpFilePath, LPCTSTR lpADSName, LPTSTR lpValue, size_t length);

	static BOOL GetADS (LPCTSTR lpFilePath, LPCTSTR lpADSName, int64& nValue);

	static BOOL CheckADS (LPCTSTR lpFilePath, LPCTSTR lpADSName);

	static BOOL RemoveADS (LPCTSTR lpFilePath, LPCTSTR lpADSName);

protected:
	static void MakeADSPath (LPTSTR lpADSPath, LPCTSTR lpFilePath, LPCTSTR lpADSName);
};

#endif // __NC_ADS_UTILITIES_H__
