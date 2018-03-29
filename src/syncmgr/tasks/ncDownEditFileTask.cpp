/***************************************************************************************************
Purpose:
	Source file for class ncDownEditFileTask.

Author:
	Chaos

Created Time:
	2016-09-10

***************************************************************************************************/

#include <abprec.h>

#include <Shlobj.h>
#include <docmgr/public/ncIDocMgr.h>
#include <docmgr/public/ncIPathMgr.h>
#include <docmgr/public/ncIFileMgr.h>
#include <docmgr/public/ncIADSMgr.h>
#include <docmgr/public/ncICacheMgr.h>
#include <syncmgr/ncSyncMgr.h>
#include <syncmgr/syncmgr.h>

#include "ncTaskErrorHandler.h"
#include "ncTaskLogger.h"
#include "ncTaskUtils.h"

#include "ncDownEditFileTask.h"

// public ctor
ncDownEditFileTask::ncDownEditFileTask (ncTaskSrc taskSrc,
										const String & localPath,
										const String & docId,
										const String & relPath,
										const String & otag,
										int docType,
										const String & typeName,
										int viewType,
										const String & viewName,
										int64 mtime,
										int64 size,
										int attr,
										bool isNonius,
										int dtType)
										: ncDownEditFileBaseTask (dtType == NC_DT_AUTO ? NC_TT_AUTO_DOWN_FILE : NC_TT_DOWN_EDIT_FILE, taskSrc, localPath,docId, relPath, otag, size, mtime, isNonius)
	, _docType (docType)
	, _typeName (typeName)
	, _viewType (viewType)
	, _viewName (viewName)
	, _attr (attr)
	, _dtType ((ncDownloadType)dtType)
{
	NC_SYNCMGR_TRACE (_T("this: 0x%p, docId: %s, relPath: %s, otag: %s, size: %I64d"),
					this, docId.getCStr (), relPath.getCStr (), otag.getCStr (), size);

	NC_SYNCMGR_CHECK_GNS_INVALID (docId);
	NC_SYNCMGR_CHECK_PATH_INVALID (relPath);

	if (!isNonius) {
		ncDownloadNonius dNonius;
		dNonius._srcPath = relPath;
		dNonius._docId =  dtType == NC_DT_AUTO ? String::EMPTY : docId;		// �Զ���������docId��Ϊ 0
		dNonius._taskSrc = taskSrc;
		dNonius._docType = docType;
		dNonius._typeName = typeName;
		dNonius._viewType = viewType;
		dNonius._viewName = viewName;
		dNonius._mtime = mtime;
		dNonius._size = size;
		dNonius._attr = attr;

		setNoniusInfo(dNonius);
	}
}

// public dtor
ncDownEditFileTask::~ncDownEditFileTask ()
{
	NC_SYNCMGR_TRACE (_T("this: 0x%p"), this);
}

/* [notxpcom] void GetTaskPath (in StringRef relPath, in StringRef relPathEx); */
NS_IMETHODIMP_(void) ncDownEditFileTask::GetTaskPath (String & relPath, String & relPathEx)
{
	relPath.assign (_relPath);

	// ���������ĵ��������·��Ϊ�䶥��Ŀ¼��
	if (_docType > NC_OT_ENTRYDOC)
		relPathEx = ncTaskUtils::GetTopDirPath (_relPath);
	else
		relPathEx.clear ();
}

/* [notxpcom] void OnUnSchedule (); */
NS_IMETHODIMP_(void) ncDownEditFileTask::OnUnSchedule()
{
	NC_SYNCMGR_TRACE (_T("this: 0x%p"), this);

	// ���������ȡ����������¼����
	if (NC_TS_CANCELD == _status) {
		_relayMgr->DeleteDownloadNonius (_relPath);
		ncTaskLogger logger;
		logger.RecordLog (NC_SLT_FAILED, _taskType, _taskSrc, _localPath, _relPath, String::EMPTY, _size, LOAD_STRING (_T("IDS_USER_CANCELLED")));

		// ������ʱ�ļ�
		nsCOMPtr<ncIDocMgr> docMgr = getter_AddRefs (ncSyncMgr::getInstance ()->GetDocMgr ());
		nsCOMPtr<ncIPathMgr> pathMgr = getter_AddRefs (docMgr->GetPathMgr (_localPath));
		File (pathMgr->GetTempPath (pathMgr->GetAbsolutePath (_relPath))).remove ();
	}

	// �����ĵ�״̬���֪ͨ��
	if (_localPath.isEmpty ()) {
		ncSyncMgr::getInstance ()->NotifyDocStateChange ();
	}
	else {
		::SHChangeNotify (SHCNE_UPDATEITEM, SHCNF_PATH, Path::combine(_localPath.beforeLast(_T('\\')), _relPath).getCStr (), NULL);
	}
}

/* [notxpcom] void Execute (); */
NS_IMETHODIMP_(void) ncDownEditFileTask::Execute()
{
	NC_SYNCMGR_TRACE (_T("this: 0x%p"), this);

	ncTaskLogger logger;
	String path, tempPath, bakPath;

	InterlockedExchange ((AtomicValue*)(&_status), NC_TS_EXECUTING);
	try {
		nsCOMPtr<ncIDocMgr> docMgr = getter_AddRefs (ncSyncMgr::getInstance ()->GetDocMgr ());
		nsCOMPtr<ncIPathMgr> pathMgr = getter_AddRefs (docMgr->GetPathMgr (_localPath));
		nsCOMPtr<ncIFileMgr> fileMgr = getter_AddRefs (docMgr->GetFileMgr (_localPath));
		nsCOMPtr<ncIADSMgr> adsMgr = getter_AddRefs (docMgr->GetADSMgr ());
		nsCOMPtr<ncICacheMgr> cacheMgr = getter_AddRefs (docMgr->GetCacheMgr (_localPath));

		path = pathMgr->GetAbsolutePath (_relPath);
		bool isFileExist = SystemFile::isFile (path);

		// ����ļ�·�������������ƣ����׳�·�������쳣��
		if (!isFileExist && pathMgr->IsTooLongPath (path, false))
			NC_SYNCMGR_THROW (LOAD_STRING (_T("IDS_PATH_TOO_LONG")), SYNCMGR_PATH_TOO_LONG);

		ncCacheInfo info, parentInfo;
		String parentRelPath = Path::getDirectoryName (_relPath);
		bool isParentCached = cacheMgr->GetInfoByPath (parentRelPath, parentInfo);
		bool isCached = (cacheMgr->GetInfoByPath (_relPath, info) && (_docId == info._docId));
		bool isEntry = (_docType > NC_OT_ENTRYDOC);

		// �����������ļ����Ҹ�Ŀ¼δ�������ͬ��ǰ��ɾ�����������ȣ������׳����Բ����쳣��
		// �����������ļ����Ҹ�Ŀ¼�����ڣ�ͬ��ǰ��ɾ�����������ȣ������׳����Բ����쳣��
		if (!isEntry) {
			if (!isParentCached)
				NC_SYNCMGR_THROW (LOAD_STRING (_T("IDS_PARENT_NOT_CACHED")), SYNCMGR_OPERATION_IGNORED);

			if (!SystemFile::isDirectory (Path::getDirectoryName (path)))
				NC_SYNCMGR_THROW (LOAD_STRING (_T("IDS_PARENT_NOT_EXIST")), SYNCMGR_OPERATION_IGNORED);
		}

		// ����ļ��ѻ�����Ҿ߱�����Ȩ�ޣ��Ҹ�Ŀ¼Ϊ�ӳ�����״̬������ʱ���ӳ�����Ŀ¼���½����ļ��ȣ������׳����Բ����쳣��
		// ����ļ��ѻ����������Ȩ��û�б�����ұ����ļ������ڣ�ͬ��ǰ��ɾ�����������ȣ������׳����Բ����쳣��
		// ����ļ��ѻ�����Ҿ߱�����Ȩ�ޣ��ұ����ļ��ѱ��޸ģ�ͬ��ǰ���޸ĵȣ������׳����Բ����쳣��
		if (isCached && (_attr & NC_ALLOW_DOWNLOAD)) {
			if (isParentCached && parentInfo._isDelay && !info._isDelay)
				NC_SYNCMGR_THROW (LOAD_STRING (_T("IDS_OBJECT_CACHED")), SYNCMGR_OPERATION_IGNORED);

			if (!isFileExist) {
				if (info._attr & NC_ALLOW_DOWNLOAD)
					NC_SYNCMGR_THROW (LOAD_STRING (_T("IDS_PATH_NOT_EXIST")), SYNCMGR_OPERATION_IGNORED);
			}
			else if (info._lastModified != SystemFile::getLastModified (path) && !info._isDelay) {
				NC_SYNCMGR_THROW (LOAD_STRING (_T("IDS_FILE_CHANGED")), SYNCMGR_OPERATION_IGNORED);
			}
		}

		// ����ļ��Ƿ�ռ��
		if (isFileExist && ncTaskUtils::IsFileOccupied(path, GENERIC_ALL, 30, _status)) {
			THROW_E (_T("Windows System"), 0x00000020L, LOAD_STRING (_T("IDS_DOC_OCCUPIED")).getCStr());
		}

		// ���������ļ���������ͼ��
		if (isEntry) {
			if (docMgr->IsNewView ()) {
				// �Զ�������Ŀ¼����ʾ���
				String topPath = pathMgr->GetAbsolutePath (_relPath.beforeFirst (Path::separator));
				fileMgr->CustomizeVrFolder (topPath, _docType);
				if (!adsMgr->CheckVrTypeInfo (topPath)) {
					adsMgr->SetVrTypeInfo (topPath, _viewType);
				}

				// �Զ�����ڶ���Ŀ¼����ʾ���
				topPath = Path::combine (topPath , _relPath.afterFirst (Path::separator).beforeFirst (Path::separator));
				fileMgr->CustomizeFolder (topPath, _docType, _typeName);
			}
			else {
				// �Զ�����ڶ���Ŀ¼����ʾ���
				String topPath = pathMgr->GetAbsolutePath (_relPath.beforeFirst (Path::separator));
				fileMgr->CustomizeFolder (topPath, _docType, _typeName);
			}
		}

		// ��������Ȩ�޵��ļ�����������ʾ��Ԥ�����ļ�����
		tempPath = pathMgr->GetTempPath (path);
		if (_dNonius._range == 0)
			fileMgr->Delete (tempPath);
		File tempFile (tempPath);


		// ��������ӳ����أ������ڲ���ʽ���ظ��ļ��������������������������Ϣ��
		ncAutoEFSPClientPointer efspClient;
		if (!DownloadFile (efspClient.get (), tempPath)) {
			if (tempFile.exists ())
				tempFile.remove ();
			_relayMgr->DeleteDownloadNonius (_relPath);
			return;
		}
		adsMgr->SetDocIdInfo (tempPath, _docId);
		tempFile.setLastModified (_mtime);
		tempFile.setReadonly (_attr & NC_READ_ONLY);

		if (SystemFile::isFile (path)) {
			bakPath = pathMgr->GetBackupPath (path);
			fileMgr->Delete (bakPath);
			fileMgr->Rename (path, bakPath);
			fileMgr->Rename (tempPath, path);
			fileMgr->Delete (bakPath);
		}
		else {
			fileMgr->Rename (tempPath, path);
		}

		// ���ø�Ŀ¼����޸�ʱ��Ϊ������ʱ��
		if (parentInfo._type != NC_OT_ENTRYDOC) {
			SystemFile::setLastModified (Path::getDirectoryName (path), parentInfo._lastModified);
		}

		// ���±��ػ�����Ϣ�������Ŀ¼������ֵ���������Ƽ̳����丸Ŀ¼����
		info._docId = _docId;
		info._otag = _otag;
		info._type = (isEntry ? _docType : (parentInfo._type & ~NC_OT_ENTRYDOC));
		info._typeName = (isEntry ? _typeName : parentInfo._typeName);
		info._viewType = (isEntry ? _viewType : (parentInfo._viewType));
		info._viewName = (isEntry ? _viewName : parentInfo._viewName);
		info._isDir = false;
		info._isDelay = false;
		info._lastModified = _mtime;
		info._size = _size;
		info._attr = _attr;
		cacheMgr->Update (_relPath, info);

		// ��ɺ�ɾ���ϵ��¼
		_relayMgr->DeleteDownloadNonius (_relPath);

		// ��¼�ɹ���־��
		logger.RecordLog (NC_SLT_SUCCEED, _taskType, _taskSrc, _localPath, _relPath, String::EMPTY, _size);
	}
	catch (Exception& e) {
		// �����쳣ʱɾ���Ѵ�������ʱ�ļ��Ͷϵ�
		// �Զ�������������ϵ�
		if (!ncTaskErrorHandler::SupportRelayError (e) || NC_TT_AUTO_DOWN_FILE == _taskType) {
			if (!tempPath.isEmpty ())
				File (tempPath).remove ();

			_relayMgr->DeleteDownloadNonius (_relPath);
		}

		// ����Ǻ��Բ����쳣�������¼������־��������ж�Ӧ��������¼ʧ����־��
		if (ncTaskErrorHandler::IsIgnoredError (e)) {
			logger.RecordLog (NC_SLT_WARNING, _taskType, _taskSrc, _localPath, _relPath, String::EMPTY, _size, e.toString ());
		}
		else {
			try{
				// ���ļ�bak�ļ����������������Է�ֹ�´�������ʱ��ɨ����ִ�е�����ļ��������ɾ��
				if (!bakPath.isEmpty ()) {
					File (bakPath).rename (path);
				}
			}
			catch(...){}

			ncTaskErrorHandler (e, _taskType, _localPath, path, String::EMPTY, (void*)&_dtType).HandleError ();
			logger.RecordLog (NC_SLT_FAILED, _taskType, _taskSrc, _localPath, _relPath, String::EMPTY, _size, e.toString ());
		}
	}
	if (_status != NC_TS_PAUSED)
		InterlockedExchange ((AtomicValue*)(&_status), NC_TS_DONE);
}
