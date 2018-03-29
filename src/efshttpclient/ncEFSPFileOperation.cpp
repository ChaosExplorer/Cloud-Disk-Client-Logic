/***************************************************************************************************
Purpose:
	efsp file operation

Author:
	Chaos
			
Creating Time:
	2013-07-24
***************************************************************************************************/
#include <abprec.h>
#include <dataapi/ncGNSUtil.h>
#include <dataapi/ncJson.h>

#include "efspclient.h"
#include "ncSimpleEnumerator.h"
#include "ncEFSPFileBlock.h"
#include "ncEFSPFileOperation.h"

#define FILE_URI _T("file")
#define RESTORED_VERSION _T("restorerevision")
#define SEARCH_URL _T("search")

const string NC_EFS_HTTP_CRLF = ("\r\n");
const string NC_EFS_HTTP_PREFIX = ("--");

NC_UMM_IMPL_THREADSAFE_ISUPPORTS1 (ncEFSPClientAllocator, ncEFSPFileOperation, ncIEFSPFileOperation)

ncEFSPFileOperation::ncEFSPFileOperation (ncEFSPClient* efspClient, const ncTokenInfo& tokenInfo)
	: _efspClient (efspClient)
	, _tokenInfo (tokenInfo)
{
	NC_EFSP_CLIENT_TRACE (_T ("this: 0x%p, efspClient: 0x%p"), this, efspClient);
}

ncEFSPFileOperation::~ncEFSPFileOperation (void)
{
	NC_EFSP_CLIENT_TRACE (_T ("this: 0x%p"), this);
}

/* [notxpcom] void DeleteFile ([const] in StringRef docId); */
NS_IMETHODIMP_(void) ncEFSPFileOperation::DeleteFile(const String & docId)
{
	NC_EFSP_CLIENT_TRACE (_T("docId: %s"), docId.getCStr ());

	CheckDocId (docId);
	String url = GetUrl (_T("delete"));

	// 构造http内容
	JSON::Object json;
	json["docid"] = toSTLString (docId);
	string content;
	content.reserve (64);
	JSON::Writer::write (json, content);

	ncOSSResponse res;
	vector<string> headers;
	_efspClient->SendToAS(url, content, headers, res);

	NC_EFSP_CLIENT_TRACE (_T("end"));
}

/* [notxpcom] void RenameFile ([const] in StringRef docId, in StringRef newName, in int ondup); */
NS_IMETHODIMP_(void)
ncEFSPFileOperation::RenameFile(const String & docId, String & newName, int ondup)
{
	NC_EFSP_CLIENT_TRACE (_T("docId: %s, newName: %s, ondup: %d"), docId.getCStr (), newName.getCStr (), ondup);

	CheckDocId (docId);
	CheckName (newName);
	String url = GetUrl (_T("rename"));

	// 构造http内容
	JSON::Object json;
	json["docid"] = toSTLString (docId);
	json["name"]  = toSTLString (newName);
	json["ondup"] = ondup;

	string content;
	JSON::Writer::write (json, content);

	ncOSSResponse res;
	vector<string> headers;
	_efspClient->SendToAS(url, content, headers, res);

	if (2 == ondup)
	{
		JSON::Value value;
		JSON::Reader::read (value, res.body.c_str (), res.body.length ());
		newName = toCFLString(value["name"].s());
	}

	NC_EFSP_CLIENT_TRACE (_T("end"));
}

/* [notxpcom] void ListVersions ([const] in StringRef docId, in ncFileVersionInfosVecRef versionInfos); */
NS_IMETHODIMP_(void)
ncEFSPFileOperation::ListVersions (const String & docId, vector<ncFileVersionInfo>& versionInfos)
{
	NC_EFSP_CLIENT_TRACE (_T("docId: %s"), docId.getCStr ());

	versionInfos.clear ();

	CheckDocId (docId);

	String url = GetUrl (_T("revisions"));

	// 构造http内容
	JSON::Object json;
	json["docid"] = toSTLString (docId);
	string content;
	content.reserve (64);
	JSON::Writer::write (json, content);

	// 发送，接收结果并解析
	ncOSSResponse res;
	vector<string> headers;
	_efspClient->SendToAS(url, content, headers, res);

	JSON::Value value;
	JSON::Reader::read (value, res.body.c_str (), res.body.length ());

	JSON::Array& versions = value.a ();
	size_t ver_size = versions.size ();

	ncFileVersionInfo fileVersionInfo;

	for (int verIndex = 0; verIndex < ver_size; ++verIndex) {
		fileVersionInfo.versionId = toCFLString (versions[verIndex]["rev"].s ());
		fileVersionInfo.fileName = toCFLString (versions[verIndex]["name"].s ());
		fileVersionInfo.editorName = toCFLString (versions[verIndex]["editor"].s ());
		fileVersionInfo.createTime = versions[verIndex]["client_mtime"].i ();
		fileVersionInfo.size = versions[verIndex]["size"].i ();
		fileVersionInfo.editTime = versions[verIndex]["modified"].i ();
		versionInfos.push_back (fileVersionInfo);
	}

	NC_EFSP_CLIENT_TRACE (_T("end"));
}

/* [notxpcom] String MoveFile ([const] in StringRef docId, [const] in StringRef destParent, in StringRef name, in int ondup); */
NS_IMETHODIMP_(String)
ncEFSPFileOperation::MoveFile (const String& docId, const String& destParent, String& name, int ondup)
{
	NC_EFSP_CLIENT_TRACE (_T("docId: %s, destParent: %s"), docId.getCStr (), destParent.getCStr ());

	CheckDocId (docId);

	String url = GetUrl (_T("move"));

	// 构造http内容
	JSON::Object json;
	json["docid"] = toSTLString (docId);
	json["destparent"] = toSTLString (destParent);
	json["ondup"] = ondup;
	
	string content;
	content.reserve (64);
	JSON::Writer::write (json, content);

	// 发送，接收结果并解析
	ncOSSResponse res;
	vector<string> headers;
	_efspClient->SendToAS(url, content, headers, res, 0);

	JSON::Value value;
	JSON::Reader::read (value, res.body.c_str (), res.body.length ());

	String newDocId = toCFLString (value["docid"]);

	// 仅当ondup=2自动重命名时回复新名称
	if (ondup == 2)
		name = toCFLString (value["name"]);

	NC_EFSP_CLIENT_TRACE (_T("end"));
	return newDocId;
}

/* [notxpcom] String CopyFile ([const] in StringRef docId, [const] in StringRef destParent, in StringRef name, in int ondup); */
NS_IMETHODIMP_(String)
ncEFSPFileOperation::CopyFile (const String& docId, const String& destParent, String& name, int ondup)
{
	NC_EFSP_CLIENT_TRACE (_T("docId: %s, destParent: %s"), docId.getCStr (), destParent.getCStr ());

	CheckDocId (docId);

	String url = GetUrl (_T("copy"));

	// 构造http内容
	JSON::Object json;
	json["docid"] = toSTLString (docId);
	json["destparent"] = toSTLString (destParent);
	json["ondup"]      = ondup;

	string content;
	content.reserve (64);
	JSON::Writer::write (json, content);

	// 发送，接收结果并解析
	ncOSSResponse res;
	vector<string> headers;
	_efspClient->SendToAS(url, content, headers, res, 0);

	JSON::Value value;
	JSON::Reader::read (value, res.body.c_str (), res.body.length ());

	String newDocId = toCFLString (value["docid"]);

	// 仅当ondup=2自动重命名时回复新名称
	if (ondup == 2)
		name = toCFLString (value["name"]);

	NC_EFSP_CLIENT_TRACE (_T("end"));
	return newDocId;
}

NS_IMETHODIMP_(bool) 
ncEFSPFileOperation::PreduploadFile (int64 length, const String& slice_md5)
{
	NC_EFSP_CLIENT_TRACE (_T("length: %I64d, slice_md5: %s"), length, slice_md5.getCStr ());

	String url = GetUrl (_T("predupload"));

	// 构造http内容
	JSON::Object json;
	json["length"] = length;
	json["slice_md5"] = toSTLString (slice_md5);

	string content;
	content.reserve (64);
	JSON::Writer::write (json, content);

	// 发送，接收结果并解析
	ncOSSResponse res;
	vector<string> headers;
	_efspClient->SendToAS(url, content, headers, res);

	JSON::Value value;
	JSON::Reader::read (value, res.body.c_str (), res.body.length ());

	NC_EFSP_CLIENT_TRACE (_T("end"));
	return value["match"];
}

NS_IMETHODIMP_(bool)
ncEFSPFileOperation::DuploadFile (String& docId,
								  String& name,
								  String& rev,
								  int ondup,
								  int64 client_mtime,
								  int64 length,
								  const String& md5,
								  const String& crc32,
								  const int csflevel)
{
	NC_EFSP_CLIENT_TRACE (_T("docId: %s, name: %s"), docId.getCStr(), name.getCStr ());

	String url = GetUrl (_T("dupload"));

	// 构造http内容
	JSON::Object json;
	json["docid"] = toSTLString(docId);
	json["name"] = toSTLString (name);
	json["ondup"] = ondup;
	json["client_mtime"] = client_mtime;
	json["length"] = length;
	json["md5"] = toSTLString (md5);
	json["crc32"] = toSTLString (crc32);
	if (csflevel) 
		json["csflevel"] = csflevel;

	string content;
	content.reserve (64);
	JSON::Writer::write (json, content);

	// 发送，接收结果并解析
	ncOSSResponse res;
	vector<string> headers;
	_efspClient->SendToAS(url, content, headers, res, 0);

	JSON::Value value;
	JSON::Reader::read (value, res.body.c_str (), res.body.length ());

	bool bSuccess = value["success"];

	if (bSuccess)
	{
		docId = value["docid"].s().c_str();
		name  = value["name"].s().c_str();
		rev = toCFLString (value["rev"].s ());
	}

	NC_EFSP_CLIENT_TRACE (_T("end"));
	return bSuccess;
}

NS_IMETHODIMP_(ncIEFSPFileBlock *)
ncEFSPFileOperation::PreviewFile (const String& docId, const String& rev)
{
	NC_EFSP_CLIENT_TRACE (_T("docId: %s, rev: %s"), docId.getCStr(), rev.getCStr ());

	String url = GetUrl (_T("preview"));

	// 构造http内容
	JSON::Object json;
	json["docid"] = toSTLString(docId);
	json["rev"] = toSTLString (rev);	

	string content;
	content.reserve (64);
	JSON::Writer::write (json, content);

	// 发送，接收结果并解析
	ncOSSResponse res;
	vector<string> headers;

	while (true) {
		try {
			headers.clear ();
			_efspClient->SendToAS(url, content, headers, res, 0);
			break;
		}
		catch (Exception& e) {
			// 首次预览时会进行转换，会返回提交转换的错误码，需要循环重试，直到发生错误或者返回二进制数据
			if (e.getErrorId () != 503002)
				throw;
			Thread::sleep (100);
		}
	}

	int64 length = res.body.length ();
	nsCOMPtr<ncIEFSPFileBlock> fileBlock = NC_NEW (ncEFSPClientAllocator, ncEFSPFileBlock) (0, (uint)length);
	memcpy (fileBlock->GetBuffer (), res.body.c_str (), length);
	fileBlock->SetWriteLength ((unsigned int)length);

	NS_ADDREF (fileBlock.get ());
	NC_EFSP_CLIENT_TRACE (_T("end"));
	return fileBlock;
}

NS_IMETHODIMP_(ncIEFSPFileBlock *)
ncEFSPFileOperation::ThumbnailFile (const String& docId, const String& rev)
{
	NC_EFSP_CLIENT_TRACE (_T("docId: %s, rev: %s"), docId.getCStr(), rev.getCStr ());

	String url = GetUrl (_T("thumbnail"));

	// 构造http内容
	JSON::Object json;
	json["docid"]        = toSTLString(docId);
	json["rev"]         = toSTLString (rev);
	json["height"]         = 9999;
	json["width"]         = 9999;

	string content;
	content.reserve (64);
	JSON::Writer::write (json, content);

	// 发送，接收结果并解析
	ncOSSResponse res;
	vector<string> headers;
	_efspClient->SendToAS(url, content, headers, res, 0);

	int64 length = res.body.length ();
	nsCOMPtr<ncIEFSPFileBlock> fileBlock = NC_NEW (ncEFSPClientAllocator, ncEFSPFileBlock) (0, (uint)length);
	memcpy (fileBlock->GetBuffer (), res.body.c_str (), length);
	fileBlock->SetWriteLength ((unsigned int)length);

	NS_ADDREF (fileBlock.get ());
	NC_EFSP_CLIENT_TRACE (_T("end"));
	return fileBlock;
}

NS_IMETHODIMP_(void)
ncEFSPFileOperation::GetSuggestFileName (const String& docId, String& name)
{
	NC_EFSP_CLIENT_TRACE (_T("docId: %s, name: %s"), docId.getCStr (), name.getCStr ());

	CheckDocId (docId);

	String url = GetUrl (_T("getsuggestname"));

	// 构造http内容
	JSON::Object json;
	json["docid"] = toSTLString (docId);
	json["name"] = toSTLString (name);

	string content;
	content.reserve (64);
	JSON::Writer::write (json, content);

	// 发送，接收结果并解析
	ncOSSResponse res;
	vector<string> headers;
	_efspClient->SendToAS(url, content, headers, res);

	JSON::Value value;
	JSON::Reader::read (value, res.body.c_str (), res.body.length ());

	name = toCFLString (value["name"]);

	NC_EFSP_CLIENT_TRACE (_T("end"));
}

NS_IMETHODIMP_(bool)
ncEFSPFileOperation::CheckFileOverdue (const String & docId, const String & rev)
{
	NC_EFSP_CLIENT_TRACE (_T("docId: %s, rev: %s"), docId.getCStr (), rev.getCStr ());

	CheckDocId (docId);

	String url = GetUrl (_T("metadata"));

	// 构造http内容
	JSON::Object json;
	json["docid"] = toSTLString (docId);

	string content;
	content.reserve (64);
	JSON::Writer::write (json, content);

	// 发送，接收结果并解析
	ncOSSResponse res;
	vector<string> headers;
	_efspClient->SendToAS(url, content, headers, res);

	JSON::Value value;
	JSON::Reader::read (value, res.body.c_str (), res.body.length ());

	String versionId = toCFLString (value["rev"].s ());

	return rev != versionId;

	NC_EFSP_CLIENT_TRACE (_T("end"));
}

NS_IMETHODIMP_(void)
ncEFSPFileOperation::GetMetadata (const String & docId, const String & rev, ncFileVersionInfo & versionInfo)
{
	NC_EFSP_CLIENT_TRACE (_T("docId: %s, rev: %s"), docId.getCStr (), rev.getCStr ());

	CheckDocId (docId);

	String url = GetUrl (_T("metadata"));

	// 构造http内容
	JSON::Object json;
	json["docid"] = toSTLString (docId);
	json["rev"] = toSTLString (rev);

	string content;
	content.reserve (64);
	JSON::Writer::write (json, content);

	// 发送，接收结果并解析
	ncOSSResponse res;
	vector<string> headers;
	_efspClient->SendToAS(url, content, headers, res);

	JSON::Value value;
	JSON::Reader::read (value, res.body.c_str (), res.body.length ());

	versionInfo.versionId = toCFLString (value["rev"].s ());
	versionInfo.fileName = toCFLString (value["name"].s ());
	versionInfo.editorName = toCFLString (value["editor"].s ());
	versionInfo.siteName = toCFLString (value["site"].s ());
	versionInfo.createTime = value["client_mtime"].i ();
	versionInfo.size = value["size"].i ();
	versionInfo.editTime = value["modified"].i ();

	NC_EFSP_CLIENT_TRACE (_T("end"));
}

NS_IMETHODIMP_(void)
ncEFSPFileOperation::GetAttribute (const String & docId, ncFileAttributeInfo & attrInfo)
{
	NC_EFSP_CLIENT_TRACE (_T("docId: %s"), docId.getCStr ());

	CheckDocId (docId);

	String url = GetUrl (_T("attribute"));

	// 构造http内容
	JSON::Object json;
	json["docid"] = toSTLString (docId);

	string content;
	content.reserve (64);
	JSON::Writer::write (json, content);

	// 发送，接收结果并解析
	ncOSSResponse res;
	vector<string> headers;
	_efspClient->SendToAS(url, content, headers, res);

	JSON::Value value;
	JSON::Reader::read (value, res.body.c_str (), res.body.length ());

	attrInfo.uniqueId = toCFLString (value["uniqueid"].s ());
	attrInfo.creator = toCFLString (value["creator"].s ());
	attrInfo.createTime = value["create_time"].i ();
	attrInfo.csflevel = (int)value["csflevel"].i ();

	JSON::Array& tags = value["tags"].a();
	for(int i = 0; i < tags.size(); ++i)
		attrInfo.tags.push_back (toCFLString (tags[i]));

	NC_EFSP_CLIENT_TRACE (_T("end"));
}

NS_IMETHODIMP_(void)
ncEFSPFileOperation::GetName (const String& docId, String& name)
{
	NC_EFSP_CLIENT_TRACE (_T("docId: %s"), docId.getCStr ());

	CheckDocId (docId);

	String url = GetUrl (_T("info"));

	JSON::Object json;
	json["docid"] = toSTLString (docId);

	string content;
	content.reserve (64);
	JSON::Writer::write (json, content);

	ncOSSResponse res;
	vector<string> headers;
	_efspClient->SendToAS(url, content, headers, res);

	JSON::Value value;
	JSON::Reader::read (value, res.body.c_str (), res.body.length ());

	name = toCFLString (value["name"].s ());

	NC_EFSP_CLIENT_TRACE (_T("end"));
}

NS_IMETHODIMP_(void)
ncEFSPFileOperation::SetCsfLevel (const String& docId, int csfLevel)
{
	NC_EFSP_CLIENT_TRACE (_T("docId: %s, csfLevel: %d"), docId.getCStr (), csfLevel);

	CheckDocId (docId);

	String url = GetUrl (_T("setcsflevel"));

	JSON::Object json;
	json["docid"] = toSTLString (docId);
	json["csflevel"] = csfLevel;

	string content;
	content.reserve (64);
	JSON::Writer::write (json, content);

	ncOSSResponse res;
	vector<string> headers;
	_efspClient->SendToAS(url, content, headers, res);

	NC_EFSP_CLIENT_TRACE (_T("end"));
}

NS_IMETHODIMP_(void)
ncEFSPFileOperation::AddTag (const String& docId, const String& tag)
{
	NC_EFSP_CLIENT_TRACE (_T ("docId: %s, tag: %s"), docId.getCStr(), tag.getCStr());

	CheckDocId (docId);

	String url = GetUrl (_T("addtag"));

	JSON::Object json;
	json["docid"] = toSTLString (docId);
	json["tag"] = toSTLString (tag);

	string content;
	content.reserve (64);
	JSON::Writer::write (json, content);

	ncOSSResponse res;
	vector<string> headers;
	_efspClient->SendToAS (url, content, headers, res);

	NC_EFSP_CLIENT_TRACE (_T("end"));
}

NS_IMETHODIMP_(void)
ncEFSPFileOperation::AddTagBatch (const String& docId, const vector<String> & tags, int & unsetNum)
{
	NC_EFSP_CLIENT_TRACE (_T ("docId: %s"), docId.getCStr());

	CheckDocId (docId);

	String url = GetUrl (_T("addtags"));

	JSON::Object json;
	json["docid"] = toSTLString (docId);

	JSON::Array& jArray = json["tags"].a ();
	for (size_t i = 0; i < tags.size (); ++i) {
		jArray.push_back (toSTLString (tags[i]));
	}

	string content;
	content.reserve (128);
	JSON::Writer::write (json, content);

	ncOSSResponse res;
	vector<string> headers;
	_efspClient->SendToAS (url, content, headers, res);

	JSON::Value value;
	JSON::Reader::read (value, res.body.c_str (), res.body.length ());

	unsetNum = value["unsettagnum"].i ();

	NC_EFSP_CLIENT_TRACE (_T("end"));
}


NS_IMETHODIMP_(void)
ncEFSPFileOperation::DeleteTag (const String& docId, const String& tag)
{
	NC_EFSP_CLIENT_TRACE (_T ("docId: %s, tag: %s"), docId.getCStr(), tag.getCStr());

	CheckDocId (docId);

	String url = GetUrl (_T("deletetag"));

	JSON::Object json;
	json["docid"] = toSTLString (docId);
	json["tag"] = toSTLString (tag);

	string content;
	content.reserve (64);
	JSON::Writer::write (json, content);

	ncOSSResponse res;
	vector<string> headers;
	_efspClient->SendToAS (url, content, headers, res);

	NC_EFSP_CLIENT_TRACE (_T("end"));
}

NS_IMETHODIMP_(void)
ncEFSPFileOperation::TagSuggest (const String& prefix, int count, vector<String>& tags)
{
	NC_EFSP_CLIENT_TRACE (_T ("prefix: %s, count: %d"), prefix.getCStr(), count);

	String url = GetSearchUrl (_T("tagsuggest"));

	JSON::Object json;
	json["prefix"] = toSTLString (prefix);
	json["count"] = count;

	string content;
	content.reserve (64);
	JSON::Writer::write (json, content);

	ncOSSResponse res;
	vector<string> headers;
	_efspClient->SendToAS (url, content, headers, res);

	JSON::Value value;
	JSON::Reader::read (value, res.body.c_str (), res.body.length ());

	JSON::Array& returnTags = value["suggestions"].a();
	for(int i = 0; i < returnTags.size(); ++i)
		tags.push_back (toCFLString (returnTags[i]));

	NC_EFSP_CLIENT_TRACE (_T("end"));
}

NS_IMETHODIMP_(int64)
ncEFSPFileOperation::GetPartMinSize()
{
	NC_EFSP_CLIENT_TRACE (_T("begin"));
	
	String url = GetUrl(_T("ospartminsize"));

	ncOSSResponse res;
	string content;
	vector<string> headers;
	_efspClient->SendToAS(url, content, headers, res);
	JSON::Value value;
	JSON::Reader::read(value, res.body.c_str(), res.body.length());

	NC_EFSP_CLIENT_TRACE (_T("end"));
	return value["size"].i();
}

NS_IMETHODIMP_(void)
ncEFSPFileOperation::GetOssPartInfo (ncOssPartInfo& ossPartInfo)
{
	NC_EFSP_CLIENT_TRACE (_T("begin"));

	String url = GetUrl(_T("osoption"));

	ncOSSResponse res;
	string content;
	vector<string> headers;
	_efspClient->SendToAS(url, content, headers, res);
	JSON::Value value;
	JSON::Reader::read(value, res.body.c_str(), res.body.length());

	ossPartInfo.partMinSize = value["partminsize"].i();
	ossPartInfo.partMaxSize = value["partmaxsize"].i();
	ossPartInfo.partMaxNum = value["partmaxnum"].i();

	NC_EFSP_CLIENT_TRACE (_T("end"));
}

NS_IMETHODIMP_(void)
ncEFSPFileOperation::StartUpLoadFile(const String& docId,
									 const String& name,
									 int64 length,
									 int64 mtime,
									 int ondup,
									 String& retdocId,
									 String &rev,
									 ncFileOssInfo& fileOssInfo,
									 int csf)
{
	NC_EFSP_CLIENT_TRACE (_T("name: %s, docid: %s, length: %I64d,)"),
		name.getCStr (), docId.getCStr (), length );

	CheckDocId(docId);

	String url = GetUrl(_T("osbeginupload"));

	// 构造http内容
	JSON::Object json;
	json["docid"] = toSTLString(docId);
	json["length"] = length;
	json["name"] = toSTLString(name);
	json["client_mtime"] = mtime;
	json["ondup"] = ondup;
	json["reqhost"] = toSTLString(_efspClient->getIP ());
	if (csf)
		json["csflevel"] = csf;

	string content;
	JSON::Writer::write(json, content);

	ncOSSResponse res;
	vector<string> headers;
	_efspClient->SendToAS(url, content, headers, res);
	JSON::Value value;
	JSON::Reader::read(value, res.body.c_str(), res.body.length());

	retdocId = toCFLString(value["docid"].s());
	rev = toCFLString(value["rev"].s());
	JSON::Array& authrequest = value["authrequest"].a();
	fileOssInfo.method = toCFLString(authrequest[0]);
	fileOssInfo.method.toUpper();
	fileOssInfo.url = toCFLString(authrequest[1]);
	fileOssInfo.headers.clear();
	for (int i = 2; i < authrequest.size(); i++){
		fileOssInfo.headers.push_back(toCFLString(authrequest[i]));
	}

	NC_EFSP_CLIENT_TRACE (_T("end"));
}

NS_IMETHODIMP_(void)
ncEFSPFileOperation::EndUpLoadFile(const String& docId,
								   const String& ver,
							   	   const String& md5,
								   const String& crc32,
								   const String& slice_md5,
								   const int csflevel)
{
	NC_EFSP_CLIENT_TRACE (_T("docid: %s, ver: %s)"), docId.getCStr (), ver.getCStr ());

	String url = GetUrl(_T("osendupload"));

	// 构造http内容
	JSON::Object json;
	json["docid"] = toSTLString(docId);
	json["rev"] = toSTLString(ver);
	json["md5"] = toSTLString(md5);
	json["crc32"] = toSTLString(crc32);
	json["slice_md5"] = toSTLString(slice_md5);
	if (csflevel)
		json["csflevel"] = csflevel;

	string content;
	JSON::Writer::write(json, content);

	ncOSSResponse res;
	vector<string> headers;
	_efspClient->SendToAS(url, content, headers, res);

	NC_EFSP_CLIENT_TRACE (_T("end"));
}

NS_IMETHODIMP_(void)
ncEFSPFileOperation::StartUpLoadFileByPart(const String& docId,
										   const String& name,
										   int64 length,
										   int64 mtime,
										   int ondup,
										   String& retdocId,
										   String& rev,	
										   String& uploadId,
										   int csf)
{
	NC_EFSP_CLIENT_TRACE (_T("name: %s, docid: %s, length: %I64d,)"),
		name.getCStr (), docId.getCStr (), length );

	CheckDocId(docId);

	String url = GetUrl(_T("osinitmultiupload"));

	// 构造http内容
	JSON::Object json;
	json["docid"] = toSTLString(docId);
	json["length"] = length;
	json["name"] = toSTLString(name);
	json["client_mtime"] = mtime;
	json["ondup"] = ondup;
	if (csf)
		json["csflevel"] = csf;

	string content;
	JSON::Writer::write(json, content);

	ncOSSResponse res;
	vector<string> headers;
	_efspClient->SendToAS(url, content, headers, res);
	JSON::Value value;
	JSON::Reader::read(value, res.body.c_str(), res.body.length());

	retdocId = toCFLString(value["docid"].s());
	rev = toCFLString(value["rev"].s ());
	uploadId = toCFLString(value["uploadid"].s());

	NC_EFSP_CLIENT_TRACE (_T("end"));
}

NS_IMETHODIMP_(void)
ncEFSPFileOperation::StartUpLoadFilePart(const String& docId,
										 const String& rev,
										 const String& uploadId,
										 const String& parts,
										 map<String, ncFilePartInfo>& filePartInfos)
{
	NC_EFSP_CLIENT_TRACE (_T("docId: %s, rev: %s, uploadId: %s, parts: %s)"),
		docId.getCStr (), rev.getCStr (), uploadId.getCStr () ,parts.getCStr ());

	String url = GetUrl(_T("osuploadpart"));

	// 构造http内容
	JSON::Object json;
	json["docid"] = toSTLString(docId);
	json["rev"] = toSTLString(rev);
	json["uploadid"] = toSTLString(uploadId);
	json["parts"] = toSTLString(parts);
	json["reqhost"] = toSTLString(_efspClient->getIP ());

	string content;
	JSON::Writer::write(json, content);

	ncOSSResponse res;
	vector<string> headers;
	_efspClient->SendToAS(url, content, headers, res);
	JSON::Value value;
	JSON::Reader::read(value, res.body.c_str(), res.body.length());

	JSON::Object& authrequests = value["authrequests"].o();
	JSON::Object::iterator ite = authrequests.begin();
	while (ite != authrequests.end())
	{
		ncFilePartInfo& filePartInfo = filePartInfos[toCFLString(ite->first)];
		ncFileOssInfo& fileOssInfo = filePartInfo.foi;
		JSON::Array& authrequest = ite->second.a();
		fileOssInfo.method = toCFLString(authrequest[0]);
		fileOssInfo.method.toUpper();
		fileOssInfo.url = toCFLString(authrequest[1]);
		fileOssInfo.headers.clear();
		for (int i = 2; i < authrequest.size(); i++){
			fileOssInfo.headers.push_back(toCFLString(authrequest[i]));
		}
		ite++;
	}

	NC_EFSP_CLIENT_TRACE (_T("end"));
}

NS_IMETHODIMP_(void)
ncEFSPFileOperation::CompleteUpLoadFilePart(const String& docId,
											const String& rev,
											const String& uploadId,
											const map<String, ncFilePartInfo>& filePartInfos)
{
	NC_EFSP_CLIENT_TRACE (_T("docId: %s, rev: %s, uploadId: %s)"),
		docId.getCStr (), rev.getCStr (), uploadId.getCStr ());

	String url = GetUrl(_T("oscompleteupload"));

	// 构造http内容
	JSON::Object json;
	json["docid"] = toSTLString(docId);
	json["rev"] = toSTLString(rev);
	json["uploadid"] = toSTLString(uploadId);
	json["reqhost"] = toSTLString(_efspClient->getIP ());
	map<String, ncFilePartInfo>::const_iterator ite = filePartInfos.begin();
	while(ite != filePartInfos.end()) {
		JSON::Array& versJson = json["partinfo"][(toSTLString(ite->first)).c_str()].a();
		versJson.push_back (JSON_MOVE (JSON::Value (JSON_MOVE (toSTLString(ite->second.etag)), false)));
		versJson.push_back ((JSON::Value(ite->second.length)));
		ite++;
	}

	string content;
	JSON::Writer::write(json, content);

	ncOSSResponse res;
	vector<string> headers;
	_efspClient->SendToAS(url, content, headers, res);

	// 从结果中解析出直连云存储信息
	string boundary;
	GetBoundary(res.headers["Content-Type"], boundary);
	ncEFSPBuffer binary;
	ncEFSPBuffer body;
	DownloadSplit(res.body, boundary, binary, body, false);
	JSON::Value value;
	JSON::Reader::read (value, body.buffer, body.length);
	JSON::Array& authrequest = value["authrequest"].a();

	String method = toCFLString(authrequest[0]);
	method.toUpper();
	url = toCFLString(authrequest[1]);
	content.assign(binary.buffer, binary.length);
	headers.clear();
	for (int i = 2; i < authrequest.size(); i++){
		headers.push_back(authrequest[i]);
	}

	// 自行发送完成信息到云存储
	_efspClient->SendToOSS(toSTLString(method), toSTLString(url), content, headers, res);

	NC_EFSP_CLIENT_TRACE (_T("end"));
}

NS_IMETHODIMP_(void)
ncEFSPFileOperation::UpLoadFileData(const ncFileOssInfo& fileOssInfo,
									ncIEFSPFileBlock* fileBlock)
{
	NC_EFSP_CLIENT_TRACE (_T ("fileBlock: 0x%p"), fileBlock);

	vector<string> inHeaders;
	vector<String>::const_iterator ite = fileOssInfo.headers.begin();
	while (ite != fileOssInfo.headers.end())
	{
		inHeaders.push_back(toSTLString(*ite));
		ite++;
	}
	ncOSSResponse res;
	string content;
	content.assign(reinterpret_cast<char*>(fileBlock->GetBuffer()), fileBlock->GetWriteLength());
	_efspClient->SendToOSS(toSTLString(fileOssInfo.method), toSTLString(fileOssInfo.url), content, inHeaders, res);

	NC_EFSP_CLIENT_TRACE (_T("end"));
}

NS_IMETHODIMP_(void)
ncEFSPFileOperation::UpLoadFileDataPart(const ncFileOssInfo& fileOssInfo,
										ncIEFSPFileBlock* fileBlock,
										String& etag)
{
	NC_EFSP_CLIENT_TRACE (_T ("fileBlock: 0x%p, etag: %s"), fileBlock, etag.getCStr ());

	vector<string> inHeaders;
	vector<String>::const_iterator ite = fileOssInfo.headers.begin();
	while (ite != fileOssInfo.headers.end())
	{
		inHeaders.push_back(toSTLString(*ite));
		ite++;
	}

	ncOSSResponse res;
	string content;
	content.assign(reinterpret_cast<char*>(fileBlock->GetBuffer()), fileBlock->GetWriteLength());
	_efspClient->SendToOSS(toSTLString(fileOssInfo.method), toSTLString(fileOssInfo.url), content, inHeaders, res);

	map<string, string>::iterator ithead = res.headers.begin();
	while(ithead != res.headers.end()){
		String headName = toCFLString(ithead->first);
		if (headName.compareIgnoreCase(_T("Etag")) == 0){
			//取消可能两面带引号的情况
			etag = toCFLString(ithead->second).trim (_T('"'));
			break;
		}
		ithead++;
	}

	NC_EFSP_CLIENT_TRACE (_T("end"));
}

NS_IMETHODIMP_(void)
ncEFSPFileOperation::RefreshUploadId(const String& docId,
									 const String& rev,
									 int64 length,
									 bool multiupload,
									 String& uploadid,
									 ncFileOssInfo& fileOssInfo)
{
	NC_EFSP_CLIENT_TRACE (_T("docId: %s, rev: %s, length: %d, multiupload:%s)"),
		docId.getCStr (), rev.getCStr (), length, multiupload?_T("true"):_T("false"));

	CheckDocId(docId);

	String url = GetUrl(_T("osuploadrefresh"));

	// 构造http内容
	JSON::Object json;
	json["docid"] = toSTLString(docId);
	json["rev"] = toSTLString(rev);
	json["length"] = length;
	json["multiupload"] = multiupload;
	json["reqhost"] = toSTLString(_efspClient->getIP ());

	string content;
	JSON::Writer::write(json, content);

	ncOSSResponse res;
	vector<string> headers;
	_efspClient->SendToAS(url, content, headers, res);
	JSON::Value value;
	JSON::Reader::read(value, res.body.c_str(), res.body.length());

	uploadid = toCFLString(value["uploadid"].s());
	JSON::Array& authrequest = value["authrequest"].a();
	fileOssInfo.method = toCFLString(authrequest[0]);
	fileOssInfo.method.toUpper();
	fileOssInfo.url = toCFLString(authrequest[1]);
	fileOssInfo.headers.clear();
	for (int i = 2; i < authrequest.size(); i++){
		fileOssInfo.headers.push_back(toCFLString(authrequest[i]));
	}

	NC_EFSP_CLIENT_TRACE (_T("end"));
}

NS_IMETHODIMP_(void)
ncEFSPFileOperation::StartDownLoadFile(const String& docId,
									   const String& rev,
									   const String& authtype,
									   ncFileVersionInfo& versionInfo,
									   ncFileOssInfo& fileOssInfo)
{
	NC_EFSP_CLIENT_TRACE (_T("docId: %s, rev: %s)"), docId.getCStr (), rev.getCStr ());

	CheckDocId(docId);

	String url = GetUrl(_T("osdownload"));

	// 构造http内容
	JSON::Object json;
	json["docid"] = toSTLString(docId);
	if (rev.getLength() > 0)
	{
		json["rev"] = toSTLString(rev);
	}
	json["reqhost"] = toSTLString(_efspClient->getIP ());

	string content;
	JSON::Writer::write(json, content);

	ncOSSResponse res;
	vector<string> headers;
	_efspClient->SendToAS(url, content, headers, res);
	JSON::Value value;
	JSON::Reader::read(value, res.body.c_str(), res.body.length());

	versionInfo.versionId = toCFLString (value["rev"].s ());
	versionInfo.fileName = toCFLString (value["name"].s ());
	versionInfo.editorName = toCFLString (value["editor"].s ());
	versionInfo.createTime = value["client_mtime"].i ();
	versionInfo.size = value["size"].i ();
	versionInfo.editTime = value["modified"].i ();

	JSON::Array& authrequest = value["authrequest"].a();
	fileOssInfo.method = toCFLString(authrequest[0]);
	fileOssInfo.method.toUpper();
	fileOssInfo.url = toCFLString(authrequest[1]);
	fileOssInfo.headers.clear();
	for (int i = 2; i < authrequest.size(); i++){
		fileOssInfo.headers.push_back(toCFLString(authrequest[i]));
	}

	NC_EFSP_CLIENT_TRACE (_T("end"));
}

NS_IMETHODIMP_(void)
ncEFSPFileOperation::StartDownLoadFileExt (const String& docId,
	const String& rev,
	const String& authtype,
	ncFileVersionInfo& versionInfo,
	ncFileOssInfo& fileOssInfo)
{
	NC_EFSP_CLIENT_TRACE(_T("docId: %s, rev: %s)"), docId.getCStr(), rev.getCStr());

	CheckDocId(docId);

	String url = GetUrl(_T("osdownloadext"));

	// 构造http内容
	JSON::Object json;

	json["authtype"] = toSTLString(_T("QUERY_STRING"));
	json["usehttps"] = false;

	json["docid"] = toSTLString(docId);
	if (rev.getLength() > 0)
	{
		json["rev"] = toSTLString(rev);
	}
	json["reqhost"] = toSTLString(_efspClient->getAddress());

	string content;
	JSON::Writer::write(json, content);

	ncOSSResponse res;
	vector<string> headers;
	_efspClient->SendToAS(url, content, headers, res);
	JSON::Value value;
	JSON::Reader::read(value, res.body.c_str(), res.body.length());

	versionInfo.versionId = toCFLString(value["rev"].s());
	versionInfo.fileName = toCFLString(value["name"].s());
	versionInfo.editorName = toCFLString(value["editor"].s());
	versionInfo.createTime = value["client_mtime"].i();
	versionInfo.size = value["size"].i();
	versionInfo.editTime = value["modified"].i();

	JSON::Array& authrequest = value["authrequest"].a();
	fileOssInfo.method = toCFLString(authrequest[0]);
	fileOssInfo.method.toUpper();
	fileOssInfo.url = toCFLString(authrequest[1]);
	fileOssInfo.headers.clear();
	for (int i = 2; i < authrequest.size(); i++) {
		fileOssInfo.headers.push_back(toCFLString(authrequest[i]));
	}

	NC_EFSP_CLIENT_TRACE(_T("end"));
}

NS_IMETHODIMP_(ncIEFSPFileBlock*)
ncEFSPFileOperation::DowndLoadFileData(const ncFileOssInfo& fileOssInfo,
									   const String& range)
{
	NC_EFSP_CLIENT_TRACE (_T("range: %s)"), range.getCStr ());

	vector<string> inHeaders;
	vector<String>::const_iterator ite = fileOssInfo.headers.begin();
	while (ite != fileOssInfo.headers.end())
	{
		inHeaders.push_back(toSTLString(*ite));
		ite++;
	}
	if (range.getLength() > 0)
	{
		string Range = "Range: ";
		Range += toSTLString(range);
		inHeaders.push_back(Range);
	}

	ncOSSResponse res;
	_efspClient->GetFromOSS(toSTLString(fileOssInfo.url), inHeaders, res);
	nsCOMPtr<ncIEFSPFileBlock> fileBlock = NC_NEW (ncEFSPClientAllocator, ncEFSPFileBlock) (0, (uint)res.body.length());
	memcpy (fileBlock->GetBuffer (),res.body.c_str(), res.body.length());
	fileBlock->SetWriteLength ((unsigned int)res.body.length());
	NS_ADDREF (fileBlock.get ());

	NC_EFSP_CLIENT_TRACE (_T("end"));
	return fileBlock;
}


NS_IMETHODIMP_(void)
ncEFSPFileOperation::RestoredVersion (const String& docId,
									  const String& rev,
									  String& name)
{
	NC_EFSP_CLIENT_TRACE (_T("docId: %s, rev: %s)"), docId.getCStr (), rev.getCStr ());

	CheckDocId(docId);

	String url = GetUrl(RESTORED_VERSION);

	// 构造http内容
	JSON::Object json;
	json["docid"] = toSTLString(docId);
	if (rev.getLength() > 0)
	{
		json["rev"] = toSTLString(rev);
	}

	string content;
	JSON::Writer::write(json, content);

	ncOSSResponse res;
	vector<string> headers;
	_efspClient->SendToAS(url, content, headers, res);

	JSON::Value value;
	JSON::Reader::read(value, res.body.c_str(), res.body.length());

	name = toCFLString (value["name"].s ());

	NC_EFSP_CLIENT_TRACE (_T("end"));
}


NS_IMETHODIMP_(void)
ncEFSPFileOperation::StartPreviewFile (const String& docId, 
									   const String& rev,
									   String& previewUrl,
									   int64& size)
{
	NC_EFSP_CLIENT_TRACE (_T("docId: %s, rev: %s"), docId.getCStr(), rev.getCStr ());

	String url = GetUrl (_T("previewoss"));

	string content;
	content.reserve (64);
	JSON::Object json;
	json["docid"] = toSTLString(docId);
	json["rev"] = toSTLString (rev);
	json["reqhost"] = toSTLString(_efspClient->getIP ());
	JSON::Writer::write (json, content);

	vector<string> headers;

	ncOSSResponse res;

	while (true) {
		try {
			headers.clear ();
			res.clear ();
			_efspClient->SendToAS(url, content, headers, res, 0);
			break;
		}
		catch (Exception& e) {
			// 首次预览时会进行转换，会返回提交转换的错误码，需要循环重试，直到发生错误或者返回url
			if (e.getErrorId () != 503002)
				throw;
			Thread::sleep (1000);
		}
	}

	JSON::Value value;
	JSON::Reader::read(value, res.body.c_str(), res.body.length());

	previewUrl = toCFLString (value["url"].s ());
	size = value["size"].i ();
}

void
ncEFSPFileOperation::CheckDocId (const String& docId)
{
	String cid = ncGNSUtil::GetCIDName (docId);
	if (cid.isEmpty () == true) {
		THROW_EFSP_CLIENT_ERROR (LOAD_STRING (_T("IDS_INVALID_DOC_ID")), INVALID_DOC_ID);
	}
}

void
ncEFSPFileOperation::CheckName (const String& name)
{
	if (name.isEmpty () == true) {
		THROW_EFSP_CLIENT_ERROR (LOAD_STRING (_T("IDS_INVALID_FILE_OR_DIR_NAME")), INVALID_FILE_OR_DIR_NAME);
	}
}

void
ncEFSPFileOperation::CheckVersionId (const String& verisonId)
{
	if (verisonId.isEmpty () == true) {
		THROW_EFSP_CLIENT_ERROR (LOAD_STRING (_T("IDS_INVALID_VERSION_ID")), INVALID_VERSION_ID);
	}
}

String
ncEFSPFileOperation::GetUrl (const String& method)
{
	String url;
	url.format (_T("%s?method=%s&userid=%s&tokenid=%s"),
		FILE_URI, 
		method.getCStr (), 
		_tokenInfo.userId.getCStr (),
		_tokenInfo.tokenId.getCStr ());

	return url;
}

String
ncEFSPFileOperation::GetSearchUrl (const String& method)
{
	String url;
	url.format (_T("%s?method=%s&userid=%s&tokenid=%s"),
		SEARCH_URL, 
		method.getCStr (), 
		_tokenInfo.userId.getCStr (),
		_tokenInfo.tokenId.getCStr ());

	return url;
}


void
ncEFSPFileOperation::GetBoundary (const string& header, string& boundary)
{
	NC_EFSP_CLIENT_TRACE (_T("begin"));

	const static string marker ("boundary=");
	size_t pos = header.find (marker);
	if (pos == string::npos) {
		NC_EFSP_CLIENT_TRACE (_T("can't find \"boundary=\""));
		THROW_EFSP_CLIENT_ERROR (LOAD_STRING (_T("IDS_GET_BOUNDARY_ERROR")), GET_BOUNDARY_ERROR);
	}
	boundary = header.substr (pos + marker.length ());

	NC_EFSP_CLIENT_TRACE (_T("end"));
}

void
ncEFSPFileOperation::DownloadSplit (const string& content, const string& boundary, ncEFSPBuffer& binary, ncEFSPBuffer& body, bool newVersion)
{
	NC_EFSP_CLIENT_TRACE (_T("begin"));

	string splitor (NC_EFS_HTTP_PREFIX);
	splitor += boundary;
	splitor += NC_EFS_HTTP_CRLF;
	if (newVersion) {
		splitor += NC_EFS_HTTP_CRLF;
	}

	size_t firstBoundary = content.find (splitor);
	if (firstBoundary == string::npos) {
		NC_EFSP_CLIENT_TRACE (_T("can't find firstBoundary"));
		THROW_EFSP_CLIENT_ERROR (LOAD_STRING (_T("IDS_SPLIT_BODY_ERROR")), SPLIT_BODY_ERROR);
	}

	firstBoundary += splitor.length ();

	// 获得数据块起始位置
	binary.buffer = content.c_str () + firstBoundary + 2;

	// 获得数据块结束位置
	size_t secondBoundary = content.find (splitor, firstBoundary);
	if (secondBoundary == string::npos) {
		NC_EFSP_CLIENT_TRACE (_T("can't find secondBoundary"));
		THROW_EFSP_CLIENT_ERROR (LOAD_STRING (_T("IDS_SPLIT_BODY_ERROR")), SPLIT_BODY_ERROR);
	}

	// 获得数据块长度
	binary.length = secondBoundary - firstBoundary - 2;

	secondBoundary += splitor.length ();

	body.buffer = content.c_str () + secondBoundary;

	// JSON串的长度不需要再行查找
	body.length = content.length () - secondBoundary;

	NC_EFSP_CLIENT_TRACE (_T("end"));
}

NS_IMETHODIMP_(void)
ncEFSPFileOperation::GetAppMetaData (const String& docId, 
									   String& appId,
									   vector<ncAppMetaDataInfo>& metaDataVec)
{
	NC_EFSP_CLIENT_TRACE (_T("docId: %s, appId: %s"), docId.getCStr(), appId.getCStr ());

	String url = GetUrl (_T("getappmetadata"));

	string content;
	content.reserve (64);
	JSON::Object json;
	json["docid"] = toSTLString(docId);
	json["appid"] = toSTLString (appId);
	JSON::Writer::write (json, content);

	ncOSSResponse res;
	vector<string> headers;
	_efspClient->SendToAS(url, content, headers, res);

	JSON::Value value;
	JSON::Reader::read(value, res.body.c_str(), res.body.length());

	JSON::Array& jArr = value.a();
	ncAppMetaDataInfo appDataInfo;

	size_t jArrSize = jArr.size();
	for (int iArrSize = 0; iArrSize < jArrSize; ++iArrSize) {
		appDataInfo.appId = toCFLString (jArr[iArrSize]["appid"].s ());
		appDataInfo.appName = toCFLString (jArr[iArrSize]["appname"].s ());
		appDataInfo.metaData = toCFLString(jArr[iArrSize]["appmetadata"]);
		metaDataVec.push_back (appDataInfo);
	}

	NC_EFSP_CLIENT_TRACE (_T("end"));
}

NS_IMETHODIMP_(void)
ncEFSPFileOperation::SetAppMetaData (const String& docId, 
									 const String& appId,
									 map<String, String>& metaDataMap)
{
	NC_EFSP_CLIENT_TRACE (_T("docId: %s, appId: %s"), docId.getCStr(), appId.getCStr ());

	String url = GetUrl (_T("setappmetadata"));

	string content;
	content.reserve (64);
	JSON::Object json;
	JSON::Object appMetaDataObj;
	json["docid"] = toSTLString(docId);
	json["appid"] = toSTLString (appId);
	for (map<String, String>::const_iterator it = metaDataMap.begin(); it != metaDataMap.end(); ++it) {
		appMetaDataObj.insert(make_pair(toSTLString(it->first), toSTLString(it->second)));
	}
	json["appmetadata"] = appMetaDataObj;
	JSON::Writer::write (json, content);

	ncOSSResponse res;
	vector<string> headers;
	_efspClient->SendToAS(url, content, headers, res);

	NC_EFSP_CLIENT_TRACE (_T("end"));
}
