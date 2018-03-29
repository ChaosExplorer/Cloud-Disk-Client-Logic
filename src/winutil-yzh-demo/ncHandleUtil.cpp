/***************************************************************************************************
ncHandleUtil.cpp:
	Copyright (c) Eisoo Software, Inc.(2004-2015), All rights reserved.

Purpose:
	Source file for handle related utilities.

Author:
	Chaos

Creating Time:
	2015-03-07
***************************************************************************************************/

#include <abprec.h>
#include <process.h>
#include "winutil.h"

// public static
BOOL ncHandleUtil::GetHandlePath (HANDLE handle, tstring& path)
{
	NTQUERYOBJECT fpNtQueryObject = 
		(NTQUERYOBJECT)GetProcAddress (GetModuleHandle (_T("ntdll")), "NtQueryObject");

	if (fpNtQueryObject == NULL)
		return FALSE;

	DWORD dwLength;
	OBJECT_NAME_INFORMATION info;
	NTSTATUS status = fpNtQueryObject (handle, ObjectNameInformation, &info, sizeof (info), &dwLength);
	if (status != STATUS_BUFFER_OVERFLOW)
		return FALSE;

	POBJECT_NAME_INFORMATION pInfo = (POBJECT_NAME_INFORMATION)malloc (dwLength);
	while (true)
	{
		status = fpNtQueryObject (handle, ObjectNameInformation, pInfo, dwLength, &dwLength);
		if (status != STATUS_BUFFER_OVERFLOW)
			break;
		pInfo = (POBJECT_NAME_INFORMATION)realloc (pInfo, dwLength);
	}

	BOOL bResult = FALSE;
	if (NT_SUCCESS (status))
	{
		path = pInfo->Name.Buffer;
		bResult = ncVolumeUtil::DevicePathToDrivePath (path);
	}

	free (pInfo);
	return bResult;
}

// public static
BOOL ncHandleUtil::FindFileHandle (LPCTSTR lpName, vector<ncFileHandle>& handles)
{
	handles.clear ();

	if (lpName == NULL)
		return FALSE;

	// �򿪡�NUL���ļ��Ա������ȡ�ļ��������ֵ��
	ncScopedHandle hTempFile = CreateFile (_T("NUL"), GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0);
	if (hTempFile == NULL)
		return FALSE;

	// ��ȡ��ǰϵͳ�����еľ����Ϣ��
	PSYSTEM_HANDLE_INFORMATION pshi = GetSystemHandleInfo ();
	if (pshi == NULL)
		return FALSE;

	BYTE nFileType = 0;
	DWORD dwCrtPid = GetCurrentProcessId ();

	// ��ȡ��ǰϵͳ���ļ��������ֵ��
	for (DWORD i = 0; i < pshi->dwCount; ++i)
	{
		if (pshi->Handles[i].dwProcessId == dwCrtPid && hTempFile == (HANDLE)pshi->Handles[i].wValue)
		{
			nFileType = pshi->Handles[i].bObjectType;
			break;
		}
	}

	HANDLE hCrtProc = GetCurrentProcess ();
	tstring path;

	for (DWORD i = 0; i < pshi->dwCount; ++i)
	{
		// ���˵����ļ����͵ľ����
		if (pshi->Handles[i].bObjectType != nFileType)
			continue;

		// ������������Ƶ���ǰ�����С�
		ncScopedHandle handle = NULL;
		ncScopedHandle hProc = OpenProcess (PROCESS_DUP_HANDLE, FALSE, pshi->Handles[i].dwProcessId);
		if (hProc == NULL ||
			!DuplicateHandle (hProc, (HANDLE)pshi->Handles[i].wValue, hCrtProc, &handle, 0, FALSE, DUPLICATE_SAME_ACCESS))
			continue;

		// ���˵��ᵼ��NtQueryObject�����ľ������ܵ��ȣ���
		if (IsBlockingHandle (handle))
			continue;

		// ��ȡ�����Ӧ���ļ�·��������ƥ���顣
		if (GetHandlePath (handle, path) && path.find (lpName) != tstring::npos)
		{
			ncFileHandle fh ((HANDLE)pshi->Handles[i].wValue, path, pshi->Handles[i].dwProcessId);
			handles.push_back (fh);
		}
	}

	free (pshi);
	return TRUE;
}

// public static
BOOL ncHandleUtil::CloseHandleEx (HANDLE handle, DWORD dwPid)
{
	if (GetCurrentProcessId () == dwPid)
		return CloseHandle (handle);

	ncScopedHandle hProcess = OpenProcess (PROCESS_DUP_HANDLE, FALSE, dwPid);
	if (hProcess == NULL)
		return FALSE;

	return DuplicateHandle (hProcess, handle, GetCurrentProcess (), NULL, 0, FALSE, DUPLICATE_CLOSE_SOURCE);
}

// protected static
PSYSTEM_HANDLE_INFORMATION ncHandleUtil::GetSystemHandleInfo ()
{
	NTQUERYSYSTEMINFORMATION fpNtQuerySystemInformation = 
		(NTQUERYSYSTEMINFORMATION)GetProcAddress (GetModuleHandle (_T("ntdll")), "NtQuerySystemInformation");

	if (fpNtQuerySystemInformation == NULL)
		return NULL;

	DWORD dwLength;
	SYSTEM_HANDLE_INFORMATION shi;
	NTSTATUS status = fpNtQuerySystemInformation (SystemHandleInformation, &shi, sizeof (shi), &dwLength);
	if (NT_SUCCESS (status))
		return NULL;

	PSYSTEM_HANDLE_INFORMATION pshi = (PSYSTEM_HANDLE_INFORMATION)malloc (dwLength);
	while (true)
	{
		status = fpNtQuerySystemInformation (SystemHandleInformation, pshi, dwLength, &dwLength);
		if (status != STATUS_INFO_LENGTH_MISMATCH)
			break;
		pshi = (PSYSTEM_HANDLE_INFORMATION)realloc (pshi, dwLength);
	}

	if (!NT_SUCCESS (status))
	{
		free (pshi);
		pshi = NULL;
	}

	return pshi;
}

//
// ���ָ������Ƿ���ܵ���NtQueryObject������
//     1.ע�����ʹ��NtQueryInformationFile������NtQueryObject���м�⣬�������
//       ����WinXPϵͳ�½����������޷�������
//
unsigned WINAPI CheckBlockThreadFunc (void* param)
{
	NTQUERYINFORMATIONFILE fpNtQueryInformationFile = 
		(NTQUERYINFORMATIONFILE)GetProcAddress (GetModuleHandle (_T("ntdll")), "NtQueryInformationFile");

	if (fpNtQueryInformationFile != NULL)
	{
		BYTE buf[1024];
		IO_STATUS_BLOCK ioStatus;
		fpNtQueryInformationFile ((HANDLE)param, &ioStatus, buf, 1024, FileNameInformation);
	}

	return 0;
}

// protected static
BOOL ncHandleUtil::IsBlockingHandle (HANDLE handle)
{
	ncScopedHandle hThread = (HANDLE)_beginthreadex(NULL, 0, CheckBlockThreadFunc, (void*)handle, 0, NULL);
	SwitchToThread();

	if (WaitForSingleObject (hThread, 10) == WAIT_OBJECT_0)
		return FALSE;

	TerminateThread (hThread, 0);
	return TRUE;
}
