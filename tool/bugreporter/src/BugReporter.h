
// BugReporter.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CBugReporterApp: 
// �йش����ʵ�֣������ BugReporter.cpp
//

class CBugReporterApp : public CWinApp
{
public:
	CBugReporterApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CBugReporterApp theApp;