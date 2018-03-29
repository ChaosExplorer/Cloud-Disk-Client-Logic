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
	// 上传定密系列
	// 请求服务端创建文件夹操作
	void CreateDir (ncIEFSPDirectoryOperation * dirOp, String& parentDocId, String& docId, const String& srcPath, String& newRelPath);

	// 上传前遍历文件夹，拿到所有的原路径和目标路径的对应关系并创建所有层级目录
	void listCsfDir (ncIEFSPDirectoryOperation * dirOp, vector<ncThirdPathCsf>& vecThirdPathCsf, String& parentDocId, const String& srcPath, String& newRelPath); 

	// 递归上传本地目录下的所有子孙对象。
	bool UploadDir (const String & srcPath, bool isRecurse = false);

	// 常规系列
	bool UploadDir (ncIEFSPDirectoryOperation * dirOp, const String & srcPath, const String & newRelPath, const String & parentDocId, bool isRecurse = false);

public:
	// 取消下载
	void Cancel ();

private:
	ncTaskType					_taskType;
	ncTaskSrc					_taskSrc;
	String						_localPath;
	String						_srcPath;
	String						_topPath;
	String						_newRelPath;
	String						_logNewRalPath;		// 记录下（同名冲突时）原路径, 供任务调度时删除任务
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
	vector<ncThirdPathCsf>		_thirdPathCsf;	// 记录一下遍历文件夹路径对应关系的结果，已判断任务是暂停重开还是第一次执行
};

#endif //__NC_DIRECT_UPLOAD_DIR_TASK_H__
