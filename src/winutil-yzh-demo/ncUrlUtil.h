/***************************************************************************************************
ncUrlUtil.h:
	Copyright (c) Eisoo Software, Inc.(2004-2015), All rights reserved.

Purpose:
	Header file for url related utilities.

Author:
	Chaos

Created Time:
	2015-05-23
***************************************************************************************************/

#ifndef __NC_URL_UTILITIES_H__
#define __NC_URL_UTILITIES_H__

#pragma once

class WINUTIL_DLLEXPORT ncUrlUtil
{
public:
	static tstring EncodeUrl (LPCTSTR lpUrl);

	static tstring DecodeUrl (LPCTSTR lpUrl);

	static void CopyUrl (LPCTSTR lpUrl);
};

#endif // __NC_URL_UTILITIES_H__
