/***************************************************************************************************
msghandler.h:
	Copyright (c) Eisoo Software Inc.(2004-2012), All rights reserved

Purpose:
	Header file for class asIIPCMsgHandler.

Author:
	Chaos

Creating Time:
	2012-6-28
***************************************************************************************************/

#ifndef __AS_IIPC_MSG_HANDLER_H__
#define __AS_IIPC_MSG_HANDLER_H__

#pragma once

class asIIPCMsgHandler
{
public:
	virtual ~asIIPCMsgHandler () {}

	/**
	 * 与消息观察器进行绑定。
	 */
	virtual void registerMsgs (asIPCMsgObserver* observer) = 0;

}; // End class asIIPCMsgHandler

#endif // __AS_IIPC_MSG_HANDLER_H__
