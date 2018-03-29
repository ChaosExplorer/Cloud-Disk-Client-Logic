/***************************************************************************************************
ncServiceUtil.cpp:
	Copyright (c) Eisoo Software, Inc.(2004-2015), All rights reserved.

Purpose:
	Source file for service related utilities.

Author:
	Chaos

Creating Time:
	2015-03-06
***************************************************************************************************/

#include <abprec.h>
#include "winutil.h"

// public static
DWORD ncServiceUtil::GetServiceProcessId (LPCTSTR lpSvcName)
{
	DWORD dwProcId = 0;

	if (lpSvcName == NULL)
		return 0;

	SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, GENERIC_READ);
	if (hSCManager != NULL)
	{
		SC_HANDLE hService = OpenService(hSCManager, lpSvcName, GENERIC_READ);
		if (hService != NULL)
		{
			SERVICE_STATUS_PROCESS ssProcess;
			DWORD dwBytesNeeded;

			if (QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO,
					(LPBYTE)&ssProcess, sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded))
			{
				dwProcId = ssProcess.dwProcessId;
			}
			CloseServiceHandle(hService);
		}
		CloseServiceHandle(hSCManager);
	}

	return dwProcId;
}
