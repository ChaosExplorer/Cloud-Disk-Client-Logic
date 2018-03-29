/***************************************************************************************************
Purpose:
	Header file for class ncIOMonitor.

Author:
	Chaos

Created Time:
	2016-8-19
***************************************************************************************************/

#ifndef __NC_IO_MONITOR_H__
#define __NC_IO_MONITOR_H__

#if PRAGMA_ONCE
	#pragma once
#endif

class ncIDocMgr;
class ncIChangeListener;
class ncIONotifier;

enum ncChangeAction
{
	ACTION_CREATE,
	ACTION_EDIT,
	ACTION_DELETE,
	ACTION_RENAME,
};

struct ncChangeInfo
{
	ncChangeAction		_action;			// ����仯����
	String				_path;				// �ĵ�·��
	String				_newPath;			// �ĵ���·��

	ncChangeInfo (ncChangeAction action,
				  const String& path,
				  const String& newPath = String::EMPTY)
		: _action (action)
		, _path (path)
		, _newPath  (newPath)
	{
	}

	bool operator== (const ncChangeInfo change)
	{
		return (_action == change._action &&
				_path == change._path &&
				_newPath == change._newPath);
	}
};

class ncIOMonitor : public Thread
{
public:
	ncIOMonitor (ncIDocMgr* docMgr,
				 ncIChangeListener* chgListener,
				 String localSyncPath);
	virtual ~ncIOMonitor ();

public:
	// ��ʼ��ء�
	void Start ();

	// ֹͣ��ء�
	void Stop ();

	void run ();

public:
	// ��ȡ��ص��ı仯��
	void GetChanges (list<ncChangeInfo>& changes);

protected:
	// ������ء�
	void SetUp ();

	// �����ء�
	void TearDown ();

private:
	AtomicValue			_stop;				// �Ƿ�ֹͣ���
	String				_dirPath;			// ���Ŀ¼·��
	String				_renameOldPath;		// δƥ��ɹ�������ԭ·��

	HANDLE				_hDir;				// ���Ŀ¼���
	HANDLE				_hCompPort;			// ��ɶ˿ھ��
	OVERLAPPED			_overlapped;		// �첽IO����

	static const int	_bufSize = 65536;	// �����С��64KB
	unsigned char*		_buf;				// �洢�仯�Ļ���ռ�

	ncIONotifier*		_notifier;			// �仯֪ͨ����
};

#endif // __NC_IO_MONITOR_H__
