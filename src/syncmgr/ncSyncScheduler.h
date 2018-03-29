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
	// ����������ȡ�
	void Start ();

	// ֹͣ������ȡ�
	void Stop ();

	// �ȴ�ִ���е�����ֹͣ��
	void WaitStop ();

	// �������Ƿ���С�
	bool IsIdle ();

	void run ();

public:
	// �������ȼ����塣
	enum ncTaskPriority
	{
		NC_TP_EXECUTING,	// ִ���У������Ŷ�
		NC_TP_URGENT,	// ��������Ŀ¼��������
		NC_TP_PRIOR,	// ��������ɾ�������������ƶ˸����ƶ����½�Ŀ¼�������ӳ��ļ�����ʷ�汾��ԭ��������ػ���
		NC_TP_NORMAL,	// ���������ļ��������ء��ļ�Ԥ������
		NC_TP_LOW,		// �ͼ������������ء�������ֱ����̽��/ɨ��/��ز������ϴ����ص�����
	};

	// ���һ������
	uint AddTask (ncISyncTask* task, ncTaskPriority priority);

	// ����һ������
	void CleanTask (const ncSyncTaskPair& task, bool bExec = true);
	// ����һ������
	void CleanHangTask (const ncSyncTaskPair& task);

	// ��ȡֹͣ������ȱ�־
	bool IsStop ()
	{
		return  _stop ? true : false;
	}

protected:
	// ִ��һ������
	bool ExecuteTask (ncSyncTaskMap& taskMap, bool isUrgent);

	// ��ȡһ���������顣
	bool GetDetail (ncTaskDetailVec& detailVec,
					const ncSyncTaskMap& taskMap,
					uint& begin,
					uint& count);

private:
	// ȡ��һ������
	void CancelTask (ncISyncTask* task);

	// ����������
	bool TaskRelatedCheck (ncISyncTask* task, ncTaskPriority& priority);

private:
	// �̳߳�
	ThreadPool					_threadPool;		// ִ���̳߳�
	// �߳���
	ThreadMutexLock				_taskLock;
	ThreadMutexLock				_pathLock;
	AtomicValue					_stop;				// �Ƿ�ֹͣ����
	
	// ����
	vector<ncSyncTaskMap>		_taskMap;		// �ּ�taskMap��
	set<uint>					_pauseSet;			// ����ִͣ�е�����

	// ·����
	ncTaskPathMap				_allPaths;			// ���������·�����������ɼ�����
	ncTaskPathMap				_invisiblePaths;			// ���ɼ������·��
	set<String>					_execPaths;			// ִ���������·��

	// ����
	uint						_taskId;			// ��������ID
	uint						_frontTaskNum;			// �ɼ�������������
	uint						_hideTaskNum;		// ���ɼ�������������
	uint						_execPauseNum;			// ִ����taskMap��ͣ��

	// ���Ƴ���
	static const int			_urgentThNum=2;		// ������ִ���߳���
	static const int			_priorThNum=3;		// �������ִ���߳���
	static const int			_normalThNum=5;		// ��󳣹�ִ���߳��� ���ɼ���

	static const int			_mapNum = 5;		// taskMap�ּ���������
};

#endif // __NC_SYNC_SCHEDULER_H__
