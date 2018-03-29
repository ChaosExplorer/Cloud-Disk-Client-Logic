/***************************************************************************************************
ncShellUtil.cpp:
	Copyright (c) Eisoo Software, Inc.(2004-2015), All rights reserved.

Purpose:
	Source file for shell related utilities.

Author:
	Chaos

Created Time:
	2015-05-24
***************************************************************************************************/

#include <abprec.h>
#include <atlbase.h>
#include <shlobj.h>
#include <shlwapi.h>
#include "winutil.h"

// public static
void ncShellUtil::RefreshShellWindows ()
{
	HWND hCWC = NULL;
	while (hCWC = FindWindowEx (NULL, hCWC, _T("CabinetWClass"), NULL)){
		PostMessage (hCWC, WM_COMMAND, 41504, NULL);
	}
}

// public static
void ncShellUtil::CloseShellWindows (LPCTSTR lpClosePath/* = NULL*/)
{
	// ��ȡIShellWindows����
	CComPtr<IShellWindows> psw;
	if (!SUCCEEDED (CoCreateInstance (CLSID_ShellWindows, NULL, CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER, IID_IShellWindows, (void**)&psw)))
		return;

	// ��ȡ����Shell���ڵ��ܸ���
	long nNum = 0;
	if (!SUCCEEDED (psw->get_Count (&nNum)))
		return;

	// ���ÿ��shell���ڽ��в���
	VARIANT var = { VT_I4 };
	for (V_I4 (&var) = nNum - 1; V_I4 (&var) >= 0; V_I4 (&var)--) {
		// ��ȡIDisPatch�������ڻ�ȡIWebBrowser����
		CComPtr<IDispatch> pdisp;
		if (!SUCCEEDED (psw->Item (var, &pdisp)) || pdisp == NULL)
			continue;

		// ��ȡIWebBrowser���󣨴���WebBrowser���Ĵ��ڣ�
		CComPtr<IWebBrowser2> pwb;
		if (!SUCCEEDED (pdisp->QueryInterface (IID_IWebBrowser2, (void**)&pwb)))
			continue;

		// ��ȡIServiceProvider�������ڻ�ȡIShellBrowser����
		CComPtr<IServiceProvider> psp;
		if (!SUCCEEDED (pwb->QueryInterface (IID_IServiceProvider, (void**)&psp)))
			continue;

		// ��ȡIShellBrowser���󣨴�����Դ��������ڣ�
		CComPtr<IShellBrowser> psb;
		if (!SUCCEEDED (psp->QueryService (SID_STopLevelBrowser, IID_IShellBrowser, (void**)&psb)))
			continue;

		// ��ȡIShellView���󣨴���������Դ������������ͼ��
		CComPtr<IShellView> psv;
		if (!SUCCEEDED (psb->QueryActiveShellView (&psv)))
			continue;

		// ��ȡIFolderView���󣨴����ļ��д�����ͼ��
		CComPtr<IFolderView> pfv;
		if (!SUCCEEDED (psv->QueryInterface (IID_IFolderView, (void**)&pfv)))
			continue;

		// ��ȡIPersistFolder2�������ڻ�ȡ�ļ��е���Ϣ��
		CComPtr<IPersistFolder2> ppf;
		if (!SUCCEEDED (pfv->GetFolder (IID_IPersistFolder2, (void**)&ppf)))
			continue;

		TCHAR szFolderPath[MAX_PATH];
		LPITEMIDLIST pidlFolder = NULL;

		// ��ȡ��ǰ�ļ���·��������Ŀ��·������ƥ���飬��ƥ����رո��ļ��д���
		if (SUCCEEDED (ppf->GetCurFolder (&pidlFolder)) && SHGetPathFromIDList (pidlFolder, szFolderPath)) {
			if (lpClosePath == NULL || ncPathUtil::IsParentPath (lpClosePath, szFolderPath, TRUE)) {
				pwb->Quit ();
			}
		}

		CoTaskMemFree (pidlFolder);
	}
}

// public static
void ncShellUtil::CustomizeFolder (LPCTSTR lpPath, LPCTSTR lpInfoTip, LPCTSTR lpIconFile, int nIconIndex)
{
	if (lpPath == NULL)
		return;

	SHFOLDERCUSTOMSETTINGS fcs = {0};
	fcs.dwSize = sizeof (SHFOLDERCUSTOMSETTINGS);

	if (lpInfoTip != NULL)
	{
		fcs.dwMask |= FCSM_INFOTIP;
		fcs.pszInfoTip = (LPTSTR)lpInfoTip;
		fcs.cchInfoTip = 0;
	}

	if (lpIconFile != NULL)
	{
		fcs.dwMask |= FCSM_ICONFILE;
		fcs.pszIconFile = (LPTSTR)lpIconFile;
		fcs.cchIconFile = 0;
		fcs.iIconIndex = nIconIndex;
	}

	SHGetSetFolderCustomSettings (&fcs, lpPath, FCS_FORCEWRITE);
}

// public static
void ncShellUtil::RegUrlProtocol (LPCTSTR lpName, LPCTSTR lpOpenCmd, LPCTSTR lpDesc, LPCTSTR lpDefIcon)
{
	if (lpName == NULL || lpOpenCmd == NULL)
		return;

	CRegKey rk;
	if (rk.Create (HKEY_CLASSES_ROOT, lpName) == ERROR_SUCCESS)
	{
		if (lpDesc != NULL)
			rk.SetStringValue (NULL, lpDesc);

		rk.SetStringValue (_T("URL Protocol"), _T(""));

		if (lpDefIcon != NULL)
			rk.SetKeyValue (_T("DefaultIcon"), lpDefIcon);

		rk.SetKeyValue (_T("shell\\open\\command"), lpOpenCmd);
	}
}

// public static
void ncShellUtil::UnregUrlProtocol (LPCTSTR lpName)
{
	if (lpName == NULL)
		return;

	SHDeleteKey (HKEY_CLASSES_ROOT, lpName);
}

// public static
void ncShellUtil::CreateLink (LPCTSTR lpObjPath, LPCTSTR lpLinkPath, LPCTSTR lpDesc)
{
	if (lpObjPath == NULL || lpLinkPath == NULL)
		return;

	CoInitialize (NULL);

	CComPtr<IShellLink> psl;
	HRESULT hres = CoCreateInstance (CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl);

	if (SUCCEEDED (hres))
	{
		psl->SetPath (lpObjPath);
		psl->SetDescription (lpDesc);

		CComPtr<IPersistFile> ppf;
		hres = psl->QueryInterface (IID_IPersistFile, (LPVOID*)&ppf);

		if (SUCCEEDED (hres))
		{
			CT2CW lpwLinkPath (lpLinkPath);
			ppf->Save (lpwLinkPath, TRUE);
		}
	}

	CoUninitialize();
}

// public static
void ncShellUtil::RemoveLink (LPCTSTR lpLinkPath)
{
	if (lpLinkPath == NULL)
		return;

	if (ncFileUtil::IsFileExists (lpLinkPath))
		DeleteFile (lpLinkPath);
}
