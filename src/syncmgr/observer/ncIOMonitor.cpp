/***************************************************************************************************
Purpose:
	Source file for class ncIOMonitor.

Author:
	Chaos

Created Time:
	2016-8-19
***************************************************************************************************/

#include <abprec.h>
#include <docmgr/public/ncIDocMgr.h>
#include <docmgr/public/ncIPathMgr.h>
#include <syncmgr/syncmgr.h>
#include "ncIONotifier.h"
#include "ncIOMonitor.h"

#define NOTIFY_FILTER														\
	(FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE)

// public ctor
ncIOMonitor::ncIOMonitor (ncIDocMgr* docMgr,
						  ncIChangeListener* chgListener,
						  String localSyncPath)
	: _stop (true)
	, _hDir (INVALID_HANDLE_VALUE)
	, _hCompPort (0)
	, _buf (0)
	, _notifier (0)
{
	NC_SYNCMGR_TRACE (_T("docMgrr: 0x%p, chgListener: 0x%p"),
					  docMgr, chgListener);

	NC_SYNCMGR_CHECK_ARGUMENT_NULL (docMgr);
	NC_SYNCMGR_CHECK_ARGUMENT_NULL (chgListener);

	::memset (&_overlapped, 0, sizeof (OVERLAPPED));
	_buf = NC_NEW_ARRAY_PT (syncMgrAlloc, unsigned char, _bufSize);

	_dirPath = docMgr->GetPathMgr (localSyncPath)->GetRootPath ();
	_notifier = new ncIONotifier (docMgr, chgListener, localSyncPath);
}

// public dtor
ncIOMonitor::~ncIOMonitor ()
{
	NC_SYNCMGR_TRACE (_T("this: 0x%p"), this);

	if (!_stop)
		Stop ();

	NC_DELETE_ARRAY_PT (syncMgrAlloc, unsigned char, _buf);
	ThreadCleanManager::getInstance ()->addCleanThread (_notifier);
}

// public
void ncIOMonitor::Start ()
{
	NC_SYNCMGR_TRACE (_T("this: 0x%p"), this);

	InterlockedExchange (&_stop, false);
	SetUp ();

	_notifier->Start ();
	this->start ();
}

// public
void ncIOMonitor::Stop ()
{
	NC_SYNCMGR_TRACE (_T("this: 0x%p"), this);

	InterlockedExchange (&_stop, true);
	TearDown ();

	if (!_notifier)
		_notifier->Stop ();
}

// public
void ncIOMonitor::run ()
{
	NC_SYNCMGR_TRACE (_T("this: 0x%p, -begin"), this);

	list<ncChangeInfo> changes;

	while (!_stop) {
		try {
			// ��ȡ�仯��
			GetChanges (changes);

			if (_stop)
				break;

			// ֪ͨ�仯��
			_notifier->AddChanges (changes);
		}
		catch (Exception& e) {
			SystemLog::getInstance ()->log (ERROR_LOG_TYPE, e.toFullString ().getCStr ());
		}
		catch (...) {
			SystemLog::getInstance ()->log (ERROR_LOG_TYPE, _T("ncIOMonitor::run () failed."));
		}
	}

	NC_SYNCMGR_TRACE (_T("this: 0x%p, -end"), this);
}

// public
void ncIOMonitor::GetChanges (list<ncChangeInfo>& changes)
{
	NC_SYNCMGR_TRACE (_T("this: 0x%p"), this);

	changes.clear ();

	String path;
	String newPath;
	DWORD dwNumBytes;
	HANDLE hDir;
	LPOVERLAPPED lpOverlapped;

	// ��ȡ�ļ��仯��
	if (!ReadDirectoryChangesW (_hDir,
								_buf,
								_bufSize,
								TRUE,
								NOTIFY_FILTER,
								&dwNumBytes,
								&_overlapped,
								NULL)) {
		goto FAILED;
	}

	// �첽�����ļ��仯��
	if (!GetQueuedCompletionStatus(_hCompPort,
								   &dwNumBytes,
								   (PULONG_PTR)&hDir,
								   &lpOverlapped,
								   INFINITE)) {
		goto FAILED;
	}

	// ����Ƿ�ֹͣ��أ����������Ч�仯����PostQueuedCompletionStatus��������
	if (_stop)
		return;

	FILE_NOTIFY_INFORMATION* pNotify = (FILE_NOTIFY_INFORMATION*)_buf;

	// �����ļ��仯��
	while (pNotify) {
		path = Path::combine (_dirPath, String (pNotify->FileName, pNotify->FileNameLength / sizeof(WCHAR)));
		// ���ڻ�ȡ�Ĳ���BUFF�Ƿֿ���ȡ�ģ����ܵ���FILE_ACTION_RENAMED_OLD_NAME��FILE_ACTION_RENAMED_NEW_NAME����һ��BUFFE��
		// ����ϴδ���δƥ��������������ƣ���ֱ��ƥ��
		if (!_renameOldPath.isEmpty()) {
			FILE_NOTIFY_INFORMATION* pTmp = (PFILE_NOTIFY_INFORMATION)(LPBYTE)pNotify;
			while (pTmp && pTmp->Action != FILE_ACTION_RENAMED_NEW_NAME) {
				if (pTmp->NextEntryOffset == 0) {
					break;			
				}
				pTmp = (PFILE_NOTIFY_INFORMATION)((LPBYTE)pTmp + pTmp->NextEntryOffset);
			}
			if (pTmp && pTmp->Action == FILE_ACTION_RENAMED_NEW_NAME) {
				newPath = Path::combine (_dirPath, String (pTmp->FileName, pTmp->FileNameLength / sizeof(WCHAR)));
				changes.push_back (ncChangeInfo (ACTION_RENAME, _renameOldPath, newPath));
				_renameOldPath = String::EMPTY;
			}
		}

		if (pNotify->Action == FILE_ACTION_ADDED) {
			changes.push_back (ncChangeInfo (ACTION_CREATE, path));
		}
		else if (pNotify->Action == FILE_ACTION_MODIFIED) {
			changes.push_back (ncChangeInfo (ACTION_EDIT, path));
		}
		else if (pNotify->Action == FILE_ACTION_REMOVED) {
			changes.push_back (ncChangeInfo (ACTION_DELETE, path));
		}
		// ����FILE_ACTION_RENAMED_OLD_NAME��FILE_ACTION_RENAMED_NEW_NAME֮����ܲ�������������������Ҫ��������һ�Խ���ƥ��
		else if (pNotify->Action == FILE_ACTION_RENAMED_OLD_NAME) {
			FILE_NOTIFY_INFORMATION* pTmp = (PFILE_NOTIFY_INFORMATION)((LPBYTE)pNotify + pNotify->NextEntryOffset);
			while (pTmp && pTmp->Action != FILE_ACTION_RENAMED_NEW_NAME) {
				// ����Ѿ������һ���ˣ�����û��ƥ�䣬���¾�·��������ѭ����
				if (pTmp->NextEntryOffset == 0) {
					_renameOldPath = path;
					break;			
				}
				pTmp = (PFILE_NOTIFY_INFORMATION)((LPBYTE)pTmp + pTmp->NextEntryOffset);
			}
			if (pTmp && pTmp->Action == FILE_ACTION_RENAMED_NEW_NAME) {
				newPath = Path::combine (_dirPath, String (pTmp->FileName, pTmp->FileNameLength / sizeof(WCHAR)));
				changes.push_back (ncChangeInfo (ACTION_RENAME, path, newPath));
			}
		}

		if (pNotify->NextEntryOffset == 0)
			break;

		pNotify = (PFILE_NOTIFY_INFORMATION)((LPBYTE)pNotify + pNotify->NextEntryOffset);
	}

	return;

FAILED:
	if (!_stop) {
		DWORD dwError = GetLastError ();
		String errMsg;
		errMsg.format (syncMgrLoader, _T("IDS_MONITOR_CHANGE_ERR"), _dirPath.getCStr ());
		NC_SYNCMGR_THROW_WSC (errMsg, SYNCMGR_MONITOR_CHANGE_ERR, dwError);
	}
}

// protected
void ncIOMonitor::SetUp ()
{
	NC_SYNCMGR_TRACE (_T("this: 0x%p"), this);

	//
	// 1.�򿪴����Ŀ¼�ľ����
	// 2.��������Ŀ¼��������ɶ˿ڣ����ڽ����ļ��仯��
	//
	_hDir = CreateFile (_dirPath.getCStr(),
						FILE_LIST_DIRECTORY,
						FILE_SHARE_READ | FILE_SHARE_WRITE,
						NULL,
						OPEN_EXISTING,
						FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
						NULL);
	if (_hDir == INVALID_HANDLE_VALUE)
		goto FAILED;

	_hCompPort = CreateIoCompletionPort (_hDir, NULL, (ULONG_PTR)_hDir, 0);
	if (_hCompPort == NULL)
		goto FAILED;

	return;

FAILED:
	DWORD dwError = GetLastError ();
	String errMsg;
	errMsg.format (syncMgrLoader, _T("IDS_SETUP_MONITOR_ERR"), _dirPath.getCStr ());
	NC_SYNCMGR_THROW_WSC (errMsg, SYNCMGR_SETUP_MONITOR_ERR, dwError);
}

// protected
void ncIOMonitor::TearDown ()
{
	NC_SYNCMGR_TRACE (_T("this: 0x%p"), this);

	//
	// 1.֪ͨ��ɶ˿����������ȴ������ر���ɶ˿ڡ�
	// 2.�رռ��Ŀ¼�ľ����
	//
	if (_hCompPort != NULL) {
		PostQueuedCompletionStatus (_hCompPort, 0, NULL, NULL);
		CloseHandle (_hCompPort);
		_hCompPort = NULL;
	}

	if (_hDir != INVALID_HANDLE_VALUE) {
		CloseHandle (_hDir);
		_hDir = INVALID_HANDLE_VALUE;
	}
}
