/***************************************************************************************************
Purpose:
	Header file for class ncIOScanner.

Author:
	Chaos

Created Time:
	2016-8-19

***************************************************************************************************/

#ifndef __NC_IO_SCANNER_H__
#define __NC_IO_SCANNER_H__

#if PRAGMA_ONCE
	#pragma once
#endif

#include <docmgr/public/ncIDocMgr.h>
#include <docmgr/public/ncICacheMgr.h>
#include <docmgr/public/ncIRelayMgr.h>
#include <syncmgr/private/ncISyncTask.h>

class ncIDocMgr;
class ncICacheMgr;
class ncIPathMgr;
class ncIADSMgr;
class ncIRelayMgr;
class ncSyncScheduler;
class ncIChangeListener;

class ncIOScanner 
	: public Thread
{
public:
	ncIOScanner (ncIDocMgr* docMgr,
				 ncSyncScheduler* scheduler,
				 ncIChangeListener* chgListener,
				 String localSyncPath);
	virtual ~ncIOScanner ();

public:
	// 开始扫描（启动扫描线程）。
	void Start ();

	// 停止扫描（停止扫描线程）。
	void Stop ();

	// 内联扫描（在调用者线程中扫描指定目录）。
	void InlineScan (const String& path);

	void run ();

protected:
	// 扫描指定目录。
	void ScanDir (const String& path);

	// 获取目录的本地子文档信息。
	bool GetLocalSubInfos (const String& path,
						   ScanInfoMap& docIdInfos,
						   set<String>& noDocIdFiles,
						   set<String>& noDocIdDirs,
						   set<String>& orgnDirs);

	void ScanNonius ();

	// 扫描检查Delay数据流，保持本地和数据库一致
	void ReviseDelayADS (const String& path);

	void ScanUnsync ();

private:
	AtomicValue						_stop;			// 是否停止扫描
	AtomicValue						_inlCnt;		// 内联扫描计数

	nsCOMPtr<ncIDocMgr>				_docMgr;		// 文档管理对象
	nsCOMPtr<ncIPathMgr>			_pathMgr;		// 路径管理对象
	nsCOMPtr<ncIADSMgr>				_adsMgr;		// 数据流管理对象
	nsCOMPtr<ncICacheMgr>			_cacheMgr;		// 缓存管理对象
	nsCOMPtr<ncIRelayMgr>			_relayMgr;		// 断点续传管理对象
	nsCOMPtr<ncIChangeListener>		_chgListener;	// 变化监听对象
	nsCOMPtr<ncSyncScheduler>		_scheduler;		// 同步调度对象

	bool							_newView;		// 新模式
	ncTaskSrc						_taskSrc;		// 任务来源
	String							_localSyncPath;	// 本地同步目录
};

#endif // __NC_IO_SCANNER_H__
