/***************************************************************************************************
Purpose:
	Source file for class ncIOScanner.

Author:
	Chaos

Created Time:
	2016-8-19

***************************************************************************************************/

#include <abprec.h>
#include <docmgr/public/ncIPathMgr.h>
#include <docmgr/public/ncIADSMgr.h>
#include <syncmgr/private/ncIChangeListener.h>
#include <syncmgr/syncmgr.h>
#include "ncIOScanner.h"
#include "ncSyncScheduler.h"

#include "tasks/ncDownEditFileTask.h"
#include "tasks/ncDirectDownloadFileTask.h"
#include "tasks/ncUpEditFileTask.h"
#include "tasks/ncDirectUploadFileTask.h"


// public ctor
ncIOScanner::ncIOScanner (ncIDocMgr* docMgr,
						  ncSyncScheduler* scheduler,
						  ncIChangeListener* chgListener,
						  String localSyncPath)
	: _stop (true)
	, _inlCnt (0)
	, _docMgr (docMgr)
	, _scheduler (scheduler)
	, _chgListener (chgListener)
	, _localSyncPath (localSyncPath)
	, _taskSrc (NC_TS_SCAN)
{
	NC_SYNCMGR_TRACE (_T("docMgr: 0x%p, chgListener: 0x%p"),
					  docMgr, chgListener);

	NC_SYNCMGR_CHECK_ARGUMENT_NULL (docMgr);
	NC_SYNCMGR_CHECK_ARGUMENT_NULL (chgListener);

	_pathMgr = _docMgr->GetPathMgr (_localSyncPath);
	_adsMgr = _docMgr->GetADSMgr ();
	_cacheMgr = _docMgr->GetCacheMgr (_localSyncPath);
	_relayMgr = _docMgr->GetRelayMgr (_localSyncPath);
	_newView = _docMgr->IsNewView ();
}

// pubilc dtor
ncIOScanner::~ncIOScanner ()
{
	NC_SYNCMGR_TRACE (_T("this: 0x%p"), this);

	if (!_stop)
		Stop ();

	// �ȴ���������ɨ�������
	while (_inlCnt > 0)
		Sleep (100);
}

// public
void ncIOScanner::Start ()
{
	NC_SYNCMGR_TRACE (_T("this: 0x%p"), this);

	InterlockedExchange (&_stop, false);

	this->start ();
}

// public
void ncIOScanner::Stop ()
{
	NC_SYNCMGR_TRACE (_T("this: 0x%p"), this);

	InterlockedExchange (&_stop, true);
}

// public
void ncIOScanner::InlineScan (const String& path)
{
	NC_SYNCMGR_TRACE (_T("path: %s, -begin"), path.getCStr ());
	NC_SYNCMGR_CHECK_PATH_INVALID (path);

	InterlockedIncrement (&_inlCnt);

	try {
		// �ڵ������߳���ɨ��ָ��Ŀ¼��
		ScanDir (path);
	}
	catch (Exception& e) {
		SystemLog::getInstance ()->log (ERROR_LOG_TYPE, e.toFullString ().getCStr ());
	}
	catch (...) {
		SystemLog::getInstance ()->log (ERROR_LOG_TYPE, _T("ncIOScanner::InlineScan () failed."));
	}

	InterlockedDecrement (&_inlCnt);

	NC_SYNCMGR_TRACE (_T("path: %s, -end"), path.getCStr ());
}

// pubilc
void ncIOScanner::run ()
{
	NC_SYNCMGR_TRACE (_T("this: 0x%p, -begin"), this);

	try {
		// Nonius Loading
		ScanNonius ();

		// �Ӹ�Ŀ¼��ʼɨ�衣
		ScanDir (_pathMgr->GetRootPath ());

		ScanUnsync ();
	}
	catch (Exception& e) {
		SystemLog::getInstance ()->log (ERROR_LOG_TYPE, e.toFullString ().getCStr ());
	}
	catch (...) {
		SystemLog::getInstance ()->log (ERROR_LOG_TYPE, _T("ncIOScanner::run () failed."));
	}

	NC_SYNCMGR_TRACE (_T("this: 0x%p, -end"), this);
}

// protected
void ncIOScanner::ScanDir (const String& path)
{
	NC_SYNCMGR_TRACE (_T("path: %s"), path.getCStr ());

	if (_stop)
		return;

	set<String> modFiles, addDirs, rmvFiles, rmvDirs, orgnDirs;
	map<String, String> rnmFiles, rnmDirs;
	String subPath, newPath;

	// ע�⣺���ڱ���ɨ�������ִ���ǲ��еģ�������ִ�л�Ӱ�쵽ɨ��ʱ��ȡ��
	// ���ݿ���Ϣ�ͱ�����Ϣ����ȷ�ԣ����ڱ���ɨ��Ӧ�Ա���ʵ�����Ϊ׼������
	// �����Ȼ�ȡ���ݿ���Ϣ���ٻ�ȡ������Ϣ���Ա�֤������Ϣ������ʵ�����

	// ��ȡ���ݿ��е����ĵ���Ϣ��
	ScanInfoMap cacheMap;
	String realPath = _pathMgr->GetRelativePath (path);
	_cacheMgr->GetScanSubInfos (realPath, cacheMap);

	// ��ȡ���ص����ĵ���Ϣ��û��docId���������ĵ����������ġ�
	ScanInfoMap localMap;
	if (!GetLocalSubInfos (path, localMap, modFiles, addDirs, orgnDirs))
		return;

	// ����ͼ��Ŀ¼�£������ļ�������ɾ���ļ��
	if (_newView && realPath.isEmpty ()) {
		vector<String> views;
		_cacheMgr->GetCacheViews (views);
		set<String>::iterator it;
		String viewPath;
		for (int i = 0; i < views.size (); i++) {
			viewPath = Path::combine (path, views[i]);
			it = orgnDirs.begin ();
			while (it != orgnDirs.end ()) {
				if (viewPath == *it) {
					break;
				}
				it++;
			}

			if (it == orgnDirs.end ()) {
				rmvDirs.insert (viewPath);
			}
		}
	}

	ScanInfoMap::iterator localIt = localMap.begin ();
	ScanInfoMap::iterator cacheIt = cacheMap.begin ();

	while (localIt != localMap.end () && cacheIt != cacheMap.end ()) {
		if (localIt->first < cacheIt->first) {
			// ��������ж����ݿ���û�У���˵�����ĵ��������ġ�
			subPath = Path::combine (path, localIt->second._name);
			localIt->second._isDir ? addDirs.insert (subPath) : modFiles.insert (subPath);
			++localIt;
		}
		else if (localIt->first > cacheIt->first) {
			// �������û�ж����ݿ����У���˵�����ĵ��ѱ�ɾ����
			subPath = Path::combine (path, cacheIt->second._name);
			if (cacheIt->second._isDir) {
				if (addDirs.find (subPath) == addDirs.end ())
					rmvDirs.insert (subPath);
			}
			else {
				if (modFiles.find (subPath) == modFiles.end ())
					rmvFiles.insert (subPath);
			}
			++cacheIt;
		}
		else {
			if (localIt->second._isDir) {
				if (localIt->second._name != cacheIt->second._name) {
					// ����������������ݿ��в�һ�£���˵����Ŀ¼�ѱ���������
					subPath = Path::combine (path, cacheIt->second._name);
					newPath = Path::combine (path, localIt->second._name);
					rnmDirs.insert (make_pair (subPath, newPath));
				}
				else {
					// ��Ŀ¼��ӵ�orgnDirs���Ա����ɨ�衣
					subPath = Path::combine (path, localIt->second._name);
					orgnDirs.insert (subPath);
				}
			}
			else {
				if (localIt->second._name != cacheIt->second._name) {
					// ����������������ݿ��в�һ�£���˵�����ļ��ѱ���������
					subPath = Path::combine (path, cacheIt->second._name);
					newPath = Path::combine (path, localIt->second._name);
					rnmFiles.insert (make_pair (subPath, newPath));
				}

				if (localIt->second._lastModified != cacheIt->second._lastModified) {
					// ��������޸�ʱ�������ݿ��в�һ�£���˵�����ļ��ѱ��޸ġ�
					subPath = Path::combine (path, localIt->second._name);
					modFiles.insert (subPath);
				}
			}
			++localIt;
			++cacheIt;
		}
	}

	// ��������ж����ݿ���û�У���˵�����ĵ��������ġ�
	for (; localIt != localMap.end (); ++localIt) {
		subPath = Path::combine (path, localIt->second._name);
		localIt->second._isDir ? addDirs.insert (subPath) : modFiles.insert (subPath);
	}

	// �������û�ж����ݿ����У���˵�����ĵ��ѱ�ɾ����
	for (; cacheIt != cacheMap.end (); ++cacheIt) {
		subPath = Path::combine (path, cacheIt->second._name);
		if (cacheIt->second._isDir) {
			if (addDirs.find (subPath) == addDirs.end ())
				rmvDirs.insert (subPath);
		}
		else {
			if (modFiles.find (subPath) == modFiles.end ())
				rmvFiles.insert (subPath);
		}
	}

	// �ȴ���ɾ�����ĵ���
	set<String>::iterator iter;
	for (iter = rmvFiles.begin (); iter != rmvFiles.end (); ++iter)
		_chgListener->OnLocalFileDeleted (_taskSrc, *iter, _localSyncPath);

	for (iter = rmvDirs.begin (); iter != rmvDirs.end (); ++iter)
		_chgListener->OnLocalDirDeleted (_taskSrc, *iter, _localSyncPath);

	// �ٴ������������ĵ���
	map<String, String>::iterator rnmIt;
	for (rnmIt = rnmFiles.begin (); rnmIt != rnmFiles.end (); ++rnmIt)
		_chgListener->OnLocalFileRenamed (_taskSrc, rnmIt->first, rnmIt->second, _localSyncPath);

	for (rnmIt = rnmDirs.begin (); rnmIt != rnmDirs.end (); ++rnmIt)
		_chgListener->OnLocalDirRenamed (_taskSrc, rnmIt->first, rnmIt->second, _localSyncPath);

	// ������������޸ĵ��ĵ���
	for (iter = modFiles.begin (); iter != modFiles.end (); ++iter) {
		// �ж��Ƿ��жϵ����
		if (!_relayMgr->HasUploadNoniusWithPath(*iter))
			_chgListener->OnLocalFileEdited (_taskSrc, *iter, _localSyncPath);
	}

	for (iter = addDirs.begin (); iter != addDirs.end (); ++iter)
		_chgListener->OnLocalDirCreated (_taskSrc, *iter, _localSyncPath);

// 	// ����ɨ����������Ŀ¼��
// 	for (iter = addDirs.begin (); iter != addDirs.end (); ++iter)
// 		ScanDir (*iter);

	// ����ɨ��ԭ�е���Ŀ¼��
	for (iter = orgnDirs.begin (); iter != orgnDirs.end (); ++iter)
		ScanDir (*iter);
}

// protected
bool ncIOScanner::GetLocalSubInfos (const String& path,
									ScanInfoMap& docIdInfos,
									set<String>& noDocIdFiles,
									set<String>& noDocIdDirs,
									set<String>& orgnDirs)
{
	NC_SYNCMGR_TRACE (_T("path: %s"), path.getCStr ());

	WIN32_FIND_DATA fd;
	HANDLE hFind = FindFirstFile ((path + _T("\\*")).getCStr (), &fd);

	if (hFind == INVALID_HANDLE_VALUE) {
		// ���ʧ��ԭ��ΪĿ¼�����ڣ���ֱ�ӷ��ء�
		DWORD dwError = GetLastError ();
		if (dwError == ERROR_PATH_NOT_FOUND)
			return false;

		// ���������ʧ��ԭ�����׳��쳣��
		String errMsg;
		errMsg.format (syncMgrLoader, _T("IDS_SCAN_LIST_DIR_ERR"), path.getCStr ());
		NC_SYNCMGR_THROW_WSC (errMsg, SYNCMGR_SCAN_DIR_ERR, dwError);
	}

	bool isDir;
	String docPath, docId;
	ncScanInfo scanInfo;

	do {
		if (ncDirUtil::IsDotsDir (fd.cFileName))
			continue;

		isDir = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
		docPath = Path::combine (path, fd.cFileName);

		if (!_adsMgr->GetDocIdInfo (docPath, docId)) {
			// ���˲�ͬ�����ڲ����ݿ��ĵ����ڲ���ͻ�ĵ��������ļ��С�
			if (_pathMgr->IsInDBPath (docPath, true) || _pathMgr->IsInConflictPath (docPath, true))
				continue;

			// ���������ļ�����ӵ�ԭ���ļ�����
			// û��docId���������ĵ���ӵ�noDocIdFiles��noDocIdDirs�С�
			if (_adsMgr->CheckVrTypeInfo(docPath)) {
				orgnDirs.insert (docPath);
			}
			else {
				isDir ? noDocIdDirs.insert (docPath) : noDocIdFiles.insert (docPath);
			}
		}
		else {
			// Ŀ¼������޸�ʱ��û��ʵ�����壬���Ĭ��Ϊ-1.
			scanInfo._name = fd.cFileName;
			scanInfo._isDir = isDir;
			scanInfo._lastModified = (isDir ? -1 : windowsTimeToUnixTime (&fd.ftLastWriteTime));

			// ���docId�������ظ������docIdInfos���Ƴ�������ӵ�noDocIdFiles��noDocIdDirs�С�
			if (!docIdInfos.insert (make_pair (docId, scanInfo)).second) {
				docIdInfos.erase (docId);
				isDir ? noDocIdDirs.insert (docPath) : noDocIdFiles.insert (docPath);
			}
			else
				ReviseDelayADS (docPath);		// �����ļ����У��isDelay�����������ݿ��һ����
		}
	} while (FindNextFile (hFind, &fd));

	FindClose(hFind);
	return true;
}


void ncIOScanner::ReviseDelayADS(const String& path)
{
	try {
		String relPath = _pathMgr->GetRelativePath (path);
		ncCacheInfo info;
		if (_cacheMgr->GetInfoByPath (relPath, info) && info._type != NC_OT_ENTRYDOC && info._isDelay != _adsMgr->CheckDelayInfo (path)) {
			
			if (info._isDir) {
				if (info._isDelay)
					_adsMgr->SetDelayInfo (path, -1);		// ��Ϊ��Ҫ���⣬isDelay��������ʧ��ɱ������Ȩ�������ԭ��
				else
					_adsMgr->DeleteDelayInfo (path);		// ���������ܷ���
			}
			else {
				if (info._isDelay) {
					_adsMgr->SetDelayInfo (path, info._size);		// ͬ��

					// �����ݿ�Ϊ׼��δ�����ļ�������򿪣�Ҳ�����ϴ��޸�.
					SystemFile::setLastModified (path, info._lastModified);			// ��������ϴ������޸�ʱ��δ�䲻���ϴ�
				}
				else {
					int64 lastModify = SystemFile::getLastModified (path);
					_adsMgr->DeleteDelayInfo (path);									// ����������
					SystemFile::setLastModified (path, lastModify);
				}
			}
		}
	}
	catch (Exception& e) {
		SystemLog::getInstance ()->log (ERROR_LOG_TYPE, e.toFullString ().getCStr ());
	}
	catch (...) {
		SystemLog::getInstance ()->log (ERROR_LOG_TYPE, _T("ReviseDelayADS failed."));
	}
}

void ncIOScanner::ScanNonius()
{
	{
		// load upload noniuses
		uploadNoniusVec uNoniusVec;
		_relayMgr->GetUploadNoniusVec (uNoniusVec, true);

		uploadNoniusVec::iterator iter = uNoniusVec.begin ();
		for (; iter != uNoniusVec.end (); ++ iter)
		{
			String tempPath = _pathMgr->GetTempPath (iter->_srcPath);
			if (File(iter->_srcPath).exists ()) {
				if (iter->_blockSerial != 0 && (iter->_lastModified != SystemFile::getLastModified(iter->_srcPath) || !File(tempPath).exists ())) {
					iter->_blockSerial = 0;
				}
			}
			else {
				_relayMgr->DeleteUploadNonius (iter->_srcPath);
				
				File tempFile(tempPath);
				if (tempFile.exists ())
					tempFile.remove ();

				continue;
			}
			
			ncTaskStatus status = (ncTaskStatus)iter->_status;
			int taskId = 0;

			if (iter->_direcMode) {
				String newRelPath = Path::combine (iter->_destPath, Path::getFileName(iter->_srcPath));
				
				ncCacheInfo info;
				if (!_cacheMgr->GetInfoByPath(iter->_destPath, info)) {
					_relayMgr->DeleteUploadNonius (iter->_srcPath);
					continue;
				}
				
				 ncDirectUploadFileTask* oriTask = NC_NEW (syncMgrAlloc, ncDirectUploadFileTask) (
					(ncTaskSrc)iter->_taskSrc, _localSyncPath, iter->_srcPath, newRelPath, info._docId, iter->_delSrc, true, iter->_onDup);
				
				oriTask->setNoniusInfo(*iter);

				nsCOMPtr<ncISyncTask> task = oriTask;
				taskId = _scheduler->AddTask(task, NC_TS_EXECUTING == status ? ncSyncScheduler::NC_TP_PRIOR : ncSyncScheduler::NC_TP_LOW);
			}
			else {
				ncUpEditFileTask* oriTask = NC_NEW (syncMgrAlloc, ncUpEditFileTask) (
					(ncTaskSrc)iter->_taskSrc, _localSyncPath, iter->_srcPath, true, iter->_onDup);

				oriTask->setNoniusInfo(*iter);

				nsCOMPtr<ncISyncTask> task = oriTask;
				int taskId = _scheduler->AddTask(task, NC_TS_EXECUTING == status ? ncSyncScheduler::NC_TP_PRIOR : ncSyncScheduler::NC_TP_LOW);
			}
		}
	}

	
	// load download noniuses
	downloadNoniusVec dNoNiusVec;
	_relayMgr->GetDownloadNoniusVec (dNoNiusVec, true);

	downloadNoniusVec::iterator iter = dNoNiusVec.begin ();
	for (; iter != dNoNiusVec.end (); ++ iter)
	{
		ncCacheInfo info;
		bool isCached = _cacheMgr->GetInfoByPath(iter->_srcPath, info);

		String tempPath;
		if (iter->_direcMode)
			tempPath = _pathMgr->GetTempPath(iter->_destPath);
		else
			tempPath = _pathMgr->GetTempPath(_pathMgr->GetAbsolutePath(iter->_srcPath));

		File tempFile (tempPath);
		
		if (iter->_docId.isEmpty ()) {
			// ���µ��Զ�ͬ������
			_relayMgr->DeleteDownloadNonius (iter->_srcPath);
			if (tempFile.exists ())
				tempFile.remove();
			continue;
		}
		else if (tempFile.exists ()) {
			// ����������
			if (isCached && !File(_pathMgr->GetAbsolutePath (iter->_srcPath)).exists ()) {
				tempFile.remove ();
				_relayMgr->DeleteDownloadNonius (iter->_srcPath);

				continue;
			}

			if (iter->_range != 0 && iter->_lastModified != tempFile.getLength ())
				iter->_range = 0;
		}
		else
			iter->_range = 0;

		ncTaskStatus status = (ncTaskStatus)iter->_status;;
		int taskId = 0;

		if (iter->_direcMode) {
	
			ncDirectDownloadFileTask* oriTask = NC_NEW (syncMgrAlloc, ncDirectDownloadFileTask) (
				(ncTaskSrc)iter->_taskSrc, _localSyncPath, iter->_docId, iter->_srcPath, iter->_otag, iter->_size, iter->_destPath, iter->_delSrc, true);

			oriTask->setNoniusInfo(*iter);

			nsCOMPtr<ncISyncTask> task = oriTask;
			status = (ncTaskStatus)iter->_status;
			taskId = _scheduler->AddTask(task, NC_TS_EXECUTING == status ? ncSyncScheduler::NC_TP_PRIOR : ncSyncScheduler::NC_TP_LOW);
		}
		else {
			
			ncDownEditFileTask* oriTask = NC_NEW (syncMgrAlloc, ncDownEditFileTask) (
				(ncTaskSrc)iter->_taskSrc, _localSyncPath, iter->_docId, iter->_srcPath, iter->_otag, iter->_docType, iter->_typeName, iter->_viewType, iter->_viewName, iter->_mtime, iter->_size, iter->_attr, true);

			oriTask->setNoniusInfo(*iter);

			nsCOMPtr<ncISyncTask> task = oriTask;
			taskId = _scheduler->AddTask(task, NC_TS_EXECUTING == status ? ncSyncScheduler::NC_TP_PRIOR : ncSyncScheduler::NC_TP_LOW);
		}
	}
}

void ncIOScanner::ScanUnsync()
{
	if (_localSyncPath.isEmpty ()) {
		vector<ncUnsyncInfo> unsyncInfos;
		_cacheMgr->Unsync_GetInfoVec (String::EMPTY, unsyncInfos, 0);

		for (vector<ncUnsyncInfo>::iterator it = unsyncInfos.begin (); it != unsyncInfos.end (); ++it) {
			if (SystemFile::getFileAttrs (it->_absPath) == INVALID_FILE) {
				_cacheMgr->Unsync_Delete (it->_absPath, it->_isDir);
			}
			else if (!ncADSUtil::CheckADS (it->_absPath.getCStr(), ADS_ERRMARK))
				ncADSUtil::SetADS (it->_absPath.getCStr(), ADS_ERRMARK, it->_errCode);
		}
	}
}
