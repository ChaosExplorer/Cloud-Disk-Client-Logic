/***************************************************************************************************
Purpose:
	Source file for class ncTaskUtils.

Author:
	Chaos

Created Time:
	2016-09-10

***************************************************************************************************/

#include <abprec.h>
#include "shlobj.h"

#include <docmgr/public/ncIDocMgr.h>
#include <docmgr/public/ncIPathMgr.h>
#include <docmgr/public/ncIFileMgr.h>
#include <docmgr/public/ncICacheMgr.h>
#include <docmgr/public/ncIADSMgr.h>
#include <syncmgr/observer/ncIOScanner.h>
#include <syncmgr/ncSyncRequester.h>
#include <syncmgr/ncSyncScheduler.h>
#include <syncmgr/ncSyncMgr.h>
#include <syncmgr/syncmgr.h>

#include "ncTaskUtils.h"

#include "ncUpCreateDirTask.h"
#include "ncUpEditFileTask.h"
#include "ncLocalCleanFileTask.h"

#include <fileextd/include/fileextd.h>
#include <dataapi/ncJson.h>

// public static
bool ncTaskUtils::HasEditedFilesInDir (const String & path)
{
	NC_SYNCMGR_TRACE (_T("path: %s"), path.getCStr ());
	NC_SYNCMGR_CHECK_PATH_INVALID (path);

	nsCOMPtr<ncIDocMgr> docMgr = getter_AddRefs (ncSyncMgr::getInstance ()->GetDocMgr ());
	nsCOMPtr<ncIPathMgr> pathMgr = getter_AddRefs (docMgr->GetPathMgr (String::EMPTY));
	nsCOMPtr<ncICacheMgr> cacheMgr = getter_AddRefs (docMgr->GetCacheMgr (String::EMPTY));

	// 列举出目录下所有的子孙文件。
	vector<tstring> files;
	ncDirUtil::ListFiles (path.getCStr (), files, TRUE);

	ncCacheInfo info;
	String subPath;

	// 检查上述文件是否发生了修改。
	vector<tstring>::iterator it;
	for (it = files.begin (); it != files.end (); ++it) {
		subPath.assign (it->c_str ());

		if (!cacheMgr->GetInfoByPath (pathMgr->GetRelativePath (subPath), info)) {
			if (!pathMgr->IsTempPath (subPath) &&
				!pathMgr->IsBackupPath (subPath) &&
				!pathMgr->IsIgnorePath (subPath))
				return true;
		}
		else {
			if (info._lastModified != SystemFile::getLastModified (subPath) && !info._isDelay)
				return true;
		}
	}

	return false;
}

// public static
void ncTaskUtils::CheckDelEntryParent (const String& localPath, const String& path, int docType)
{
	NC_SYNCMGR_TRACE (_T("path: %s"), path.getCStr ());
	NC_SYNCMGR_CHECK_PATH_INVALID (path);

	nsCOMPtr<ncIDocMgr> docMgr = getter_AddRefs (ncSyncMgr::getInstance ()->GetDocMgr ());
	nsCOMPtr<ncIPathMgr> pathMgr = getter_AddRefs (docMgr->GetPathMgr (localPath));
	nsCOMPtr<ncIFileMgr> fileMgr = getter_AddRefs (docMgr->GetFileMgr (localPath));
	nsCOMPtr<ncICacheMgr> cacheMgr = getter_AddRefs (docMgr->GetCacheMgr (localPath));

	String parentPath  = Path::getDirectoryName (path);
	String parentRelPath = pathMgr->GetRelativePath (parentPath);
	String parentRelPathTemp;
	ncCacheInfo info;

	while (!parentRelPath.isEmpty ()) {
		// 如果层级父目录存在缓存信息，则说明它下面还有其他入口，不能删除。
		// 在新视图模式下，最上层的时候，视图下有文件或者类型是非归档库，则不删除。
		parentRelPathTemp = Path::getDirectoryName (parentRelPath);
		if (docMgr->IsNewView () && parentRelPath.freq (Path::separator) == 0 && cacheMgr->HasInfoByView (parentRelPath))
			return;
		else if (cacheMgr->GetInfoByPath (parentRelPath, info))
			return;

		// 如果删除时目录下有被修改的文件，则保存对应副本。
		if (HasEditedFilesInDir (parentPath)) {
			fileMgr->CreateDupDoc (parentPath);
		}
		else {
			fileMgr->InternalDelete (parentPath);
		}

		parentPath = Path::getDirectoryName (parentPath);
		parentRelPath = Path::getDirectoryName (parentRelPath);
	}
}

// public static
void ncTaskUtils::UploadRenamedDir (ncTaskSrc taskSrc, const String& localPath, const String& path)
{
	NC_SYNCMGR_TRACE (_T("path: %s"), path.getCStr ());
	NC_SYNCMGR_CHECK_PATH_INVALID (path);

	if (!SystemFile::isDirectory (path))
		return;

	nsCOMPtr<ncSyncScheduler> scheduler = getter_AddRefs (
		reinterpret_cast<ncSyncScheduler*> (ncSyncMgr::getInstance ()->GetScheduler ()));

	// 添加目录新建任务。
	nsCOMPtr<ncISyncTask> task = NC_NEW (syncMgrAlloc, ncUpCreateDirTask) (
		taskSrc, localPath, path);
	scheduler->AddTask (task, ncSyncScheduler::NC_TP_PRIOR);

	// 扫描目录下子孙文档的变化。
	ncIOScanner* scanner = ncSyncMgr::getInstance ()->GetScanner (localPath);
	scanner->InlineScan (path);
}

// public static
void ncTaskUtils::UploadRenamedFile (ncTaskSrc taskSrc, const String& localPath, const String& path)
{
	NC_SYNCMGR_TRACE (_T("path: %s"), path.getCStr ());
	NC_SYNCMGR_CHECK_PATH_INVALID (path);

	if (!SystemFile::isFile (path))
		return;

	nsCOMPtr<ncSyncScheduler> scheduler = getter_AddRefs (
		reinterpret_cast<ncSyncScheduler*> (ncSyncMgr::getInstance ()->GetScheduler ()));

	// 添加文件编辑任务。
	nsCOMPtr<ncISyncTask> task = NC_NEW (syncMgrAlloc, ncUpEditFileTask) (
		taskSrc, localPath, path);
	scheduler->AddTask (task, ncSyncScheduler::NC_TP_LOW);
}

// public static
void ncTaskUtils::CheckDirChange (const String& localPath, const String& path)
{
	NC_SYNCMGR_TRACE (_T("path: %s"), path.getCStr ());
	NC_SYNCMGR_CHECK_PATH_INVALID (path);

	// 扫描目录下子孙文档的变化。
	ncIOScanner* scanner = ncSyncMgr::getInstance ()->GetScanner (localPath);
	scanner->InlineScan (path);
}

// public static
void ncTaskUtils::CheckRenamedFileChange (ncTaskSrc taskSrc, const String& localPath, const String& path)
{
	NC_SYNCMGR_TRACE (_T("path: %s"), path.getCStr ());
	NC_SYNCMGR_CHECK_PATH_INVALID (path);

	nsCOMPtr<ncIDocMgr> docMgr = getter_AddRefs (ncSyncMgr::getInstance ()->GetDocMgr ());
	nsCOMPtr<ncIADSMgr> adsMgr = getter_AddRefs (docMgr->GetADSMgr ());
	if (adsMgr->CheckErrMarkInfo (path))
		return;

	nsCOMPtr<ncIPathMgr> pathMgr = getter_AddRefs (docMgr->GetPathMgr (localPath));
	nsCOMPtr<ncICacheMgr> cacheMgr = getter_AddRefs (docMgr->GetCacheMgr (localPath));

	// 检查文件是否发生了修改。
	ncCacheInfo info;
	if (!cacheMgr->GetInfoByPath (pathMgr->GetRelativePath (path), info) ||
		info._lastModified == SystemFile::getLastModified (path))
		return;

	nsCOMPtr<ncSyncScheduler> scheduler = getter_AddRefs (
		reinterpret_cast<ncSyncScheduler*> (ncSyncMgr::getInstance ()->GetScheduler ()));

	// 添加文件编辑任务。
	nsCOMPtr<ncISyncTask> task = NC_NEW (syncMgrAlloc, ncUpEditFileTask) (
		taskSrc, localPath, path);
	scheduler->AddTask (task, ncSyncScheduler::NC_TP_LOW);
}

// public static
bool ncTaskUtils::RestoreDocIdChanged (const String & path, const String & docId, bool isDir /*= false*/)
{
	nsCOMPtr<ncIDocMgr> docMgr = getter_AddRefs (ncSyncMgr::getInstance ()->GetDocMgr ());
	nsCOMPtr<ncIADSMgr> adsMgr = getter_AddRefs (docMgr->GetADSMgr ());

	// 已存在的docId数据流
	String existDocId;
	try {
		if (!adsMgr->GetDocIdInfo (path, existDocId) || existDocId != docId) {

			if (!isDir) {
				// 设置前先记录文件的时间属性，设置后还原这些时间属性，避免Excel等
				// 程序自身内部会去校验时间信息，若不一致则会提示文件已被他人修改。
				ncScopedHandle handle = CreateFile (path.getCStr (), FILE_WRITE_ATTRIBUTES,
					FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0, OPEN_EXISTING, 0, 0);

				FILE_BASIC_INFO fileInfo;
				GetFileInformationByHandleEx (handle, FileBasicInfo, &fileInfo, sizeof(fileInfo));
				adsMgr->SetDocIdInfo (path, docId);
				SetFileInformationByHandle (handle, FileBasicInfo, &fileInfo, sizeof(fileInfo));
			}
			else
				adsMgr->SetDocIdInfo (path, docId);

			return true;
		}
	}
	catch (Exception& e) {
		SystemLog::getInstance ()->log (ERROR_LOG_TYPE, e.toFullString ().getCStr ());
	}

	return false;
}

// public static
bool ncTaskUtils::IsFileOccupied (const String& path, DWORD dwLockedAccess, int checLastTime, ncTaskStatus& status)
{
	NC_SYNCMGR_TRACE (_T("path: %s"), path.getCStr ());
	NC_SYNCMGR_CHECK_PATH_INVALID (path);

	checLastTime *= 2;
	// 如果文件在一段时间内（checLastTime秒）都不能被读取，则认为被其他程序独占。
	for (int i = 0; i <  checLastTime && !ncSyncMgr::getInstance ()->IsStop (); ++i) {
		if (!ncFileUtil::IsFileLocked (path.getCStr (), dwLockedAccess) || NC_TS_EXECUTING != status)
			return false;
		Thread::sleep (500);
	}

	return true;
}

void ncTaskUtils::ClearFileCache(ncTaskSrc taskSrc, const String& relPath )
{
	nsCOMPtr<ncSyncScheduler> scheduler = getter_AddRefs (reinterpret_cast<ncSyncScheduler*> (ncSyncMgr::getInstance ()->GetScheduler ()));

	nsCOMPtr<ncISyncTask> task = NC_NEW (syncMgrAlloc, ncLocalCleanFileTask) (
		taskSrc, relPath);
	scheduler->AddTask (task, ncSyncScheduler::NC_TP_LOW);
}

String ncTaskUtils::GetTopDirPath( const String& relPath )
{
	String result (relPath.beforeFirst (Path::separator));

	nsCOMPtr<ncIDocMgr> docMgr = getter_AddRefs (ncSyncMgr::getInstance ()->GetDocMgr ());
	
	if (docMgr->IsNewView()) {
		result = Path::combine(result, relPath.afterFirst (Path::separator).beforeFirst(Path::separator));
	}

	return result;
}

// public static
bool ncTaskUtils::GetFileLabelInfo(const String filePath, const String docId, String& securityInfo)
{
	NC_SYNCMGR_TRACE (_T("path: %s, docId: %s"), filePath.getCStr (), docId.getCStr ());
	NC_SYNCMGR_CHECK_PATH_INVALID (filePath);

	// 创建共享内存空间来给两个进程通信传输数据
	String fileName = Path::getFileName (filePath);
	fileName.replace (_T(' '), _T('|'), true);
	String mapName = _T("Local\\") + docId + fileName;
	int64 mapSize = 1024 * 1024;

	HANDLE hMapFile = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		0,
		mapSize,
		mapName.getCStr());
	if (hMapFile == NULL){
		DWORD dwErrorId = GetLastError();
		String err;
		err.format(_T("__FILE__ = %s, __LINE__ = %d, GetLastError = %d"), String(__FILE__).getCStr(), __LINE__, dwErrorId);
		SystemLog::getInstance ()->log (ERROR_LOG_TYPE, err.getCStr ());
		return false;
	}

	// 工具地址
	String appPath = Path::combine (Path::getCurrentModulePath (), _T("getThirdSecurity.exe"));

	// 定义传入参数(修复32位系统拼接字符串无法拼接四个参数)
	String tempStr;
	tempStr.format(_T("-thirdSecurity %s %d"), mapName.getCStr(), mapSize);
	String cmdLine;
	cmdLine.format (_T("%s %s"), tempStr.getCStr(), filePath.getCStr());

	// 执行文件操作
	SHELLEXECUTEINFO sei = { sizeof (SHELLEXECUTEINFO) };
	sei.lpFile = appPath.getCStr ();
	sei.lpParameters = cmdLine.getCStr ();
	sei.nShow = SW_HIDE;
	sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC;

	if (!ShellExecuteEx (&sei)) {
		DWORD dwErrorId = GetLastError();
		String err;
		err.format(_T("__FILE__ = %s, __LINE__ = %d, GetLastError = %d"), String(__FILE__).getCStr(), __LINE__, dwErrorId);
		SystemLog::getInstance ()->log (ERROR_LOG_TYPE, err.getCStr ());
		CloseHandle(hMapFile);
		return false;
	}

	// 等待工具进程执行完毕
	WaitForSingleObject (sei.hProcess, INFINITE);

	// 从共享内存获取工具进程返回数据
	LPCTSTR pBuf = (LPTSTR) MapViewOfFile(hMapFile,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		mapSize);

	String security (pBuf);

	UnmapViewOfFile(pBuf);
	CloseHandle(hMapFile);
	
	securityInfo = security;

	JSON::Value value;
	JSON::Reader::read(value, toSTLString(security).c_str(), toSTLString(security).length());

	return value["ret"].i();
}

// public static
void ncTaskUtils::SetThirdSecurityInfo (ncIEFSPClient* efspClient, const String& appId, const String& docId, const String& securityInfo)
{
	nsCOMPtr<ncIEFSPFileOperation> efspFileOperation = getter_AddRefs (efspClient->CreateFileOperation ());

	SetThirdSecurityInfoEx (efspFileOperation, appId, docId, securityInfo);
}

// public static
void ncTaskUtils::SetThirdSecurityInfoEx (ncIEFSPFileOperation* efspFileOperation, const String& appId, const String& docId, const String& securityInfo)
{
	JSON::Object tempObj;
	JSON::Value value;
	JSON::Reader::read(value, toSTLString(securityInfo).c_str(), toSTLString(securityInfo).length());
	tempObj = value.o();
	String objKey = _T("ret");
	tempObj.erase(toSTLString(objKey));

	string content;

	if (!tempObj.empty()) {
		content.reserve (64);
		JSON::Writer::write(tempObj, content);
	}

	String key = _T("classification_info");
	map<String, String> metaDataMap;
	metaDataMap.insert(make_pair(key, toCFLString(content)));
	efspFileOperation->SetAppMetaData (docId, appId, metaDataMap);
}

// public static
bool ncTaskUtils::isSecurityInfoEmpty (const String& securityInfo)
{
	JSON::Object tempObj;
	JSON::Value value;
	JSON::Reader::read(value, toSTLString(securityInfo).c_str(), toSTLString(securityInfo).length());
	tempObj = value.o();
	String objKey = _T("ret");
	tempObj.erase(toSTLString(objKey));

	return tempObj.empty ();
}

void ncTaskUtils::TaskStatusCheck( ncTaskStatus& status )
{
	if (NC_TS_CANCELD == status)
		THROW_E (_T("EFSPClient"), 0x0000000DL, LOAD_STRING (_T("IDS_USER_CANCELLED")).getCStr ());
	else if (NC_TS_PAUSED == status)
		THROW_E (_T("EFSPClient"), 0x00000011L, _T(""));
}

void ncTaskUtils::GetThirdSecurityLevel (map<int, String>csfMapEnum, String securityInfo, int& csf)
{
	JSON::Object tempObj;
	JSON::Value value;
	JSON::Reader::read(value, toSTLString(securityInfo).c_str(), toSTLString(securityInfo).length());

	if (JSON::Type::NIL != value["zh-cn"].type ()) {
		tempObj = value["zh-cn"];
		JSON::Object::iterator it = tempObj.begin();
		String csfLevel = toCFLString(it->second);

		for (map<int, String>::const_iterator it = csfMapEnum.begin (); it != csfMapEnum.end (); ++it) {
			if (it->second == csfLevel) {
				csf = it->first;
				break;
			}
		}
	}
	else
		csf = 0x7FFF;
}



HRESULT ncTaskUtils::MutiSHCopyMove( vector<String>& srcPaths, const String& dstPath, int wFunc, int fFlags )
{
	if (!SystemFile::isDirectory (dstPath) && ~fFlags & FOF_NOCONFIRMMKDIR ) {
		Directory (dstPath).create ();
	}

	SHFILEOPSTRUCT fileOp = { 0 };
	fileOp.wFunc = wFunc;	
	fileOp.fFlags = fFlags;

	String sourceDir = String::EMPTY;
	for (vector<String>::iterator iter = srcPaths.begin (); iter != srcPaths.end (); ++iter) {
		sourceDir += *iter;
		sourceDir.append (_T('\0'), 1);
	}
	fileOp.pFrom = sourceDir.getCStr ();

	String destinationPath = dstPath;
	destinationPath.append (_T('\0'), 1);
	fileOp.pTo = destinationPath.getCStr ();

	return SHFileOperation (&fileOp);
}

HRESULT ncTaskUtils::SHCopyMove( const String& srcPath, const String& dstPath, int wFunc, int fFlags )
{
	SHFILEOPSTRUCT fileOp = { 0 };
	fileOp.wFunc = wFunc;	
	fileOp.fFlags = fFlags;

	String sourceDir = srcPath;
	sourceDir.append (_T('\0'), 1);
	fileOp.pFrom = sourceDir.getCStr ();

	String destinationPath = dstPath;
	destinationPath.append (_T('\0'), 1);
	fileOp.pTo = destinationPath.getCStr ();

	return SHFileOperation (&fileOp);
}

// dirPath、dstPath 绝对路径
void ncTaskUtils::TransferUnsyncInDir( const String& localPath, const String& dirPath, const String& dstPath, bool isMove, bool needExpand /*= true*/ )
{
	nsCOMPtr<ncIDocMgr> docMgr = getter_AddRefs (ncSyncMgr::getInstance ()->GetDocMgr ());
	nsCOMPtr<ncICacheMgr> cacheMgr = getter_AddRefs (docMgr->GetCacheMgr (String::EMPTY));
	nsCOMPtr<ncIPathMgr> pathMgr = getter_AddRefs (docMgr->GetPathMgr (localPath));

	vector<ncUnsyncInfo> unsyncInfos;
	cacheMgr->Unsync_GetInfoVec (dirPath, unsyncInfos, 0);

	if (!unsyncInfos.size())
		return;

	int rootOffset = (int) dirPath.getLength ();
	String relPath;
	map<String, String> pathMap;			// map<dst, src>
	for (vector<ncUnsyncInfo>::iterator it = unsyncInfos.begin (); it != unsyncInfos.end (); ++it) {
		relPath = it->_absPath.subString (rootOffset).trim (Path::separator);
		pathMap.insert (make_pair (it->_absPath, Path::combine (dstPath, Path::getDirectoryName (relPath))));
	}

	// start move
	nsCOMPtr<ncSyncRequester> requester = getter_AddRefs (
		reinterpret_cast<ncSyncRequester*> (ncSyncMgr::getInstance ()->GetRequester ()));
	nsCOMPtr<ncSyncScheduler> scheduler = getter_AddRefs (
		reinterpret_cast<ncSyncScheduler*> (ncSyncMgr::getInstance ()->GetScheduler ()));

	int taskId;
	String sameDst;
	vector<String> srcPaths;
	map<String, String>::iterator ite = pathMap.begin ();
	while (ite != pathMap.end ()) {
		srcPaths.clear ();
		srcPaths.push_back (ite->first);
		sameDst = ite->second;
		while (++ite != pathMap.end () && ite->second == sameDst) {
			srcPaths.push_back (ite->first);
		}

		if (needExpand) {
			// 展开目录结构
			Sleep (1000);			// 云端复制移动，后台完整完成需要时间。若太快List，不完整。完全优化需要后端配合，优先整合目录结构，再移动数据。
			taskId = requester->RequestDownloadDir (localPath, pathMgr->GetRelativePath (sameDst), true);
			scheduler->WaitTaskDone (taskId);
		}

		MutiSHCopyMove (srcPaths, sameDst, isMove ? FO_MOVE : FO_COPY, FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR);
	}

	// clean ADS and database records
	String destPath;
	for (ite = pathMap.begin (); ite != pathMap.end (); ++ ite)
	{
		if (isMove && SystemFile::getFileAttrs (ite->first) == INVALID_FILE) {
			cacheMgr->Unsync_Delete (ite->first, false);
		}
		
		destPath = Path::combine (ite->second, Path::getFileName (ite->first));
		ncADSUtil::RemoveADS (destPath.getCStr (), ADS_ERRMARK);
	}
}
