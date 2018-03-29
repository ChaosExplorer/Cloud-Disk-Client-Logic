/***************************************************************************************************
ncDBMgr.cpp:

Purpose:
	Source file for class ncDBMgr.

Author:
	Chaos

Created Time:
	2015-09-10
***************************************************************************************************/

#include <abprec.h>
#include "ncDBMgr.h"
#include "docmgr.h"

NC_UMM_IMPL_THREADSAFE_ISUPPORTS1 (docMgrAlloc, ncDBMgr, ncIDBMgr)

// public
ncDBMgr::ncDBMgr ()
	: _db (0)
{
	NC_DOCMGR_TRACE (_T("this: 0x%p"), this);
}

// public
ncDBMgr::~ncDBMgr ()
{
	NC_DOCMGR_TRACE (_T("this: 0x%p"), this);

	Close ();
}

/*[notxpcom] void Open ([const] in StringRef path);*/
NS_IMETHODIMP_(void) ncDBMgr::Open (const String& path)
{
	NC_DOCMGR_TRACE (_T("path: %s"), path.getCStr ());
	NC_DOCMGR_CHECK_PATH_INVALID (path);

	int result = sqlite3_open (toSTLString (path).c_str (), &_db);

	if (SQLITE_OK != result) {
		String errMsg;
		errMsg.format (_T("Open database failed. Path: %s. Error: %d"), path.getCStr (), result);
		NC_DOCMGR_THROW (errMsg, DOCMGR_INTERNAL_ERR);
	}

	//
	// 设置sqlite参数以处理并发访问和提升性能： //linzimo777.blog.51cto.com/5334766/1546802
	// 注意保持对应的设置顺序，否则可能导致某些设置无法正确执行的问题。
	// 1.设置数据库并发访问的最长等待时间为5秒。
	// 2.设置数据库缓存大小为8000（默认为2000），以提升性能。
	// 3.设置数据库日志模式为WAL，以提升性能。
	// 4.设置数据库WAL回写检查点为10000（默认为1000），以提升性能。
	//
	ExecuteSql (_T("PRAGMA busy_timeout=300000;"));
	ExecuteSql (_T("PRAGMA cache_size=8000;"));
	ExecuteSql (_T("PRAGMA journal_mode=WAL;"));
	ExecuteSql (_T("PRAGMA wal_autocheckpoint=10000;"));
}

/*[notxpcom] void Close ();*/
NS_IMETHODIMP_(void) ncDBMgr::Close ()
{
	NC_DOCMGR_TRACE (_T("this: 0x%p"), this);

	if (_db) {
		int result = sqlite3_close (_db);
		_db = 0;

		if (SQLITE_OK != result) {
			String errMsg;
			errMsg.format (_T("Close database failed. Error: %d"), result);
			NC_DOCMGR_THROW (errMsg, DOCMGR_INTERNAL_ERR);
		}
	}
}

/*[notxpcom] void ExecuteSql ([const] in StringRef sql);*/
NS_IMETHODIMP_(void) ncDBMgr::ExecuteSql (const String& sql)
{
	NC_DOCMGR_TRACE (_T("sql: %s"), sql.getCStr ());

	char* err = 0;
	int result;

	result = sqlite3_exec (_db, toSTLString(sql).c_str (), 0, 0, &err);

	if (SQLITE_OK != result) {
		String errMsg;
		errMsg.format (_T("Execute sql failed. Sql: %s. Error: %s"), sql.getCStr (), String(err).getCStr ());
		sqlite3_free (err);
		NC_DOCMGR_THROW (errMsg, DOCMGR_INTERNAL_ERR);
	}
}

/*[notxpcom] bool IsColumnExists ([const] in StringRef tableName, [const] in StringRef columnName);*/
NS_IMETHODIMP_(bool) ncDBMgr::IsColumnExists (const String& tableName, const String& columnName)
{
	NC_DOCMGR_TRACE (_T("tableName: %s, columnName: %s"), tableName.getCStr (), columnName.getCStr ());

	char** ppRet = 0;
	int row = 0, col = 0;

	String sql;
	sql.format(_T("select * from sqlite_master where name='%s' COLLATE NOCASE and sql like '%%%s%%' limit 1"), 
			tableName.getCStr (),
			columnName.getCStr ());

	ncAutoGetTable autoGetTable(this, sql, row, col, ppRet);
	return row != 0;
}

/*[notxpcom] bool IsTableExists ([const] in StringRef tableName);*/
NS_IMETHODIMP_(bool) ncDBMgr::IsTableExists (const String& tableName)
{
	NC_DOCMGR_TRACE (_T("tableName: %s"), tableName.getCStr ());

	char** ppRet = 0;
	int row = 0, col = 0;

	String sql;
	sql.format(_T("select * from sqlite_master where type='table' and name='%s' COLLATE NOCASE limit 1"), tableName.getCStr ());

	ncAutoGetTable autoGetTable(this, sql, row, col, ppRet);
	return row != 0;
}

/*[notxpcom] void BeginTransaction ();*/
NS_IMETHODIMP_(void) ncDBMgr::BeginTransaction ()
{
	NC_DOCMGR_TRACE (_T("this: 0x%p"), this);

	_lock.lock ();
	ExecuteSql (_T("begin transaction"));
}

/*[notxpcom] void CommitTransaction ();*/
NS_IMETHODIMP_(void) ncDBMgr::CommitTransaction ()
{
	NC_DOCMGR_TRACE (_T("this: 0x%p"), this);

	ExecuteSql (_T("commit transaction"));
	_lock.unlock ();
}

/*[notxpcom] void RollbackTransaction ();*/
NS_IMETHODIMP_(void) ncDBMgr::RollbackTransaction ()
{
	NC_DOCMGR_TRACE (_T("this: 0x%p"), this);

	ExecuteSql (_T("rollback transaction"));
	_lock.unlock ();
}

// public
void ncDBMgr::GetTable (const String& sql, int& row, int& col, char**& ppRet)
{
	NC_DOCMGR_TRACE (_T("sql: %s"), sql.getCStr ());

	char* err = 0;
	int result;

	result = sqlite3_get_table (_db, toSTLString (sql).c_str (), &ppRet, &row, &col, &err);

	if (SQLITE_OK != result) {
		String errMsg;
		errMsg.format (_T("Get table failed. Sql: %s. Error: %s"), sql.getCStr (), String (err).getCStr ());
		sqlite3_free (err);
		NC_DOCMGR_THROW (errMsg, DOCMGR_INTERNAL_ERR);
	}
}
