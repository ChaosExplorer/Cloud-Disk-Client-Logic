/***************************************************************************************************
msgobserver.h:
	Copyright (c) Eisoo Software Inc.(2004-2012), All rights reserved

Purpose:
	Header file for class asIPCMsgObserver.

Author:
	Chaos

Creating Time:
	2012-6-28
***************************************************************************************************/

#ifndef __AS_IPC_MSG_OBSERVER_H__
#define __AS_IPC_MSG_OBSERVER_H__

#pragma once

class asIIPCMsgHandler;

typedef void (asIIPCMsgHandler::*asIPCMsgFunction) (asIPCPipeMessage& requestMsg);
typedef pair<asIIPCMsgHandler*, asIPCMsgFunction> MsgEntry;

#define asIPCMsgFunHandler(func)											\
	static_cast<asIPCMsgFunction>(&func)

//
// ����Ϣ����������Ϣ�۲�����
//	
#define REGISTER_WINIPC_MSG(msgObserver, msgHandler)						\
	msgHandler->registerMsgs (msgObserver);

//
// ������Ϣӳ���
//
#define DECLARE_WINIPC_DYN_MSG()											\
	public:																	\
		void registerMsgs (asIPCMsgObserver* observer);

//
// ʵ����Ϣӳ���
//
#define BEGIN_WINIPC_DYN_MSG(msgHandlerClass)								\
	void msgHandlerClass::registerMsgs (asIPCMsgObserver* observer)			\
	{

#define DYN_WINIPC_MSG_HANDLER(id, func)									\
	observer->addMsgEntry (id, MsgEntry(this, asIPCMsgFunHandler(func)));

#define END_WINIPC_DYN_MSG()												\
	}

/////////////////////////////////////////////////////////////////////////////
// class asIPCMsgObserver

class WINUTIL_DLLEXPORT asIPCMsgObserver
{
public:
	asIPCMsgObserver ();
	
	~asIPCMsgObserver ();

	/**
	 * ���һ����Ϣӳ�䡣
	 */
	void addMsgEntry (int msgid, MsgEntry entry);

	/**
	 * ɾ��һ����Ϣӳ�䡣
	 */
	void removeMsgEntry (int msgid);

	/**
	 * ӳ����Ϣ����Ӧ�Ĵ������̡�
	 */
	void onMessage (asIPCPipeMessage& requestMsg);

private:
	typedef std::map<int, MsgEntry> MsgEntryMap;
	MsgEntryMap _entries;

}; // End class asIPCMsgObserver

#endif // __AS_IPC_MSG_OBSERVER_H__
