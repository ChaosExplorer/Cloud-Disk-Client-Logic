/***************************************************************************************************
pipeclient.cpp:
	Copyright (c) Eisoo Software Inc.(2004-2012), All rights reserved

Purpose:
	Source file for class asIPCPipeClient.

Author:
	Chaos

Creating Time:
	2012-7-3
***************************************************************************************************/

#include <abprec.h>
#include "winipc.h"

/////////////////////////////////////////////////////////////////////////////
// class asIPCPipeClient

// public ctor
asIPCPipeClient::asIPCPipeClient (LPCTSTR lpPipeName)
	: _hPipe (INVALID_HANDLE_VALUE)
{
	WINIPC_TRACE (_T("asIPCPipeClient::asIPCPipeClient: %p"), this);

	TCHAR szUserName[PIPE_NAMELEN];
	TCHAR szDomainName[PIPE_NAMELEN];
	::ExpandEnvironmentStrings (_T("%USERNAME%"), szUserName, PIPE_NAMELEN);
	::ExpandEnvironmentStrings (_T("%USERDOMAIN%"), szDomainName, PIPE_NAMELEN);

	_sntprintf_s(_szPipeName, PIPE_NAMELEN, _TRUNCATE, _T("%s_%s_%s"), lpPipeName, szUserName, szDomainName);
}

// public dtor
asIPCPipeClient::~asIPCPipeClient ()
{
	closePipeClient ();

	WINIPC_TRACE (_T("asIPCPipeClient::~asIPCPipeClient: %p"), this);
}

// public
void
asIPCPipeClient::sendRequest (const asIPCPipeMessage& requestMsg)
{
	WINIPC_TRACE (_T("begin - sendRequest: %p"), this);

	connectPipeServer ();
	requestMsg.writeToPipe (_hPipe);

	WINIPC_TRACE (_T("end - sendRequest: %p"), this);
}

// public
void
asIPCPipeClient::getResult (asIPCPipeMessage& resultMsg)
{
	WINIPC_TRACE (_T("begin - getResult: %p"), this);

	resultMsg.readFromPipe (_hPipe);
	closePipeClient ();

	WINIPC_TRACE (_T("end - getResult: %p"), this);
}

// protected
void
asIPCPipeClient::connectPipeServer ()
{
	WINIPC_TRACE (_T("begin - connectPipeServer: %p"), this);

	//
	// 连接管道服务端，最多等待30秒。
	//
	while (TRUE) {
		if (!::WaitNamedPipe (_szPipeName, 30000))
			asThrowIPCException (WINIPC_WAIT_PIPE_ERROR);

		_hPipe = ::CreateFile (_szPipeName,
						GENERIC_READ | GENERIC_WRITE,
						0,
						NULL,
						OPEN_EXISTING,
						0,
						NULL);

		if (_hPipe != INVALID_HANDLE_VALUE)
			break;

		if (::GetLastError () != ERROR_PIPE_BUSY)
			asThrowIPCException (WINIPC_CONNECT_PIPE_ERROR);
	}

	WINIPC_TRACE (_T("end - connectPipeServer: %p, _hPipe: %p"), this, _hPipe);
}

// protected
void
asIPCPipeClient::closePipeClient ()
{
	WINIPC_TRACE (_T("begin - closePipeClient: %p, _hPipe: %p"), this, _hPipe);

	if (_hPipe != INVALID_HANDLE_VALUE) {
		::CloseHandle (_hPipe);
		_hPipe = INVALID_HANDLE_VALUE;
	}

	WINIPC_TRACE (_T("end - closePipeClient: %p"), this);
}
