/***************************************************************************************************
ncShellUtil.h:
	Copyright (c) Eisoo Software, Inc.(2004-2015), All rights reserved.

Purpose:
	Header file for shell related utilities.

Author:
	Chaos

Created Time:
	2015-05-24
***************************************************************************************************/

#ifndef __NC_SHELL_UTILITIES_H__
#define __NC_SHELL_UTILITIES_H__

#pragma once

class WINUTIL_DLLEXPORT ncShellUtil
{
public:
	// ˢ��������Դ���������ڡ�
	static void RefreshShellWindows ();

	// �ر�ָ����Դ���������ڣ����lpClosePathΪNULL����ر�������Դ���������ڣ���
	static void CloseShellWindows (LPCTSTR lpClosePath = NULL);

	// �Զ����ļ�����ʾ���
	static void CustomizeFolder (LPCTSTR lpPath, LPCTSTR lpInfotip, LPCTSTR lpIconFile, int nIconIndex);

	// ע���Զ����URLЭ�顣
	static void RegUrlProtocol (LPCTSTR lpName, LPCTSTR lpOpenCmd, LPCTSTR lpDesc, LPCTSTR lpDefIcon);

	// ��ע���Զ����URLЭ�顣
	static void UnregUrlProtocol (LPCTSTR lpName);

	// ������ݷ�ʽ��
	static void CreateLink (LPCTSTR lpObjPath, LPCTSTR lpLinkPath, LPCTSTR lpDesc);

	// ɾ����ݷ�ʽ��
	static void RemoveLink (LPCTSTR lpLinkPath);
};

#endif // __NC_SHELL_UTILITIES_H__
