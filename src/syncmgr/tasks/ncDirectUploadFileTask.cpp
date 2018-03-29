/***************************************************************************************************
Purpose:
	Source file for class ncDirectUploadFileTask.

Author:
	Chaos

Created Time:
	2016-02-02

***************************************************************************************************/

#include <abprec.h>
#include <Shlobj.h>
#include <docmgr/public/ncIDocMgr.h>
#include <docmgr/public/ncIPathMgr.h>
#include <syncmgr/observer/ncDetector.h>
#include <syncmgr/ncSyncScheduler.h>
#include <syncmgr/ncSyncMgr.h>
#include <syncmgr/syncmgr.h>

#include "ncTaskErrorHandler.h"
#include "ncTaskLogger.h"
#include "ncTaskUtils.h"

#include "ncDirectUploadFileTask.h"

// public ctor
ncDirectUploadFileTask::ncDirectUploadFileTask (ncTaskSrc taskSrc,
												const String & localPath,
												const String & srcPath,
												const String & newRelPath,
												const String & parentDocId,
												bool delSrc,
												bool isNonius,
												int onDup/* = 1*/)
												: ncUpEditFileBaseTask (NC_TT_DIRECT_UPLOAD_FILE, taskSrc, localPath, srcPath, srcPath, onDup, isNonius)
	, _newRelPath (newRelPath)
	, _parentDocId (parentDocId)
	, _failed (false)
	, _logNewRelPath (newRelPath)
{
	NC_SYNCMGR_TRACE (_T("this: 0x%p, srcPath: %s, newRelPath: %s, parentDocId: %s, delSrc: %d, onDup: %d"),
					  this, srcPath.getCStr (), newRelPath.getCStr (), parentDocId.getCStr (), delSrc, onDup);

	NC_SYNCMGR_CHECK_PATH_INVALID (srcPath);
	NC_SYNCMGR_CHECK_PATH_INVALID (newRelPath);
	NC_SYNCMGR_CHECK_GNS_INVALID (parentDocId);

	if (!_isNonius) {
		ncUploadNonius uNonius;
		uNonius._taskSrc = taskSrc;
		uNonius._srcPath = srcPath;
		uNonius._destPath = Path::getDirectoryName (newRelPath);
		uNonius._onDup = onDup;
		uNonius._direcMode = true;
		uNonius._delSrc = delSrc;

		setNoniusInfo(uNonius);
	}

	_tsConfig = ncSyncMgr::getInstance ()->getThirdSecurityConfig ();

	// 上传之前检查一下密级对照表里有没有这个任务，如果有说明这个任务是上传文件夹时的子任务，如果没有说明是单独的上传文件任务
	CheckPathInVec ();
}

// public dtor
ncDirectUploadFileTask::~ncDirectUploadFileTask ()
{
	NC_SYNCMGR_TRACE (_T("this: 0x%p"), this);
}

/* [notxpcom] void GetTaskPath (in StringRef relPath, in StringRef relPathEx); */
NS_IMETHODIMP_(void) ncDirectUploadFileTask::GetTaskPath(String & relPath, String & relPathEx)
{
	relPath.assign (_newRelPath);
	relPathEx.assign (_srcPath);
}

/* [notxpcom] void OnUnSchedule (); */
NS_IMETHODIMP_(void) ncDirectUploadFileTask::OnUnSchedule()
{
	NC_SYNCMGR_TRACE (_T("this: 0x%p"), this);

	// 挂起任务的取消，清理、记录处理
	if (NC_TS_CANCELD == _status) {
		_relayMgr->DeleteUploadNonius (_srcPath);
		ncTaskLogger logger;
		logger.RecordLog (NC_SLT_FAILED, _taskType, _taskSrc, _localPath, _srcPath, _logNewRelPath, _totalSize, LOAD_STRING (_T("IDS_USER_CANCELLED")));

		// 清理可能存在的临时文件
		String tempPath;
		{
			nsCOMPtr<ncIDocMgr> docMgr = getter_AddRefs (ncSyncMgr::getInstance ()->GetDocMgr ());
			nsCOMPtr<ncIPathMgr> pathMgr = getter_AddRefs (docMgr->GetPathMgr (_localPath));
			tempPath = pathMgr->GetTempPath(_srcPath);
		}
		if (SystemFile::isFile (tempPath))
			File (tempPath).remove ();
	}
	// 上传正常进行完
	else if (!_failed) {
		if (SystemFile::isFile (_srcPath)) {
			// 如果文件在上传期间被修改，则重新上传该文件。
			// 如果文件在上传期间被独占，则等待一段时间后再重试上传。
			if (IsFileModified ()) {
				ReUploadFile ();
			}
			else if (_isUsing) {
				Thread::sleep (1000);
				ReUploadFile ();
			}
		}

		// 刷新目标位置的变化。
		ncDetector* detector = ncSyncMgr::getInstance ()->GetDetector (_localPath);
		detector->InlineDetect (_parentDocId, Path::getDirectoryName (_newRelPath), NC_DM_DELAY);

		// 发送文档状态变更通知。
		if (_localPath.isEmpty ()) {
			ncSyncMgr::getInstance ()->NotifyDocStateChange ();
		}
		else {
			::SHChangeNotify (SHCNE_UPDATEITEM, SHCNF_PATH, Path::combine(_localPath.beforeLast(_T('\\')), _relPath).getCStr (), NULL);
		}
	}
}

/* [notxpcom] void Execute (); */
NS_IMETHODIMP_(void) ncDirectUploadFileTask::Execute()
{
	NC_SYNCMGR_TRACE (_T("this: 0x%p"), this);

	ncAutoEFSPClientPointer efspClient;
	Execute (efspClient.get ());
}

// private
void ncDirectUploadFileTask::Execute (ncIEFSPClient * efspClient)
{
	NC_SYNCMGR_TRACE (_T("this: 0x%p, efspClient: 0x%p"), this, efspClient);

	ncTaskLogger logger;
	String newPath;

	InterlockedExchange ((AtomicValue*)(&_status), NC_TS_EXECUTING);
	try {
		nsCOMPtr<ncIDocMgr> docMgr = getter_AddRefs (ncSyncMgr::getInstance ()->GetDocMgr ());
		nsCOMPtr<ncIPathMgr> pathMgr = getter_AddRefs (docMgr->GetPathMgr (_localPath));
		newPath = pathMgr->GetAbsolutePath (_newRelPath);

		String docId = _parentDocId;
		String docName = Path::getFileName (_srcPath);
		String otag;

		// 上传文件到云端。
		bool isDupload = false;
		if (!UploadFile (efspClient, docId, docName, otag, isDupload)) {
			ncSyncMgr::getInstance ()->DeleteAllSameTopPathBySrcPath (_srcPath, _localPath);
			return;
		}

		// 检查并删除本地数据源。
		if (_uNonius._delSrc) {
			SystemFile::setFileAttrs (_srcPath, NORMAL);
			File (_srcPath).remove ();
		}

		// 完成后删除断点记录
		_relayMgr->DeleteUploadNonius (_srcPath);

		// 记录成功日志。
		ncTaskType taskType = (isDupload ? NC_TT_UP_EDIT_DUP_FILE : _taskType);
		logger.RecordLog (NC_SLT_SUCCEED, taskType, _taskSrc, _localPath, _srcPath, _logNewRelPath, _totalSize);
	}
	catch (Exception& e) {
		// 发生异常时，删除对应的路径关系
		if (_status != NC_TS_PAUSED && _status != NC_TS_CANCELD) 
			ncSyncMgr::getInstance ()->DeleteAllSameTopPathBySrcPath (_srcPath, _localPath);
		
		ncTaskErrorType errType = ncTaskErrorHandler::GetErrorType(e);
		// 发生异常时删除断点
		if (!ncTaskErrorHandler::SupportRelayError (errType))
			_relayMgr->DeleteUploadNonius (_srcPath);

		// 如果是忽略操作异常，则仅记录警告日志，否则进行对应错误处理并记录失败日志。
		if (ncTaskErrorHandler::IsIgnoredError (e)) {
			logger.RecordLog (NC_SLT_WARNING, _taskType, _taskSrc, _localPath, _srcPath, _logNewRelPath, _totalSize, e.toString ());
		}
		else {
			ncTaskErrorHandler (e, _taskType, _localPath, _srcPath, newPath, &_uNonius._delSrc).HandleError ();
			logger.RecordLog (NC_SLT_FAILED, _taskType, _taskSrc, _localPath, _srcPath, _logNewRelPath, _totalSize, e.toString ());

			if (NC_TET_OPERATION_CANCELLED != errType)
				_failed = true;
		}
	}
	if (_status != NC_TS_PAUSED)
		InterlockedExchange ((AtomicValue*)(&_status), NC_TS_DONE);
}

// private
void ncDirectUploadFileTask::CheckPathInVec ()
{
	if (_tsConfig.appId != DEFINE_AS_SECURITY_ID)
		return;

	if (SystemFile::isFile (_srcPath)) {
		if (!ncSyncMgr::getInstance ()->isInBySrcPath (_srcPath, _localPath) || !ncSyncMgr::getInstance ()->isInByRelPath (_newRelPath)) {
			ncThirdPathCsf fileThirdPathCsf;
			fileThirdPathCsf.srcPath = _srcPath;
			fileThirdPathCsf.destPath = _newRelPath;
			ncSyncMgr::getInstance ()->AddThirdPathMatch (fileThirdPathCsf);
		}
	}
}

// private
void ncDirectUploadFileTask::HandleConflict (const Exception& e)
{
	NC_SYNCMGR_TRACE (_T("this: 0x%p"), this);

	nsCOMPtr<ncIDocMgr> docMgr = getter_AddRefs (ncSyncMgr::getInstance ()->GetDocMgr ());
	nsCOMPtr<ncIPathMgr> pathMgr = getter_AddRefs (docMgr->GetPathMgr (_localPath));

	String newPath = pathMgr->GetAbsolutePath (_newRelPath);
	String renamedName = ncTaskErrorHandler (e, _taskType, _localPath, newPath, newPath, &_uNonius._delSrc).HandleConflict (_uNonius._onDup);

	ncTaskUtils::TaskStatusCheck (_status);
	
	// 重命名
	if (_uNonius._onDup == 2)
		_logNewRelPath = Path::combine (Path::getDirectoryName (_logNewRelPath), renamedName);
}

// private
void ncDirectUploadFileTask::ReUploadFile ()
{
	NC_SYNCMGR_TRACE (_T("this: 0x%p"), this);

	nsCOMPtr<ncSyncScheduler> scheduler = getter_AddRefs (
		reinterpret_cast<ncSyncScheduler*> (ncSyncMgr::getInstance ()->GetScheduler ()));

	nsCOMPtr<ncISyncTask> task = NC_NEW (syncMgrAlloc, ncDirectUploadFileTask) (
		_taskSrc, _localPath, _srcPath, _newRelPath, _parentDocId, _uNonius._delSrc, _uNonius._onDup);
	scheduler->AddTask (task, ncSyncScheduler::NC_TP_NORMAL);
}
