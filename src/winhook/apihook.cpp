#define _WIN32_IE 0x0700

#include <abprec.h>
#include <atlbase.h>
#include "minhook/hook.h"
#include "winhook.h"
#include "ncHookUtil.h"
#include "ncHookAgent.h"
#include "ncFopImpl.h"
#include "ncApiImpl.h"
#include "apihook.h"

extern ncOSVersion g_osv;
extern tstring g_procPath;
extern tstring g_procName;

NTRESUMETHREAD fpNtResumeThread = NULL;
CREATEPROCESSW fpCreateProcessW = NULL;
NTCREATEFILE fpNtCreateFile = NULL;
GETFILEATTRIBUTESEXW fpGetFileAttributesExW = NULL;
SHCREATESHELLFOLDERVIEW fpSHCreateShellFolderView = NULL;
NTQUERYDIRECTORYFILE fpNtQueryDirectoryFile = NULL;
SHFILEOPERATIONW fpSHFileOperationW = NULL;
COCREATEINSTANCE fpCoCreateInstance = NULL;
MOVEITEMS fpMoveItems = NULL;
COPYITEMS fpCopyItems = NULL;
STARTDOCW fpStartDocW = NULL;
STARTPAGE fpStartPage = NULL;
BITBLT fpBitBlt = NULL;

template <typename T>
static inline MH_STATUS MH_CreateHookEx(void* pTarget, void* const pDetour, T** ppOriginal)
{
	return MH_CreateHook(pTarget, pDetour, reinterpret_cast<void**>(ppOriginal));
}

static inline PVOID GetInterfaceMethod(PVOID intf, DWORD methodIndex)
{
	return *(PVOID*)(*(ULONG_PTR*)intf + methodIndex * sizeof(PVOID));
}

//
// API Hook处理：
//     1.如果是ShareClient进程，则只需hook其NtResumeThread函数。
//     2.如果是其他进程，则hook所有需要的API。
//
BOOL HookAllAPI()
{
	WINHOOK_TRACE(_T("begin - HookAllAPI"));

	if (MH_Initialize() != MH_OK)
		return FALSE;

	vector<ncHookInfoLimit> limitPath;
	vector<ncHookInfoLimit> limitProcess;
	ncWinHookUtil::GetLimitInfo(limitPath, limitProcess);
	bool needLimit = false;
	unsigned long limit;
	tstring temp;
	for (size_t i = 0; i < limitPath.size(); i++) {
		if (limitPath[i].dest.length() <= g_procPath.length()) {
			temp = g_procPath.substr(0, limitPath[i].dest.length());
			if (_tcsicmp(temp.c_str(), limitPath[i].dest.c_str()) == 0) {
				needLimit = true;
				limit = limitPath[i].limit;
				break;
			}
		}
	}
	if (!needLimit) {
		for (size_t i = 0; i < limitProcess.size(); i++) {
			if (_tcsicmp(g_procName.c_str(), limitProcess[i].dest.c_str()) == 0) {
				needLimit = true;
				limit = limitProcess[i].limit;
				break;
			}
		}
	}

	HMODULE hNtDll = GetModuleHandle(_T("ntdll"));
	if (needLimit) {
		if (!(limit & LIMIT_FUN_NtResumeThread)) {
			MH_CreateHookEx(GetProcAddress(hNtDll, "NtResumeThread"), &MyNtResumeThread, &fpNtResumeThread);
		}
		
		if (!(limit & LIMIT_FUN_NtCreateFile)) {
			MH_CreateHookEx(GetProcAddress(hNtDll, "NtCreateFile"), &MyNtCreateFile, &fpNtCreateFile);
		}
		
		if (!(limit & LIMIT_FUN_CreateProcess)) {
			MH_CreateHookEx(&CreateProcessW, &MyCreateProcessW, &fpCreateProcessW);
		}
		
		if (!(limit & LIMIT_FUN_GetFileAttributesEx)) {
			MH_CreateHookEx(&GetFileAttributesExW, &MyGetFileAttributesExW, &fpGetFileAttributesExW);
		}
		
		if (!(limit & LIMIT_FUN_SHCreateShellFolderView)) {
			MH_CreateHookEx(&SHCreateShellFolderView, &MySHCreateShellFolderView, &fpSHCreateShellFolderView);
		}
		
		if (!(limit & LIMIT_FUN_NtQueryDirectoryFile)) {
			MH_CreateHookEx(GetProcAddress(hNtDll, "NtQueryDirectoryFile"), &MyNtQueryDirectoryFile, &fpNtQueryDirectoryFile);
		}
		
		if (!(limit & LIMIT_FUN_CoCreateInstance) && g_osv >= OSV_WINVISTA) {
			MH_CreateHookEx(&CoCreateInstance, &MyCoCreateInstance, &fpCoCreateInstance);
		}
		
		if (!(limit & LIMIT_FUN_SHFileOperation) && g_osv < OSV_WINVISTA) {
			MH_CreateHookEx(&SHFileOperationW, &MySHFileOperationW, &fpSHFileOperationW);
		}
		if (!(limit & LIMIT_FUN_StartDoc)) {
			MH_CreateHookEx(&StartDocW, &MyStartDocW, &fpStartDocW);
		}

		if (!(limit & LIMIT_FUN_StartPage)) {
			MH_CreateHookEx(&StartPage, &MyStartPage, &fpStartPage);
		}

		if (!(limit & LIMIT_FUN_BitBlt)) {
			MH_CreateHookEx(&BitBlt, &MyBitBlt, &fpBitBlt);
		}
	}
	else {
		if (ncHookUtil::IsShareClientProcess(g_procName.c_str()) || 
			ncHookUtil::IsSvcHostProcess(g_procName.c_str()) || 
			ncHookUtil::IsRundll32Process(g_procName.c_str())) {
			MH_CreateHookEx(GetProcAddress(hNtDll, "NtResumeThread"), &MyNtResumeThread, &fpNtResumeThread);
		}
		else if (ncHookUtil::IsDllhostProcess(g_procName.c_str())) {
			MH_CreateHookEx(GetProcAddress(hNtDll, "NtCreateFile"), &MyNtCreateFile, &fpNtCreateFile);
		}
		else {
			MH_CreateHookEx(GetProcAddress(hNtDll, "NtResumeThread"), &MyNtResumeThread, &fpNtResumeThread);
			MH_CreateHookEx(&CreateProcessW, &MyCreateProcessW, &fpCreateProcessW);
			MH_CreateHookEx(GetProcAddress(hNtDll, "NtCreateFile"), &MyNtCreateFile, &fpNtCreateFile);
			MH_CreateHookEx(&GetFileAttributesExW, &MyGetFileAttributesExW, &fpGetFileAttributesExW);
			MH_CreateHookEx(&SHCreateShellFolderView, &MySHCreateShellFolderView, &fpSHCreateShellFolderView);
			MH_CreateHookEx(GetProcAddress(hNtDll, "NtQueryDirectoryFile"), &MyNtQueryDirectoryFile, &fpNtQueryDirectoryFile);
			if (g_osv >= OSV_WINVISTA)
				MH_CreateHookEx(&CoCreateInstance, &MyCoCreateInstance, &fpCoCreateInstance);
			else
				MH_CreateHookEx(&SHFileOperationW, &MySHFileOperationW, &fpSHFileOperationW);
			MH_CreateHookEx(&StartDocW, &MyStartDocW, &fpStartDocW);
			MH_CreateHookEx(&StartPage, &MyStartPage, &fpStartPage);
			MH_CreateHookEx(&BitBlt, &MyBitBlt, &fpBitBlt);
		}
	}

	if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK)
		return FALSE;

	WINHOOK_TRACE(_T("end - HookAllAPI"));
	return TRUE;
}

//
// API Unhook处理。
//
BOOL UnhookAllAPI()
{
	WINHOOK_TRACE(_T("begin - UnhookAllAPI"));

	if (MH_Uninitialize() != MH_OK)
		return FALSE;

	WINHOOK_TRACE(_T("end - UnhookAllAPI"));
	return TRUE;
}

//
// NtResumeThread的回调处理：
//     1.为避免注入过程因当前进程结束而意外中断，需要通过代理子进程来进行注入。
//
unsigned WINAPI NtResumeThreadCallback(void* param)
{
	DWORD dwProcId = (DWORD)param;
	ncScopedHandle hProcess = OpenProcess(MAXIMUM_ALLOWED, FALSE, dwProcId);
	if (hProcess != NULL)
	{
		ncHookAgent::HookProcessByAgent(hProcess, TRUE);
	}
	return 0;
}

//
// NtResumeThread的hook处理：
//     1.检查要恢复的线程是否为新创建进程的主线程，若是则对新进程进行注入。
//     2.为保证新进程的主线程顺利恢复以完成初始化，需要开启从线程对新进程进行注入。
//     3.为保证从线程的处理不被意外中断，需要等待从线程执行完成，最多等待1秒钟。
//
NTSTATUS WINAPI MyNtResumeThread(HANDLE ThreadHandle, PULONG SuspendCount)
{
	DWORD dwProcId = ncProcessUtil::GetProcessIdOfThread(ThreadHandle);
	if (dwProcId == 0 || dwProcId == GetCurrentProcessId() || ncProcessUtil::GetProcessThreadCount(dwProcId) != 1)
		return fpNtResumeThread(ThreadHandle, SuspendCount);

	WINHOOK_TRACE(_T("NtResumeThread new process: %d"), dwProcId);

	ncScopedHandle hThread = (HANDLE)_beginthreadex(NULL, 0, NtResumeThreadCallback, (void*)dwProcId, 0, NULL);
	SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL);
	ncScopedHandle hProcess = OpenProcess(MAXIMUM_ALLOWED, FALSE, dwProcId);
	tstring proName = ncProcessUtil::GetProcessName(hProcess);
	DWORD dwMilliseconds = 1000;
	// rundll32.exe则延迟等待时间，保证在其结束前注入完成
	if (ncHookUtil::IsRundll32Process(proName.c_str()) || ncHookUtil::IsRundll32Process(g_procName.c_str())) {
		dwMilliseconds = 5000;
	}
	WaitForSingleObject(hThread, dwMilliseconds);

	return fpNtResumeThread(ThreadHandle, SuspendCount);
}

//
// CreateProcessInfo：进程创建结构体信息。
//
struct CreateProcessInfo
{
	tstring _appPath;
	tstring _cmdLine;
	DWORD _dwPid;
	DWORD _dwTid;

	CreateProcessInfo(LPCTSTR lpAppPath, LPCTSTR lpCmdLine, DWORD dwPid, DWORD dwTid)
		: _dwPid (dwPid)
		, _dwTid (dwTid)
	{
		lpAppPath == NULL ? _appPath.clear() : _appPath.assign(lpAppPath);
		lpCmdLine == NULL ? _cmdLine.clear() : _cmdLine.assign(lpCmdLine);
	}
};

//
// WinXP系统下CreateProcess的回调处理：
//     1.如果处理成功，且之前进程创建成功，则恢复该进程的执行。
//     2.如果处理成功，但之前进程创建失败，则重新启动该进程。
//     3.如果处理失败，但之前进程创建成功，则强制结束该进程。
//     4.如果处理失败，且之前进程创建失败，则直接返回。
//
unsigned WINAPI CreateProcessCallback(void* param)
{
	CreateProcessInfo* lpInfo = reinterpret_cast<CreateProcessInfo*>(param);

	WINHOOK_TRACE(_T("CreateProcessCallback: lpAppPath: %s, lpCmdLine: %s, dwPid: %d, dwTid: %d"),
		lpInfo->_appPath.c_str(), lpInfo->_cmdLine.c_str(), lpInfo->_dwPid, lpInfo->_dwTid);

	LPCTSTR lpAppPath = (lpInfo->_appPath.empty() ? NULL : lpInfo->_appPath.c_str());
	LPCTSTR lpCmdLine = (lpInfo->_cmdLine.empty() ? NULL : lpInfo->_cmdLine.c_str());

	if (ncApiImpl::PreCreateProcess(lpAppPath, lpCmdLine))
	{
		if (lpInfo->_dwTid != 0)
		{
			ncScopedHandle hThread = OpenThread(MAXIMUM_ALLOWED, FALSE, lpInfo->_dwTid);
			ResumeThread(hThread);
		}
		else
		{
			ShellExecute(NULL, NULL, lpAppPath, lpCmdLine, NULL, SW_SHOWNORMAL);
		}
	}
	else if (lpInfo->_dwPid != 0)
	{
		ncScopedHandle hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, lpInfo->_dwPid);
		TerminateProcess(hProcess, 0);
	}

	delete lpInfo;
	return 0;
}

//
// CreateProcess的hook处理：
//     1.在WinXP系统下，CreateProcess的调用是阻塞式的，所以其自定义处理需要放到线程中进行。
//     2.在Vista以上系统下，CreateProcess的调用是非阻塞式的，可以直接进行自定义处理。
//     3.如果Vista以上系统中的自定义处理失败，则返回TRUE以避免后续系统错误提示。
//
BOOL WINAPI MyCreateProcessW(LPCWSTR lpApplicationName,
		LPWSTR lpCommandLine,
		LPSECURITY_ATTRIBUTES lpProcessAttributes,
		LPSECURITY_ATTRIBUTES lpThreadAttributes,
		BOOL bInheritHandles,
		DWORD dwCreationFlags,
		LPVOID lpEnvironment,
		LPCWSTR lpCurrentDirectory,
		LPSTARTUPINFOW lpStartupInfo,
		LPPROCESS_INFORMATION lpProcessInformation)
{
	CW2CT szAppPath(lpApplicationName);
	CW2CT szCmdLine((LPCWSTR)lpCommandLine);

	CW2CT szCurrentDir(lpCurrentDirectory);
	if (!ncADSUtil::CheckADS(szCurrentDir, ADS_DOCID))
		goto FPINVOKE;

	if (g_osv < OSV_WINVISTA)
	{
		BOOL bReady = ncApiImpl::PreCreateProcessXP(szAppPath, szCmdLine);

		if (!bReady)
			dwCreationFlags |= CREATE_SUSPENDED;

		BOOL result = fpCreateProcessW(lpApplicationName,
			lpCommandLine,
			lpProcessAttributes,
			lpThreadAttributes,
			bInheritHandles,
			dwCreationFlags,
			lpEnvironment,
			lpCurrentDirectory,
			lpStartupInfo,
			lpProcessInformation);

		if (!bReady)
		{
			CreateProcessInfo* lpInfo = new CreateProcessInfo(
				szAppPath, szCmdLine, lpProcessInformation->dwProcessId, lpProcessInformation->dwThreadId);
			::_beginthreadex(NULL, 0, CreateProcessCallback, (void*)(lpInfo), 0, NULL);
		}

		return result;
	}
	else
	{
		if (!ncApiImpl::PreCreateProcess(szAppPath, szCmdLine))
			return TRUE;

FPINVOKE:
		return fpCreateProcessW(lpApplicationName,
			lpCommandLine,
			lpProcessAttributes,
			lpThreadAttributes,
			bInheritHandles,
			dwCreationFlags,
			lpEnvironment,
			lpCurrentDirectory,
			lpStartupInfo,
			lpProcessInformation);
	}
}

//
// 检查是否为系统自身发起的CreateFile调用。
//     1.Explorer进程中除XP系统家族的图片打开操作外，都是系统自身调用。
//     2.Dllhost进程中除VISTA系统家族的图片打开操作外，都是系统自身调用。
//     3.其他进程中除通用文件打开/保存对话框中的操作外，都不是系统自身调用。
//		@param bNeedHandleSysC : 需要支持预览处理的系统调用
// 
static BOOL IsSystemCreateFile(LPTSTR lpPath, BOOL& bNeedHandleSysC)
{
	if (ncHookUtil::IsExplorerProcess(g_procName.c_str()))
	{
		if (g_osv >= OSV_WINVISTA)
			return TRUE;

		TCHAR szClassName[100];
		bool ret;
		ret = (GetClassName(GetForegroundWindow(), szClassName, 100) == 0 || _tcsicmp(szClassName, _T("ShImgVw:CPreviewWnd")) != 0);
		if (lpPath != NULL && !ret)
		{
			IQueryAssociations *pAssoc;
			if (SUCCEEDED(AssocCreate (CLSID_QueryAssociations, IID_PPV_ARGS(&pAssoc))))
			{
				int len = lstrlen(lpPath);
				while (len > 0)
				{
					if (lpPath[len-1] == L'.')
						break;
					len--;
				}
				LPTSTR pExtension = lpPath + len -1;
				if (SUCCEEDED(pAssoc->Init(ASSOCF_INIT_DEFAULTTOFOLDER, pExtension, NULL, NULL)))
				{
					HRESULT hr;
					DWORD size = MAX_PATH;
					LPWSTR buffer = (LPWSTR)malloc(size);
					hr = pAssoc->GetString(ASSOCF_NOTRUNCATE, ASSOCSTR_COMMAND, NULL, buffer, &size);
					if (hr == S_FALSE || hr == E_POINTER) {
						buffer = (LPWSTR)realloc (buffer, size);
						pAssoc->GetString(ASSOCF_NOTRUNCATE, ASSOCSTR_COMMAND, NULL, buffer, &size);
					}
					ret = (_wcsicmp(buffer, L"rundll32.exe C:\\WINDOWS\\system32\\shimgvw.dll,ImageView_Fullscreen %1") != 0);
					bNeedHandleSysC = !ret;

					free(buffer);
				}
			}
		}
		return ret;
	}
	else if (ncHookUtil::IsDllhostProcess(g_procName.c_str()))
	{
		LPTSTR lpCmdLine = GetCommandLine();
		bNeedHandleSysC = _tcsstr(lpCmdLine, _T("/Processid:{76D0CB12-7604-4048-B83C-1005C7DDC503}")) != NULL;

		return !bNeedHandleSysC;
	}
	else
	{
		HWND hWnd = GetForegroundWindow();
		if (!FindWindowEx(hWnd, NULL, _T("DUIViewWndClassName"), NULL) &&
			!FindWindowEx(hWnd, NULL, _T("SHELLDLL_DefView"), NULL))
			return FALSE;

		GUITHREADINFO gui;
		gui.cbSize = sizeof(GUITHREADINFO);
		if (!GetGUIThreadInfo(GetCurrentThreadId(), &gui))
			return FALSE;

		bNeedHandleSysC = gui.hwndActive != NULL && gui.hwndCapture == NULL &&
			gui.hwndFocus != NULL && gui.hwndFocus == gui.hwndCaret;

		return !bNeedHandleSysC;
	}
}

//
// NtCreateFile的hook处理：
//     1.过滤掉系统自身发起的NtCreateFile调用。
//     2.过滤掉非文件对象（设备、串口、邮槽、管道等）。
//     3.如果自定义处理失败，则设置错误码为ERROR_ACCESS_DENIED，并返回STATUS_FILE_NOT_AVAILABLE。
//
NTSTATUS WINAPI MyNtCreateFile(PHANDLE FileHandle,
		ACCESS_MASK DesiredAccess,
		POBJECT_ATTRIBUTES ObjectAttributes,
		PIO_STATUS_BLOCK IoStatusBlock,
		PLARGE_INTEGER AllocationSize,
		ULONG FileAttributes,
		ULONG ShareAccess,
		ULONG CreateDisposition,
		ULONG CreateOptions,
		PVOID EaBuffer,
		ULONG EaLength)
{
	if (CreateDisposition == FILE_OPEN && (CreateOptions & FILE_NON_DIRECTORY_FILE))
	{
		LPTSTR lpPath = _tcsstr(ObjectAttributes->ObjectName->Buffer, _T("\\??\\"));
		if (lpPath == NULL)
			goto End;

		lpPath += 4;

		if (PathGetDriveNumber(lpPath) == -1)
			goto End;

		BOOL bNeedHandleSysC = FALSE;		// 需要处理的系统自身调用（图片打开等）, dllHost已启动标识
		if (IsSystemCreateFile(lpPath, bNeedHandleSysC))
			goto End;

		if (!ncApiImpl::PreCreateFile(lpPath, bNeedHandleSysC ? CF_OPEN_IMAGE : CF_NTCREATEFILE))
		{
			SetLastError(ERROR_ACCESS_DENIED);
			return STATUS_FILE_NOT_AVAILABLE;
		}
	}

End:
	return fpNtCreateFile(FileHandle,
		DesiredAccess,
		ObjectAttributes,
		IoStatusBlock,
		AllocationSize,
		FileAttributes,
		ShareAccess,
		CreateDisposition,
		CreateOptions,
		EaBuffer,
		EaLength);
}

//
// GetFileAttributesEx的hook处理：
//     1.可在自定义处理中修改文件的相关属性信息等。
//
BOOL WINAPI MyGetFileAttributesExW(LPCWSTR lpFileName,
		GET_FILEEX_INFO_LEVELS fInfoLevelId,
		LPVOID lpFileInformation)
{
	BOOL result = fpGetFileAttributesExW(lpFileName, fInfoLevelId, lpFileInformation);

	if (!result || lpFileInformation == NULL)
		return result;

	CW2CT szFileName(lpFileName);

	ncApiImpl::PostGetFileAttributesEx(szFileName, (WIN32_FILE_ATTRIBUTE_DATA*)lpFileInformation);

	return result;
}

//
// SHCreateShellFolderView的hook处理：
//     1.可在自定义处理中进行目录打开前的相关处理。
//
HRESULT STDAPICALLTYPE MySHCreateShellFolderView(const SFV_CREATE *pcsfv, IShellView **ppsv)
{
	CComQIPtr<IPersistFolder2> ppf2;
	if (SUCCEEDED(pcsfv->pshf->QueryInterface(IID_IPersistFolder2, (void**)&ppf2)))
	{
		LPITEMIDLIST pidl = NULL;
		if (SUCCEEDED(ppf2->GetCurFolder(&pidl)))
		{
			TCHAR szPath[MAX_PATH];
			if (SHGetPathFromIDList(pidl, szPath))
			{
				ncApiImpl::PreSHCreateShellFolderView(szPath);
			}
		}
	}

	return fpSHCreateShellFolderView(pcsfv, ppsv);
}

//
// NtQueryDirectoryFile的hook处理:
//     1.可在自定义处理中修改子对象的相关属性信息等。
//
NTSTATUS WINAPI MyNtQueryDirectoryFile(HANDLE FileHandle,
		HANDLE Event,
		PIO_APC_ROUTINE ApcRoutine,
		PVOID ApcContext,
		PIO_STATUS_BLOCK IoStatusBlock,
		PVOID FileInformation,
		ULONG Length,
		FILE_INFORMATION_CLASS FileInformationClass,
		BOOLEAN ReturnSingleEntry,
		PUNICODE_STRING FileName,
		BOOLEAN RestartScan)
{
	NTSTATUS result = fpNtQueryDirectoryFile(FileHandle,
		Event,
		ApcRoutine,
		ApcContext,
		IoStatusBlock,
		FileInformation,
		Length,
		FileInformationClass,
		ReturnSingleEntry,
		FileName,
		RestartScan);

	if (NT_SUCCESS(result))
		ncApiImpl::PostNtQueryDirectoryFile(FileHandle, FileInformation, FileInformationClass);

	return result;
}

//
// SHFileOperation的hook处理：
//     1.如果前期的自定义处理失败，则返回1以取消后续操作。
//
int WINAPI MySHFileOperationW(LPSHFILEOPSTRUCTW lpFileOp)
{
	LPWSTR lpFrom = (LPWSTR)lpFileOp->pFrom;
	CW2CT lpTo(lpFileOp->pTo);

	vector<tstring> vFrom;
	while (*lpFrom != 0)
	{
		// 过滤父目录移动到子目录，由explorer处理
		if (!ncPathUtil::IsParentPath (lpFrom, lpTo, TRUE))
			vFrom.push_back(lpFrom);

		lpFrom += wcslen(lpFrom) + 1;
	}

	if (!ncApiImpl::PreSHFileOperation(lpFileOp->wFunc, vFrom, lpTo, lpFileOp->fFlags))
		return 1;

	int result = fpSHFileOperationW(lpFileOp);

	if (result == 0)
		ncApiImpl::PostSHFileOperation(lpFileOp->wFunc, vFrom, lpTo, lpFileOp->fFlags);

	return result;
}

//
// CoCreateInstance的hook处理：
//     1.为了hook组件对象的成员方法，必须先创建该对象并获取其对应成员方法的地址，然后才能进行hook。
//
HRESULT STDAPICALLTYPE MyCoCreateInstance(REFCLSID rclsid,
		LPUNKNOWN pUnkOuter,
		DWORD dwClsContext,
		REFIID riid,
		LPVOID *ppv)
{
	HRESULT result = fpCoCreateInstance(rclsid, pUnkOuter, dwClsContext, riid, ppv);

	if (!SUCCEEDED(result))
		return result;

	if (IsEqualCLSID(rclsid, CLSID_FileOperation))
	{
		if (fpMoveItems == NULL && fpCopyItems == NULL)
		{
			MOVEITEMS oldMoveItems = (MOVEITEMS)GetInterfaceMethod(*ppv, 15);
			COPYITEMS oldCopyItems = (COPYITEMS)GetInterfaceMethod(*ppv, 17);

			if (MH_CreateHookEx(oldMoveItems, &MyMoveItems, &fpMoveItems) == MH_OK)
				MH_QueueEnableHook(oldMoveItems);

			if (MH_CreateHookEx(oldCopyItems, &MyCopyItems, &fpCopyItems) == MH_OK)
				MH_QueueEnableHook(oldCopyItems);

			MH_ApplyQueued();
			WINHOOK_TRACE(_T("MoveItems/CopyItems is hooked."));
		}
	}

	return result;
}

//
// IFileOperation::MoveItems的hook处理。
//     1.如果自定义处理失败，则返回S_OK以取消此次移动且不影响后续操作。
//
HRESULT STDMETHODCALLTYPE MyMoveItems(IFileOperation *pThis,
		IUnknown *punkItems,
		IShellItem *psiDestinationFolder)
{
	BOOL needAdvise = FALSE;	// 需要回调处理

	if (!ncFopImpl::PreMoveItems(pThis, punkItems, psiDestinationFolder, needAdvise))
		return S_OK;

	if (needAdvise) {
		CComPtr<ncFopImpl> fopImpl = new ncFopImpl;
		DWORD dwCookie;
		pThis->Advise(fopImpl, &dwCookie);
	}

	return fpMoveItems(pThis, punkItems, psiDestinationFolder);
}

//
// IFileOperation::CopyItems的hook处理。
//     1.如果自定义处理失败，则返回S_OK以取消此次复制且不影响后续操作。
//
HRESULT STDMETHODCALLTYPE MyCopyItems(IFileOperation *pThis,
		IUnknown *punkItems,
		IShellItem *psiDestinationFolder)
{
	BOOL needAdvise = FALSE;	// 需要回调处理

	if (!ncFopImpl::PreCopyItems(pThis, punkItems, psiDestinationFolder, needAdvise))
		return S_OK;

	if (needAdvise) {
		CComPtr<ncFopImpl> fopImpl = new ncFopImpl;
		DWORD dwCookie;
		pThis->Advise(fopImpl, &dwCookie);
	}

	return fpCopyItems(pThis, punkItems, psiDestinationFolder);
}

//
// StartDoc的hook处理：
//     1.如果自定义处理失败，则设置错误码为ERROR_ACCESS_DENIED，并返回0以取消后续操作。
//
int WINAPI MyStartDocW(HDC hdc, CONST DOCINFOW* lpdi)
{
	if (!ncApiImpl::PrePrintDoc())
	{
		SetLastError(ERROR_ACCESS_DENIED);
		return 0;
	}

	return fpStartDocW(hdc, lpdi);
}

//
// StartPage的hook处理：
//     1.如果自定义处理失败，则设置错误码为ERROR_ACCESS_DENIED，并返回0以取消后续操作。
//
int WINAPI MyStartPage(HDC hDC)
{
	if (!ncApiImpl::PrePrintDoc())
	{
		SetLastError(ERROR_ACCESS_DENIED);
		return 0;
	}

	return fpStartPage(hDC);
}

//
// BitBlt的hook处理：
//     1.如果自定义处理失败，则设置错误码为ERROR_ACCESS_DENIED，并返回FALSE以取消后续操作。
//
BOOL WINAPI MyBitBlt(HDC hdcDest,
		int nXDest,
		int nYDest,
		int nWidth,
		int nHeight,
		HDC hdcSrc,
		int nXSrc,
		int nYSrc,
		DWORD dwRop)
{
	if ((dwRop & CAPTUREBLT) && (dwRop & SRCCOPY) &&
		hdcSrc != NULL && WindowFromDC(hdcSrc) == GetDesktopWindow() &&
		hdcDest != NULL && WindowFromDC(hdcDest) == NULL && GetCurrentObject(hdcDest, OBJ_BITMAP) != NULL)
	{
		if (!ncApiImpl::PreCaptureScreen())
		{
			SetLastError(ERROR_ACCESS_DENIED);
			return FALSE;
		}
	}

	return fpBitBlt(hdcDest,
		nXDest,
		nYDest,
		nWidth,
		nHeight,
		hdcSrc,
		nXSrc,
		nYSrc,
		dwRop);
}
