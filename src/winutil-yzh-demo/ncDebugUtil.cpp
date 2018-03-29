/***************************************************************************************************
ncDebugUtil.h:
	Copyright (c) Eisoo Software, Inc.(2004-2015), All rights reserved.

Purpose:
	Source file for debug related utilities.

Author:
	Chaos

Creating Time:
	2015-03-08
***************************************************************************************************/

#include <abprec.h>
#include <imagehlp.h>
#include "winutil.h"

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

// public static
tstring ncDebugUtil::GetCurrentImageDirPath ()
{
	TCHAR szImagePath[MAX_PATH];
	if (GetModuleFileName ((HINSTANCE)&__ImageBase, szImagePath, MAX_PATH) == 0)
		return _T("");

	TCHAR* lpStr = _tcsrchr (szImagePath, _T('\\'));
	*lpStr = _T('\0');
	return szImagePath;
}

// public static
DWORD ncDebugUtil::GetImageFuncAddress (LPCSTR lpImagePath, LPCSTR lpFuncName)
{
	DWORD dwFuncAddr = 0;

	if (lpImagePath == NULL || lpFuncName == NULL)
		return 0;

	_LOADED_IMAGE LoadedImage;
	if (!MapAndLoad (lpImagePath, NULL, &LoadedImage, TRUE, TRUE))
		return 0;

	ULONG cDirSize;
	_IMAGE_EXPORT_DIRECTORY* pImgExport = (_IMAGE_EXPORT_DIRECTORY*) ImageDirectoryEntryToData (
		LoadedImage.MappedAddress, FALSE, IMAGE_DIRECTORY_ENTRY_EXPORT, &cDirSize);
	if (pImgExport == NULL)
	{
		UnMapAndLoad (&LoadedImage);
		return 0;
	}

	DWORD* names = (DWORD*)ImageRvaToVa (
		LoadedImage.FileHeader, LoadedImage.MappedAddress, pImgExport->AddressOfNames, NULL);
	DWORD* addrs = (DWORD*)ImageRvaToVa (
		LoadedImage.FileHeader, LoadedImage.MappedAddress, pImgExport->AddressOfFunctions, NULL);
	WORD* odns = (WORD*)ImageRvaToVa (
		LoadedImage.FileHeader, LoadedImage.MappedAddress, pImgExport->AddressOfNameOrdinals, NULL);

	if (names != NULL && addrs != NULL && odns != NULL)
	{
		CHAR* name = NULL;
		for (size_t i = 0; i < pImgExport->NumberOfNames; i++)
		{
			name = (CHAR*)ImageRvaToVa (LoadedImage.FileHeader, LoadedImage.MappedAddress, names[i], NULL);
			if (name != NULL && strcmp (name, lpFuncName) == 0 && odns[i] <= pImgExport->NumberOfFunctions)
			{
				dwFuncAddr = addrs[odns[i]];
				break;
			}
		}
	}

	UnMapAndLoad(&LoadedImage);
	return dwFuncAddr;
}
