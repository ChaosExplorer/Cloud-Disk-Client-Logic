/***************************************************************************************************
ncVolumeUtil.cpp:
	Copyright (c) Eisoo Software, Inc.(2004-2015), All rights reserved.

Purpose:
	Source file for volume related utilities.

Author:
	Chaos

Created Time:
	2015-03-09
***************************************************************************************************/

#include <abprec.h>
#include <atlbase.h>
#include "winutil.h"

static tstring GetSymLinkTarget (const tstring& lpLink)
{
	HMODULE hNtDll = GetModuleHandle(_T("ntdll"));
	RTLINITUNICODESTRING fpRtlInitUnicodeString = (RTLINITUNICODESTRING)GetProcAddress (hNtDll, "RtlInitUnicodeString");
	NTOPENSYMBOLICLINKOBJECT fpNtOpenSymbolicLinkObject = (NTOPENSYMBOLICLINKOBJECT)GetProcAddress (hNtDll, "NtOpenSymbolicLinkObject");
	NTQUERYSYMBOLICLINKOBJECT fpNtQuerySymbolicLinkObject = (NTQUERYSYMBOLICLINKOBJECT)GetProcAddress (hNtDll, "NtQuerySymbolicLinkObject");
	NTCLOSE fpNtClose =  (NTCLOSE)GetProcAddress (hNtDll, "NtClose");

	tstring sLink (lpLink);

	while (true) {
		UNICODE_STRING usLink = {0};
		fpRtlInitUnicodeString (&usLink, CT2CW (sLink.c_str ()));

		OBJECT_ATTRIBUTES Object = {0};
		InitializeObjectAttributes (&Object, &usLink, OBJ_CASE_INSENSITIVE, 0, 0);

		HANDLE hLink = NULL;
		NTSTATUS status = fpNtOpenSymbolicLinkObject (&hLink, GENERIC_READ, &Object);
		if (!NT_SUCCESS (status))
			break;

		WCHAR* pTarget = NULL;
		UNICODE_STRING usTarget = {0};
		ULONG retLen = 0;

		if (fpNtQuerySymbolicLinkObject (hLink, &usTarget, &retLen) == STATUS_BUFFER_TOO_SMALL) {
			pTarget = new WCHAR[retLen];
			usTarget.MaximumLength = (USHORT)retLen;
			usTarget.Buffer = pTarget;
		}

		status = fpNtQuerySymbolicLinkObject (hLink, &usTarget, &retLen);
		if (NT_SUCCESS (status)) {
			sLink = usTarget.Buffer;
		}

		delete pTarget;
		fpNtClose (hLink);

		if (!NT_SUCCESS (status))
			break;
	}

	return sLink;
}

// public static
void ncVolumeUtil::GetDeviceDriveMap (map<tstring, tstring>& devDrvMap)
{
	const DWORD BUF_SIZE = 512;
	const tstring DOS_PREFIX (_T("\\DosDevices\\"));

	TCHAR szDrives[BUF_SIZE];
	if (!GetLogicalDriveStrings (BUF_SIZE - 1, szDrives))
		return;

	TCHAR* lpDrives = szDrives;
	TCHAR szDrive[3] = _T(" :");
	tstring sDevice;

	do {
		*szDrive = *lpDrives;

		sDevice = GetSymLinkTarget (DOS_PREFIX + szDrive);
		devDrvMap[sDevice] = szDrive;

		while (*lpDrives++);
	} while (*lpDrives);
}

// public static
BOOL ncVolumeUtil::DevicePathToDrivePath (tstring& path)
{
	static map<tstring, tstring> devDrvMap;

	if (devDrvMap.empty ())
		GetDeviceDriveMap (devDrvMap);

	size_t nLength;
	map<tstring, tstring>::const_iterator iter;

	for (iter = devDrvMap.begin (); iter != devDrvMap.end (); ++iter) {
		nLength = iter->first.length ();
		if (_tcsnicmp (iter->first.c_str (), path.c_str (), nLength) == 0) {
			path.replace (0, nLength, iter->second);
			return TRUE;
		}
	}

	return FALSE;
}
