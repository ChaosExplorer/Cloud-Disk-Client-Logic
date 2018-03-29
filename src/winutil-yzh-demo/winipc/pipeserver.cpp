/***************************************************************************************************
pipeserver.cpp:
	Copyright (c) Eisoo Software Inc.(2004-2012), All rights reserved

Purpose:
	Source file for class asIPCPipeServer.

Author:
	Chaos

Creating Time:
	2012-7-3
***************************************************************************************************/

#include <abprec.h>
#include "winipc.h"

// public ctor
asIPCPipeServer::asIPCPipeServer (LPCTSTR lpPipeName)
	: _hPipe (INVALID_HANDLE_VALUE)
	, _bStop (FALSE)
{
	WINIPC_TRACE (_T("asIPCPipeServer::asIPCPipeServer: %p"), this);

	TCHAR szUserName[PIPE_NAMELEN];
	TCHAR szDomainName[PIPE_NAMELEN];
	::ExpandEnvironmentStrings (_T("%USERNAME%"), szUserName, PIPE_NAMELEN);
	::ExpandEnvironmentStrings (_T("%USERDOMAIN%"), szDomainName, PIPE_NAMELEN);

	_sntprintf_s(_szPipeName, PIPE_NAMELEN, _TRUNCATE, _T("%s_%s_%s"), lpPipeName, szUserName, szDomainName);

	createPipeServer ();
}

// public dtor
asIPCPipeServer::~asIPCPipeServer ()
{
	closePipeServer ();

	WINIPC_TRACE (_T("asIPCPipeServer::~asIPCPipeServer: %p"), this);
}

// public
BOOL
asIPCPipeServer::getRequest (asIPCPipeMessage& requestMsg)
{
	WINIPC_TRACE (_T("begin - getRequest: %p"), this);

	if (!::ConnectNamedPipe (_hPipe, NULL) && ::GetLastError() != ERROR_PIPE_CONNECTED)
		asThrowIPCException (WINIPC_OPEN_PIPE_ERROR);

	if (_bStop)
		return FALSE;

	requestMsg.readFromPipe (_hPipe);

	WINIPC_TRACE (_T("end - getRequest: %p"), this);
	return TRUE;
}

// public
void
asIPCPipeServer::sendResult (const asIPCPipeMessage& resultMsg)
{
	WINIPC_TRACE (_T("begin - sendResult: %p"), this);

	resultMsg.writeToPipe (_hPipe);

	::FlushFileBuffers (_hPipe);
	::DisconnectNamedPipe (_hPipe);

	WINIPC_TRACE (_T("end - sendResult: %p"), this);
}

// public
void
asIPCPipeServer::stopListen ()
{
	WINIPC_TRACE (_T("begin - stopListen: %p"), this);

	_bStop = TRUE;

	if (::WaitNamedPipe (_szPipeName, 0)) {
		HANDLE hPipe = ::CreateFile (_szPipeName,
							GENERIC_READ | GENERIC_WRITE,
							0,
							NULL,
							OPEN_EXISTING,
							0,
							NULL);
		if (hPipe != INVALID_HANDLE_VALUE) {
			CloseHandle (hPipe);
		}
	}

	WINIPC_TRACE (_T("end - stopListen: %p"), this);
}

// protected
void
asIPCPipeServer::createPipeServer ()
{
	WINIPC_TRACE (_T("begin - createPipeServer: %p"), this);

	//
	// 设置管道安全属性。
	//
	BYTE sd[SECURITY_DESCRIPTOR_MIN_LENGTH];
	SECURITY_ATTRIBUTES  sa;
	sa.nLength = sizeof (sa);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = &sd;
	InitializeSecurityDescriptor (&sd, SECURITY_DESCRIPTOR_REVISION);
	SetSecurityDescriptorDacl (&sd, TRUE, (PACL) 0, FALSE);

	_hPipe = ::CreateNamedPipe (_szPipeName,
					PIPE_ACCESS_DUPLEX,
					PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
					PIPE_UNLIMITED_INSTANCES,
					PIPE_BUFSIZE,
					PIPE_BUFSIZE,
					0,
					&sa);	// 管道安全属性，允许所有用户访问（解决WIN7下UAC问题）

	if (_hPipe == INVALID_HANDLE_VALUE)
		asThrowIPCException (WINIPC_CREATE_PIPE_ERROR);

	WINIPC_TRACE (_T("end - createPipeServer: %p, _hPipe: %p"), this, _hPipe);
}

// protected
void
asIPCPipeServer::closePipeServer ()
{
	WINIPC_TRACE (_T("begin - closePipeServer: %p, _hPipe: %p"), this, _hPipe);

	if (_hPipe != INVALID_HANDLE_VALUE) {
		::CloseHandle (_hPipe);
		_hPipe = INVALID_HANDLE_VALUE;
	}

	WINIPC_TRACE (_T("end - closePipeServer: %p"), this);
}
