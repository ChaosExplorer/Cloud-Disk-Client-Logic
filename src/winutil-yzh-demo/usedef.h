/***************************************************************************************************
usedef.h:
	Copyright (c) Eisoo Software Inc.(2004-2015), All rights reserved

Purpose:
	Header file for making use of winutil library.

Author:
	Chaos

Creating Time:
	2015-5-15
***************************************************************************************************/

#ifndef __USE_WINUTIL_H__
#define __USE_WINUTIL_H__

#pragma once

////////////////////////////////////////////////////////////////////////////////////////////////////
// App Name
//

#define WINUTIL_APP_NAME		_T("AnyShare")

////////////////////////////////////////////////////////////////////////////////////////////////////
// ADS Name
//

#define ADS_DELAY				_T("isDelay")
#define ADS_DOCID				_T("docId")
#define ADS_VRTYPE				_T("vrType")
#define ADS_ERRMARK			_T("errMark")

////////////////////////////////////////////////////////////////////////////////////////////////////
// Disk CLSID
//

#define CLSID_SHAER_DISK		_T("{D045FE63-1AA5-4f55-B91E-AE76567F8E46}")
#define CLSID_DISK_PROPSH		_T("{4063CECE-327C-4AC4-BD04-880996F0FE21}")
#define DISK_PROPSH_NAME		_T(".AnySharePropSheet")

////////////////////////////////////////////////////////////////////////////////////////////////////
// Pipe Name
//

#define HOOK_PIPE_NAME			_T("\\\\.\\pipe\\ipchook")
#define SHELL_PIPE_NAME			_T("\\\\.\\pipe\\ipcshell")

////////////////////////////////////////////////////////////////////////////////////////////////////
// Pipe Messages
//

#define DEF_IPC_MSG_ID_VALUE(msgID, value)				const int msgID = value;

// Hook Pipe Messages
DEF_IPC_MSG_ID_VALUE (HOOK_DOWNLOAD_FILE,				1);
DEF_IPC_MSG_ID_VALUE (HOOK_DOWNLOAD_DIR,				2);
DEF_IPC_MSG_ID_VALUE (HOOK_IS_DOWNLOADED,				3);
DEF_IPC_MSG_ID_VALUE (HOOK_LOCK_FILE,					4);
DEF_IPC_MSG_ID_VALUE (HOOK_COPY_MOVE,					5);
DEF_IPC_MSG_ID_VALUE (HOOK_CHECK_PRINT_DOC,				6);
DEF_IPC_MSG_ID_VALUE (HOOK_CHECK_CAPTRUE_SCREEN,		7);
DEF_IPC_MSG_ID_VALUE (HOOK_CHECK_FILE_PERMISSION,		8);
DEF_IPC_MSG_ID_VALUE (HOOK_PREVIEW_FILE,				9);
DEF_IPC_MSG_ID_VALUE (HOOK_REJECT_FILE_OPEN,			10);

// Shell Pipe Messages
DEF_IPC_MSG_ID_VALUE (SHELL_GET_DOC_STATE,				1);

#endif // __USE_WINUTIL_H__
