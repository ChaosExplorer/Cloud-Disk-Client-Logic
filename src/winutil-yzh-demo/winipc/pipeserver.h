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
	 * ��������
	 */
	BOOL getRequest (asIPCPipeMessage& requestMsg);

	/**
	 * ���ͽ����
	 */
	void sendResult (const asIPCPipeMessage& resultMsg);

	/**
	 * ֹͣ��������
	 */
	void stopListen ();

protected:
	/**
	 * �����ܵ�����ˡ�
	 */
	void createPipeServer ();

	/**
	 * �رչܵ�����ˡ�
	 */
	void closePipeServer ();

private:
	TCHAR			_szPipeName[PIPE_NAMELEN];		// �ܵ�����
	HANDLE			_hPipe;							// �ܵ����
	BOOL			_bStop;							// ֹͣ������ʶ

}; // End class asIPCPipeServer

#endif // __AS_IPC_PIPE_SERVER_H__
