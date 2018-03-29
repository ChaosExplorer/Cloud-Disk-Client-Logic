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
// 绑定消息处理器和消息观察器。
//	
#define REGISTER_WINIPC_MSG(msgObserver, msgHandler)						\
	msgHandler->registerMsgs (msgObserver);

//
// 声明消息映射表
//
#define DECLARE_WINIPC_DYN_MSG()											\
	public:																	\
		void registerMsgs (asIPCMsgObserver* observer);

//
// 实现消息映射表
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
	 * 添加一条消息映射。
	 */
	void addMsgEntry (int msgid, MsgEntry entry);

	/**
	 * 删除一条消息映射。
	 */
	void removeMsgEntry (int msgid);

	/**
	 * 映射消息到对应的处理例程。
	 */
	void onMessage (asIPCPipeMessage& requestMsg);

private:
	typedef std::map<int, MsgEntry> MsgEntryMap;
	MsgEntryMap _entries;

}; // End class asIPCMsgObserver

#endif // __AS_IPC_MSG_OBSERVER_H__
