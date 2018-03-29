/***************************************************************************************************
ncIRelayMgr.cpp:

Purpose:
	�ϴ����ضϵ����ӿ��ļ���

Author:
	Chaos

Created Time:
	2016-09-01
***************************************************************************************************/

#include <abprec.h>
#include "ncDBMgr.h"
#include "ncRelayMgr.h"
#include "docmgr.h"

NC_UMM_IMPL_THREADSAFE_ISUPPORTS1(docMgrAlloc, ncRelayMgr, ncIRelayMgr)

// public ctor
ncRelayMgr::ncRelayMgr ()
	: _dbMgr (0)
{
	NC_DOCMGR_TRACE (_T("this: 0x%p"), this);

	_dbMgr = NC_NEW (docMgrAlloc, ncDBMgr) ();
}

// public dtor
ncRelayMgr::~ncRelayMgr ()
{
	NC_DOCMGR_TRACE (_T("this: 0x%p"), this);

	Close ();
}

// public
void ncRelayMgr::Init (const String& path)
{
	NC_DOCMGR_TRACE (_T("path: %s"), path.getCStr ());
	NC_DOCMGR_CHECK_PATH_INVALID (path);

	//
	// �ϵ�����ݵİ�ȫ��Ҫ��ߣ�����ʹ��NORMALģʽ���������ܡ�
	//
	_dbMgr->Open (path);
	_dbMgr->ExecuteSql (_T("PRAGMA synchronous=NORMAL;"));

	// ��鲢�����ϴ��ϵ��
	String sql;
	sql.format (_T("create table if not exists %s (			\
					srcPath text PRIMARY KEY,					\
					destPath text,							\
					docId text,							\
					md5 text,							\
					crc32 text,							\
					sliceMd5 text,					\
					uploadId text,					\
					lastModified text,				\
					otag text,							\
					partSize text,						\
					blockSerial int,					\
					direcMode int,					\
					status int,							\
					taskSrc int,							\
					onDup int,							\
					delSrc int,							\
					ratios int,							\
					csf int)"),
				UNONIUS_TABLE);
	_dbMgr->ExecuteSql (sql);

	// ��������srcPath��destPath����������������srcPath��destPath�����ϲ�ѯЧ��
	String indexName;
	indexName.format (_T("%s_src_dest_path_idx"), UNONIUS_TABLE);
	sql.format (_T("create unique index if not exists %s on %s(srcPath, destPath)"),indexName.getCStr (), UNONIUS_TABLE);
	_dbMgr->ExecuteSql(sql);

	// ��鲢�������ضϵ��
	sql.format (_T("create table if not exists %s (			\
				   srcPath text PRIMARY KEY,					\
				   destPath text,							\
				   docId text,							\
				   lastModified text,				\
				   otag text,							\
				   docType int,						\
				   typeName text,					\
				   viewType int,						\
				   viewName text,					\
				   mtime text,							\
				   size text,								\
				   attr int,								\
				   range text,							\
				   direcMode int	,					\
				   status int,							\
				   taskSrc int,							\
				   delSrc int,							\
				   ratios int)"),
				   DNONIUS_TABLE);
	_dbMgr->ExecuteSql (sql);

	// ��������srcPath��destPath����������������srcPath��destPath�����ϲ�ѯЧ��
	indexName.format (_T("%s_src_dest_path_idx"), DNONIUS_TABLE);
	sql.format (_T("create unique index if not exists %s on %s(srcPath, destPath)"),indexName.getCStr (), DNONIUS_TABLE);
	_dbMgr->ExecuteSql(sql);
}

// public
void ncRelayMgr::Close ()
{
	NC_DOCMGR_TRACE (_T("this: 0x%p"), this);

	if (_dbMgr)
		_dbMgr->Close ();
}

/**
* �����ϴ��ϵ�
* @param absPath:	�ϴ��ļ�����·��
* @param uNonius:	�ϵ���Ϣ
* @throw exception:	����ʧ��
*/
/* [notxpcom] void UpdateUploadNonius ([const] in StringRef absPath, [const] in ncUploadNoniusRef uNonius); */
NS_IMETHODIMP_(void) ncRelayMgr::UpdateUploadNonius(const ncUploadNonius& uNonius)
{
	NC_DOCMGR_TRACE (_T("absPath: %s"), uNonius._srcPath.getCStr ());

	String sql;
	sql.format(_T("replace into %s(srcPath, destPath, docId, md5, crc32, sliceMd5, uploadId, lastModified, otag, partSize, blockSerial, direcMode, status, taskSrc, onDup, delSrc, ratios, csf)	\
				values('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d')"),
				UNONIUS_TABLE,
				ncDBMgr::DealSingleQuote (uNonius._srcPath).getCStr (),
				ncDBMgr::DealSingleQuote (uNonius._destPath).getCStr (),
				uNonius._docId.getCStr (),
				uNonius._md5.getCStr (),
				uNonius._crc32.getCStr (),
				uNonius._sliceMd5.getCStr (),
				uNonius._uploadId.getCStr (),
				Int64::toString (uNonius._lastModified).getCStr (),
				uNonius._otag.getCStr (),
				Int64::toString (uNonius._partSize).getCStr (),
				uNonius._blockSerial,
				uNonius._direcMode,
				uNonius._status,
				uNonius._taskSrc,
				uNonius._onDup,
				uNonius._delSrc,
				(int)(uNonius._ratios * 10000),
				uNonius._csf
				);

	_dbMgr->ExecuteSql (sql);
}

/**
* ��ȡ�ϴ��ϵ�
* @param absPath:	�ϴ��ļ�����·��
* @param uNonius:	�ϵ���Ϣ
* @return: �Ƿ��ȡ����Ч�ϵ�
* @throw exception:	��ȡʧ��
*/
/* [notxpcom] bool GetUploadNoniusByPath ([const] in StringRef absPath, in ncUploadNoniusRef uNonius); */
NS_IMETHODIMP_(bool) ncRelayMgr::GetUploadNoniusByPath(const String& absPath, ncUploadNonius& uNonius)
{
	NC_DOCMGR_TRACE (_T("absPath: %s"), absPath.getCStr ());

	String sql;
	sql.format(_T("select srcPath, destPath, docId, md5, crc32, sliceMd5, uploadId, lastModified, otag, partSize, blockSerial, direcMode, status, taskSrc, onDup, delSrc, ratios, csf	\
				  from %s where srcPath='%s' limit 1"),
				  UNONIUS_TABLE,
				  ncDBMgr::DealSingleQuote (absPath).getCStr ());

	char** ppRet = 0;
	int row = 0, col = 0;
	ncAutoGetTable autoGetTable (_dbMgr, sql, row, col, ppRet);

	if (row) {
		uNonius._srcPath = absPath;
		uNonius._destPath = ::toCFLString (ppRet[row * col + 1]);
		uNonius._docId = ::toCFLString (ppRet[row * col + 2]);
		uNonius._md5 = ::toCFLString (ppRet[row * col + 3]);
		uNonius._crc32 = ::toCFLString (ppRet[row * col + 4]);
		uNonius._sliceMd5 = ::toCFLString (ppRet[row * col + 5]);
		uNonius._uploadId = ::toCFLString (ppRet[row * col + 6]);
		uNonius._lastModified = Int64::getValue (::toCFLString (ppRet[row * col + 7]));
		uNonius._otag = ::toCFLString (ppRet[row * col + 8]);
		uNonius._partSize = Int64::getValue (::toCFLString (ppRet[row * col + 9]));
		uNonius._blockSerial = Int::getValue (::toCFLString (ppRet[row * col + 10]));
		uNonius._direcMode = Int::getValue (::toCFLString (ppRet[row * col + 11]));
		uNonius._status = Int::getValue (::toCFLString (ppRet[row * col + 12]));
		uNonius._taskSrc = Int::getValue (::toCFLString (ppRet[row * col + 13]));
		uNonius._onDup = Int::getValue (::toCFLString (ppRet[row * col + 14]));
		uNonius._delSrc = Int::getValue (::toCFLString (ppRet[row * col + 15]));
		uNonius._ratios = (double)Int::getValue (::toCFLString (ppRet[row * col + 16])) / 10000;
		uNonius._csf = Int::getValue (::toCFLString (ppRet[row * col + 17]));

		return true;
	}

	return false;
}

/**
	* ����Ƿ����ϴ��ϵ�
	* @param absPath:	�ϴ��ļ�����·��
	* @return: �Ƿ���ڶϵ�
	* @throw exception:	��ȡʧ��
*/
/* [notxpcom] bool HasUploadNoniusWithPath ([const] in StringRef absPath); */
NS_IMETHODIMP_(bool) ncRelayMgr::HasUploadNoniusWithPath(const String& absPath)
{
	NC_DOCMGR_TRACE (_T("absPath: %s"), absPath.getCStr ());

	String sql;
	sql.format(_T("select uploadId from %s where srcPath='%s' limit 1"),
				  UNONIUS_TABLE,
				  ncDBMgr::DealSingleQuote (absPath).getCStr ());

	char** ppRet = 0;
	int row = 0, col = 0;
	ncAutoGetTable autoGetTable (_dbMgr, sql, row, col, ppRet);

	return row != 0;
}

/**
* ��ȡ�����ϴ��ϵ�
* @param uNoniusVec:	�ϵ���Ϣ����
* @return: �Ƿ��ȡ����Ч�ϵ�
* @throw exception:	��ȡʧ��
*/
/* [notxpcom] bool GetUploadNoniusVec (in uploadNoniusVecRef uNoniusVec); */
NS_IMETHODIMP_(bool) ncRelayMgr::GetUploadNoniusVec(uploadNoniusVec& uNoniusVec, bool getAll)
{
	String sql;
	if (getAll) {
		sql.format(_T("select srcPath, destPath, docId, md5, crc32, sliceMd5, uploadId, lastModified, otag, partSize, blockSerial, direcMode, status, taskSrc, onDup, delSrc, ratios, csf	\
					from %s order by status desc"),
					UNONIUS_TABLE);
	}
	else {
		sql.format(_T("select srcPath, destPath, docId, md5, crc32, sliceMd5, uploadId, lastModified, otag, partSize, blockSerial, direcMode, status, taskSrc, onDup, delSrc, ratios, csf	\
					  from %s  where status!=2"),
					  UNONIUS_TABLE);
	}


	char** ppRet = 0;
	int row = 0, col = 0;
	ncAutoGetTable autoGetTable (_dbMgr, sql, row, col, ppRet);

	ncUploadNonius uNonius;

	for (int i = 1; i <=row; ++i) {
	  uNonius._srcPath = ::toCFLString (ppRet[i * col]);
	  uNonius._destPath = ::toCFLString (ppRet[i * col + 1]);
	  uNonius._docId = ::toCFLString (ppRet[i * col + 2]);
	  uNonius._md5 = ::toCFLString (ppRet[i * col + 3]);
	  uNonius._crc32 = ::toCFLString (ppRet[i * col + 4]);
	  uNonius._sliceMd5 = ::toCFLString (ppRet[i * col + 5]);
	  uNonius._uploadId = ::toCFLString (ppRet[i * col + 6]);
	  uNonius._lastModified = Int64::getValue (::toCFLString (ppRet[i * col + 7]));
	  uNonius._otag = ::toCFLString (ppRet[i * col + 8]);
	  uNonius._partSize = Int64::getValue (::toCFLString (ppRet[i * col + 9]));
	  uNonius._blockSerial = Int::getValue (::toCFLString (ppRet[i * col + 10]));
	  uNonius._direcMode = Int::getValue (::toCFLString (ppRet[i * col + 11]));
	  uNonius._status = Int::getValue (::toCFLString (ppRet[i * col + 12]));
	  uNonius._taskSrc = Int::getValue (::toCFLString (ppRet[i * col + 13]));
	  uNonius._onDup = Int::getValue (::toCFLString (ppRet[i * col + 14]));
	  uNonius._delSrc = Int::getValue (::toCFLString (ppRet[i * col + 15]));
	  uNonius._ratios = (double)Int::getValue (::toCFLString (ppRet[i * col + 16])) / 10000;
	  uNonius._csf = Int::getValue (::toCFLString (ppRet[row * col + 17]));

	  uNoniusVec.push_back(uNonius);
	}

	return row != 0;
}

/**
* ��ȡ��ͣ���ϴ��ϵ�
* @param uNoniusVec:	�ϵ���Ϣ����
* @return: �Ƿ��ȡ����Ч�ϵ�
* @throw exception:	��ȡʧ��
*/
/* [notxpcom] bool GetHangUploadNoniusVec (in uploadNoniusVecRef uNoniusVec); */
NS_IMETHODIMP_(bool) ncRelayMgr::GetHangUploadNoniusVec(uploadNoniusVec& uNoniusVec)
{
	String sql;
	sql.format(_T("select srcPath, destPath, docId, md5, crc32, sliceMd5, uploadId, lastModified, otag, partSize, blockSerial, direcMode, status, taskSrc, onDup, delSrc, ratios, csf	\
				from %s where status=2"),
				UNONIUS_TABLE);

	char** ppRet = 0;
	int row = 0, col = 0;
	ncAutoGetTable autoGetTable (_dbMgr, sql, row, col, ppRet);

	ncUploadNonius uNonius;

	for (int i = 1; i <=row; ++i) {
	  uNonius._srcPath = ::toCFLString (ppRet[i * col]);
	  uNonius._destPath = ::toCFLString (ppRet[i * col + 1]);
	  uNonius._docId = ::toCFLString (ppRet[i * col + 2]);
	  uNonius._md5 = ::toCFLString (ppRet[i * col + 3]);
	  uNonius._crc32 = ::toCFLString (ppRet[i * col + 4]);
	  uNonius._sliceMd5 = ::toCFLString (ppRet[i * col + 5]);
	  uNonius._uploadId = ::toCFLString (ppRet[i * col + 6]);
	  uNonius._lastModified = Int64::getValue (::toCFLString (ppRet[i * col + 7]));
	  uNonius._otag = ::toCFLString (ppRet[i * col + 8]);
	  uNonius._partSize = Int64::getValue (::toCFLString (ppRet[i * col + 9]));
	  uNonius._blockSerial = Int::getValue (::toCFLString (ppRet[i * col + 10]));
	  uNonius._direcMode = Int::getValue (::toCFLString (ppRet[i * col + 11]));
	  uNonius._status = Int::getValue (::toCFLString (ppRet[i * col + 12]));
	  uNonius._taskSrc = Int::getValue (::toCFLString (ppRet[i * col + 13]));
	  uNonius._onDup = Int::getValue (::toCFLString (ppRet[i * col + 14]));
	  uNonius._delSrc = Int::getValue (::toCFLString (ppRet[i * col + 15]));
	  uNonius._ratios = (double)Int::getValue (::toCFLString (ppRet[i * col + 16])) / 10000;
	  uNonius._csf = Int::getValue (::toCFLString (ppRet[row * col + 17]));

	  uNoniusVec.push_back(uNonius);
	}

	return row != 0;
}

/**
* �����ϴ��ϵ��ʵʱ���
* @param absPath:	�ϴ��ļ�����·��
* @param blockSerial:	�ϴ��ϵ�����ɿ��
* @param status:	����״̬
* @throw exception:	����ʧ��
*/
/* [notxpcom] void UpdateUploadNoniusBlockSerial ([const] in StringRef absPath, [const] in int blockSerial); */
NS_IMETHODIMP_(void) ncRelayMgr::UpdateUploadNoniusBlockSerial(const String& absPath, const int blockSerial, const int status)
{
	NC_DOCMGR_TRACE (_T("absPath: %s"), absPath.getCStr ());

	String sql;
	sql.format (_T("update %s set blockSerial='%d', status='%d' where srcPath='%s'"),
				UNONIUS_TABLE,
				blockSerial,
				status,
				ncDBMgr::DealSingleQuote (absPath).getCStr ());

	_dbMgr->ExecuteSql (sql);
}

/**
	* ��ͣʱ�����ϴ��ϵ����Ϣ
	* @param absPath:	�ϴ��ļ�����·��
	* @param ratios:	����ʱ����ɱ���
	* @throw exception:	����ʧ��
*/
/* [notxpcom] void UpdateDownloadNonius_Pause ([const] in StringRef absPath, [const] in double ratios, [const] in int status); */
NS_IMETHODIMP_(void) ncRelayMgr::UpdateUploadNonius_Pause(const String& absPath, const double ratios, const int status)
{
	NC_DOCMGR_TRACE (_T("absPath: %s"), absPath.getCStr ());

	String sql;
	sql.format (_T("update %s set ratios='%d', status='%d' where srcPath='%s'"),
		UNONIUS_TABLE,
		(int) (ratios * 10000),
		status,
		ncDBMgr::DealSingleQuote (absPath).getCStr ());

	_dbMgr->ExecuteSql (sql);
}

/**
* ɾ���ϴ��ϵ�
* @param absPath:	�ϴ��ļ�����·��
* @throw exception:	����ʧ��
*/
/* [notxpcom] void DeleteUploadNonius ([const] in StringRef absPath); */
NS_IMETHODIMP_(void) ncRelayMgr::DeleteUploadNonius(const String& absPath)
{
	NC_DOCMGR_TRACE (_T("absPath: %s"), absPath.getCStr ());

	String sql;
	sql.format (_T("delete from %s where srcPath='%s'"),
	  UNONIUS_TABLE,
	  ncDBMgr::DealSingleQuote (absPath).getCStr ());

	_dbMgr->ExecuteSql (sql);
}

/*****************************************************************************************************/
/**
* �������ضϵ�
* @param absPath:	�����ļ�����·��
* @param dNonius:	�ϵ���Ϣ
* @throw exception:	����ʧ��
*/
/* [notxpcom] void UpdateDownloadNonius ([const] in StringRef absPath, [const] in ncDownloadNoniusRef dNonius); */
NS_IMETHODIMP_(void) ncRelayMgr::UpdateDownloadNonius(const ncDownloadNonius& dNonius)
{
	NC_DOCMGR_TRACE (_T("absPath: %s"), dNonius._srcPath.getCStr ());

	String sql;
	sql.format(_T("replace into %s(srcPath, destPath, docId, lastModified, otag, docType, typeName, viewType, viewName, mtime, size, attr, range, direcMode, status, taskSrc, delSrc, ratios)	\
				  values('%s', '%s', '%s', '%s', '%s', '%d', '%s', '%d', '%s', '%s', '%s', '%d', '%s', '%d', '%d', '%d', '%d', '%d')"),
				  DNONIUS_TABLE,
				  ncDBMgr::DealSingleQuote (dNonius._srcPath).getCStr (),
				  ncDBMgr::DealSingleQuote (dNonius._destPath).getCStr (),
				  dNonius._docId.getCStr (),
				  Int64::toString (dNonius._lastModified).getCStr (),
				  dNonius._otag.getCStr (),
				  dNonius._docType,
				  ncDBMgr::DealSingleQuote (dNonius._typeName).getCStr (),
				  dNonius._viewType,
				  ncDBMgr::DealSingleQuote (dNonius._viewName).getCStr (),
				  Int64::toString (dNonius._mtime).getCStr (),
				  Int64::toString (dNonius._size).getCStr (),
				  dNonius._attr,
				  Int64::toString (dNonius._range).getCStr (),
				  dNonius._direcMode,
				  dNonius._status,
				  dNonius._taskSrc,
				  dNonius._delSrc,
				  (int)(dNonius._ratios * 10000)
				  );

	_dbMgr->ExecuteSql (sql);
}

/**
* ��ȡ���ضϵ�
* @param absPath:	�����ļ�����·��
* @param dNonius:	�ϵ���Ϣ
* @return: �Ƿ��ȡ����Ч�ϵ�
* @throw exception:	��ȡʧ��
*/
/* [notxpcom] bool GetDownloadNoniusByPath ([const] in StringRef absPath, [const] in ncDownloadNoniusRef dNonius); */
NS_IMETHODIMP_(bool) ncRelayMgr::GetDownloadNoniusByPath(const String& absPath, ncDownloadNonius& dNonius)
{
	NC_DOCMGR_TRACE (_T("absPath: %s"), absPath.getCStr ());

	String sql;
	sql.format(_T("select srcPath, destPath, docId, lastModified, otag, docType, typeName, viewType, viewName, mtime, size, attr,	\
				  range, direcMode, status, taskSrc, delSrc, ratios	\
				  from %s where srcPath='%s' limit 1"),
				  DNONIUS_TABLE,
				  ncDBMgr::DealSingleQuote (absPath).getCStr ());

	char** ppRet = 0;
	int row = 0, col = 0;
	ncAutoGetTable autoGetTable (_dbMgr, sql, row, col, ppRet);

	if (row) {
		dNonius._srcPath = absPath;
		dNonius._destPath = ::toCFLString (ppRet[row * col + 1]);
		dNonius._docId = ::toCFLString (ppRet[row * col + 2]);
		dNonius._lastModified = Int64::getValue(::toCFLString (ppRet[row * col + 3]));
		dNonius._otag = ::toCFLString (ppRet[row * col + 4]);
		dNonius._docType = Int::getValue(::toCFLString (ppRet[row * col + 5]));
		dNonius._typeName = ::toCFLString (ppRet[row * col + 6]);
		dNonius._viewType = Int::getValue(::toCFLString (ppRet[row * col + 7]));
		dNonius._viewName = ::toCFLString (ppRet[row * col + 8]);
		dNonius._mtime = Int64::getValue(::toCFLString (ppRet[row * col + 9]));
		dNonius._size = Int64::getValue(::toCFLString (ppRet[row * col + 10]));
		dNonius._attr = Int::getValue(::toCFLString (ppRet[row * col + 11]));
		dNonius._range = Int64::getValue(::toCFLString (ppRet[row * col + 12]));
		dNonius._direcMode = Int::getValue (::toCFLString (ppRet[row * col + 13]));
		dNonius._status = Int::getValue(::toCFLString (ppRet[row * col + 14]));
		dNonius._taskSrc = Int::getValue(::toCFLString (ppRet[row * col + 15]));
		dNonius._delSrc = Int::getValue(::toCFLString (ppRet[row * col + 16]));
		dNonius._ratios = (double)Int::getValue(::toCFLString (ppRet[row * col + 17])) / 10000;

		return true;
	}

	return false;
}

/**
* ��ȡ�������ضϵ�
* @param dNoniusVec:	�ϵ���Ϣ����
* @return: �Ƿ��ȡ����Ч�ϵ�
* @throw exception:	��ȡʧ��
*/
/* [notxpcom] bool GetDownloadNoniusVec (in downLoadNoniusVecRef dNoniusVec); */
NS_IMETHODIMP_(bool) ncRelayMgr::GetDownloadNoniusVec(downloadNoniusVec& dNoniusVec, bool getAll)
{
	String sql;
	if (getAll) {
		sql.format(_T("select srcPath, destPath, docId, lastModified, otag, docType, typeName, viewType, viewName, mtime, size, attr,	\
						range, direcMode, status, taskSrc, delSrc, ratios	\
						from %s  order by status desc"),
					  DNONIUS_TABLE);
	}
	else {
		sql.format(_T("select srcPath, destPath, docId, lastModified, otag, docType, typeName, viewType, viewName, mtime, size, attr,	\
					  range, direcMode, status, taskSrc, delSrc, ratios	\
					  from %s  where status!=2"),
					  DNONIUS_TABLE);
	}

	char** ppRet = 0;
	int row = 0, col = 0;
	ncAutoGetTable autoGetTable (_dbMgr, sql, row, col, ppRet);

	ncDownloadNonius dNonius;

	for (int i =1; i <=row; ++i) {
		dNonius._srcPath = ::toCFLString (ppRet[i * col]);
		dNonius._destPath = ::toCFLString (ppRet[i * col + 1]);
		dNonius._docId = ::toCFLString (ppRet[i * col + 2]);
		dNonius._lastModified = Int64::getValue(::toCFLString (ppRet[i * col + 3]));
		dNonius._otag = ::toCFLString (ppRet[i * col + 4]);
		dNonius._docType = Int::getValue(::toCFLString (ppRet[i * col + 5]));
		dNonius._typeName = ::toCFLString (ppRet[i * col + 6]);
		dNonius._viewType = Int::getValue(::toCFLString (ppRet[i * col + 7]));
		dNonius._viewName = ::toCFLString (ppRet[i * col + 8]);
		dNonius._mtime = Int64::getValue(::toCFLString (ppRet[i * col + 9]));
		dNonius._size = Int64::getValue(::toCFLString (ppRet[i * col + 10]));
		dNonius._attr = Int::getValue(::toCFLString (ppRet[i * col + 11]));
		dNonius._range = Int64::getValue(::toCFLString (ppRet[i * col + 12]));
		dNonius._direcMode = Int::getValue (::toCFLString (ppRet[i * col + 13]));
		dNonius._status = Int::getValue(::toCFLString (ppRet[i * col + 14]));
		dNonius._taskSrc = Int::getValue(::toCFLString (ppRet[i * col + 15]));
		dNonius._delSrc = Int::getValue(::toCFLString (ppRet[i * col + 16]));
		dNonius._ratios = (double)Int::getValue(::toCFLString (ppRet[i * col + 17])) / 10000;

		dNoniusVec.push_back(dNonius);
	}

	return row != 0;
}

/**
* ��ȡ���й�������ضϵ�
* @param dNoniusVec:	�ϵ���Ϣ����
* @return: �Ƿ��ȡ����Ч�ϵ�
* @throw exception:	��ȡʧ��
*/
/* [notxpcom] bool GetHangDownloadNoniusVec (in downLoadNoniusVecRef dNoniusVec); */
NS_IMETHODIMP_(bool) ncRelayMgr::GetHangDownloadNoniusVec(downloadNoniusVec& dNoniusVec)
{
	String sql;
	sql.format(_T("select srcPath, destPath, docId, lastModified, otag, docType, typeName, viewType, viewName, mtime, size, attr,	\
				  range, direcMode, status, taskSrc, delSrc, ratios	\
				  from %s  where status=2"),
				  DNONIUS_TABLE);

	char** ppRet = 0;
	int row = 0, col = 0;
	ncAutoGetTable autoGetTable (_dbMgr, sql, row, col, ppRet);

	ncDownloadNonius dNonius;

	for (int i =1; i <=row; ++i) {
		dNonius._srcPath = ::toCFLString (ppRet[i * col]);
		dNonius._destPath = ::toCFLString (ppRet[i * col + 1]);
		dNonius._docId = ::toCFLString (ppRet[i * col + 2]);
		dNonius._lastModified = Int64::getValue(::toCFLString (ppRet[i * col + 3]));
		dNonius._otag = ::toCFLString (ppRet[i * col + 4]);
		dNonius._docType = Int::getValue(::toCFLString (ppRet[i * col + 5]));
		dNonius._typeName = ::toCFLString (ppRet[i * col + 6]);
		dNonius._viewType = Int::getValue(::toCFLString (ppRet[i * col + 7]));
		dNonius._viewName = ::toCFLString (ppRet[i * col + 8]);
		dNonius._mtime = Int64::getValue(::toCFLString (ppRet[i * col + 9]));
		dNonius._size = Int64::getValue(::toCFLString (ppRet[i * col + 10]));
		dNonius._attr = Int::getValue(::toCFLString (ppRet[i * col + 11]));
		dNonius._range = Int64::getValue(::toCFLString (ppRet[i * col + 12]));
		dNonius._direcMode = Int::getValue (::toCFLString (ppRet[i * col + 13]));
		dNonius._status = Int::getValue(::toCFLString (ppRet[i * col + 14]));
		dNonius._taskSrc = Int::getValue(::toCFLString (ppRet[i * col + 15]));
		dNonius._delSrc = Int::getValue(::toCFLString (ppRet[i * col + 16]));
		dNonius._ratios = (double)Int::getValue(::toCFLString (ppRet[i * col + 17])) / 10000;

		dNoniusVec.push_back(dNonius);
	}

	return row != 0;
}

/**
* �������ضϵ�ʵʱ���
* @param absPath:	�����ļ�����·��
* @param range:	���ضϵ�����ɿ��
* @param status:	����״̬
* @param lastModified:	������ʱ���ݵ��޸�ʱ��
* @throw exception:	����ʧ��
*/
/* [notxpcom] void UpdateDownloadNoniusRange ([const] in StringRef absPath, [const] in int64Ref range); */
NS_IMETHODIMP_(void) ncRelayMgr::UpdateDownloadNoniusRange(const String& absPath, const int64& range, const int status, const int64& lastModified)
{
	NC_DOCMGR_TRACE (_T("absPath: %s"), absPath.getCStr ());

	String sql;
	sql.format (_T("update %s set range='%s', status='%d', lastModified='%s' where srcPath='%s'"),
		DNONIUS_TABLE,
		Int64::toString (range).getCStr (),
		status,
		Int64::toString (lastModified).getCStr (),
		ncDBMgr::DealSingleQuote (absPath).getCStr ());

	_dbMgr->ExecuteSql (sql);
}

/**
	* ��ͣʱ�������ضϵ����Ϣ
	* @param absPath:	�ϴ��ļ�����·��
	* @param ratios:	����ʱ����ɱ���
	* @throw exception:	����ʧ��
*/
/* [notxpcom] void UpdateDownloadNonius_Pause ([const] in StringRef absPath, [const] in double ratios, [const] in int status); */
NS_IMETHODIMP_(void) ncRelayMgr::UpdateDownloadNonius_Pause(const String& absPath, const double ratios, const int status)
{
	NC_DOCMGR_TRACE (_T("absPath: %s"), absPath.getCStr ());

	String sql;
	sql.format (_T("update %s set ratios='%d', status='%d' where srcPath='%s'"),
		DNONIUS_TABLE,
		(int) (ratios * 10000),
		status,
		ncDBMgr::DealSingleQuote (absPath).getCStr ());

	_dbMgr->ExecuteSql (sql);
}


/**
* ɾ�����ضϵ�
* @param absPath:	�����ļ�����·��
* @throw exception:	����ʧ��
*/
/* [notxpcom] void DeleteDownloadNonius ([const] in StringRef absPath); */
NS_IMETHODIMP_(void) ncRelayMgr::DeleteDownloadNonius(const String& absPath)
{
	NC_DOCMGR_TRACE (_T("absPath: %s"), absPath.getCStr ());

	String sql;
	sql.format (_T("delete from %s where srcPath='%s'"),
		DNONIUS_TABLE,
		ncDBMgr::DealSingleQuote (absPath).getCStr ());

	_dbMgr->ExecuteSql (sql);
}