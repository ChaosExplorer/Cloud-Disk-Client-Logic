/***************************************************************************************************
pipeclient.h:
	Copyright (c) Eisoo Software Inc.(2004-2012), All rights reserved

Purpose:
	Header file for class  asIPCPipeClient.

Author:
	Chaos

Creating Time:
	2012-6-28
***************************************************************************************************/

#ifndef __AS_IPC_PIPE_CLIENT_H__
#define __AS_IPC_PIPE_CLIENT_H__

#pragma once

/////////////////////////////////////////////////////////////////////////////
// class asIPCPipeClient

class WINUTIL_DLLEXPORT asIPCPipeClient
{
public:
	asIPCPipeClient (LPCTSTR lpPipeName);
	~asIPCPipeClient ();

	/**
	 * 发送请求。
	 */
	void sendRequest (const asIPCPipeMessage& requestMsg);

	/**
	 * 接收结果。
	 */
	void getResult (asIPCPipeMessage& resultMsg);

protected:
	/**
	 * 连接管道服务端。
	 */
	void connectPipeServer ();

	/**
	 * 关闭管道客户端。
	 */
	void closePipeClient ();

private:
	TCHAR			_szPipeName[PIPE_NAMELEN];		// 管道名称
	HANDLE			_hPipe;							// 管道句柄

}; // End class asIPCPipeClient

#endif // __AS_IPC_PIPE_CLIENT_H__
