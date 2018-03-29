/***************************************************************************************************
ncAppUtil.h:
	Copyright (c) Eisoo Software, Inc.(2004-2015), All rights reserved.

Purpose:
	Header file for application related utilities.

Author:
	Chaos

Created Time:
	2015-05-25
***************************************************************************************************/

#ifndef __NC_APPLICATION_UTILITIES_H__
#define __NC_APPLICATION_UTILITIES_H__

#pragma once

class WINUTIL_DLLEXPORT ncAppUtil
{
public:
	// ���ó��򿪻����������lpAppPathΪNULL����ȡ��������������
	static void SetAutoRun (LPCTSTR lpAppName, LPCTSTR lpAppPath);

	// �������Ƿ�Ϊ����������
	static BOOL GetAutoRun (LPCTSTR lpAppName);
};

#endif // __NC_APPLICATION_UTILITIES_H__
