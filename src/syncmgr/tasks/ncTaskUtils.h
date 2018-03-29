/***************************************************************************************************
Purpose:
	Header file for class ncTaskUtils.

Author:
	Chaos

Created Time:
	2016-09-10

***************************************************************************************************/

#ifndef __NC_TASK_UTILS_H__
#define __NC_TASK_UTILS_H__

#if PRAGMA_ONCE
	#pragma once
#endif

#include <efshttpclient/public/ncIEFSPClient.h>
#include <syncmgr/private/ncISyncTask.h>
#include <syncmgr/public/ncISyncScheduler.h>
#include <docmgr/public/ncICacheMgr.h>
#include <docMgr/public/ncIADSMgr.h>

class ncTaskUtils
{
public:
	// 检查目录下是否有被修改的文件。
	static bool HasEditedFilesInDir (const String & path);

	// 检查并删除入口的层级父目录。
	static void CheckDelEntryParent (const String& localPath, const String& path, int docType);

	// 上传重命名的新目录。
	static void UploadRenamedDir (ncTaskSrc taskSrc, const String& localPath, const String& path);

	// 上传重命名的新文件。
	static void UploadRenamedFile (ncTaskSrc taskSrc, const String& localPath, const String& path);

	// 检查重命名的新目录的关联变化。
	static void CheckDirChange (const String& localPath, const String& path);

	// 检查重命名的新文件的关联变化。
	static void CheckRenamedFileChange (ncTaskSrc taskSrc, const String& localPath, const String& path);

	// 检查并还原可能丢失或错误的docId数据流。
	static bool RestoreDocIdChanged (const String & path, const String & docId, bool isDir = false);

	/**
	* 检查文件是否被独占。
	* @param path : 绝对路径
	* @param dwLockedAccess : 需要查询的权限
	* @param checkLasttime : 检查认定占用时长（单位：秒）
	* @param status: 任务状态
	*/ 
	static bool IsFileOccupied (const String& path, DWORD dwLockedAccess, int checLastTime, ncTaskStatus& status);

	// 添加对文件的缓存清除任务
	static void ClearFileCache (ncTaskSrc taskSrc, const String& relPath);

	// 获取顶层目录路径 （兼容虚拟目录模式）
	static String GetTopDirPath (const String& relPath);

	// 上传之前检查第三方标密软件是否将文件标密，并获取标密信息
	static bool GetFileLabelInfo (const String filePath, const String docId, String& securityInfo);

	// 封装向服务端设置标密信息
	static void SetThirdSecurityInfo (ncIEFSPClient* efspClient, const String& appId, const String& docId, const String& securityInfo);
	static void SetThirdSecurityInfoEx (ncIEFSPFileOperation* efspFileOperation, const String& appId, const String& docId, const String& securityInfo);

	// 做第三方标密密级的提取
	static void GetThirdSecurityLevel (map<int, String>csfMapEnum, String securityInfo, int& csf);

	// 判断密级信息是否为空
	static bool isSecurityInfoEmpty (const String& securityInfo);

	// 耗时操作后的状态检查
	static void TaskStatusCheck (ncTaskStatus& status);

	// 批量复制移动
	static HRESULT MutiSHCopyMove (vector<String>& srcPaths, const String& dstPath, int wFunc, int fFlags);

	// 复制移动
	static HRESULT SHCopyMove (const String& srcPath, const String& dstPath, int wFunc, int fFlags);

	// 将目录下的未同步文档复制移动到目的地
	static void TransferUnsyncInDir (const String& localPath, const String& dirPath, const String& dstPath, bool isMove, bool needExpand = true);
};

#endif  //__NC_TASK_UTILS_H__
