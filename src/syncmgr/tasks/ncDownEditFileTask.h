/***************************************************************************************************
Purpose:
	Header file for class ncDownEditFileTask.

Author:
	Chaos

Created Time:
	2016-09-10

***************************************************************************************************/

#ifndef __NC_DOWN_EDIT_FILE_TASK_H__
#define __NC_DOWN_EDIT_FILE_TASK_H__

#if PRAGMA_ONCE
	#pragma once
#endif

#include "ncDownEditFileBaseTask.h"

enum ncDownloadType {
	NC_DT_REQUEST, 		// 请求下载
	NC_DT_OPEN,			// 打开操作，触发下载
	NC_DT_AUTO,			// 自动下载
};

class ncDownEditFileTask 
	: public ncDownEditFileBaseTask
{
public:
	ncDownEditFileTask (ncTaskSrc taskSrc,
						const String & localPath,
						const String & docId,
						const String & relPath,
						const String & otag,
						int docType,
						const String & typeName,
						int viewType,
						const String & viewName,
						int64 mtime,
						int64 size,
						int attr,
						bool isNonius = false,
						int dtType = 0);
	~ncDownEditFileTask ();

public:
	NS_IMETHOD_(void) GetTaskPath(String & relPath, String & relPathEx);
	NS_IMETHOD_(void) OnUnSchedule(void);
	NS_IMETHOD_(void) Execute(void);

private:
	int							_docType;
	String						_typeName;
	int							_viewType;
	String						_viewName;
	int							_attr;
	ncDownloadType		_dtType;
};

#endif //__NC_DOWN_EDIT_FILE_TASK_H__
