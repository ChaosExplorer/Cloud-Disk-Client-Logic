/***************************************************************************************************
ncVolumeUtil.h:
	Copyright (c) Eisoo Software, Inc.(2004-2015), All rights reserved.

Purpose:
	Header file for volume related utilities.

Author:
	Chaos

Created Time:
	2015-03-09
***************************************************************************************************/

#ifndef __NC_VOLUME_UTILITIES_H__
#define __NC_VOLUME_UTILITIES_H__

#pragma once

class WINUTIL_DLLEXPORT ncVolumeUtil
{
public:
	static void GetDeviceDriveMap (map<tstring, tstring>& devDrvMap);

	static BOOL DevicePathToDrivePath (tstring& path);
};

#endif // __NC_VOLUME_UTILITIES_H__
