#ifndef __API_HOOK_H__
#define __API_HOOK_H__

#pragma once

#include <shlobj.h>
#include <shellapi.h>

BOOL HookAllAPI();
BOOL UnhookAllAPI();

NTSTATUS WINAPI MyNtResumeThread(HANDLE ThreadHandle, PULONG SuspendCount);

BOOL WINAPI MyCreateProcessW(LPCWSTR lpApplicationName,
		LPWSTR lpCommandLine,
		LPSECURITY_ATTRIBUTES lpProcessAttributes,
		LPSECURITY_ATTRIBUTES lpThreadAttributes,
		BOOL bInheritHandles,
		DWORD dwCreationFlags,
		LPVOID lpEnvironment,
		LPCWSTR lpCurrentDirectory,
		LPSTARTUPINFOW lpStartupInfo,
		LPPROCESS_INFORMATION lpProcessInformation);
typedef BOOL (WINAPI *CREATEPROCESSW)(LPCWSTR lpApplicationName,
		LPWSTR lpCommandLine, 
		LPSECURITY_ATTRIBUTES lpProcessAttributes,
		LPSECURITY_ATTRIBUTES lpThreadAttributes,
		BOOL bInheritHandles,
		DWORD dwCreationFlags,
		LPVOID lpEnvironment,
		LPCWSTR lpCurrentDirectory,
		LPSTARTUPINFOW lpStartupInfo,
		LPPROCESS_INFORMATION lpProcessInformation);

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
		ULONG EaLength);

BOOL WINAPI MyGetFileAttributesExW(
		__in LPCWSTR lpFileName,
		__in GET_FILEEX_INFO_LEVELS fInfoLevelId,
		__out LPVOID lpFileInformation);
typedef BOOL (WINAPI *GETFILEATTRIBUTESEXW)(
		__in LPCWSTR lpFileName,
		__in GET_FILEEX_INFO_LEVELS fInfoLevelId,
		__out LPVOID lpFileInformation);

HRESULT STDAPICALLTYPE MySHCreateShellFolderView(const SFV_CREATE *pcsfv, IShellView **ppsv);
typedef HRESULT (STDAPICALLTYPE *SHCREATESHELLFOLDERVIEW)(const SFV_CREATE *pcsfv, IShellView **ppsv);

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
		BOOLEAN RestartScan);

int WINAPI MySHFileOperationW(LPSHFILEOPSTRUCTW lpFileOp);
typedef int (WINAPI *SHFILEOPERATIONW)(LPSHFILEOPSTRUCTW lpFileOp);

HRESULT STDAPICALLTYPE MyCoCreateInstance(REFCLSID rclsid,
		LPUNKNOWN pUnkOuter,
		DWORD dwClsContext,
		REFIID riid,
		LPVOID *ppv);
typedef HRESULT (STDAPICALLTYPE *COCREATEINSTANCE)(REFCLSID rclsid,
		LPUNKNOWN pUnkOuter,
		DWORD dwClsContext,
		REFIID riid,
		LPVOID *ppv);

HRESULT STDMETHODCALLTYPE MyMoveItems(IFileOperation *pThis,
		IUnknown *punkItems,
		IShellItem *psiDestinationFolder);
typedef HRESULT (STDMETHODCALLTYPE *MOVEITEMS)(IFileOperation *pThis,
		IUnknown *punkItems,
		IShellItem *psiDestinationFolder);

HRESULT STDMETHODCALLTYPE MyCopyItems(IFileOperation *pThis,
		IUnknown *punkItems,
		IShellItem *psiDestinationFolder);
typedef HRESULT (STDMETHODCALLTYPE *COPYITEMS)(IFileOperation *pThis,
		IUnknown *punkItems,
		IShellItem *psiDestinationFolder);

HRESULT STDMETHODCALLTYPE MyDeleteItems(IFileOperation *pThis, IUnknown *punkItems);
typedef HRESULT (STDMETHODCALLTYPE *DELETEITEMS)(IFileOperation *pThis, IUnknown *punkItems);

int WINAPI MyStartDocW(HDC hdc, CONST DOCINFOW* lpdi);
typedef int (WINAPI *STARTDOCW)(HDC hdc, CONST DOCINFOW* lpdi);

int WINAPI MyStartPage(HDC hDC);
typedef int (WINAPI *STARTPAGE)(HDC hDC);

BOOL WINAPI MyBitBlt(HDC hdcDest,
		int nXDest,
		int nYDest,
		int nWidth,
		int nHeight,
		HDC hdcSrc,
		int nXSrc,
		int nYSrc,
		DWORD dwRop);
typedef BOOL (WINAPI *BITBLT)(HDC hdcDest,
		int nXDest,
		int nYDest,
		int nWidth,
		int nHeight,
		HDC hdcSrc,
		int nXSrc,
		int nYSrc,
		DWORD dwRop);

#endif // __API_HOOK_H__
