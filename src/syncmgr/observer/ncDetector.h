/***************************************************************************************************
Purpose:
	Header file for class ncDetector.

Author:
	Chaos

Creating Time:
	2016-8-19
***************************************************************************************************/

#ifndef __NC_DETECTOR_H__
#define __NC_DETECTOR_H__

#if PRAGMA_ONCE
	#pragma once
#endif

#include <docmgr/public/ncICacheMgr.h>
#include <syncmgr/private/ncISyncTask.h>

class ncIEACPClient;
class ncIEFSPClient;
class ncSyncPolicyMgr;
class ncSyncScheduler;
class ncChangeListener;

// 探测模式定义。
enum ncDetectMode
{
	NC_DM_DELAY,		// 延迟模式（探测时跳过延迟下载的目录，并对新增对象进行延迟下载）
	NC_DM_POLICY,		// 策略模式（探测时检查延迟下载策略，并根据策略对新增对象进行对应下载）
	NC_DM_FULL,			// 完整模式（探测时不跳过延迟下载的目录，并对新增对象进行完全下载）
};

class ncDetector 
	: public Thread
{
public:
	ncDetector (ncIEACPClient* eacpClient,
				ncIEFSPClient* efspClient,
				ncIDocMgr* docMgr,
				ncSyncPolicyMgr* policyMgr,
				ncSyncScheduler* scheduler,
				ncChangeListener* chgListener,
				String localSyncPath);
	virtual ~ncDetector ();

public:
	// 开始探测（启动探测线程）。
	void Start ();

	// 停止探测（停止探测线程）。
	void Stop ();

	// 内联探测（在调用者线程中探测指定目录）。
	void InlineDetect (const String& docId, const String& relPath, ncDetectMode mode);

	// 内联探测（在调用者线程中探测下载指定路径）
	void InlineDetectExStart (const String& parrentDocId, const String& relPath, const String& docId, bool isDir);
	void InlineDetectExEnd ();

	void run ();

protected:
	// 探测指定目录。
	bool DetectDir (const String& docId,
					const String& relPath,
					ncDetectMode mode,
					bool isDelay = false);

	// 获取目录的云端子文档信息。
	bool GetServerSubInfos (const String& docId, DetectInfoMap& svrMap);

	// 获取目录的云端指定子文档信息。
	bool GetServerSubInfo (const String& docId, const String& subDocId, ncDetectInfo& subInfo, bool isDir);

	// 获取文档重命名信息。
	void GetRenameInfo (const String& svrName,
						const String& cacheName,
						map<String, String>& rnmDirs,
						map<String, String>& rnmFiles,
						bool isDir);

	// 获取目录的云端子文档信息。
	bool CheckDir (const String& docId);

private:
	AtomicValue						_stop;			// 是否停止探测
	AtomicValue						_inlCnt;		// 内联探测计数

	nsCOMPtr<ncIEACPClient>			_eacpClient;	// EACP客户端对象
	nsCOMPtr<ncIEFSPClient>			_efspClient;	// EFSP客户端对象
	nsCOMPtr<ncIDocMgr>				_docMgr;		// 文档管理对象
	nsCOMPtr<ncICacheMgr>			_cacheMgr;		// 缓存管理对象
	nsCOMPtr<ncSyncPolicyMgr>		_policyMgr;		// 同步策略管理对象
	nsCOMPtr<ncSyncScheduler>		_scheduler;		// 同步调度对象
	nsCOMPtr<ncChangeListener>		_chgListener;	// 变化监听对象

	bool							_newView;		// 新模式
	ncTaskSrc						_taskSrc;		// 任务来源
	String							_localSyncPath;	// 本地同步目录
};

#endif // __NC_DETECTOR_H__
