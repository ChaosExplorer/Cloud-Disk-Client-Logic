/**************************************************************************************************
Purpose:
	Source file for class ncDetector.

Author:
	Chaos

Creating Time:
	2016-8-19
***************************************************************************************************/

#include <abprec.h>
#include <docmgr/public/ncIDocMgr.h>
#include <docmgr/public/ncIADSMgr.h>
#include <eachttpclient/public/ncIEACPClient.h>
#include <efshttpclient/public/ncIEFSPClient.h>
#include <syncmgr/tasks/ncTaskErrorHandler.h>
#include <syncmgr/ncSyncPolicyMgr.h>
#include <syncmgr/ncSyncScheduler.h>
#include <syncmgr/ncChangeListener.h>
#include <syncmgr/ncSyncUtils.h>
#include <syncmgr/ncSyncMgr.h>
#include <syncmgr/syncmgr.h>
#include "ncDetector.h"

// public ctor
ncDetector::ncDetector (ncIEACPClient* eacpClient,
						ncIEFSPClient* efspClient,
						ncIDocMgr* docMgr,
						ncSyncPolicyMgr* policyMgr,
						ncSyncScheduler* scheduler,
						ncChangeListener* chgListener,
						String localSyncPath)
	: _stop (true)
	, _inlCnt (0)
	, _eacpClient (0)
	, _efspClient (0)
	, _docMgr (docMgr)
	, _policyMgr (policyMgr)
	, _scheduler (scheduler)
	, _chgListener (chgListener)
	, _localSyncPath (localSyncPath)
	, _taskSrc (NC_TS_DETECT)
{
	NC_SYNCMGR_TRACE (_T("eacpClient: 0x%p, efspClient: 0x%p, docMgr: 0x%p, \
						  policyMgr: 0x%p, scheduler: 0x%p, chgListener: 0x%p"),
					  eacpClient, efspClient, docMgr, policyMgr, scheduler, chgListener);

	NC_SYNCMGR_CHECK_ARGUMENT_NULL (eacpClient);
	NC_SYNCMGR_CHECK_ARGUMENT_NULL (efspClient);
	NC_SYNCMGR_CHECK_ARGUMENT_NULL (docMgr);
	NC_SYNCMGR_CHECK_ARGUMENT_NULL (policyMgr);
	NC_SYNCMGR_CHECK_ARGUMENT_NULL (scheduler);
	NC_SYNCMGR_CHECK_ARGUMENT_NULL (chgListener);

	_eacpClient = getter_AddRefs (eacpClient->Clone ());
	_efspClient = getter_AddRefs (efspClient->Clone ());

	_cacheMgr = _docMgr->GetCacheMgr (_localSyncPath);
	_policyMgr = policyMgr;
	_scheduler = scheduler;
	_chgListener = chgListener;
	_newView = _docMgr->IsNewView ();
}

// public dtor
ncDetector::~ncDetector ()
{
	NC_SYNCMGR_TRACE (_T("this: 0x%p"), this);

	if (!_stop)
		Stop ();

	// 等待所有内联探测结束。
	while (_inlCnt > 0)
		Sleep (100);
}

// public
void ncDetector::Start ()
{
	NC_SYNCMGR_TRACE (_T("this: 0x%p"), this);

	InterlockedExchange (&_stop, false);

	this->start ();
}

// public
void ncDetector::Stop ()
{
	NC_SYNCMGR_TRACE (_T("this: 0x%p"), this);

	InterlockedExchange (&_stop, true);

	if (_eacpClient)
		_eacpClient->Stop ();

	if (_efspClient)
		_efspClient->Stop ();
}

// public
void ncDetector::InlineDetect (const String& docId, const String& relPath, ncDetectMode mode)
{
	NC_SYNCMGR_TRACE (_T("docId: %s, relPath: %s, mode: %d, -begin"), docId.getCStr (), relPath.getCStr (), mode);

	// 如果是根目录，则docId和relPath为空。
	if (!docId.isEmpty () || !relPath.isEmpty ()) {
		NC_SYNCMGR_CHECK_GNS_INVALID (docId);
		NC_SYNCMGR_CHECK_PATH_INVALID (relPath);
	}

	InterlockedIncrement (&_inlCnt);

	try {
		// 在调用者线程中探测指定目录。
		DetectDir (docId, relPath, mode);
	}
	catch (Exception& e) {
		SystemLog::getInstance ()->log (ERROR_LOG_TYPE, e.toFullString ().getCStr ());
	}
	catch (...) {
		SystemLog::getInstance ()->log (ERROR_LOG_TYPE, _T("ncDetector::InlineDetect () failed."));
	}

	InterlockedDecrement (&_inlCnt);

	NC_SYNCMGR_TRACE (_T("docId: %s, relPath: %s, mode: %d, -end"), docId.getCStr (), relPath.getCStr (), mode);
}

// pubilc
void ncDetector::run ()
{
	NC_SYNCMGR_TRACE (_T("this: 0x%p, -begin"), this);

	while (!_stop) {
		try {
			if (_localSyncPath.isEmpty ()) {
				// 探测前先应用同步策略。
				_policyMgr->ApplyPolicy ();

				// 从根目录开始探测。
				DetectDir (String::EMPTY, String::EMPTY, NC_DM_DELAY);
			}
			else {
				 String docId;
				_docMgr->GetADSMgr ()->GetDocIdInfo(_localSyncPath, docId);
				if (!docId.isEmpty ()) {
					if (CheckDir (docId)) {
						DetectDir (docId, _localSyncPath.afterLast (Path::separator), NC_DM_DELAY);
					}
					else {
						ncSyncMgr::getInstance ()->NotifyLocalSyncInvalid (_localSyncPath);
					}
				}
			}

			// 间隔一段时间再启动下次探测。
			int waitCount = 0;
			while (!_stop && ++waitCount < 300)
				sleep (100);

			// 探测前先等待调度空闲。
			while (!_stop && !_scheduler->IsIdle () && _inlCnt > 0)
				sleep (100);
		}
		catch (Exception& e) {
			sleep (100);
			SystemLog::getInstance ()->log (ERROR_LOG_TYPE, e.toFullString ().getCStr ());
		}
		catch (...) {
			sleep (100);
			SystemLog::getInstance ()->log (ERROR_LOG_TYPE, _T("ncDetector::run () failed."));
		}
	}

	NC_SYNCMGR_TRACE (_T("this: 0x%p, -end"), this);
}

// protected
bool ncDetector::DetectDir (const String& docId,
							const String& relPath,
							ncDetectMode mode,
							bool isDelay/* = false*/)
{
	NC_SYNCMGR_TRACE (_T("docId: %s, relPath: %s, mode: %d, isDelay: %d"),
					  docId.getCStr (), relPath.getCStr (), mode, isDelay);

	if (_stop)
		return true;

	DetectInfoMap modFiles, addDirs, modDirs, modTimeDirs;
	set<String> rmvFiles, rmvDirs;
	map<String, String> rnmFiles, rnmDirs, tnMap;
	map<String, int> attrMap;

	// 注意：由于云端探测和任务执行是并行的，而任务执行会影响到探测时获取的
	// 数据库信息和云端信息的正确性，鉴于云端探测应以云端实际情况为准，所以
	// 必须先获取数据库信息，再获取云端信息，以保证云端信息符合真实情况。

	// 获取数据库中所有子文档信息。
	DetectInfoMap cacheMap;
	_cacheMgr->GetDetectSubInfos (docId, cacheMap);

	// 获取云端所有子文档信息。
	DetectInfoMap svrMap;
	if (!GetServerSubInfos (docId, svrMap))
		return true;

	DetectInfoMap::iterator svrIt = svrMap.begin ();
	DetectInfoMap::iterator cacheIt = cacheMap.begin ();

	while (svrIt != svrMap.end () && cacheIt != cacheMap.end ()) {
		if (svrIt->first < cacheIt->first) {
			// 如果云端有而数据库中没有，则说明该文档是新增的。
			(svrIt->second._isDir ? addDirs : modFiles).insert (*svrIt);
			++svrIt;
		}
		else if (svrIt->first > cacheIt->first) {
			// 如果云端没有而数据库中有，则说明该文档已被删除。
			(cacheIt->second._isDir ? rmvDirs : rmvFiles).insert ((_newView && docId.isEmpty ()) ? (Path::combine (cacheIt->second._viewName, cacheIt->second._name)) : cacheIt->second._name);
			++cacheIt;
		}
		else {
			// 修正云端文档的延迟下载标记。
			svrIt->second._isDelay = cacheIt->second._isDelay;

			if (svrIt->second._isDir) {
				// 如果云端名称与数据库中不一致，则说明该目录已被重命名。
				if (svrIt->second._name != cacheIt->second._name)
					GetRenameInfo ((_newView && docId.isEmpty ()) ? (Path::combine (svrIt->second._viewName, svrIt->second._name)) : svrIt->second._name,
									(_newView && docId.isEmpty ()) ? (Path::combine (cacheIt->second._viewName, cacheIt->second._name)) : cacheIt->second._name,
									rnmDirs, rnmFiles, true);

				// 如果云端otag与数据库中不一致，则说明该目录下有文档变化。
				if (svrIt->second._otag != cacheIt->second._otag)
					modDirs.insert (*svrIt);

				// 如果云端属性与数据库中不一致，则说明该目录属性已被修改。
				// 由于属性的继承特性，所以添加到modDirs中以继续探测子孙对象的属性变化。
				if (svrIt->second._attr != cacheIt->second._attr) {
					attrMap.insert (make_pair ((_newView && docId.isEmpty ()) ? (Path::combine (svrIt->second._viewName, svrIt->second._name)) : svrIt->second._name, svrIt->second._attr));
					modDirs.insert (*svrIt);
				}

				// 如果云端修改时间与数据库中不一致，则该目录需要更新修改时间。
				if (svrIt->second._mtime != cacheIt->second._mtime)
					modTimeDirs.insert (*svrIt);
			}
			else {
				// 如果云端名称与数据库中不一致，则说明该文件已被重命名。
				if (svrIt->second._name != cacheIt->second._name)
					GetRenameInfo ((_newView && docId.isEmpty ()) ? (Path::combine (svrIt->second._viewName, svrIt->second._name)) : svrIt->second._name,
									(_newView && docId.isEmpty ()) ? (Path::combine (cacheIt->second._viewName, cacheIt->second._name)) : cacheIt->second._name,
									rnmDirs, rnmFiles, false);

				// 如果云端otag与数据库中不一致，则说明该文件已被修改。
				if (svrIt->second._otag != cacheIt->second._otag) {
					if (mode != NC_DM_DELAY || svrIt->second._isDelay)
						modFiles.insert (*svrIt);		// 已缓存不自动更新
				}

				// 如果云端属性与数据库中不一致，则说明该文件属性已被修改。
				if (svrIt->second._attr != cacheIt->second._attr) {
					attrMap.insert (make_pair ((_newView && docId.isEmpty ()) ? (Path::combine (svrIt->second._viewName, svrIt->second._name)) : svrIt->second._name, svrIt->second._attr));
				}
			}

			// 如果入口的云端类型名称与数据库中不一致，则说明该入口的类型名称已被修改。
			if (svrIt->second._type > NC_OT_ENTRYDOC) {
				if (svrIt->second._typeName != cacheIt->second._typeName) {
					tnMap.insert (make_pair ((_newView && docId.isEmpty ()) ? (Path::combine (svrIt->second._viewName, svrIt->second._name)) : svrIt->second._name, svrIt->second._typeName));
				}
			}

			++svrIt;
			++cacheIt;
		}
	}

	// 如果云端有而数据库中没有，则说明该文档是新增的。
	for (; svrIt != svrMap.end (); ++svrIt) {
		(svrIt->second._isDir ? addDirs : modFiles).insert (*svrIt);
	}

	// 如果云端没有而数据库中有，则说明该文档已被删除。
	for (; cacheIt != cacheMap.end (); ++cacheIt) {
		(cacheIt->second._isDir ? rmvDirs : rmvFiles).insert ((_newView && docId.isEmpty ()) ? (Path::combine (cacheIt->second._viewName, cacheIt->second._name)) : cacheIt->second._name);
	}

	// 先处理删除的文档。
	set<String>::iterator rmvIt;
	for (rmvIt = rmvFiles.begin (); rmvIt != rmvFiles.end (); ++rmvIt)
		_chgListener->OnSvrFileDeleted (_taskSrc, Path::combine (relPath, *rmvIt), _localSyncPath);

	for (rmvIt = rmvDirs.begin (); rmvIt != rmvDirs.end (); ++rmvIt)
		_chgListener->OnSvrDirDeleted (_taskSrc, Path::combine (relPath, *rmvIt), _localSyncPath);

	// 再处理重命名的文档。
	map<String, String>::iterator rnmIt;
	for (rnmIt = rnmFiles.begin (); rnmIt != rnmFiles.end (); ++rnmIt)
		_chgListener->OnSvrFileRenamed (_taskSrc, Path::combine (relPath, rnmIt->first), Path::combine (relPath, rnmIt->second), _localSyncPath);

	for (rnmIt = rnmDirs.begin (); rnmIt != rnmDirs.end (); ++rnmIt)
		_chgListener->OnSvrDirRenamed (_taskSrc, Path::combine (relPath, rnmIt->first), Path::combine (relPath, rnmIt->second), _localSyncPath);

	// 再处理属性修改的文档。
	map<String, int>::iterator attrIt;
	for (attrIt = attrMap.begin (); attrIt != attrMap.end (); ++attrIt)
		_chgListener->OnSvrAttrUpdated (_taskSrc, Path::combine (relPath, attrIt->first), attrIt->second, _localSyncPath);

	// 再处理类型名称修改的文档。
	map<String, String>::iterator tnIt;
	for (tnIt = tnMap.begin (); tnIt != tnMap.end (); ++tnIt)
		_chgListener->OnSvrTypeNameUpdated (_taskSrc, Path::combine (relPath, tnIt->first), tnIt->second, _localSyncPath);

	DetectInfoMap::iterator iter;
	String subPath, extName;
	ncPathRelation relation;

	// 再处理新增和修改的文件。
	for (iter = modFiles.begin (); iter != modFiles.end (); ++iter) {
		bool isAuto = true;			// 是否为自动下载任务（探测、自动同步）
		if (mode == NC_DM_POLICY) {
			extName = Path::getExtensionName (iter->second._name);
			iter->second._isDelay = _policyMgr->CheckDelayFilePolicy (extName, iter->second._size) || !(iter->second._attr & NC_ALLOW_DOWNLOAD);
		}
		else if (mode == NC_DM_FULL) {
			iter->second._isDelay = ! (iter->second._attr & NC_ALLOW_DOWNLOAD);
			isAuto = false;			// 手动下载
		}

		_chgListener->OnSvrFileEdited (_taskSrc, iter->first, Path::combine (relPath, (_newView && docId.isEmpty ()) ? (Path::combine (iter->second._viewName, iter->second._name)) : iter->second._name),
			iter->second._otag, iter->second._type, iter->second._typeName, iter->second._viewType, iter->second._viewName, iter->second._mtime,
			iter->second._size, iter->second._attr, iter->second._isDelay, _localSyncPath, isAuto);
	}

	// 再处理新增的目录。
	for (iter = addDirs.begin (); iter != addDirs.end (); ++iter) {
		subPath = Path::combine (relPath, (_newView && docId.isEmpty ()) ? (Path::combine (iter->second._viewName, iter->second._name)) : iter->second._name);
		relation = _policyMgr->CheckNoDelayDirPolicy (subPath);

		if (mode == NC_DM_DELAY && relation == NC_PR_INDEPENDENT) {
			_chgListener->OnSvrDirCreated (_taskSrc, iter->first, subPath, iter->second._otag,
				iter->second._type, iter->second._typeName, iter->second._viewType, iter->second._viewName,iter->second._mtime, iter->second._attr, true, _localSyncPath);
		}
		else {
			_chgListener->OnSvrDirCreated (_taskSrc, iter->first, subPath, String::EMPTY,
				iter->second._type, iter->second._typeName, iter->second._viewType, iter->second._viewName, iter->second._mtime, iter->second._attr, true, _localSyncPath);

			ncDetectMode subMode = (mode == NC_DM_DELAY && relation != NC_PR_ANCESTOR ? NC_DM_POLICY : mode);
			if (!DetectDir (iter->first, subPath, subMode, true))
				_chgListener->OnSvrOtagUpdated (_taskSrc, subPath, iter->second._otag, _localSyncPath);
		}
	}

	// 再处理修改的目录。
	for (iter = modDirs.begin (); iter != modDirs.end (); ++iter) {
		subPath = Path::combine (relPath, (_newView && docId.isEmpty ()) ? (Path::combine (iter->second._viewName, iter->second._name)) : iter->second._name);
		relation = _policyMgr->CheckNoDelayDirPolicy (subPath);

		if (mode == NC_DM_DELAY && relation == NC_PR_INDEPENDENT) {
			if (iter->second._isDelay || !DetectDir (iter->first, subPath, mode))
				_chgListener->OnSvrOtagUpdated (_taskSrc, subPath, iter->second._otag, _localSyncPath);
		}
		else {
			ncDetectMode subMode = (mode == NC_DM_DELAY && relation != NC_PR_ANCESTOR ? NC_DM_POLICY : mode);
			if (!DetectDir (iter->first, subPath, subMode, iter->second._isDelay))
				_chgListener->OnSvrOtagUpdated (_taskSrc, subPath, iter->second._otag, _localSyncPath);
		}
	}

	// 再处理修改时间变化的目录。
	for (iter = modTimeDirs.begin (); iter != modTimeDirs.end (); ++iter) {
		_chgListener->OnSvrDirModifyTimeUpdated (_taskSrc, Path::combine (relPath, (_newView && docId.isEmpty ()) ? (Path::combine (iter->second._viewName, iter->second._name)) : iter->second._name), iter->second._mtime, _localSyncPath);
	}

	// 最后更新自身的延迟下载标记。
	if (isDelay) {
		_chgListener->OnSvrDelayUpdated (_taskSrc, relPath, false, _localSyncPath);
	}

	// 如果探测到了变化，则返回true，否则返回false。
	return !(modFiles.empty () && addDirs.empty () && modDirs.empty () &&
			 rmvFiles.empty () && rmvDirs.empty () && rnmFiles.empty () &&
			 rnmDirs.empty () && attrMap.empty () && tnMap.empty ());
}

// protected
bool ncDetector::GetServerSubInfos (const String& docId, DetectInfoMap& svrMap)
{
	NC_SYNCMGR_TRACE (_T("docId: %s"), docId.getCStr ());

	String subDocId;
	ncDetectInfo subInfo;

	while (true) {
		try {
			if (docId.isEmpty ()) {
				// 通过EACP获取所有可访问的入口文档。
				vector<ncDocInfo> docInfos;
				nsCOMPtr<ncIEACPDocOperation> docOp = getter_AddRefs (_eacpClient->CreateDocOperation ());
				docOp->Get (docInfos);

				vector<ncDocInfo>::iterator it;
				for (it = docInfos.begin (); it != docInfos.end (); ++it) {
					subDocId = it->docId;
					subInfo._name = it->docName;
					subInfo._otag = it->otag;
					subInfo._type = it->docType | NC_OT_ENTRYDOC;
					subInfo._typeName = it->typeName;
					subInfo._viewType = it->viewType;
					subInfo._viewName = it->viewName;
					subInfo._isDir = (it->size == -1);
					subInfo._isDelay = true;
					subInfo._mtime = it->modified;
					subInfo._size = it->size;
					subInfo._attr = it->attr;
					svrMap.insert (make_pair (subDocId, subInfo));
				}
			}
			else {
				// 通过EFSP获取所有可访问的子文档。
				list<ncEFSPListItem> dirs, files;
				nsCOMPtr<ncIEFSPDirectoryOperation> dirOp = getter_AddRefs (_efspClient->CreateDirectoryOperation ());
				dirOp->ListDirectory2 (docId, AS_DOCID, false, true, &dirs, &files);

				list<ncEFSPListItem>::iterator it;
				for (it = dirs.begin (); it != dirs.end (); ++it) {
					subDocId = it->docId;
					subInfo._name = it->name;
					subInfo._otag = it->rev;
					subInfo._type = NC_OT_UNKNOWN;
					subInfo._typeName.clear ();
					subInfo._isDir = true;
					subInfo._isDelay = true;
					subInfo._mtime = it->modified;
					subInfo._size = -1;
					subInfo._attr = it->attr;
					svrMap.insert (make_pair (subDocId, subInfo));
				}

				for (it = files.begin (); it != files.end (); ++it) {
					subDocId = it->docId;
					subInfo._name = it->name;
					subInfo._otag = it->rev;
					subInfo._type = NC_OT_UNKNOWN;
					subInfo._typeName.clear ();
					subInfo._isDir = false;
					subInfo._isDelay = true;
					subInfo._mtime = it->modified;
					subInfo._size = it->size;
					subInfo._attr = it->attr;
					svrMap.insert (make_pair (subDocId, subInfo));
				}
			}

			return true;
		}
		catch (Exception& e) {
			ncTaskErrorType errType = ncTaskErrorHandler::GetErrorType (e);
			if (errType != NC_TET_NETWORK_ERROR && errType != NC_TET_NETWORK_DISCONNECTED)
				throw;
			else if (!_stop)
				Thread::sleep (100);
			else
				return false;
		}
	}
}

bool ncDetector::CheckDir(const String& docId)
{
	NC_SYNCMGR_TRACE (_T("docId: %s"), docId.getCStr ());
	while (true) {
		try {
			String name;
			nsCOMPtr<ncIEFSPFileOperation> fileOp = getter_AddRefs (_efspClient->CreateFileOperation ());
			fileOp->GetName(docId, name);
			if (name == _localSyncPath.afterLast(_T('\\'))) {
				return true;
			}
			else {
				return false;
			}
		}
		catch (Exception& e) {
			ncTaskErrorType errType = ncTaskErrorHandler::GetErrorType (e);
			if (errType == NC_TET_OBJECT_NOT_EXIST ||
				errType == NC_TET_PERMISSION_DENIED ||
				errType == NC_TET_CLASSIFIED_DENIED) {
					InterlockedExchange (&_stop, true);
					return false;
			}
			else if (errType != NC_TET_NETWORK_ERROR && errType != NC_TET_NETWORK_DISCONNECTED)
				throw;
			else if (!_stop)
				Thread::sleep (100);
			else
				return true;
		}
	}
}

// protected
void ncDetector::GetRenameInfo (const String& svrName,
								const String& cacheName,
								map<String, String>& rnmDirs,
								map<String, String>& rnmFiles,
								bool isDir)
{
	NC_SYNCMGR_TRACE (_T("svrName: %s, cacheName: %s"), svrName.getCStr (), cacheName.getCStr ());

	int depth = svrName.trim (Path::separator).freq (Path::separator) + 1;

	// 如果名称只有一层，则一定是自身发生了重命名。
	if (depth == 1) {
		(isDir ? rnmDirs : rnmFiles).insert (make_pair (cacheName, svrName));
		return;
	}

	// 如果名称不止一层，则有可能是层级父目录发生了重命名。
	String sName = ncSyncUtils::GetFileName (svrName);
	String cName = ncSyncUtils::GetFileName (cacheName);
	String sPath = Path::getDirectoryName (svrName);
	String cPath = Path::getDirectoryName (cacheName);

	if (sName != cName) {
		(isDir ? rnmDirs : rnmFiles).insert (make_pair (Path::combine (sPath, cName), Path::combine (sPath, sName)));
	}

	while (sPath != cPath) {
		sName = ncSyncUtils::GetFileName (sPath);
		cName = ncSyncUtils::GetFileName (cPath);
		sPath = Path::getDirectoryName (sPath);
		cPath = Path::getDirectoryName (cPath);

		if (sName != cName) {
			rnmDirs.insert (make_pair (Path::combine (sPath, cName), Path::combine (sPath, sName)));
		}
	}
}

void ncDetector::InlineDetectExStart( const String& parrentDocId, const String& relPath, const String& docId,  bool isDir )
{
	NC_SYNCMGR_TRACE (_T("parrentDocId: %s, docId: %s, -begin"), parrentDocId.getCStr (), docId.getCStr ());

	NC_SYNCMGR_CHECK_GNS_INVALID (parrentDocId);
	NC_SYNCMGR_CHECK_GNS_INVALID (docId);

	InterlockedIncrement (&_inlCnt);

	try {
		ncDetectInfo srvInfo;
		if (GetServerSubInfo (parrentDocId, docId, srvInfo, isDir)) {
			if (isDir) {
				String subPath = Path::combine (relPath, srvInfo._name);

				_chgListener->OnSvrDirCreated (_taskSrc, docId, subPath, srvInfo._otag,
					srvInfo._type, srvInfo._typeName, srvInfo._viewType, srvInfo._viewName,srvInfo._mtime, srvInfo._attr, true, _localSyncPath);
			}
			else {
				_chgListener->OnSvrFileEdited (_taskSrc, docId, Path::combine (relPath, srvInfo._name),
					srvInfo._otag, srvInfo._type, srvInfo._typeName, srvInfo._viewType, srvInfo._viewName, srvInfo._mtime,
					srvInfo._size, srvInfo._attr, srvInfo._isDelay, _localSyncPath, true);
			}
		}
	}
	catch (Exception& e) {
		SystemLog::getInstance ()->log (ERROR_LOG_TYPE, e.toFullString ().getCStr ());
	}
	catch (...) {
		SystemLog::getInstance ()->log (ERROR_LOG_TYPE, _T("ncDetector::InlineDetectEx () failed."));
	}

	NC_SYNCMGR_TRACE (_T("parrentDocId: %s, docId: %s"), parrentDocId.getCStr (), docId.getCStr ());

}

void ncDetector::InlineDetectExEnd()
{
	InterlockedDecrement (&_inlCnt);
}

bool ncDetector::GetServerSubInfo( const String& docId, const String& subDocId, ncDetectInfo& subInfo, bool isDir )
{
	NC_SYNCMGR_TRACE (_T("docId: %s"), docId.getCStr ());

	while (true) {
		try {
			// 通过EFSP获取所有可访问的子文档。
			list<ncEFSPListItem> dirs, files;
			nsCOMPtr<ncIEFSPDirectoryOperation> dirOp = getter_AddRefs (_efspClient->CreateDirectoryOperation ());
			dirOp->ListDirectory2 (docId, AS_DOCID, false, true, &dirs, &files);

			list<ncEFSPListItem>::iterator it;

			if (isDir) {
				for (it = dirs.begin (); it != dirs.end (); ++it) {

					if (it->docId == subDocId) {
						subInfo._name = it->name;
						subInfo._otag = it->rev;
						subInfo._type = NC_OT_UNKNOWN;
						subInfo._typeName.clear ();
						subInfo._isDir = true;
						subInfo._isDelay = true;
						subInfo._mtime = it->modified;
						subInfo._size = -1;
						subInfo._attr = it->attr;

						return true;
					}
				}
			}
			else {
				for (it = files.begin (); it != files.end (); ++it) {

					if (it->docId == subDocId) {

						subInfo._name = it->name;
						subInfo._otag = it->rev;
						subInfo._type = NC_OT_UNKNOWN;
						subInfo._typeName.clear ();
						subInfo._isDir = false;
						subInfo._isDelay = true;
						subInfo._mtime = it->modified;
						subInfo._size = it->size;
						subInfo._attr = it->attr;

						return true;
					}
				}
			}
		}
		catch (Exception& e) {
			ncTaskErrorType errType = ncTaskErrorHandler::GetErrorType (e);
			if (errType != NC_TET_NETWORK_ERROR && errType != NC_TET_NETWORK_DISCONNECTED)
				throw;
			else if (!_stop)
				Thread::sleep (100);
		}

		return false;
	}

	return false;
}