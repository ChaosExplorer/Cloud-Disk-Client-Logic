#include "stdafx.h"
#include "FileUtil.h"

FileUtil::FileUtil()
{
}


FileUtil::~FileUtil()
{
}

void FileUtil::CopyDirectory(CString source, CString target , bool create)
{
	if (create) {
		if (!PathIsDirectory(target))
			CreateDirectory(target, NULL);
	}

    CFileFind finder;  
    CString path;
	path = source + _T("\\*.*");

    bool bWorking = finder.FindFile(path);  

    while(bWorking){  
        bWorking = finder.FindNextFile();  

        if(finder.IsDirectory() && !finder.IsDots())
		{
            CopyDirectory(finder.GetFilePath(),target+_T("\\")+finder.GetFileName(), create);
        }  
        else{ 
            CopyFile(finder.GetFilePath(),target+_T("\\")+finder.GetFileName(), FALSE);  
        }  
    }  
}

string FileUtil::CString2str(const CString& tchar)
{
	 int iLen = WideCharToMultiByte(CP_ACP, 0, tchar, -1, NULL, 0, NULL, NULL);

	 char* chRtn =new char[iLen*sizeof(char)];

	 WideCharToMultiByte(CP_ACP, 0, tchar, -1, chRtn, iLen, NULL, NULL);

	string str(chRtn);

	return str;
}