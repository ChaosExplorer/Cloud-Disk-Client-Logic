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

	// �ȴ���������̽�������
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

	// ����Ǹ�Ŀ¼����docId��relPathΪ�ա�
	if (!docId.isEmpty () || !relPath.isEmpty ()) {
		NC_SYNCMGR_CHECK_GNS_INVALID (docId);
		NC_SYNCMGR_CHECK_PATH_INVALID (relPath);
	}

	InterlockedIncrement (&_inlCnt);

	try {
		// �ڵ������߳���̽��ָ��Ŀ¼��
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
				// ̽��ǰ��Ӧ��ͬ�����ԡ�
				_policyMgr->ApplyPolicy ();

				// �Ӹ�Ŀ¼��ʼ̽�⡣
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

			// ���һ��ʱ���������´�̽�⡣
			int waitCount = 0;
			while (!_stop && ++waitCount < 300)
				sleep (100);

			// ̽��ǰ�ȵȴ����ȿ��С�
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

	// ע�⣺�����ƶ�̽�������ִ���ǲ��еģ�������ִ�л�Ӱ�쵽̽��ʱ��ȡ��
	// ���ݿ���Ϣ���ƶ���Ϣ����ȷ�ԣ������ƶ�̽��Ӧ���ƶ�ʵ�����Ϊ׼������
	// �����Ȼ�ȡ���ݿ���Ϣ���ٻ�ȡ�ƶ���Ϣ���Ա�֤�ƶ���Ϣ������ʵ�����

	// ��ȡ���ݿ����������ĵ���Ϣ��
	DetectInfoMap cacheMap;
	_cacheMgr->GetDetectSubInfos (docId, cacheMap);

	// ��ȡ�ƶ��������ĵ���Ϣ��
	DetectInfoMap svrMap;
	if (!GetServerSubInfos (docId, svrMap))
		return true;

	DetectInfoMap::iterator svrIt = svrMap.begin ();
	DetectInfoMap::iterator cacheIt = cacheMap.begin ();

	while (svrIt != svrMap.end () && cacheIt != cacheMap.end ()) {
		if (svrIt->first < cacheIt->first) {
			// ����ƶ��ж����ݿ���û�У���˵�����ĵ��������ġ�
			(svrIt->second._isDir ? addDirs : modFiles).insert (*svrIt);
			++svrIt;
		}
		else if (svrIt->first > cacheIt->first) {
			// ����ƶ�û�ж����ݿ����У���˵�����ĵ��ѱ�ɾ����
			(cacheIt->second._isDir ? rmvDirs : rmvFiles).insert ((_newView && docId.isEmpty ()) ? (Path::combine (cacheIt->second._viewName, cacheIt->second._name)) : cacheIt->second._name);
			++cacheIt;
		}
		else {
			// �����ƶ��ĵ����ӳ����ر�ǡ�
			svrIt->second._isDelay = cacheIt->second._isDelay;

			if (svrIt->second._isDir) {
				// ����ƶ����������ݿ��в�һ�£���˵����Ŀ¼�ѱ���������
				if (svrIt->second._name != cacheIt->second._name)
					GetRenameInfo ((_newView && docId.isEmpty ()) ? (Path::combine (svrIt->second._viewName, svrIt->second._name)) : svrIt->second._name,
									(_newView && docId.isEmpty ()) ? (Path::combine (cacheIt->second._viewName, cacheIt->second._name)) : cacheIt->second._name,
									rnmDirs, rnmFiles, true);

				// ����ƶ�otag�����ݿ��в�һ�£���˵����Ŀ¼�����ĵ��仯��
				if (svrIt->second._otag != cacheIt->second._otag)
					modDirs.insert (*svrIt);

				// ����ƶ����������ݿ��в�һ�£���˵����Ŀ¼�����ѱ��޸ġ�
				// �������Եļ̳����ԣ�������ӵ�modDirs���Լ���̽�������������Ա仯��
				if (svrIt->second._attr != cacheIt->second._attr) {
					attrMap.insert (make_pair ((_newView && docId.isEmpty ()) ? (Path::combine (svrIt->second._viewName, svrIt->second._name)) : svrIt->second._name, svrIt->second._attr));
					modDirs.insert (*svrIt);
				}

				// ����ƶ��޸�ʱ�������ݿ��в�һ�£����Ŀ¼��Ҫ�����޸�ʱ�䡣
				if (svrIt->second._mtime != cacheIt->second._mtime)
					modTimeDirs.insert (*svrIt);
			}
			else {
				// ����ƶ����������ݿ��в�һ�£���˵�����ļ��ѱ���������
				if (svrIt->second._name != cacheIt->second._name)
					GetRenameInfo ((_newView && docId.isEmpty ()) ? (Path::combine (svrIt->second._viewName, svrIt->second._name)) : svrIt->second._name,
									(_newView && docId.isEmpty ()) ? (Path::combine (cacheIt->second._viewName, cacheIt->second._name)) : cacheIt->second._name,
									rnmDirs, rnmFiles, false);

				// ����ƶ�otag�����ݿ��в�һ�£���˵�����ļ��ѱ��޸ġ�
				if (svrIt->second._otag != cacheIt->second._otag) {
					if (mode != NC_DM_DELAY || svrIt->second._isDelay)
						modFiles.insert (*svrIt);		// �ѻ��治�Զ�����
				}

				// ����ƶ����������ݿ��в�һ�£���˵�����ļ������ѱ��޸ġ�
				if (svrIt->second._attr != cacheIt->second._attr) {
					attrMap.insert (make_pair ((_newView && docId.isEmpty ()) ? (Path::combine (svrIt->second._viewName, svrIt->second._name)) : svrIt->second._name, svrIt->second._attr));
				}
			}

			// �����ڵ��ƶ��������������ݿ��в�һ�£���˵������ڵ����������ѱ��޸ġ�
			if (svrIt->second._type > NC_OT_ENTRYDOC) {
				if (svrIt->second._typeName != cacheIt->second._typeName) {
					tnMap.insert (make_pair ((_newView && docId.isEmpty ()) ? (Path::combine (svrIt->second._viewName, svrIt->second._name)) : svrIt->second._name, svrIt->second._typeName));
				}
			}

			++svrIt;
			++cacheIt;
		}
	}

	// ����ƶ��ж����ݿ���û�У���˵�����ĵ��������ġ�
	for (; svrIt != svrMap.end (); ++svrIt) {
		(svrIt->second._isDir ? addDirs : modFiles).insert (*svrIt);
	}

	// ����ƶ�û�ж����ݿ����У���˵�����ĵ��ѱ�ɾ����
	for (; cacheIt != cacheMap.end (); ++cacheIt) {
		(cacheIt->second._isDir ? rmvDirs : rmvFiles).insert ((_newView && docId.isEmpty ()) ? (Path::combine (cacheIt->second._viewName, cacheIt->second._name)) : cacheIt->second._name);
	}

	// �ȴ���ɾ�����ĵ���
	set<String>::iterator rmvIt;
	for (rmvIt = rmvFiles.begin (); rmvIt != rmvFiles.end (); ++rmvIt)
		_chgListener->OnSvrFileDeleted (_taskSrc, Path::combine (relPath, *rmvIt), _localSyncPath);

	for (rmvIt = rmvDirs.begin (); rmvIt != rmvDirs.end (); ++rmvIt)
		_chgListener->OnSvrDirDeleted (_taskSrc, Path::combine (relPath, *rmvIt), _localSyncPath);

	// �ٴ������������ĵ���
	map<String, String>::iterator rnmIt;
	for (rnmIt = rnmFiles.begin (); rnmIt != rnmFiles.end (); ++rnmIt)
		_chgListener->OnSvrFileRenamed (_taskSrc, Path::combine (relPath, rnmIt->first), Path::combine (relPath, rnmIt->second), _localSyncPath);

	for (rnmIt = rnmDirs.begin (); rnmIt != rnmDirs.end (); ++rnmIt)
		_chgListener->OnSvrDirRenamed (_taskSrc, Path::combine (relPath, rnmIt->first), Path::combine (relPath, rnmIt->second), _localSyncPath);

	// �ٴ��������޸ĵ��ĵ���
	map<String, int>::iterator attrIt;
	for (attrIt = attrMap.begin (); attrIt != attrMap.end (); ++attrIt)
		_chgListener->OnSvrAttrUpdated (_taskSrc, Path::combine (relPath, attrIt->first), attrIt->second, _localSyncPath);

	// �ٴ������������޸ĵ��ĵ���
	map<String, String>::iterator tnIt;
	for (tnIt = tnMap.begin (); tnIt != tnMap.end (); ++tnIt)
		_chgListener->OnSvrTypeNameUpdated (_taskSrc, Path::combine (relPath, tnIt->first), tnIt->second, _localSyncPath);

	DetectInfoMap::iterator iter;
	String subPath, extName;
	ncPathRelation relation;

	// �ٴ����������޸ĵ��ļ���
	for (iter = modFiles.begin (); iter != modFiles.end (); ++iter) {
		bool isAuto = true;			// �Ƿ�Ϊ�Զ���������̽�⡢�Զ�ͬ����
		if (mode == NC_DM_POLICY) {
			extName = Path::getExtensionName (iter->second._name);
			iter->second._isDelay = _policyMgr->CheckDelayFilePolicy (extName, iter->second._size) || !(iter->second._attr & NC_ALLOW_DOWNLOAD);
		}
		else if (mode == NC_DM_FULL) {
			iter->second._isDelay = ! (iter->second._attr & NC_ALLOW_DOWNLOAD);
			isAuto = false;			// �ֶ�����
		}

		_chgListener->OnSvrFileEdited (_taskSrc, iter->first, Path::combine (relPath, (_newView && docId.isEmpty ()) ? (Path::combine (iter->second._viewName, iter->second._name)) : iter->second._name),
			iter->second._otag, iter->second._type, iter->second._typeName, iter->second._viewType, iter->second._viewName, iter->second._mtime,
			iter->second._size, iter->second._attr, iter->second._isDelay, _localSyncPath, isAuto);
	}

	// �ٴ���������Ŀ¼��
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

	// �ٴ����޸ĵ�Ŀ¼��
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

	// �ٴ����޸�ʱ��仯��Ŀ¼��
	for (iter = modTimeDirs.begin (); iter != modTimeDirs.end (); ++iter) {
		_chgListener->OnSvrDirModifyTimeUpdated (_taskSrc, Path::combine (relPath, (_newView && docId.isEmpty ()) ? (Path::combine (iter->second._viewName, iter->second._name)) : iter->second._name), iter->second._mtime, _localSyncPath);
	}

	// ������������ӳ����ر�ǡ�
	if (isDelay) {
		_chgListener->OnSvrDelayUpdated (_taskSrc, relPath, false, _localSyncPath);
	}

	// ���̽�⵽�˱仯���򷵻�true�����򷵻�false��
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
				// ͨ��EACP��ȡ���пɷ��ʵ�����ĵ���
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
				// ͨ��EFSP��ȡ���пɷ��ʵ����ĵ���
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

	// �������ֻ��һ�㣬��һ��������������������
	if (depth == 1) {
		(isDir ? rnmDirs : rnmFiles).insert (make_pair (cacheName, svrName));
		return;
	}

	// ������Ʋ�ֹһ�㣬���п����ǲ㼶��Ŀ¼��������������
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
			// ͨ��EFSP��ȡ���пɷ��ʵ����ĵ���
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