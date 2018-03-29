/***************************************************************************************************
msgobserver.cpp:
	Copyright (c) Eisoo Software Inc.(2004-2012), All rights reserved

Purpose:
	Source file for class asIPCMsgObserver.

Author:
	Chaos

Creating Time:
	2012-6-30
***************************************************************************************************/

#include <abprec.h>
#include "winipc.h"

// public ctor
asIPCMsgObserver::asIPCMsgObserver ()
	: _entries ()
{
}

// public dtor
asIPCMsgObserver::~asIPCMsgObserver ()
{
}

// public
void
asIPCMsgObserver::addMsgEntry (int msgid, MsgEntry entry)
{
	if (_entries.find (msgid) != _entries.end ()) {
		TCHAR szMsg[512];
		_sntprintf_s (szMsg, 512, _TRUNCATE, _T("Duplicated msgid: %d"), msgid);
		throw asIPCException (WINIPC_DUPLICATED_MSGID, szMsg);
	}

	_entries.insert (make_pair (msgid, entry));
}

// public
void
asIPCMsgObserver::removeMsgEntry (int msgid)
{
	_entries.erase (msgid);
}

// public
void
asIPCMsgObserver::onMessage (asIPCPipeMessage& requestMsg)
{
	int msgid;
	requestMsg >> msgid;

	MsgEntryMap::iterator iter = _entries.find (msgid);

	if (iter == _entries.end ()) {
		TCHAR szMsg[512];
		_sntprintf_s (szMsg, 512, _TRUNCATE, _T("Invalid msgid: %d"), msgid);
		throw asIPCException (WINIPC_INVALID_MSGID, szMsg);
	}

	MsgEntry entry = iter->second;
	(entry.first->*((asIPCMsgFunction) (entry.second))) (requestMsg);
}
