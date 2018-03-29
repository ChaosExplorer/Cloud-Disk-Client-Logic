/***************************************************************************************************
Purpose:
	efsp file operation

Author:
	Chaos
			
Creating Time:
	2013-07-24
***************************************************************************************************/
#ifndef __NC_EFSP_FILE_OPERATION_H__
#define __NC_EFSP_FILE_OPERATION_H__

#include "./public/ncIEFSPFileOperation.h"
#include "ncEFSPClient.h"

struct ncEFSPBuffer
{
	ncEFSPBuffer (void)
		: buffer (0)
		, length (0)
	{
	}
	const char* buffer;
	size_t length;
};

class ncEFSPFileOperation : public ncIEFSPFileOperation
{
public:
	NS_DECL_ISUPPORTS
	NS_DECL_NCIEFSPFILEOPERATION

	ncEFSPFileOperation (ncEFSPClient* efspClient, const ncTokenInfo& tokenInfo);

private:
	~ncEFSPFileOperation ();

	void CheckDocId (const String& docId);
	void CheckName (const String& name);
	void CheckVersionId (const String& verisonId);
	String GetUrl (const String& method);
	String GetSearchUrl (const String& method);
	void GetBoundary (const string& header, string& boundary);
	void DownloadSplit (const string& content, const string& boundary, ncEFSPBuffer& binary, ncEFSPBuffer& body, bool newVersion);

private:
	nsCOMPtr<ncEFSPClient>		_efspClient;
	ncTokenInfo					_tokenInfo;
};

#endif // End of __NC_EFSP_FILE_OPERATION_H__
