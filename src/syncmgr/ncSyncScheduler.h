/***************************************************************************************************
Purpose:
	Header file for class ncSyncScheduler.

Author:
	Chaos

Created Time:
	2016-8-19
***************************************************************************************************/

#ifndef __NC_SYNC_SCHEDULER_H__
#define __NC_SYNC_SCHEDULER_H__

#if PRAGMA_ONCE
	#pragma once
#endif

#include "public/ncISyncScheduler.h"

class ncISyncTask;

struct ncTaskInfo
{
	nsCOMPtr<ncISyncTask>	_task;
	bool			_bVisible;
	bool			_bHang;

	ncTaskInfo ()
		: _task (0)
		, _bVisible (true)
		, _bHang (false)
	{}
};


typedef pair<uint, ncTaskInfo> ncSyncTaskPair;
typedef map<uint, ncTaskInfo> ncSyncTaskMap;
typedef map<String, list<ncTaskType> > ncTaskPathMap;

class ncSyncScheduler : public ncISyncScheduler, public Thread
{
public:
	NS_DECL_ISUPPORTS
	NS_DECL_NCISYNCSCHEDULER

	ncSyncScheduler();
	virtual ~ncSyncScheduler();

public:
	// 启动任务调度。
	void Start ();

	// 停止任务调度。
	void Stop ();

	// 等待执行中的任务停止。
	void WaitStop ();

	// 检查调度是否空闲。
	bool IsIdle ();

	void run ();

public:
	// 任务优先级定义。
	enum ncTaskPriority
	{
		NC_TP_EXECUTING,	// 执行中，不在排队
		NC_TP_URGENT,	// 紧急任务：目录触发下载
		NC_TP_PRIOR,	// 优先任务：删除、重命名、云端复制移动、新建目录、下载延迟文件、历史版本还原、清除本地缓存
		NC_TP_NORMAL,	// 常规任务：文件触发下载、文件预览下载
		NC_TP_LOW,		// 低级任务：立即下载、盘内外直传、探测/扫描/监控产生的上传下载等任务
	};

	// 添加一个任务。
	uint AddTask (ncISyncTask* task, ncTaskPriority priority);

	// 清理一个任务。
	void CleanTask (const ncSyncTaskPair& task, bool bExec = true);
	// 清理一个任务。
	void CleanHangTask (const ncSyncTaskPair& task);

	// 获取停止任务调度标志
	bool IsStop ()
	{
		return  _stop ? true : false;
	}

protected:
	// 执行一个任务。
	bool ExecuteTask (ncSyncTaskMap& taskMap, bool isUrgent);

	// 获取一类任务详情。
	bool GetDetail (ncTaskDetailVec& detailVec,
					const ncSyncTaskMap& taskMap,
					uint& begin,
					uint& count);

private:
	// 取消一个任务。
	void CancelTask (ncISyncTask* task);

	// 任务关联检查
	bool TaskRelatedCheck (ncISyncTask* task, ncTaskPriority& priority);

private:
	// 线程池
	ThreadPool					_threadPool;		// 执行线程池
	// 线程锁
	ThreadMutexLock				_taskLock;
	ThreadMutexLock				_pathLock;
	AtomicValue					_stop;				// 是否停止调度
	
	// 任务集
	vector<ncSyncTaskMap>		_taskMap;		// 分级taskMap组
	set<uint>					_pauseSet;			// 已暂停执行的任务

	// 路径集
	ncTaskPathMap				_allPaths;			// 所有任务的路径（仅包含可见任务）
	ncTaskPathMap				_invisiblePaths;			// 不可见任务的路径
	set<String>					_execPaths;			// 执行中任务的路径

	// 计数
	uint						_taskId;			// 最新任务ID
	uint						_frontTaskNum;			// 可见任务总任务数
	uint						_hideTaskNum;		// 不可见任务总任务数
	uint						_execPauseNum;			// 执行中taskMap暂停数

	// 限制常量
	static const int			_urgentThNum=2;		// 最大紧急执行线程数
	static const int			_priorThNum=3;		// 最大优先执行线程数
	static const int			_normalThNum=5;		// 最大常规执行线程数 （可见）

	static const int			_mapNum = 5;		// taskMap分级队列数量
};

#endif // __NC_SYNC_SCHEDULER_H__
