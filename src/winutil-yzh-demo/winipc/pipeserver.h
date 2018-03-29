/***************************************************************************************************
pipeserver.h:
	Copyright (c) Eisoo Software Inc.(2004-2012), All rights reserved

Purpose:
	Header file for class asIPCPipeServer.

Author:
	Chaos

Creating Time:
	2012-6-28
***************************************************************************************************/

#ifndef __AS_IPC_PIPE_SERVER_H__
#define __AS_IPC_PIPE_SERVER_H__

#pragma once

class WINUTIL_DLLEXPORT asIPCPipeServer
{
public:
	asIPCPipeServer (LPCTSTR lpPipeName);
	~asIPCPipeServer ();

public:
	/**
	 * 接收请求。
	 */
	BOOL getRequest (asIPCPipeMessage& requestMsg);

	/**
	 * 发送结果。
	 */
	void sendResult (const asIPCPipeMessage& resultMsg);

	/**
	 * 停止监听请求。
	 */
	void stopListen ();

protected:
	/**
	 * 创建管道服务端。
	 */
	void createPipeServer ();

	/**
	 * 关闭管道服务端。
	 */
	void closePipeServer ();

private:
	TCHAR			_szPipeName[PIPE_NAMELEN];		// 管道名称
	HANDLE			_hPipe;							// 管道句柄
	BOOL			_bStop;							// 停止监听标识

}; // End class asIPCPipeServer

#endif // __AS_IPC_PIPE_SERVER_H__
