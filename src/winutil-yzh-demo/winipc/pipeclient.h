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
	 * ��������
	 */
	void sendRequest (const asIPCPipeMessage& requestMsg);

	/**
	 * ���ս����
	 */
	void getResult (asIPCPipeMessage& resultMsg);

protected:
	/**
	 * ���ӹܵ�����ˡ�
	 */
	void connectPipeServer ();

	/**
	 * �رչܵ��ͻ��ˡ�
	 */
	void closePipeClient ();

private:
	TCHAR			_szPipeName[PIPE_NAMELEN];		// �ܵ�����
	HANDLE			_hPipe;							// �ܵ����

}; // End class asIPCPipeClient

#endif // __AS_IPC_PIPE_CLIENT_H__
