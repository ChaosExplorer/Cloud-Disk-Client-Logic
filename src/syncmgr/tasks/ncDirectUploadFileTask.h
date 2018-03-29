/***************************************************************************************************
Purpose:
	Header file for class ncDirectUploadFileTask.

Author:
	Chaos

Created Time:
	2016-02-02

***************************************************************************************************/

#ifndef __NC_DIRECT_UPLOAD_FILE_TASK_H__
#define __NC_DIRECT_UPLOAD_FILE_TASK_H__

#if PRAGMA_ONCE
	#pragma once
#endif

#include "ncUpEditFileBaseTask.h"

class ncIEFSPClient;

class ncDirectUploadFileTask 
	: public ncUpEditFileBaseTask
{
public:
	ncDirectUploadFileTask (ncTaskSrc taskSrc,
							const String & localPath,
							const String & srcPath,
							const String & newRelPath,
							const String & parentDocId,
							bool delSrc,
							bool isNonius,
							int onDup = 1);
	~ncDirectUploadFileTask ();

public:
	NS_IMETHOD_(void) GetTaskPath(String & relPath, String & relPathEx);
	NS_IMETHOD_(void) OnUnSchedule(void);
	NS_IMETHOD_(void) Execute(void);

private:
	// ִ���ϴ�����
	void Execute (ncIEFSPClient * efspClient);

	// ����ͬ����ͻ��
	virtual void HandleConflict (const Exception& e);

	// ����ܼ������Ƿ������ǰ·��
	void CheckPathInVec ();

private:
	// ���������ϴ��ļ���
	void ReUploadFile ();

private:
	friend class	ncDirectUploadDirTask;
	String			_newRelPath;
	String			_parentDocId;
	String			_logNewRelPath;
	bool			_failed;

	ncThirdSecurityConfig		_tsConfig;
};

#endif //__NC_DIRECT_UPLOAD_FILE_TASK_H__
