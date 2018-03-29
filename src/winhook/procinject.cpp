#include <abprec.h>
#include <atlbase.h>
#include <Psapi.h>
#include "winhook.h"
#include "ncHookUtil.h"
#include "ncHookAgent.h"
#include "procinject.h"

extern TCHAR g_szDirPath[MAX_PATH];

//
// ��ȡԶ���߳�ע���ַ��
//     1.�����ǰ������Ŀ�����λ����ͬ����ֱ��ʹ�õ�ǰ���̵�LoadLibraryA��ַ��Ϊע���ַ��
//     2.�����ǰ����Ϊ64λ��Ŀ�����Ϊ32λ������Ҫ��Ŀ��������������LoadLibraryA��ַ��Ϊע���ַ��
//     3.�����ǰ����Ϊ32λ��Ŀ�����Ϊ64λ����ͨ��������̽���ע�롣
//
static PVOID GetInjectAddress(HANDLE hProcess, BOOL bFrom32Bit, BOOL bTo32Bit)
{
	PVOID result = NULL;

	if (bFrom32Bit == bTo32Bit)
		result = GetProcAddress(GetModuleHandle(_T("kernel32")), "LoadLibraryA");
	else if (bTo32Bit)
		result = ncProcessUtil::GetProcessFuncAddress(hProcess, _T("kernel32.dll"), "LoadLibraryA");
	else
		ncHookAgent::HookProcessByAgent(hProcess, TRUE);

	return result;
}

//
// ��ȡԶ���߳̽�ע���ַ��
//     1.�����ǰ������Ŀ�����λ����ͬ����ֱ��ʹ�õ�ǰ���̵�FreeLibrary��ַ��Ϊ��ע���ַ��
//     2.�����ǰ����Ϊ64λ��Ŀ�����Ϊ32λ������Ҫ��Ŀ��������������FreeLibrary��ַ��Ϊ��ע���ַ��
//     3.�����ǰ����Ϊ32λ��Ŀ�����Ϊ64λ����ͨ��������̽��н�ע�롣
//
static PVOID GetEjectAddress(HANDLE hProcess, BOOL bFrom32Bit, BOOL bTo32Bit)
{
	PVOID result = NULL;

	if (bFrom32Bit == bTo32Bit)
		result = GetProcAddress(GetModuleHandle(_T("kernel32")), "FreeLibrary");
	else if (bTo32Bit)
		result = ncProcessUtil::GetProcessFuncAddress(hProcess, _T("kernel32.dll"), "FreeLibrary");
	else
		ncHookAgent::HookProcessByAgent(hProcess, FALSE);

	return result;
}

//
// ִ�н���ע�룺
//     1.���Ŀ������ѱ�ע��������ظ�ע�롣
//     2.��ȡĿ�����ע���ַ��ִ��Զ���߳�ע�롣
//
void InjectProcess(HANDLE hProcess, tstring procPath, tstring procName)
{
	WINHOOK_TRACE(_T("begin - InjectProcess: dwProcId: %d"), GetProcessId(hProcess));

	// ע��ȴ���ĳЩ������Ҫ�ӳ�ע��
	vector<ncHookInfoDelay> delayPath;
	vector<ncHookInfoDelay> delayProcess;
	ncWinHookUtil::GetDelayInfo(delayPath, delayProcess);

	bool needDelay = false;
	unsigned long mode;
	tstring condtion;
	tstring temp;

	for (size_t i = 0; i < delayPath.size(); i++) {
		if (delayPath[i].dest.length() <= procPath.length()) {
			temp = procPath.substr(0, delayPath[i].dest.length());
			if (_tcsicmp(temp.c_str(), delayPath[i].dest.c_str()) == 0) {
				needDelay = true;
				mode = delayPath[i].mode;
				condtion = delayPath[i].condtion;
				break;
			}
		}
	}
	if (!needDelay) {
		for (size_t i = 0; i < delayProcess.size(); i++) {
			if (_tcsicmp(procName.c_str(), delayProcess[i].dest.c_str()) == 0) {
				needDelay = true;
				mode = delayProcess[i].mode;
				condtion = delayProcess[i].condtion;
				break;
			}
		}
	}

	if (needDelay) {
		if (mode == DEALY_MODE_WINDOW) {
			// �����ӳ�ǰ�ȵȴ�һ�룬ĳЩ����½��̳����ˣ����ǵȴ��Ĵ��ڻ�û����
			Sleep(1000);
			while (TRUE) {
				HWND wHandle = FindWindowEx(NULL, NULL, condtion.c_str(), NULL);
				if (wHandle) {
					DWORD dwProcessId;
					if (GetWindowThreadProcessId (wHandle, &dwProcessId) && dwProcessId != GetProcessId(hProcess)) {
						break;
					}
				}
				else{
					break;
				}
				Sleep(1000);
			}
		}
		else if (mode == DEALY_MODE_TIME) {
			unsigned long delayTime = _tcstoul(condtion.c_str(), 0, 10);
			Sleep(delayTime);
		}
	}

	BOOL bFrom32Bit = ncProcessUtil::Is32BitProcess(GetCurrentProcess());
	BOOL bTo32Bit = ncProcessUtil::Is32BitProcess(hProcess);

	HMODULE hMod = ncProcessUtil::GetProcessModuleHandle(hProcess, (bTo32Bit ? HOOK_DLL_NAME : HOOK_DLL64_NAME));
	if (hMod != NULL)
		return;

	PVOID pfnRemote = GetInjectAddress(hProcess, bFrom32Bit, bTo32Bit);
	if (pfnRemote == NULL)
		return;

	TCHAR szDllPath[MAX_PATH];
	_stprintf(szDllPath, _T("%s\\%s"), g_szDirPath, (bTo32Bit ? HOOK_DLL_NAME : HOOK_DLL64_NAME));

	CT2CA lpDllPath((LPCTSTR)szDllPath);
	size_t dwSize = strlen(lpDllPath) + 1;

	PVOID pRemoteMem = VirtualAllocEx(hProcess, NULL, dwSize, MEM_COMMIT, PAGE_READWRITE);
	if(pRemoteMem == NULL)
	{
		ncHookUtil::WriteHookLog(hProcess, _T("InjectProcess-VirtualAllocEx failed"));
		VirtualFreeEx(hProcess, pRemoteMem, 0, MEM_RELEASE);
		return;
	}
	else if(!WriteProcessMemory(hProcess, pRemoteMem, lpDllPath, dwSize, NULL))
	{
		ncHookUtil::WriteHookLog(hProcess, _T("InjectProcess-WriteProcessMemory failed"));
		VirtualFreeEx(hProcess, pRemoteMem, 0, MEM_RELEASE);
		return;
	}

	// ����������ͼƬ�鿴���Ƚ��̵���CreateRemoteThread���ܻ�ʧ�ܣ���Ҫ�����Լ��Σ��������1���ӡ�
	int nRetry = 0;
	ncScopedHandle hThread = CreateRemoteThread(hProcess, NULL, 0, (PTHREAD_START_ROUTINE)pfnRemote, pRemoteMem, 0, NULL);
	while (hThread == NULL && nRetry < 50)
	{
		Sleep(20);
		hThread = CreateRemoteThread(hProcess, NULL, 0, (PTHREAD_START_ROUTINE)pfnRemote, pRemoteMem, 0, NULL);
		nRetry++;
	}

	if (hThread == NULL)
	{
		ncHookUtil::WriteHookLog(hProcess, _T("InjectProcess-CreateRemoteThread failed"));
		VirtualFreeEx(hProcess, pRemoteMem, 0, MEM_RELEASE);
		return;
	}

	SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL);
	SwitchToThread();

	// ���Զ���߳�ִ�г�ʱ����Ҫ�ͷ������ڴ棬�������Զ���̷߳��ʸ��ڴ�ʱ����Ұָ�����⡣
	if (WaitForSingleObject(hThread, 5000) == WAIT_TIMEOUT)
		ncHookUtil::WriteHookLog(hProcess, _T("InjectProcess-WaitForSingleObject failed"));
	else
		VirtualFreeEx(hProcess, pRemoteMem, 0, MEM_RELEASE);

	WINHOOK_TRACE(_T("end - InjectProcess: dwProcId: %d"), GetProcessId(hProcess));
}

//
// ִ�н��̽�ע�룺
//     1.���Ŀ�����δ��ע�����������н�ע�롣
//     2.��ȡĿ����̽�ע���ַ��ִ��Զ���߳̽�ע�롣
//
void EjectProcess(HANDLE hProcess)
{
	WINHOOK_TRACE(_T("begin - EjectProcess: dwProcId: %d"), GetProcessId(hProcess));

	BOOL bFrom32Bit = ncProcessUtil::Is32BitProcess(GetCurrentProcess());
	BOOL bTo32Bit = ncProcessUtil::Is32BitProcess(hProcess);

	HMODULE hMod = ncProcessUtil::GetProcessModuleHandle(hProcess, (bTo32Bit ? HOOK_DLL_NAME : HOOK_DLL64_NAME));
	if (hMod == NULL)
		return;

	PVOID pfnRemote = GetEjectAddress(hProcess, bFrom32Bit, bTo32Bit);
	if (pfnRemote == NULL)
		return;

	ncScopedHandle hThread = CreateRemoteThread(hProcess, NULL, 0, (PTHREAD_START_ROUTINE)pfnRemote, hMod, 0, NULL);
	if (hThread == NULL)
	{
		ncHookUtil::WriteHookLog(hProcess, _T("EjectProcess-CreateRemoteThread failed"));
		return;
	}

	SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL);
	SwitchToThread();

	if (WaitForSingleObject(hThread, 5000) == WAIT_TIMEOUT)
		ncHookUtil::WriteHookLog(hProcess, _T("EjectProcess-WaitForSingleObject failed"));

	WINHOOK_TRACE(_T("end - EjectProcess: dwProcId: %d"), GetProcessId(hProcess));
}
