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
	ncChangeAction		_action;			// 具体变化动作
	String				_path;				// 文档路径
	String				_newPath;			// 文档新路径

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
	// 开始监控。
	void Start ();

	// 停止监控。
	void Stop ();

	void run ();

public:
	// 获取监控到的变化。
	void GetChanges (list<ncChangeInfo>& changes);

protected:
	// 建立监控。
	void SetUp ();

	// 解除监控。
	void TearDown ();

private:
	AtomicValue			_stop;				// 是否停止监控
	String				_dirPath;			// 监控目录路径
	String				_renameOldPath;		// 未匹配成功重命名原路径

	HANDLE				_hDir;				// 监控目录句柄
	HANDLE				_hCompPort;			// 完成端口句柄
	OVERLAPPED			_overlapped;		// 异步IO数据

	static const int	_bufSize = 65536;	// 缓存大小：64KB
	unsigned char*		_buf;				// 存储变化的缓存空间

	ncIONotifier*		_notifier;			// 变化通知对象
};

#endif // __NC_IO_MONITOR_H__
