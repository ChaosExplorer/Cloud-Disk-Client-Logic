/***************************************************************************************************
winipc.h:
	Copyright (c) Eisoo Software Inc.(2004-2012), All rights reserved

Purpose:
	Header file for winipc framework.

Author:
	Chaos

Creating Time:
	2012-6-28
***************************************************************************************************/

#ifndef __AS_WIN_IPC_H__
#define __AS_WIN_IPC_H__

#pragma once

#include "../winutil.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// WinIPC Definitions
//

#define PIPE_NAMELEN			256						// ���Ƴ��ȣ�256
#define PIPE_BUFSIZE			1048576					// �����С��1MB

////////////////////////////////////////////////////////////////////////////////////////////////////
// WinIPC Trace
//

#define WINIPC_TRACE(...)		WINUTIL_TRACE(_T("winipc"), __VA_ARGS__)

////////////////////////////////////////////////////////////////////////////////////////////////////
// WinIPC Error ID
//

#define WINIPC_CREATE_PIPE_ERROR	0x00000001L			// �����ܵ�ʧ��
#define WINIPC_OPEN_PIPE_ERROR		0x00000002L			// �򿪹ܵ�ʧ��
#define WINIPC_WAIT_PIPE_ERROR		0x00000003L			// �ȴ��ܵ�ʧ��
#define WINIPC_CONNECT_PIPE_ERROR	0x00000004L			// ���ӹܵ�ʧ��
#define WINIPC_READ_PIPE_ERROR		0x00000005L			// ���ܵ�ʧ��
#define WINIPC_WRITE_PIPE_ERROR		0x00000006L			// д�ܵ�ʧ��
#define WINIPC_DUPLICATED_MSGID		0x00000007L			// �ظ��Ĺܵ���ϢID
#define WINIPC_INVALID_MSGID		0x00000008L			// ��Ч�Ĺܵ���ϢID

////////////////////////////////////////////////////////////////////////////////////////////////////
// WinIPC Exception
//

class asIPCException
{
public:
	asIPCException (DWORD dwErrId, LPCTSTR lpMsg = NULL)
		: _dwErrId (dwErrId)
	{
		_tcsncpy_s (_szMsg, 512, lpMsg, _TRUNCATE);
	}

	DWORD getErrId () const
	{
		return _dwErrId;
	}

	LPCTSTR getMsg () const
	{
		return _szMsg;
	}
	
private:
	DWORD		_dwErrId;
	TCHAR		_szMsg[512];
};

inline void asThrowIPCException (DWORD dwErrId)
{
	TCHAR szMsg[100];
	_sntprintf_s (szMsg, 100, _TRUNCATE, _T("WINIPC error: %d, cause error: %d"), dwErrId, ::GetLastError ());
	throw asIPCException (dwErrId, szMsg);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// WinIPC Headers
//

#include "pipemsg.h"
#include "pipeclient.h"
#include "pipeserver.h"
#include "msgobserver.h"
#include "msghandler.h"

#endif // __AS_WIN_IPC_H__
