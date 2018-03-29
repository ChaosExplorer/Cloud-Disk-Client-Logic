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
		dNonius._docId =  dtType == NC_DT_AUTO ? String::EMPTY : docId;		// 自动下载任务将docId设为 0
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

	// 如果是入口文档，则关联路径为其顶层目录。
	if (_docType > NC_OT_ENTRYDOC)
		relPathEx = ncTaskUtils::GetTopDirPath (_relPath);
	else
		relPathEx.clear ();
}

/* [notxpcom] void OnUnSchedule (); */
NS_IMETHODIMP_(void) ncDownEditFileTask::OnUnSchedule()
{
	NC_SYNCMGR_TRACE (_T("this: 0x%p"), this);

	// 挂起任务的取消，清理、记录处理
	if (NC_TS_CANCELD == _status) {
		_relayMgr->DeleteDownloadNonius (_relPath);
		ncTaskLogger logger;
		logger.RecordLog (NC_SLT_FAILED, _taskType, _taskSrc, _localPath, _relPath, String::EMPTY, _size, LOAD_STRING (_T("IDS_USER_CANCELLED")));

		// 清理临时文件
		nsCOMPtr<ncIDocMgr> docMgr = getter_AddRefs (ncSyncMgr::getInstance ()->GetDocMgr ());
		nsCOMPtr<ncIPathMgr> pathMgr = getter_AddRefs (docMgr->GetPathMgr (_localPath));
		File (pathMgr->GetTempPath (pathMgr->GetAbsolutePath (_relPath))).remove ();
	}

	// 发送文档状态变更通知。
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

		// 如果文件路径超出长度限制，则抛出路径过长异常。
		if (!isFileExist && pathMgr->IsTooLongPath (path, false))
			NC_SYNCMGR_THROW (LOAD_STRING (_T("IDS_PATH_TOO_LONG")), SYNCMGR_PATH_TOO_LONG);

		ncCacheInfo info, parentInfo;
		String parentRelPath = Path::getDirectoryName (_relPath);
		bool isParentCached = cacheMgr->GetInfoByPath (parentRelPath, parentInfo);
		bool isCached = (cacheMgr->GetInfoByPath (_relPath, info) && (_docId == info._docId));
		bool isEntry = (_docType > NC_OT_ENTRYDOC);

		// 如果不是入口文件，且父目录未缓存过（同步前被删除或重命名等），则抛出忽略操作异常。
		// 如果不是入口文件，且父目录不存在（同步前被删除或重命名等），则抛出忽略操作异常。
		if (!isEntry) {
			if (!isParentCached)
				NC_SYNCMGR_THROW (LOAD_STRING (_T("IDS_PARENT_NOT_CACHED")), SYNCMGR_OPERATION_IGNORED);

			if (!SystemFile::isDirectory (Path::getDirectoryName (path)))
				NC_SYNCMGR_THROW (LOAD_STRING (_T("IDS_PARENT_NOT_EXIST")), SYNCMGR_OPERATION_IGNORED);
		}

		// 如果文件已缓存过且具备下载权限，且父目录为延迟下载状态（离线时往延迟下载目录下新建子文件等），则抛出忽略操作异常。
		// 如果文件已缓存过且下载权限没有变更，且本地文件不存在（同步前被删除或重命名等），则抛出忽略操作异常。
		// 如果文件已缓存过且具备下载权限，且本地文件已被修改（同步前被修改等），则抛出忽略操作异常。
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

		// 检查文件是否被占用
		if (isFileExist && ncTaskUtils::IsFileOccupied(path, GENERIC_ALL, 30, _status)) {
			THROW_E (_T("Windows System"), 0x00000020L, LOAD_STRING (_T("IDS_DOC_OCCUPIED")).getCStr());
		}

		// 如果是入口文件，更新视图。
		if (isEntry) {
			if (docMgr->IsNewView ()) {
				// 自定义虚拟目录的显示风格。
				String topPath = pathMgr->GetAbsolutePath (_relPath.beforeFirst (Path::separator));
				fileMgr->CustomizeVrFolder (topPath, _docType);
				if (!adsMgr->CheckVrTypeInfo (topPath)) {
					adsMgr->SetVrTypeInfo (topPath, _viewType);
				}

				// 自定义入口顶级目录的显示风格。
				topPath = Path::combine (topPath , _relPath.afterFirst (Path::separator).beforeFirst (Path::separator));
				fileMgr->CustomizeFolder (topPath, _docType, _typeName);
			}
			else {
				// 自定义入口顶级目录的显示风格。
				String topPath = pathMgr->GetAbsolutePath (_relPath.beforeFirst (Path::separator));
				fileMgr->CustomizeFolder (topPath, _docType, _typeName);
			}
		}

		// 下载所有权限的文件（包括仅显示或预览的文件）。
		tempPath = pathMgr->GetTempPath (path);
		if (_dNonius._range == 0)
			fileMgr->Delete (tempPath);
		File tempFile (tempPath);


		// 如果不是延迟下载，则以内部方式下载该文件，并设置相关数据流和属性信息。
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

		// 设置父目录最后修改时间为服务器时间
		if (parentInfo._type != NC_OT_ENTRYDOC) {
			SystemFile::setLastModified (Path::getDirectoryName (path), parentInfo._lastModified);
		}

		// 更新本地缓存信息（非入口目录的类型值和类型名称继承自其父目录）。
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

		// 完成后删除断点记录
		_relayMgr->DeleteDownloadNonius (_relPath);

		// 记录成功日志。
		logger.RecordLog (NC_SLT_SUCCEED, _taskType, _taskSrc, _localPath, _relPath, String::EMPTY, _size);
	}
	catch (Exception& e) {
		// 发生异常时删除已创建的临时文件和断点
		// 自动下载任务无需断点
		if (!ncTaskErrorHandler::SupportRelayError (e) || NC_TT_AUTO_DOWN_FILE == _taskType) {
			if (!tempPath.isEmpty ())
				File (tempPath).remove ();

			_relayMgr->DeleteDownloadNonius (_relPath);
		}

		// 如果是忽略操作异常，则仅记录警告日志，否则进行对应错误处理并记录失败日志。
		if (ncTaskErrorHandler::IsIgnoredError (e)) {
			logger.RecordLog (NC_SLT_WARNING, _taskType, _taskSrc, _localPath, _relPath, String::EMPTY, _size, e.toString ());
		}
		else {
			try{
				// 将文件bak文件重新命名回来，以防止下次启动的时候，扫描先执行到这个文件会进行误删。
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
