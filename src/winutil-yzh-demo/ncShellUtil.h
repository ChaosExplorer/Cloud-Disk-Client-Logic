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
	// 刷新所有资源管理器窗口。
	static void RefreshShellWindows ();

	// 关闭指定资源管理器窗口（如果lpClosePath为NULL，则关闭所有资源管理器窗口）。
	static void CloseShellWindows (LPCTSTR lpClosePath = NULL);

	// 自定义文件夹显示风格。
	static void CustomizeFolder (LPCTSTR lpPath, LPCTSTR lpInfotip, LPCTSTR lpIconFile, int nIconIndex);

	// 注册自定义的URL协议。
	static void RegUrlProtocol (LPCTSTR lpName, LPCTSTR lpOpenCmd, LPCTSTR lpDesc, LPCTSTR lpDefIcon);

	// 解注册自定义的URL协议。
	static void UnregUrlProtocol (LPCTSTR lpName);

	// 创建快捷方式。
	static void CreateLink (LPCTSTR lpObjPath, LPCTSTR lpLinkPath, LPCTSTR lpDesc);

	// 删除快捷方式。
	static void RemoveLink (LPCTSTR lpLinkPath);
};

#endif // __NC_SHELL_UTILITIES_H__
