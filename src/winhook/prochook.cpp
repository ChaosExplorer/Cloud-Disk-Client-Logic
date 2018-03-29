#include <abprec.h>
#include <psapi.h>
#include "winhook.h"
#include "ncHookUtil.h"
#include "ncHookAgent.h"
#include "procinject.h"

//
// ����Ƿ���Ҫע��ָ�����̣�
//     1.���û���صĽ��̲���Ҫע�롣
//     2.���ڱ����ԵĽ��̲���Ҫע�롣
//     3.δ��ʼ���Ľ�����Ҫ�ȴ���ʼ�������ע�롣
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
// ע��ָ�����̣�
//     1.ע��ǰ��Ҫ��������ǰ���̵Ĳ���Ȩ�ޡ�
//.....2.���Ŀ������뵱ǰ�����ڲ�ͬ�Ự�У���ע���ʧ�ܡ�
//     3.ע�����Ŀ����̵��ӽ��̲�����ע�롣
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
// ��ע��ָ�����̣�
//     1.��ע��ǰ��Ҫ��������ǰ���̵Ĳ���Ȩ�ޡ�
//.....2.���Ŀ������뵱ǰ�����ڲ�ͬ�Ự�У����ע���ʧ�ܡ�
//     3.��ע�����Ŀ����̵��ӽ��̲����н�ע�롣
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
// ע�����н��̣�
//     1.ע��ǰ��Ҫ��������ǰ���̵Ĳ���Ȩ�ޡ�
//     2.���Ŀ������뵱ǰ�����ڲ�ͬ�Ự�У��򲻽���ע�롣
//     3.���ע���ʱ���������¼ϵͳ��־��
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
// ��ע�����н��̣�
//     1.��ע��ǰ��Ҫ��������ǰ���̵Ĳ���Ȩ�ޡ�
//     2.���Ŀ������뵱ǰ�����ڲ�ͬ�Ự�У��򲻽��н�ע�롣
//     3.�����ע���ʱ���������¼ϵͳ��־��
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
