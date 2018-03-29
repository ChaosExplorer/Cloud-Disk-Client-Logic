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

	// �ϴ�֮ǰ���һ���ܼ����ձ�����û��������������˵������������ϴ��ļ���ʱ�����������û��˵���ǵ������ϴ��ļ�����
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

	// ���������ȡ����������¼����
	if (NC_TS_CANCELD == _status) {
		_relayMgr->DeleteUploadNonius (_srcPath);
		ncTaskLogger logger;
		logger.RecordLog (NC_SLT_FAILED, _taskType, _taskSrc, _localPath, _srcPath, _logNewRelPath, _totalSize, LOAD_STRING (_T("IDS_USER_CANCELLED")));

		// ������ܴ��ڵ���ʱ�ļ�
		String tempPath;
		{
			nsCOMPtr<ncIDocMgr> docMgr = getter_AddRefs (ncSyncMgr::getInstance ()->GetDocMgr ());
			nsCOMPtr<ncIPathMgr> pathMgr = getter_AddRefs (docMgr->GetPathMgr (_localPath));
			tempPath = pathMgr->GetTempPath(_srcPath);
		}
		if (SystemFile::isFile (tempPath))
			File (tempPath).remove ();
	}
	// �ϴ�����������
	else if (!_failed) {
		if (SystemFile::isFile (_srcPath)) {
			// ����ļ����ϴ��ڼ䱻�޸ģ��������ϴ����ļ���
			// ����ļ����ϴ��ڼ䱻��ռ����ȴ�һ��ʱ����������ϴ���
			if (IsFileModified ()) {
				ReUploadFile ();
			}
			else if (_isUsing) {
				Thread::sleep (1000);
				ReUploadFile ();
			}
		}

		// ˢ��Ŀ��λ�õı仯��
		ncDetector* detector = ncSyncMgr::getInstance ()->GetDetector (_localPath);
		detector->InlineDetect (_parentDocId, Path::getDirectoryName (_newRelPath), NC_DM_DELAY);

		// �����ĵ�״̬���֪ͨ��
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

		// �ϴ��ļ����ƶˡ�
		bool isDupload = false;
		if (!UploadFile (efspClient, docId, docName, otag, isDupload)) {
			ncSyncMgr::getInstance ()->DeleteAllSameTopPathBySrcPath (_srcPath, _localPath);
			return;
		}

		// ��鲢ɾ����������Դ��
		if (_uNonius._delSrc) {
			SystemFile::setFileAttrs (_srcPath, NORMAL);
			File (_srcPath).remove ();
		}

		// ��ɺ�ɾ���ϵ��¼
		_relayMgr->DeleteUploadNonius (_srcPath);

		// ��¼�ɹ���־��
		ncTaskType taskType = (isDupload ? NC_TT_UP_EDIT_DUP_FILE : _taskType);
		logger.RecordLog (NC_SLT_SUCCEED, taskType, _taskSrc, _localPath, _srcPath, _logNewRelPath, _totalSize);
	}
	catch (Exception& e) {
		// �����쳣ʱ��ɾ����Ӧ��·����ϵ
		if (_status != NC_TS_PAUSED && _status != NC_TS_CANCELD) 
			ncSyncMgr::getInstance ()->DeleteAllSameTopPathBySrcPath (_srcPath, _localPath);
		
		ncTaskErrorType errType = ncTaskErrorHandler::GetErrorType(e);
		// �����쳣ʱɾ���ϵ�
		if (!ncTaskErrorHandler::SupportRelayError (errType))
			_relayMgr->DeleteUploadNonius (_srcPath);

		// ����Ǻ��Բ����쳣�������¼������־��������ж�Ӧ��������¼ʧ����־��
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
	
	// ������
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
