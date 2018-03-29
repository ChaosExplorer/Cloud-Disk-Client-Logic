/***************************************************************************************************
ncDBMgr.h:

Purpose:
	Header file for class ncDBMgr.

Author:
	Chaos

Created Time:
	2015-09-10
***************************************************************************************************/

#ifndef __NC_DB_MGR_H__
#define __NC_DB_MGR_H__

#if PRAGMA_ONCE
	#pragma once
#endif

#include <sqlite3.h>
#include "private/ncIDBMgr.h"
#include "ncDocUtils.h"
#include "docmgr.h"

//
// 数据库管理对象。
//
class ncDBMgr : public ncIDBMgr
{
public:
	NS_DECL_ISUPPORTS
	NS_DECL_NCIDBMGR

	ncDBMgr ();
	virtual ~ncDBMgr();

public:
	void GetTable (const String& sql, int& row, int& col, char**& ppRet);

	// 对单引号进行转义处理。
	static String DealSingleQuote (const String& str)
	{
		return ncDocUtils::ReplaceStr (str, _T("'"), _T("''"), true);
	}

private:
	sqlite3*				_db;
	ThreadMutexLock			_lock;
};

//
// 智能数据库查询对象。
//
class ncAutoGetTable
{
	ncAutoGetTable (const ncAutoGetTable&);
	ncAutoGetTable& operator = (const ncAutoGetTable&);

public:
	ncAutoGetTable (ncDBMgr* dbMgr, const String& sql, int& row , int& col, char**& ppRet)
		: _ppRet (0)
	{
		NC_DOCMGR_CHECK_ARGUMENT_NULL (dbMgr);

		dbMgr->GetTable (sql, row, col, ppRet);
		_ppRet = ppRet;
	}

	virtual ~ncAutoGetTable ()
	{
		if(_ppRet) {
			sqlite3_free_table (_ppRet);
		}
	}

private:
	char**				_ppRet;
};

#endif  //__NC_DB_MGR_H__
