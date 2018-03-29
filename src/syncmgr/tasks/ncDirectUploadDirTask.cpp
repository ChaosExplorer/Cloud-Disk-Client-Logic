/***************************************************************************************************
Purpose:
	Source file for class ncDirectUploadDirTask.

Author:
	Chaos

Created Time:
	2016-02-02

***************************************************************************************************/

#include <abprec.h>

#include <efshttpclient/public/ncIEFSPClient.h>
#include <docmgr/public/ncIDocMgr.h>
#include <syncmgr/observer/ncDetector.h>
#include <syncmgr/ncSyncMgr.h>
#include <syncmgr/syncmgr.h>

#include "ncTaskErrorHandler.h"
#include "ncTaskLogger.h"
#include "ncTaskUtils.h"

#include "ncDirectUploadDirTask.h"

NC_UMM_IMPL_THREADSAFE_ISUPPORTS1(syncMgrAlloc, ncDirectUploadDirTask, ncISyncTask)

// public ctor
ncDirectUploadDirTask::ncDirectUploadDirTask (ncTaskSrc taskSrc,
											  const String & localPath,
											  const String & srcPath,
											  const String & newRelPath,
											  const String & parentDocId,
											  bool delSrc,
											  int onDup/* = 1*/)
	: _taskType (NC_TT_DIRECT_UPLOAD_DIR)
	, _taskSrc (taskSrc)
	, _localPath (localPath)
	, _srcPath (srcPath)
	, _newRelPath (newRelPath)
	, _logNewRalPath (newRelPath)
	, _parentDocId (parentDocId)
	, _delSrc (delSrc)
	, _onDup (onDup)
	, _efspClient (0)
	, _doneSize (0)
	, _failed (false)
	, _curFileTask (0)
	, _isNonius (false)
	, _onSchCount (0)
	, _ratios (0)
{
	NC_SYNCMGR_TRACE (_T("this: 0x%p, srcPath: %s, newRelPath: %s, parentDocId: %s, delSrc: %d, onDup: %d"),
					  this, srcPath.getCStr (), newRelPath.getCStr (), parentDocId.getCStr (), delSrc, onDup);

	NC_SYNCMGR_CHECK_PATH_INVALID (srcPath);
	NC_SYNCMGR_CHECK_PATH_INVALID (newRelPath);
	NC_SYNCMGR_CHECK_GNS_INVALID (parentDocId);

	nsCOMPtr<ncIDocMgr> docMgr = getter_AddRefs (ncSyncMgr::getInstance ()->GetDocMgr ());
	_relayMgr = getter_AddRefs (docMgr->GetRelayMgr (_localPath));
	_pathMgr = getter_AddRefs (docMgr->GetPathMgr (_localPath));
	_fileMgr = getter_AddRefs (docMgr->GetFileMgr (_localPath));

	// 获取本地目录下所有文件的总大小。
	_totalSize = 0;
	DWORD dirCount = 0, fileCount = 0;
	ncDirUtil::GetDirSize (_srcPath.getCStr (), _totalSize, dirCount, fileCount, TRUE);

	_tsConfig = ncSyncMgr::getInstance ()->getThirdSecurityConfig ();
}

// public dtor
ncDirectUploadDirTask::~ncDirectUploadDirTask ()
{
	NC_SYNCMGR_TRACE (_T("this: 0x%p"), this);
}

/* [notxpcom] ncTaskType GetTaskType (); */
NS_IMETHODIMP_(ncTaskType) ncDirectUploadDirTask::GetTaskType()
{
	return _taskType;
}

/* [notxpcom] void GetTaskPath (in StringRef relPath, in StringRef relPathEx); */
NS_IMETHODIMP_(void) ncDirectUploadDirTask::GetTaskPath(String & relPath, String & relPathEx)
{
	relPath.assign (_logNewRalPath);
	relPathEx.assign (_srcPath);
}

/* [notxpcom] bool GetDetail (in ncTaskDetailRef detail); */
NS_IMETHODIMP_(bool) ncDirectUploadDirTask::GetDetail(ncTaskDetail & detail)
{
	detail._type = _taskType;
	detail._relPath  = _srcPath;
	if (!_localPath.isEmpty ()) {
		detail._relPath = Path::combine (_localPath.beforeLast (_T('\\')), detail._relPath);
	}

	detail._status = _status;
	detail._size = _totalSize;

	if (_status == NC_TS_PAUSED || _status == NC_TS_WAITING) {
		// 暂停中，或者暂停恢复后在排队。此时_efspClient指向的传输可能正在被其他任务使用
		detail._ratios = _ratios;
		detail._rate = 0;
	}
	else if (_efspClient) {
		int64 ultotal, ulnow;
		_efspClient->GetProgress (ultotal, ulnow, detail._rate, true);

		detail._ratios = (_totalSize <= 0 ? 0 : (double) (_doneSize + (_curFileTask ? _curFileTask->_doneSize : 0) + ulnow) / _totalSize);
		if (detail._ratios > 1)
			detail._ratios = 1;
		else if (detail._ratios < _ratios)
			detail._ratios = _ratios;
	}

	return true;
}

/* [notxpcom] void OnSchedule (); */
NS_IMETHODIMP_(void) ncDirectUploadDirTask::OnSchedule()
{
	InterlockedExchange ((AtomicValue*)(&_status), NC_TS_WAITING);

	if (++_onSchCount > 1)
		_isNonius = true;
}

/* [notxpcom] void OnUnSchedule (); */
NS_IMETHODIMP_(void) ncDirectUploadDirTask::OnUnSchedule()
{
	if (NC_TS_CANCELD == _status) {
		ncTaskLogger logger;
		logger.RecordLog (NC_SLT_FAILED, _taskType, _taskSrc, _localPath, _srcPath, _newRelPath, -1, LOAD_STRING (_T("IDS_USER_CANCELLED")));

		if (_curFileTask) {
			_relayMgr->DeleteUploadNonius (_curFileTask->_srcPath);

			String tempPath;
			tempPath = _pathMgr->GetTempPath(_curFileTask->_srcPath);
			if (SystemFile::isFile (tempPath))
				File (tempPath).remove ();
		}
	}

	// 刷新目标位置的变化。
	if (!_failed) {
		ncDetector* detector = ncSyncMgr::getInstance ()->GetDetector (_localPath);
		detector->InlineDetect (_parentDocId, Path::getDirectoryName (_newRelPath), NC_DM_DELAY);
	}
}

/* [notxpcom] void Execute (); */
NS_IMETHODIMP_(void) ncDirectUploadDirTask::Execute()
{
	NC_SYNCMGR_TRACE (_T("this: 0x%p"), this);

	ncAutoEFSPClientPointer efspClient;
	_efspClient = efspClient.get ();

	ncTaskLogger logger;
	String newPath;

	InterlockedExchange ((AtomicValue*)(&_status), NC_TS_EXECUTING);
	try {
		newPath = _pathMgr->GetAbsolutePath (_newRelPath);

		nsCOMPtr<ncIEFSPDirectoryOperation> dirOp = getter_AddRefs (efspClient->CreateDirectoryOperation ());

		// 上传定密系列
		if (_tsConfig.appId == DEFINE_AS_SECURITY_ID) {
			if (_thirdPathCsf.empty()) {
				// 上传之前遍历文件夹，拿到所有原路径和目标路径的对应关系，并将对应关系保存在公共的syncmgr里面，暂时先用内存保存的方式做，后续优化持久化。
				_topPath = _srcPath;
				listCsfDir (dirOp, _thirdPathCsf, _parentDocId, _srcPath, _newRelPath);
				ncSyncMgr::getInstance ()->AddThirdPathMatch (_thirdPathCsf); 
			}
			
			// 上传所有子孙对象，如果中途失败则终止整个上传过程。
			if (!UploadDir (_srcPath)){
				if (_status != NC_TS_PAUSED)
					InterlockedExchange ((AtomicValue*)(&_status), NC_TS_DONE);
				return;
			}
		}
		// 常规系列
		else {
			if (!UploadDir (dirOp, _srcPath, _newRelPath, _parentDocId)){
				if (_status != NC_TS_PAUSED)
					InterlockedExchange ((AtomicValue*)(&_status), NC_TS_DONE);
				return;
			}
		}

		// 上传完后检查并删除本地目录。
		if (_delSrc && NC_TS_EXECUTING == _status) {
			_fileMgr->Delete (_srcPath);
		}
	}
	catch (Exception& e) {
		// 如果是忽略操作异常，则仅记录警告日志，否则进行对应错误处理并记录失败日志。
		if (ncTaskErrorHandler::IsIgnoredError (e)) {
			logger.RecordLog (NC_SLT_WARNING, _taskType, _taskSrc, _localPath, _srcPath, _newRelPath, -1, e.toString ());
		}
		else {
			ncTaskErrorHandler (e, _taskType, _localPath, _srcPath, newPath, &_delSrc).HandleError ();
			logger.RecordLog (NC_SLT_FAILED, _taskType, _taskSrc, _localPath, _srcPath, _newRelPath, -1, e.toString ());

			_failed = true;
		}
	}

	if (_status != NC_TS_PAUSED)
		InterlockedExchange ((AtomicValue*)(&_status), NC_TS_DONE);
}

/* [notxpcom] void Pause (); */
NS_IMETHODIMP_(void) ncDirectUploadDirTask::Pause()
{
	NC_SYNCMGR_TRACE (_T("this: 0x%p"), this);

	InterlockedExchange ((AtomicValue*)(&_status), NC_TS_PAUSED);

	int64 ultotal, ulnow = 0;
	double rate;
	if (_efspClient)
		_efspClient->GetProgress (ultotal, ulnow, rate, true);
	_ratios = (_totalSize == 0 ? 0 : (double) (_doneSize + (_curFileTask ? _curFileTask->_doneSize : 0) + ulnow) / _totalSize);


	// 暂停数据传输。
	if (_efspClient) {
		_efspClient->PauseOperation ();
	}

	if (_curFileTask)
		_curFileTask->Pause ();		// 冗余操作，更改正执行子任务的状态。
}

/* [notxpcom] void Resume (); */
NS_IMETHODIMP_(void) ncDirectUploadDirTask::Resume()
{
	NC_SYNCMGR_TRACE (_T("this: 0x%p"), this);

	InterlockedExchange ((AtomicValue*)(&_status), NC_TS_EXECUTING);

	// 恢复数据传输。
	if (_efspClient) {
		_efspClient->ResumeOperation ();
	}
}

// protected
void ncDirectUploadDirTask::CreateDir (ncIEFSPDirectoryOperation * dirOp, String& parentDocId, String& docId, const String& srcPath, String& newRelPath)
{
	String newPath;
	newPath = _pathMgr->GetAbsolutePath (newRelPath);
	ncTaskLogger logger;
	int onDup = _onDup;

	// 请求服务端创建目录自身
	String otag;
	map<String, String>::iterator doneIt = _donePaths.find (srcPath);
	if (doneIt == _donePaths.end ()) {
		while (true) {
			try {
				dirOp->CreateDirectory (parentDocId, Path::getFileName (srcPath), docId, otag, onDup);
				_donePaths.insert (make_pair (srcPath, docId));
				break;
			}
			catch (Exception& e) {
				// 如果发生同名冲突则进行冲突处理。
				if (ncTaskErrorHandler::GetErrorType (e) != NC_TET_NAME_CONFLICT)
					throw;
				String renamedName = ncTaskErrorHandler (e, _taskType, _localPath, srcPath, newPath, &_delSrc).HandleConflict (onDup);
				ncTaskUtils::TaskStatusCheck(_status);

				// 重命名
				if (onDup == 2)
					newRelPath = Path::combine (Path::getDirectoryName (newRelPath), renamedName);
			}
		}
	}
	else
		docId = doneIt->second;
}

// protected
void ncDirectUploadDirTask::listCsfDir (ncIEFSPDirectoryOperation* dirOp, vector<ncThirdPathCsf>& vecThirdPathCsf, String& parentDocId, const String& srcPath, String& newRelPath)
{
	// 先创建目录本身
	String docId;
	CreateDir (dirOp, parentDocId, docId, srcPath, newRelPath);
	vector<String> subs;
	Directory (srcPath).list(subs);

	vector<String>::iterator it;
	for (it = subs.begin (); it != subs.end (); ++it) {
		ncThirdPathCsf thirdPathCsf;

		// 跳过内部使用的临时文件
		if (_pathMgr->IsTempPath (*it) || _pathMgr->IsBackupPath (*it))
			continue;

		// 文件记录路径
		if (SystemFile::isFile (*it)) {
			thirdPathCsf.srcPath = *it; 
			thirdPathCsf.destPath = Path::combine (newRelPath, Path::getFileName (*it));
			thirdPathCsf.topPath = _topPath;
			thirdPathCsf.parentDocId = docId;
			vecThirdPathCsf.push_back (thirdPathCsf);
		}
		// 文件夹递归处理
		else if (SystemFile::isDirectory (*it)) 
			listCsfDir (dirOp, vecThirdPathCsf, docId, *it, Path::combine (newRelPath, Path::getFileName (*it)));
	}
}

// protected
bool ncDirectUploadDirTask::UploadDir (const String & srcPath,
									   bool isRecurse)
{
	NC_SYNCMGR_TRACE (_T("this: 0x%p, srcPath: %s"),
					  this, srcPath.getCStr ());

	if (NC_TS_EXECUTING != _status)
		return false;

	ncTaskLogger logger;
	String newPath;
	newPath = _pathMgr->GetAbsolutePath (_newRelPath);

	try {
		// 如果是断点续传，先将_curFileTask续传
		if (_isNonius && _curFileTask) {
			_curFileTask->_isNonius = true;
			_curFileTask->_uNonius._onDup = 3;		// 自动覆盖
			_curFileTask->Execute (_efspClient);

			if (!_curFileTask->_failed && NC_TS_EXECUTING == _status) {
				_doneSize += _curFileTask->_totalSize;
				_donePaths.insert (make_pair (_curFileTask->_srcPath, String::EMPTY));
			}
		}

		vector<String> subs;
		Directory (srcPath).list (subs);

		// 如果是子文件则直接上传，如果是子目录则继续递归上传。
		vector<String>::iterator it;
		for (it = subs.begin (); it != subs.end () && NC_TS_EXECUTING == _status; ++it) {
			// 跳过内部使用的临时文件
			if (_pathMgr->IsTempPath (*it) || _pathMgr->IsBackupPath (*it))
				continue;

			if (SystemFile::isFile (*it)) {
				if (_donePaths.find (*it) == _donePaths.end ()) {
					String docId, relPath;
					if (!ncSyncMgr::getInstance ()->GetParentDocIdAndRelPathBySrcPath (*it, docId, relPath)) {
						ncSyncMgr::getInstance ()->DeleteAllSameTopPathBySrcPath (*it, _localPath);
						break;
					}

					nsCOMPtr<ncDirectUploadFileTask> task = NC_NEW (syncMgrAlloc, ncDirectUploadFileTask) ( 
						_taskSrc, _localPath, *it, relPath, docId , _delSrc, false, _onDup);
					_curFileTask = task;

					_relayMgr->UpdateUploadNonius (task->_uNonius);
					task->Execute (_efspClient);

					if (task->_failed || NC_TS_EXECUTING != _status) {
						if (_status == NC_TS_PAUSED) {
							task->_uNonius._csf = task->_csf;
							_relayMgr->UpdateUploadNonius (task->_uNonius);
						}
						else
							ncSyncMgr::getInstance ()->DeleteAllSameTopPathBySrcPath (*it, _localPath);
						break;
					}
					
					_doneSize += task->_totalSize;
					_donePaths.insert (make_pair (*it, String::EMPTY));
					docId = String::EMPTY;
				}
			}
			else if (SystemFile::isDirectory (*it)) {
				if (!UploadDir (*it, true)) {
					ncSyncMgr::getInstance ()->DeleteAllSameTopPathBySrcPath (*it, _localPath);
					break;
				}	
			}
		}

		// 日志只记录顶层
		if (!isRecurse) {
			if (it != subs.end ()) {
				ncTaskUtils::TaskStatusCheck (_status);
				return false;
			}

			// 记录日志。
			logger.RecordLog (NC_SLT_SUCCEED, _taskType, _taskSrc, _localPath, srcPath, _newRelPath);
		}
	}
	catch (Exception& e) {
		// 如果是忽略操作异常，则仅记录警告日志，否则进行对应错误处理并记录失败日志。
		if (ncTaskErrorHandler::IsIgnoredError (e)) {
			logger.RecordLog (NC_SLT_WARNING, _taskType, _taskSrc, _localPath, srcPath, _newRelPath, -1, e.toString ());
		}
		else {
			ncTaskErrorHandler (e, _taskType, _localPath, srcPath, newPath, &_delSrc).HandleError ();
			logger.RecordLog (NC_SLT_FAILED, _taskType, _taskSrc, _localPath, srcPath, _newRelPath, -1, e.toString ());
			return false;
		}
	}

	return true;
}

// protected
bool ncDirectUploadDirTask::UploadDir (ncIEFSPDirectoryOperation * dirOp,
									   const String & srcPath,
									   const String & newRelPath,
									   const String & parentDocId,
									   bool isRecurse)
{
	NC_SYNCMGR_TRACE (_T("this: 0x%p, srcPath: %s, newRelPath: %s, parentDocId: %s"),
		this, srcPath.getCStr (), newRelPath.getCStr (), parentDocId.getCStr ());

	if (NC_TS_EXECUTING != _status)
		return false;

	ncTaskLogger logger;
	String newPath;
	int onDup = _onDup;

	try {
		newPath = _pathMgr->GetAbsolutePath (newRelPath);

		// 请求服务端创建目录自身。
		String docId, otag;
		map<String, String>::iterator doneIt = _donePaths.find (srcPath);
		if (doneIt == _donePaths.end ()) {
			while (true) {
				try {
					dirOp->CreateDirectory (parentDocId, Path::getFileName (srcPath), docId, otag, onDup);
					_donePaths.insert (make_pair (srcPath, docId));
					break;
				}
				catch (Exception& e) {
					// 如果发生同名冲突则进行冲突处理。
					if (ncTaskErrorHandler::GetErrorType (e) != NC_TET_NAME_CONFLICT)
						throw;
					String renamedName = ncTaskErrorHandler (e, _taskType, _localPath, srcPath, newPath, &_delSrc).HandleConflict (onDup);
					ncTaskUtils::TaskStatusCheck(_status);

					// 重命名
					if (onDup == 2)
						_newRelPath = Path::combine (Path::getDirectoryName (_newRelPath), renamedName);
				}
			}
		}
		else
			docId = doneIt->second;

		// 如果是断点续传，先将_curFileTask续传
		if (_isNonius && _curFileTask) {
			_curFileTask->_isNonius = true;
			_curFileTask->_uNonius._onDup = 3;		// 自动覆盖
			_curFileTask->Execute (_efspClient);

			if (!_curFileTask->_failed && NC_TS_EXECUTING == _status) {
				_doneSize += _curFileTask->_totalSize;
				_donePaths.insert (make_pair (_curFileTask->_srcPath, String::EMPTY));
			}
		}

		vector<String> subs;
		Directory (srcPath).list (subs);

		// 如果是子文件则直接上传，如果是子目录则继续递归上传。
		vector<String>::iterator it;
		for (it = subs.begin (); it != subs.end () && NC_TS_EXECUTING == _status; ++it) {

			// 跳过内部使用的临时文件
			if (_pathMgr->IsTempPath (*it) || _pathMgr->IsBackupPath (*it))
				continue;

			if (SystemFile::isFile (*it)) {
				if (_donePaths.find (*it) == _donePaths.end ()) {
					nsCOMPtr<ncDirectUploadFileTask> task = NC_NEW (syncMgrAlloc, ncDirectUploadFileTask) (
						_taskSrc, _localPath, *it, Path::combine (_newRelPath, Path::getFileName (*it)), docId, _delSrc, false, _onDup);
					_curFileTask = task;

					_relayMgr->UpdateUploadNonius (task->_uNonius);
					task->Execute (_efspClient);

					if (task->_failed || NC_TS_EXECUTING != _status)
						break;

					_doneSize += task->_totalSize;
					_donePaths.insert (make_pair (*it, String::EMPTY));
				}
			}
			else if (SystemFile::isDirectory (*it)) {
				if (!UploadDir (dirOp, *it, Path::combine (_newRelPath, Path::getFileName (*it)), docId, true))
					break;
			}
		}

		// 日志只记录顶层
		if (!isRecurse) {
			if (it != subs.end ()) {
				ncTaskUtils::TaskStatusCheck (_status);
				return false;
			}

			// 记录日志。
			logger.RecordLog (NC_SLT_SUCCEED, _taskType, _taskSrc, _localPath, srcPath, _newRelPath);
		}
	}
	catch (Exception& e) {
		// 如果是忽略操作异常，则仅记录警告日志，否则进行对应错误处理并记录失败日志。
		if (ncTaskErrorHandler::IsIgnoredError (e)) {
			logger.RecordLog (NC_SLT_WARNING, _taskType, _taskSrc, _localPath, srcPath, _newRelPath, -1, e.toString ());
		}
		else {
			ncTaskErrorHandler (e, _taskType, _localPath, srcPath, newPath, &_delSrc).HandleError ();
			logger.RecordLog (NC_SLT_FAILED, _taskType, _taskSrc, _localPath, srcPath, _newRelPath, -1, e.toString ());
			return false;
		}
	}

	return true;
}

void ncDirectUploadDirTask::Cancel()
{
	NC_SYNCMGR_TRACE (_T("this: 0x%p"), this);

	// 标记为取消
	InterlockedExchange ((AtomicValue*)(&_status), NC_TS_CANCELD);

	if (_efspClient)
		_efspClient->AbortOperation ();

	if (_curFileTask)
		_curFileTask->Cancel ();
}