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
	// 获取IShellWindows对象
	CComPtr<IShellWindows> psw;
	if (!SUCCEEDED (CoCreateInstance (CLSID_ShellWindows, NULL, CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER, IID_IShellWindows, (void**)&psw)))
		return;

	// 获取所有Shell窗口的总个数
	long nNum = 0;
	if (!SUCCEEDED (psw->get_Count (&nNum)))
		return;

	// 针对每个shell窗口进行操作
	VARIANT var = { VT_I4 };
	for (V_I4 (&var) = nNum - 1; V_I4 (&var) >= 0; V_I4 (&var)--) {
		// 获取IDisPatch对象（用于获取IWebBrowser对象）
		CComPtr<IDispatch> pdisp;
		if (!SUCCEEDED (psw->Item (var, &pdisp)) || pdisp == NULL)
			continue;

		// 获取IWebBrowser对象（代表WebBrowser风格的窗口）
		CComPtr<IWebBrowser2> pwb;
		if (!SUCCEEDED (pdisp->QueryInterface (IID_IWebBrowser2, (void**)&pwb)))
			continue;

		// 获取IServiceProvider对象（用于获取IShellBrowser对象）
		CComPtr<IServiceProvider> psp;
		if (!SUCCEEDED (pwb->QueryInterface (IID_IServiceProvider, (void**)&psp)))
			continue;

		// 获取IShellBrowser对象（代表资源浏览器窗口）
		CComPtr<IShellBrowser> psb;
		if (!SUCCEEDED (psp->QueryService (SID_STopLevelBrowser, IID_IShellBrowser, (void**)&psb)))
			continue;

		// 获取IShellView对象（代表整个资源管理器窗口视图）
		CComPtr<IShellView> psv;
		if (!SUCCEEDED (psb->QueryActiveShellView (&psv)))
			continue;

		// 获取IFolderView对象（代表文件夹窗口视图）
		CComPtr<IFolderView> pfv;
		if (!SUCCEEDED (psv->QueryInterface (IID_IFolderView, (void**)&pfv)))
			continue;

		// 获取IPersistFolder2对象（用于获取文件夹的信息）
		CComPtr<IPersistFolder2> ppf;
		if (!SUCCEEDED (pfv->GetFolder (IID_IPersistFolder2, (void**)&ppf)))
			continue;

		TCHAR szFolderPath[MAX_PATH];
		LPITEMIDLIST pidlFolder = NULL;

		// 获取当前文件夹路径，并与目标路径进行匹配检查，若匹配则关闭该文件夹窗口
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
