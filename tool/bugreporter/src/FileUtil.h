/**
*	@Created Time : 2016-6-29
*	@Author : Chaos
**/

#pragma once

#include <string>
using namespace std;


class FileUtil
{
public:
	FileUtil();
	~FileUtil();

public:

	static void CopyDirectory(CString source, CString target, bool create = true);

	static string CString2str(const CString& tchar);
};

