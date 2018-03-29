/***************************************************************************************************
Purpose:
	Source file for class ncSyncScheduler.

Author:
	Chaos

Created Time:
	2016-8-19
***************************************************************************************************/

#include <abprec.h>
#include "private/ncISyncTask.h"
#include "ncSyncScheduler.h"
#include "ncSyncUtils.h"
#include "syncmgr.h"

//
// ��������ִ�ж���
//
class ncRunnableTask : public IRunnable
{
public:
	ncRunnableTask (ncSyncScheduler* scheduler, const ncSyncTaskPair& task)
		: _scheduler (scheduler)
		, _task (task)
	{
		NC_SYNCMGR_TRACE (_T("this: 0x%p"), this);
	}

	void run ()
	{
		NC_SYNCMGR_TRACE (_T("this: 0x%p"), this);

		// ִ������
		try {
			_task.second._task->Execute ();
		}
		catch (Exception& e) {
			SystemLog::getInstance ()->log (ERROR_LOG_TYPE, e.toFullString ().getCStr ());
		}
		catch (...) {
			SystemLog::getInstance ()->log (ERROR_LOG_TYPE, _T("ncRunnableTask::run () { Execute () } failed."));
		}

		// δ��������������
		try {
			ncTaskDetail detail;
			_task.second._task->GetDetail (detail);
			if (detail._status != NC_TS_PAUSED)
				_scheduler->CleanTask (_task);
			else
				_scheduler->CleanHangTask (_task);

		}
		catch (Exception& e) {
			SystemLog::getInstance ()->log (ERROR_LOG_TYPE, e.toFullString ().getCStr ());
		}
		catch (...) {
			SystemLog::getInstance ()->log (ERROR_LOG_TYPE, _T("ncRunnableTask::run () { CleanTask () } failed."));
		}

		// �ͷ���Դ��
		NC_DELETE (syncMgrAlloc, ncRunnableTask, this);
	}

public:
	nsCOMPtr<ncSyncScheduler>	_scheduler;
	ncSyncTaskPair				_task;
};

NC_UMM_IMPL_THREADSAFE_ISUPPORTS1(syncMgrAlloc, ncSyncScheduler, ncISyncScheduler)

// public ctor
ncSyncScheduler::ncSyncScheduler()
	: _stop (true)
	, _taskId (0)
	, _frontTaskNum (0)
	, _hideTaskNum (0)
	, _execPauseNum (0)
	, _threadPool (_T("SyncScheduler"), _normalThNum, _normalThNum + _urgentThNum)
{
	NC_SYNCMGR_TRACE (_T("this: 0x%p"), this);

	// ��ʼ������
	_taskMap = vector<ncSyncTaskMap> (_mapNum);
}

// public dtor
ncSyncScheduler::~ncSyncScheduler()
{
	NC_SYNCMGR_TRACE (_T("this: 0x%p"), this);
	
	if (!_stop)
		Stop ();
}

// public
void ncSyncScheduler::Start ()
{
	NC_SYNCMGR_TRACE (_T("this: 0x%p"), this);

	InterlockedExchange (&_stop, false);

	// ����������ȡ�
	this->start ();
}

// public
void ncSyncScheduler::Stop ()
{
	NC_SYNCMGR_TRACE (_T("this: 0x%p"), this);

	// ֹͣ������ȡ�
	InterlockedExchange (&_stop, true);

	// ���������Ϣ��
	{
		AutoLock<ThreadMutexLock> taskLock (&_taskLock);
		for (int i = 0; i < _mapNum; ++i)
			_taskMap [i].clear ();
		
		_pauseSet.clear ();
		_taskId = 0;
		_frontTaskNum = 0;
		_hideTaskNum = 0;
		_execPauseNum = 0;
	}

	// ���·����Ϣ��
	{
		AutoLock<ThreadMutexLock> pathLock (&_pathLock);
		_allPaths.clear ();
		_invisiblePaths.clear ();
		_execPaths.clear ();
	}
}

// public
void ncSyncScheduler::WaitStop ()
{
	NC_SYNCMGR_TRACE (_T("this: 0x%p"), this);

	// �ȴ������߳̽�����
	_threadPool.joinAll ();

	NC_SYNCMGR_TRACE (_T(" exit this: 0x%p"), this);
}

// public
bool ncSyncScheduler::IsIdle ()
{
	AutoLock<ThreadMutexLock> taskLock (&_taskLock);

	// ������ͣ���������ڳ���ִ���߳���������Ϊ�����ǿ��еġ�
	return (_hideTaskNum < _normalThNum);
}

// public
void ncSyncScheduler::run ()
{
	NC_SYNCMGR_TRACE (_T("this: 0x%p, -begin"), this);
	this->AddRef ();

	int execNum = 0;

	while (!_stop) {
		try {
			AutoLock<ThreadMutexLock> taskLock (&_taskLock);

			// ��������ִ����������
			execNum = (int)_taskMap[NC_TP_EXECUTING].size () - _execPauseNum;

			// ������ִ���������������桢���ȼ�����ִ���߳���������ִͣ��������
			if (execNum >= _normalThNum + _priorThNum +_urgentThNum)
				goto SLEEP;

			// ��鲢ִ�н�������
			if (ExecuteTask (_taskMap[NC_TP_URGENT], true))
				continue;

			// ������ִ���������������桢����ִ���߳���������ִͣ����������
			if (execNum >= _normalThNum + _priorThNum)
				goto SLEEP;

			// ��鲢ִ����������
			if (ExecuteTask (_taskMap[NC_TP_PRIOR], false))
				continue;

			// ������ִ����������������ִ���߳���������ִͣ�г��桢�����ȼ�����
			if (execNum >= _normalThNum)
				goto SLEEP;

			// ��鲢ִ�г�������
			if (ExecuteTask (_taskMap[NC_TP_NORMAL], false))
				continue;

			// ��鲢ִ�е����ȼ�����
			if (ExecuteTask (_taskMap[NC_TP_LOW], false))
				continue;
		}
		catch (Exception& e) {
			SystemLog::getInstance ()->log (ERROR_LOG_TYPE, e.toFullString ().getCStr ());
		}
		catch (...) {
			SystemLog::getInstance ()->log (ERROR_LOG_TYPE, _T("ncSyncScheduler::run () failed."));
		}

SLEEP:
		if (!_stop)
			sleep (100);
	}

	this->Release ();
	NC_SYNCMGR_TRACE (_T("this: 0x%p, -end"), this);
}

/* [notxpcom] void GetSummary (in ncTaskSummaryRef summary); */
NS_IMETHODIMP_(void) ncSyncScheduler::GetSummary(ncTaskSummary & summary)
{
	NC_SYNCMGR_TRACE (_T("this: 0x%p"), this);

	AutoLock<ThreadMutexLock> lock (&_taskLock);

	summary._num = _frontTaskNum;
}

/* [notxpcom] void GetDetail (in ncTaskDetailVecRef detailVec,
							  in uint begin,
							  in uint end,
							  in uintRef total); */
NS_IMETHODIMP_(void) ncSyncScheduler::GetDetail(ncTaskDetailVec & detailVec,
							  uint begin,
							  uint end,
							  uint& total,
							  bool& bAllPaused)
{
	NC_SYNCMGR_TRACE (_T("begin: %u, end: %u"), begin, end);

	detailVec.clear ();

	AutoLock<ThreadMutexLock> lock (&_taskLock);

	// ��ȡ�����������������ɼ����񣩡�
	total = _frontTaskNum;
	bAllPaused = _frontTaskNum == _pauseSet.size ();

	if (total == 0)
		return;

	// ��������Ƿ�Խ�硣
	if (begin > end || begin > total) {
		String errMsg;
		errMsg.format (_T("Invalid param: begin��%u��end��%u, total��%u"), begin, end, total);
		NC_SYNCMGR_THROW (errMsg, SYNCMGR_INVALID_ARGUMENT);
	}

	// ���������ڵ���������
	uint count = total > end ? end - begin : total - begin;

	// ������״̬��ȡ��Ӧ�����е��������顣
	for (int i = 0; i< _mapNum; ++i) {
		if (GetDetail(detailVec, _taskMap[i], begin, count))
			return;
	}
}

/* [notxpcom] void PauseTask (in uint taskId); */
NS_IMETHODIMP_(void) ncSyncScheduler::PauseTask(uint taskId)
{
	NC_SYNCMGR_TRACE (_T("taskId: %u"), taskId);

	AutoLock<ThreadMutexLock> lock (&_taskLock);

	if (_stop)
		return;

	// ���ҳ��������ڶ��С�
	int i = 0;
	ncSyncTaskMap::iterator taskIt;
	while (i < _mapNum) {
		taskIt = _taskMap[i].find (taskId);

		if (taskIt != _taskMap[i].end ()) {
			// ��ǹ���
			taskIt->second._bHang = true;
			// ��ֹ�����߳�
			taskIt->second._task->Pause ();

			// ���������_pauseSet.
			_pauseSet.insert (taskId);

			if (i == NC_TP_EXECUTING)
				++_execPauseNum;

			break;
		}

		++i;
	}
}

/* [notxpcom] void ResumeTask (in uint taskId); */
NS_IMETHODIMP_(void) ncSyncScheduler::ResumeTask(uint taskId)
{
	NC_SYNCMGR_TRACE (_T("taskId: %u"), taskId);

	AutoLock<ThreadMutexLock> lock (&_taskLock);

	if (_stop)
		return;

	// ���ҳ��������ڶ��С�
	int i = 0;
	ncSyncTaskMap::iterator taskIt;
	while (i < _mapNum) {
		taskIt = _taskMap[i].find (taskId);

		if (taskIt != _taskMap[i].end ()) {
			// ��ǹ���
			taskIt->second._bHang = false;

			// �������_pauseSet���Ƴ���
			_pauseSet.erase (taskId);
			taskIt->second._task->OnSchedule ();

			if (i == NC_TP_EXECUTING) {
				--_execPauseNum;

				// �ύ������ȡ�
				_taskMap[NC_TP_NORMAL].insert (*taskIt);
				_taskMap [i].erase (taskIt);
			}

			break;
		}

		++i;
	}
}

/* [notxpcom] void CancelTask (in uint taskId); */
NS_IMETHODIMP_(void) ncSyncScheduler::CancelTask(uint taskId)
{
	NC_SYNCMGR_TRACE (_T("taskId: %u"), taskId);

	bool isExecuting = false;

	// ��ȡ����ID��Ӧ��������󣬲����������������Ƴ���
	{
		AutoLock<ThreadMutexLock> lock (&_taskLock);
		ncSyncTaskMap::iterator taskIt;
		int i = 0;
		bool bInExecMap = false;
		while (i < _mapNum) {
			taskIt = _taskMap[i].find (taskId);

			if (taskIt != _taskMap[i].end ()) {				
				bInExecMap = NC_TP_EXECUTING == i;

				if (taskIt->second._bHang) {
					// �������_pauseSet���Ƴ���
					_pauseSet.erase (taskId);

					if (bInExecMap)
						--_execPauseNum;

				}
				// ����ִ����û�й���
				else if (bInExecMap) {
					isExecuting = true;		// ����ִ��
				}

				// �����������ִ�У���ȡ����ִ�У������ֶ����������
				if (taskIt->second._task != 0) {
					CancelTask (taskIt->second._task);

					// ����ִ�У��ֶ�����
					if (!isExecuting) {
						CleanTask (*taskIt, false);
						_taskMap[i].erase (taskIt);		// ��taskMap�Ƴ�
					}
				}

				break;
			}

			++i;
		}
	}
}

/* [notxpcom] void WaitTaskDone (in uint taskId); */
NS_IMETHODIMP_(void) ncSyncScheduler::WaitTaskDone(uint taskId)
{
	NC_SYNCMGR_TRACE (_T("taskId: %u"), taskId);

	while (HasIDTask (taskId)) {
		Sleep (50);
	}
}

/* [notxpcom] bool HasIDTask (in uint taskId); */
NS_IMETHODIMP_(bool) ncSyncScheduler::HasIDTask(uint taskId)
{
	NC_SYNCMGR_TRACE (_T("taskId: %u"), taskId);

	AutoLock<ThreadMutexLock> lock (&_taskLock);

	// ����Ƿ����ָ������ID.
	for (int i = 0; i < _mapNum; ++i) {
		if (_taskMap[i].find (taskId) != _taskMap[i].end ())
			return true;
	}
	return false;
}

/* [notxpcom] bool HasPathTask ([const] in StringRef relPath, in bool checkChild); */
NS_IMETHODIMP_(bool) ncSyncScheduler::HasPathTask(const String & relPath, bool checkChild)
{
	NC_SYNCMGR_TRACE (_T("relPath: %s, checkChild: %d"), relPath, checkChild);
	NC_SYNCMGR_CHECK_PATH_INVALID (relPath);

	AutoLock<ThreadMutexLock> lock (&_pathLock);

	if (!checkChild) {
		// ����Ƿ����ָ��·����
		return _allPaths.find (relPath) != _allPaths.end () || _invisiblePaths.find (relPath) != _invisiblePaths.end ();
	}
	else {
		// ����Ƿ����ָ��·����������·����
		ncTaskPathMap::iterator it = _allPaths.lower_bound (relPath);
		ncTaskPathMap::iterator it2 = _invisiblePaths.lower_bound (relPath);
		return (it == _allPaths.end () ? false : ncSyncUtils::IsParentPath (relPath, it->first, true)) || (it2 == _invisiblePaths.end () ? false : ncSyncUtils::IsParentPath (relPath, it2->first, true));
	}
}

/* [notxpcom] bool HasAnyTask (); */
NS_IMETHODIMP_(bool) ncSyncScheduler::HasAnyTask()
{
	NC_SYNCMGR_TRACE (_T("this: 0x%p"), this);

	// ����Ƿ�����κ�����
	AutoLock<ThreadMutexLock> lock (&_taskLock);
	for (int i = 0; i < _mapNum; ++i) {
		if (!_taskMap[i].empty ())
			return true;
	}

	return false;
}

// ����Ƿ�Ϊ��������
inline bool IsTaskUpward (const ncSyncTaskPair& task)
{
	return IsUpwardTask (task.second._task->GetTaskType ());
}

/* [notxpcom] bool HasUpwardTask (); */
NS_IMETHODIMP_(bool) ncSyncScheduler::HasUpwardTask()
{
	NC_SYNCMGR_TRACE (_T("this: 0x%p"), this);

	AutoLock<ThreadMutexLock> lock (&_taskLock);

	// ��������������Ƿ������������
	for (int i = 0; i < _mapNum; ++i) {
		if (find_if (_taskMap[i].begin (), _taskMap[i].end (), IsTaskUpward) != _taskMap[i].end ())
			return true;
	}
	
	return false;
}

// private
// ����������
bool ncSyncScheduler::TaskRelatedCheck (ncISyncTask* task, ncTaskPriority& priority)
{
	NC_SYNCMGR_TRACE (_T("task: 0x%p, priority: %d"), task, priority);

	ncTaskType taskType = task->GetTaskType ();
	bool _bVisible = IsVisibleTask (taskType);

	String relPath, relPathEx;
	task->GetTaskPath (relPath, relPathEx);

	{
		AutoLock<ThreadMutexLock> lock (&_pathLock);
		ncTaskPathMap::iterator it;

		if (_stop)
			return false;

		// �����ӳٶ��󣨲��ɼ��������ض�Ӧ�����������
		if (NC_TT_DOWN_DELAY_FILE == taskType)
		{
			it = _allPaths.find (relPath);
			if (it != _allPaths.end () && NC_TT_DOWN_EDIT_FILE == it->second.back ())
				return false;

			it = _invisiblePaths.find (relPath);
			if (it != _invisiblePaths.end () && NC_TT_AUTO_DOWN_FILE == it->second.back ())
				return false;
		}

		// ������Ѵ���������·���������򽵵���ִ�����ȼ���
		if (NC_TP_URGENT < priority && priority < NC_TP_LOW) {
			for (it = _allPaths.begin (); it != _allPaths.end (); ++it) {
				if (ncSyncUtils::IsRelatedPath(relPath, it->first) ||
					(!relPathEx.isEmpty () && ncSyncUtils::IsRelatedPath(relPathEx, it->first))) {
						priority = NC_TP_LOW;
						break;
				}
			}

			for (it = _invisiblePaths.begin (); it != _invisiblePaths.end (); ++it) {
				if (ncSyncUtils::IsRelatedPath(relPath, it->first) ||
					(!relPathEx.isEmpty () && ncSyncUtils::IsRelatedPath(relPathEx, it->first))) {
						priority = NC_TP_LOW;
						break;
				}
			}
		}

		// ������ظ���������˵���
		if (_bVisible) {
			it = _allPaths.find (relPath);
			if (it == _allPaths.end ())
				_allPaths.insert (make_pair (relPath, list<ncTaskType> (1, taskType)));
			else if (it->second.back () != taskType)
				it->second.push_back (taskType);
			else
				return false;
		}
		// ���ɼ���һЩ����Ҳ������ظ�����
		else if (IsNoOverlapTask (taskType)) {
			it = _invisiblePaths.find (relPath);
			if (it == _invisiblePaths.end ())
				_invisiblePaths.insert (make_pair (relPath, list<ncTaskType> (1, taskType)));
			else if (it->second.back () != taskType)
				it->second.push_back (taskType);
			else
				return false;
		}
	}
	return true;
}

// public
uint ncSyncScheduler::AddTask(ncISyncTask* task, ncTaskPriority priority)
{
	NC_SYNCMGR_TRACE (_T("task: 0x%p, priority: %d"), task, priority);
	NC_SYNCMGR_CHECK_ARGUMENT_NULL (task);

	String relPath, relPathEx;
	task->GetTaskPath (relPath, relPathEx);
	ncTaskType taskType = task->GetTaskType ();

	try {

		// ���������鴦��
		if (!TaskRelatedCheck(task, priority))
			return 0;		// invalid task

		uint taskId = 0;

		// �ύ������ȡ�
		task->OnSchedule ();

		// ����taskMap��Ϣ
		ncTaskInfo taskInfo;

		
		taskInfo._bVisible = IsVisibleTask (taskType);

		ncTaskDetail detail;
		task->GetDetail (detail);
		taskInfo._bHang = detail._status == NC_TS_PAUSED;		// ��ͣ�ϵ��Թ���״̬����

		taskInfo._task = task;

		// ���� taskMap(_urgentMap, priorMap,  _normalMap��_lowMap).
		{
			AutoLock<ThreadMutexLock> lock (&_taskLock);
			taskId = ++_taskId;
			
			if (priority < _mapNum)			// safe
				_taskMap[priority].insert (make_pair (taskId, taskInfo));

			if (taskInfo._bHang)
				_pauseSet.insert (taskId);

			if (taskInfo._bVisible)
				++_frontTaskNum;
			else
				++ _hideTaskNum;
		}

		return taskId;
	}
	catch (Exception& e) {
		SystemLog::getInstance ()->log (ERROR_LOG_TYPE, e.toFullString ().getCStr ());
	}
	catch (...) {
		String errMgs;
		errMgs.format (_T("ncSyncScheduler::addTask () failed. path: %s, type: %d"), relPath.getCStr (), (int)taskType);
		SystemLog::getInstance ()->log (ERROR_LOG_TYPE, errMgs.getCStr ());
	}
	return 0;
}
// public

void ncSyncScheduler::CleanHangTask( const ncSyncTaskPair& task )
{
	NC_SYNCMGR_TRACE (_T("taskId: %u"), task.first);

	String relPath, relPathEx;
	task.second._task->GetTaskPath (relPath, relPathEx);

	// ����_execPaths.
	{
		AutoLock<ThreadMutexLock> lock (&_pathLock);

		_execPaths.erase (relPath);
		if (!relPathEx.isEmpty ())
			_execPaths.erase (relPathEx);
	}
}

// public
void ncSyncScheduler::CleanTask (const ncSyncTaskPair& task, bool bExec)
{
	NC_SYNCMGR_TRACE (_T("taskId: %u"), task.first);

	ncTaskType taskType = task.second._task->GetTaskType ();

	String relPath, relPathEx;
	task.second._task->GetTaskPath (relPath, relPathEx);

	// ����_allPaths��_execPaths.
	{
		AutoLock<ThreadMutexLock> lock (&_pathLock);

		if (_stop)
			return;

		if (task.second._bVisible) {
			ncTaskPathMap::iterator it = _allPaths.find (relPath);
			if (it != _allPaths.end ()) {
				if (it->second.size () <= 1)
					_allPaths.erase (it);
				else
					it->second.pop_front ();
			}
		}
		else if (IsNoOverlapTask (taskType)){
			ncTaskPathMap::iterator it = _invisiblePaths.find (relPath);
			if (it != _invisiblePaths.end ()) {
				if (it->second.size () <= 1)
					_invisiblePaths.erase (it);
				else
					it->second.pop_front ();
			}
		}

		if (bExec) {
			// ��ִ���У�û�й���
			_execPaths.erase (relPath);
			if (!relPathEx.isEmpty ())
				_execPaths.erase (relPathEx);
		}
	}

	// ���¼���.
	{
		AutoLock<ThreadMutexLock> lock (&_taskLock);
		if (bExec)
			_taskMap [NC_TP_EXECUTING].erase (task.first);
		
		if (task.second._bVisible) {
			if (_frontTaskNum > 0)
				--_frontTaskNum;

			// ����ͣ��δ���ü��ص�����������ɣ����봫��taskId������_pauseSet��
			_pauseSet.erase(task.first);
		}
		else if (_hideTaskNum > 0)
			--_hideTaskNum;
	}

	// ���������ȡ�
	task.second._task->OnUnSchedule ();
}

// protected
bool ncSyncScheduler::ExecuteTask (ncSyncTaskMap& taskMap, bool isUrgent)
{
	NC_SYNCMGR_TRACE (_T("this: 0x%p"), this);

	AutoLock<ThreadMutexLock> pathLock (&_pathLock);

	if (_stop)
		return false;

	ncSyncTaskMap::iterator taskIt;
	set<String>::iterator pathIt;
	ncTaskType taskType;
	String relPath, relPathEx;

	for (taskIt = taskMap.begin (); taskIt != taskMap.end (); ++taskIt) {
		// ��������ͣ������
		if (taskIt->second._bHang)
			continue;

		taskType = taskIt->second._task->GetTaskType ();
		taskIt->second._task->GetTaskPath (relPath, relPathEx);

		// �������ִ���������·������������
		if (!taskIt->second._bVisible && IsNoOverlapTask (taskType) &&  _invisiblePaths[relPath].front () != taskType)
			continue;

		// ����������ִ���������·������������
		for (pathIt = _execPaths.begin (); pathIt != _execPaths.end (); ++pathIt) {
			if (ncSyncUtils::IsRelatedPath (relPath, *pathIt, isUrgent) ||
				(!relPathEx.isEmpty () && ncSyncUtils::IsRelatedPath (relPathEx, *pathIt, isUrgent))) {
				break;
			}
		}
		if (pathIt == _execPaths.end ())
			break;
	}

	// û�к�������ʱֱ�ӷ��ء�
	if (taskIt == taskMap.end ())
		return false;

	// �ύ�����̳߳���ִ�С�
	NC_SYNCMGR_TRACE (_T("taskId: %u"), taskIt->first);
	ncRunnableTask* task = NC_NEW (syncMgrAlloc, ncRunnableTask) (this, *taskIt);
	_threadPool.start (task);

	// ����_execPaths.
	_execPaths.insert (relPath);
	if (!relPathEx.isEmpty ())
		_execPaths.insert (relPathEx);

	// ����_execMap��taskMap.
	_taskMap[NC_TP_EXECUTING].insert (*taskIt);
	taskMap.erase (taskIt);

	return true;
}

// protected
bool ncSyncScheduler::GetDetail (ncTaskDetailVec& detailVec,
								 const ncSyncTaskMap& taskMap,
								 uint& begin,
								 uint& count)
{
	NC_SYNCMGR_TRACE (_T("begin: %u, count: %u"), begin, count);

	ncTaskDetail detail;

	// �ҵ�����������Ӧ�ĵ�����λ�á�
	ncSyncTaskMap::const_iterator taskIt = taskMap.begin ();
	for (; begin > 0 && taskIt != taskMap.end (); ++taskIt) {
		if (taskIt->second._bVisible)
			--begin;
	}

	// ������λ�ÿ�ʼ��ȡָ���������������顣
	for (; count > 0 && taskIt != taskMap.end (); ++taskIt) {
		if (taskIt->second._bVisible && taskIt->second._task->GetDetail (detail)) {
			detailVec.push_back (make_pair (taskIt->first, detail));
			--count;
		}
	}

	return (count == 0);
}

#include "tasks/ncDownEditFileBaseTask.h"
#include "tasks/ncUpEditFileBaseTask.h"
#include "tasks/ncDirectUploadDirTask.h"
#include "tasks/ncDirectDownloadDirTask.h"

// private
void ncSyncScheduler::CancelTask (ncISyncTask* task)
{
	ncTaskType taskType = task->GetTaskType ();

	switch (taskType) {
		case NC_TT_DOWN_EDIT_FILE:
		case NC_TT_DIRECT_DOWNLOAD_FILE: 
			{
				nsCOMPtr<ncDownEditFileBaseTask> t = reinterpret_cast<ncDownEditFileBaseTask*> (task);
				t->Cancel ();
				break;
			}
		case NC_TT_UP_EDIT_FILE:
		case NC_TT_DIRECT_UPLOAD_FILE:
			{
				nsCOMPtr<ncUpEditFileBaseTask> t = reinterpret_cast<ncUpEditFileBaseTask*> (task);
				t->Cancel ();
				break;
			}
		case NC_TT_DIRECT_UPLOAD_DIR:
			{
				nsCOMPtr<ncDirectUploadDirTask> t = reinterpret_cast<ncDirectUploadDirTask*> (task);
				t->Cancel ();
				break;
			}
		case NC_TT_DIRECT_DOWNLOAD_DIR:
			{
				nsCOMPtr<ncDirectDownloadDirTask> t = reinterpret_cast<ncDirectDownloadDirTask*> (task);
				t->Cancel ();
				break;
			}
		default:
			break;
	}
}

NS_IMETHODIMP_(void) ncSyncScheduler::PauseAll()
{
	AutoLock<ThreadMutexLock> lock (&_taskLock);

	if (_stop)
		return;

	ncSyncTaskMap::iterator taskIt;

	for (int i = 0; i < _mapNum; ++i) {
		for (taskIt = _taskMap [i].begin (); taskIt != _taskMap[i].end (); ++taskIt) {
			// �����ͣ���пɼ�����
			if (taskIt->second._bVisible && !taskIt->second._bHang) {
				taskIt->second._bHang = true;
				taskIt->second._task->Pause ();

				_pauseSet.insert (taskIt->first);

				if (NC_TP_EXECUTING == i)
					++_execPauseNum;
			}
		}
	}
}

NS_IMETHODIMP_(void) ncSyncScheduler::ResumeAll()
{
	AutoLock<ThreadMutexLock> lock (&_taskLock);

	if (_stop)
		return;

	ncSyncTaskMap::iterator taskIt;

	for (int i = 0; i < _mapNum; ++i) {
		for (taskIt = _taskMap [i].begin (); taskIt != _taskMap[i].end ();) {
			// ����ָ����пɼ�����
			if (taskIt->second._bVisible && taskIt->second._bHang) {
				taskIt->second._bHang = false;
				_pauseSet.erase (taskIt->first);
				taskIt->second._task->OnSchedule ();

				if (i == NC_TP_EXECUTING) {
					--_execPauseNum;

					// �ύ������ȡ�
					_taskMap[NC_TP_NORMAL].insert (*taskIt);
					taskIt = _taskMap [i].erase (taskIt);
					continue;
				}
			}

			++ taskIt;
		}
	}
}

NS_IMETHODIMP_(void) ncSyncScheduler::CancelAll()
{
	AutoLock<ThreadMutexLock> lock (&_taskLock);
	nsCOMPtr<ncISyncTask> task = 0;
	bool isExecuting = false;

	ncSyncTaskMap::iterator taskIt;
	bool bInExecMap = false;

	for (int i = 0; i < _mapNum; ++i) {

		for (taskIt = _taskMap [i].begin (); taskIt != _taskMap[i].end (); ) {
			bInExecMap = NC_TP_EXECUTING == i;
			isExecuting = false;

			// ���ȡ�����пɼ�����
			if (taskIt->second._bVisible) {
				task = taskIt->second._task;

				if (taskIt->second._bHang) {
					// �������_pauseSet���Ƴ���
					_pauseSet.erase (taskIt->first);

					if (bInExecMap)
						--_execPauseNum;

				}
				// ����ִ����û�й���
				else if (bInExecMap) {
					isExecuting = true;		// ����ִ��
				}

				// ����
				if (task != 0) {
					// ����ļ��������������쳣����������Ӱ�����
					try {
						CancelTask (task);

						// ����ִ�У��ֶ�����
						if (!isExecuting) {					
							CleanTask (*taskIt, false);
							taskIt = _taskMap[i].erase (taskIt);
							continue;
						}
					}
					catch (Exception& e) {
						SystemLog::getInstance ()->log (ERROR_LOG_TYPE, e.toFullString ().getCStr ());
					}
				}
			}

			++taskIt;
		}
	}
}
