/***************************************************************************************************
ncIRelayMgr.h:

Purpose:
	上传下载断点管理接口文件。

Author:
	Chaos

Created Time:
	2016-09-01
***************************************************************************************************/

#ifndef __NC_RELAY_MGR_H__
#define __NC_RELAY_MGR_H_

#if PRAGMA_ONCE
	#pragma once
#endif

#include "public/ncIRelayMgr.h"

class ncDBMgr;

#define NONIUS_DB					_T("nonius.db")
#define UNONIUS_TABLE				_T("tb_upload_nonius")
#define DNONIUS_TABLE				_T("tb_download_nonius")

class ncRelayMgr : public ncIRelayMgr
{
public:
	NS_DECL_ISUPPORTS
	NS_DECL_NCIRELAYMGR

	ncRelayMgr ();
	virtual ~ncRelayMgr ();

public:
	void Init (const String& path);
	void Close ();

private:
	nsCOMPtr<ncDBMgr>		_dbMgr;
};

#endif // __NC_RELAY_MGR_H_
