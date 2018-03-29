/***************************************************************************************************
Purpose:
	Header file for class ncDirectUploadDirTask.

Author:
	Chaos

Created Time:
	2016-02-02

***************************************************************************************************/

#ifndef __NC_DIRECT_UPLOAD_DIR_TASK_H__
#define __NC_DIRECT_UPLOAD_DIR_TASK_H__

#if PRAGMA_ONCE
	#pragma once
#endif

#include <syncmgr/private/ncISyncTask.h>
#include <docmgr/public/ncIPathMgr.h>
#include <docmgr/public/ncIFileMgr.h>
#include "ncDirectUploadFileTask.h"

class ncIEFSPClient;
class ncIEFSPDirectoryOperation;

class ncDirectUploadDirTask 
	: public ncISyncTask
{
public:
	NS_DECL_ISUPPORTS
	NS_DECL_NCISYNCTASK

	ncDirectUploadDirTask (ncTaskSrc taskSrc,
						   const String & localPath,
						   const String & srcPath,
						   const String & newRelPath,
						   const String & parentDocId,
						   bool delSrc,
						   int onDup = 1);
	~ncDirectUploadDirTask ();

protected:
	// �ϴ�����ϵ��
	// �������˴����ļ��в���
	void CreateDir (ncIEFSPDirectoryOperation * dirOp, String& parentDocId, String& docId, const String& srcPath, String& newRelPath);

	// �ϴ�ǰ�����ļ��У��õ����е�ԭ·����Ŀ��·���Ķ�Ӧ��ϵ���������в㼶Ŀ¼
	void listCsfDir (ncIEFSPDirectoryOperation * dirOp, vector<ncThirdPathCsf>& vecThirdPathCsf, String& parentDocId, const String& srcPath, String& newRelPath); 

	// �ݹ��ϴ�����Ŀ¼�µ������������
	bool UploadDir (const String & srcPath, bool isRecurse = false);

	// ����ϵ��
	bool UploadDir (ncIEFSPDirectoryOperation * dirOp, const String & srcPath, const String & newRelPath, const String & parentDocId, bool isRecurse = false);

public:
	// ȡ������
	void Cancel ();

private:
	ncTaskType					_taskType;
	ncTaskSrc					_taskSrc;
	String						_localPath;
	String						_srcPath;
	String						_topPath;
	String						_newRelPath;
	String						_logNewRalPath;		// ��¼�£�ͬ����ͻʱ��ԭ·��, ���������ʱɾ������
	String						_parentDocId;
	bool						_delSrc;
	int							_onDup;

	int				_onSchCount;
	bool				_isNonius;
	map<String, String>	_donePaths;
	double			_ratios;

	nsCOMPtr<ncDirectUploadFileTask>	_curFileTask;
	nsCOMPtr<ncIEFSPClient>		_efspClient;
	nsCOMPtr<ncIRelayMgr>		_relayMgr;
	nsCOMPtr<ncIPathMgr>		_pathMgr;
	nsCOMPtr<ncIFileMgr>		_fileMgr; 
	int64						_totalSize;

	int64						_doneSize;

	bool						_failed;
	ncTaskStatus				_status;

	ncThirdSecurityConfig		_tsConfig;
	vector<ncThirdPathCsf>		_thirdPathCsf;	// ��¼һ�±����ļ���·����Ӧ��ϵ�Ľ�������ж���������ͣ�ؿ����ǵ�һ��ִ��
};

#endif //__NC_DIRECT_UPLOAD_DIR_TASK_H__
