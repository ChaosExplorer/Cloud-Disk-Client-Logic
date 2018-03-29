#include <abprec.h>
#include <psapi.h>
#include "winhook.h"
#include "ncHookUtil.h"
#include "ncHookAgent.h"
#include "procinject.h"

//
// 检查是否需要注入指定进程：
//     1.非用户相关的进程不需要注入。
//     2.正在被调试的进程不需要注入。
//     3.未初始化的进程需要等待初始化完成再注入。
//
static BOOL CheckCanHook(HANDLE hProcess)
{
	if (!ncHookUtil::IsUserProcess(hProcess) && !ncHookUtil::IsDcomLaunchProcess(hProcess))
		return FALSE;

	ncProcessFlags flags = ncProcessUtil::GetProcessFlags(hProcess);

	if (flags & NC_PF_DEBUGGED)
		return FALSE;

	if (flags & NC_PF_UNINITIALIZED)
		WaitForInputIdle(hProcess, 5000);

	return TRUE;
}

//
// 注入指定进程：
//     1.注入前需要先提升当前进程的操作权限。
//.....2.如果目标进程与当前进程在不同会话中，则注入会失败。
//     3.注入后检查目标进程的子进程并进行注入。
//
void WINAPI HookOneProcess(DWORD dwProcId)
{
	WINHOOK_TRACE(_T("begin - HookOneProcess: dwProcId: %d"), dwProcId);

	ncProcessUtil::AdjustProcessPrivilege(GetCurrentProcess(), SE_DEBUG_NAME, TRUE);

	unsigned long		hookMode;
	vector<tstring>		excludePaths;
	vector<tstring>		excludeProcesses;
	vector<tstring>		includePaths;
	vector<tstring>		includeProcesses;
	ncWinHookUtil::GetInjecInfo(hookMode, excludePaths, excludeProcesses, includePaths, includeProcesses);

	ncScopedHandle hProcess = OpenProcess(MAXIMUM_ALLOWED, FALSE, dwProcId);
	if (hProcess != NULL && CheckCanHook(hProcess))
	{
		tstring procPath, procName;
		ncProcessUtil::GetProcessPathAndName(hProcess, procPath, procName);
		if (!procPath.empty() && !procName.empty() &&
			ncHookUtil::NeedInject(procPath, procName, hookMode, excludePaths, excludeProcesses, includePaths, includeProcesses)) {
			InjectProcess(hProcess, procPath, procName);
		}
	}

	vector<DWORD> dwPids;
	ncProcessUtil::GetProcessChildren(dwProcId, dwPids);

	vector<DWORD>::iterator iter;
	for (iter = dwPids.begin(); iter != dwPids.end(); ++iter)
	{
		hProcess = OpenProcess(MAXIMUM_ALLOWED, FALSE, *iter);
		if (hProcess != NULL)
		{
			ncHookAgent::HookProcessByAgent(hProcess, TRUE);
		}
	}

	WINHOOK_TRACE(_T("end - HookOneProcess: dwProcId: %d"), dwProcId);
}

//
// 解注入指定进程：
//     1.解注入前需要先提升当前进程的操作权限。
//.....2.如果目标进程与当前进程在不同会话中，则解注入会失败。
//     3.解注入后检查目标进程的子进程并进行解注入。
//
void WINAPI UnhookOneProcess(DWORD dwProcId)
{
	WINHOOK_TRACE(_T("begin - UnhookOneProcess: dwProcId: %d"), dwProcId);

	ncProcessUtil::AdjustProcessPrivilege(GetCurrentProcess(), SE_DEBUG_NAME, TRUE);

	ncScopedHandle hProcess = OpenProcess(MAXIMUM_ALLOWED, FALSE, dwProcId);
	if (hProcess != NULL)
	{
		EjectProcess(hProcess);
	}

	vector<DWORD> dwPids;
	ncProcessUtil::GetProcessChildren(dwProcId, dwPids);

	vector<DWORD>::iterator iter;
	for (iter = dwPids.begin(); iter != dwPids.end(); ++iter)
	{
		hProcess = OpenProcess(MAXIMUM_ALLOWED, FALSE, *iter);
		if (hProcess != NULL)
		{
			ncHookAgent::HookProcessByAgent(hProcess, FALSE);
		}
	}

	WINHOOK_TRACE(_T("end - UnhookOneProcess: dwProcId: %d"), dwProcId);
}

//
// 注入所有进程：
//     1.注入前需要先提升当前进程的操作权限。
//     2.如果目标进程与当前进程在不同会话中，则不进行注入。
//     3.如果注入耗时过长，则记录系统日志。
//
void WINAPI HookAllProcess()
{
	WINHOOK_TRACE(_T("begin - HookAllProcess"));

	DWORD dwStartTime = ::GetTickCount();

	ncProcessUtil::AdjustProcessPrivilege(GetCurrentProcess(), SE_DEBUG_NAME, TRUE);

	DWORD dwProcIds[512];
	DWORD dwBytesReturned;
	EnumProcesses(dwProcIds, sizeof(dwProcIds), &dwBytesReturned);

	DWORD dwFromSesId;
	if (!ProcessIdToSessionId(GetCurrentProcessId(), &dwFromSesId))
		return;

	DWORD dwToSesId;
	ncScopedHandle hProcess = NULL;

	unsigned long		hookMode = HOOK_MODE_MAX;
	vector<tstring>		excludePaths;
	vector<tstring>		excludeProcesses;
	vector<tstring>		includePaths;
	vector<tstring>		includeProcesses;
	ncWinHookUtil::GetInjecInfo(hookMode, excludePaths, excludeProcesses, includePaths, includeProcesses);

	for (DWORD i = 0; i < dwBytesReturned / sizeof(DWORD); ++i)
	{
		if (!ProcessIdToSessionId(dwProcIds[i], &dwToSesId) || dwToSesId != dwFromSesId)
			continue;

		hProcess = OpenProcess(MAXIMUM_ALLOWED, FALSE, dwProcIds[i]);
		if (hProcess != NULL && CheckCanHook(hProcess))
		{
			tstring procPath, procName;
			ncProcessUtil::GetProcessPathAndName(hProcess, procPath, procName);
			if (!procPath.empty() && !procName.empty() && 
				ncHookUtil::NeedInject(procPath, procName, hookMode, excludePaths, excludeProcesses, includePaths, includeProcesses)) {
				InjectProcess(hProcess, procPath, procName);
			}
		}
	}

	DWORD dwUsedTime = ::GetTickCount() - dwStartTime;
	if (dwUsedTime > 5000)
	{
		ncWriteEventLog(EVENTLOG_INFORMATION_TYPE, _T("HookAllProcess cost %d seconds."), dwUsedTime / 1000);
	}

	WINHOOK_TRACE(_T("end - HookAllProcess"));
}

//
// 解注入所有进程：
//     1.解注入前需要先提升当前进程的操作权限。
//     2.如果目标进程与当前进程在不同会话中，则不进行解注入。
//     3.如果解注入耗时过长，则记录系统日志。
//
void WINAPI UnhookAllProcess()
{
	WINHOOK_TRACE(_T("begin - UnhookAllProcess"));

	DWORD dwStartTime = ::GetTickCount();

	ncProcessUtil::AdjustProcessPrivilege(GetCurrentProcess(), SE_DEBUG_NAME, TRUE);

	DWORD dwProcIds[512];
	DWORD dwBytesReturned;
	EnumProcesses(dwProcIds, sizeof(dwProcIds), &dwBytesReturned);

	DWORD dwFromSesId;
	if (!ProcessIdToSessionId(GetCurrentProcessId(), &dwFromSesId))
		return;

	DWORD dwToSesId;
	ncScopedHandle hProcess = NULL;

	for (DWORD i = 0; i < dwBytesReturned / sizeof(DWORD); ++i)
	{
		if (!ProcessIdToSessionId(dwProcIds[i], &dwToSesId) || dwToSesId != dwFromSesId)
			continue;

		hProcess = OpenProcess(MAXIMUM_ALLOWED, FALSE, dwProcIds[i]);
		if (hProcess != NULL)
		{
			EjectProcess(hProcess);
		}
	}

	DWORD dwUsedTime = ::GetTickCount() - dwStartTime;
	if (dwUsedTime > 5000)
	{
		ncWriteEventLog(EVENTLOG_INFORMATION_TYPE, _T("UnhookAllProcess cost %d seconds."), dwUsedTime / 1000);
	}

	WINHOOK_TRACE(_T("end - UnhookAllProcess"));
}
