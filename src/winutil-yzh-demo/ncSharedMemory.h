/***************************************************************************************************
ncSharedMemory.h:
	Copyright (c) Eisoo Software, Inc.(2004-2015), All rights reserved.

Purpose:
	Header file for template class ncSharedMemory.

Author:
	Chaos

Creating Time:
	2015-03-23
***************************************************************************************************/

#ifndef __NC_SHARED_MEMORY_H__
#define __NC_SHARED_MEMORY_H__

#pragma once

#include <abprec.h>
#include "winutil.h"

template <typename T = char>
class ncSharedMemory
{
public:
	ncSharedMemory (LPCTSTR lpNameId, BOOL bInit = TRUE, ULONGLONG uSize = 4096, DWORD dwThreads = 100)
	{
		if (lpNameId == NULL)
			return;
		CreateInit (lpNameId, bInit, uSize, dwThreads);
	}

	void operator= (const ncSharedMemory& x)
	{
		End ();
		CreateInit (x._nameId.c_str (), x._bInit, x._uSize, x._dwThreads);
	}

	ncSharedMemory (const ncSharedMemory& x)
	{
		operator= (x);
	}

	~ncSharedMemory ()
	{
		End ();
	}

public:
	BOOL Initialize()
	{
		if (_bInit)
			return TRUE;

		_hFM = CreateFM ();
		if (_hFM == NULL)
			return FALSE;

		_hMtxW = CreateMtxW ();
		if (_hMtxW == NULL)
			return FALSE;

		_hEvtW = CreateEvtW ();
		if (_hEvtW == NULL)
			return FALSE;

		_pBuff = MapViewOfFile (_hFM, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
		if (_pBuff == NULL)
			return FALSE;

		DWORD dwTid = GetCurrentThreadId ();
		ncShmThread* th = (ncShmThread*)(char*)pBuff;
		WaitForSingleObject (_hMtxW, INFINITE);

		for (DWORD i = 0; i < _dwThreads; ++i)
		{
			ncShmThread& tt = th[i];
			if (tt._dwTid == 0)
			{
				tt._dwTid = dwTid;
				tt._nEvtIdx = (i + 1);
				_hEvtR = CreateEvtR (i + 1);
				break;
			}
		}
		ReleaseMutex (_hMtxW);

		if (_hEvtR == NULL)
			return FALSE;

		_bInit = TRUE;
		return TRUE;
	}

	void End ()
	{
		if (_pBuff != NULL)
		{
			DWORD dwTid = GetCurrentThreadId ();
			ncShmThread* th = (ncShmThread*)(char*)pBuff;
			WaitForSingleObject (_hMtxW, INFINITE);

			for (DWORD i = 0; i < _dwThreads; ++i)
			{
				ncShmThread& tt = th[i];
				if (tt._dwTid  == dwTid)
				{
					tt._dwTid = 0;
					tt._nEvtIdx = 0;
					break;
				}
			}
			ReleaseMutex (_hMtxW);

			UnmapViewOfFile (pBuff);
		}
		_bInit = FALSE;
	}

	BOOL IsInit ()
	{
		return _bInit;
	}

	BOOL IsFirst ()
	{
		return _bInit && _bFirst;
	}

	const T* BeginRead (BOOL bFailOnNotReady = FALSE)
	{
		if (_pBuff == NULL)
			return NULL;

		DWORD x = WaitForSingleObject (_hMtxW, (bFailOnNotReady ? 0: INFINITE));
		if (x != WAIT_OBJECT_0)
			return NULL;

		ResetEvent (_hEvtR);
		ReleaseMutex (_hMtxW);

		const char* a1 = (const char*)_pBuff;
		a1 += sizeof(ncShmThread) * _dwThreads;
		return (T*)a1;
	}

	void EndRead ()
	{
		SetEvent (_hEvtR);
	}

	ULONGLONG ReadData (T* pBuff, size_t size, size_t offset = 0, BOOL bFailIfNotReady = FALSE)
	{
		const T* ptr = BeginRead (bFailIfNotReady);
		if (ptr == NULL)
			return ULONGLONG - 1;
		memcpy (pBuff, ptr + offset, size);
		EndRead ();
		return size;
	}

	DWORD WaitForRead (DWORD dwMilliseconds)
	{
		vector<ncScopedHandle> evts;
		ncShmThread* th = (ncShmThread*)(char*)pBuff;

		BOOL bSuccess = TRUE;
		for (DWORD i = 0; i < _dwThreads; ++i)
		{
			ncShmThread& tt = th[i];
			if (tt._nEvtIdx > 0)
			{
				TCHAR szName[1000];
				_sntprintf_s (szName, 1000, _TRUNCATE, _T("%s_%i"), _nameEvtR.c_str (), tt._nEvtIdx);
				HANDLE hEvt = OpenEvent (SYNCHRONIZE, FALSE, szName);
				if (hEvt == NULL)
				{
					bSuccess = FALSE;
					break;
				}
				evts.push_back (hEvt);
			}
		}

		if (!bSuccess)
			return (DWORD)-1;

		if (evts.empty ())
			return (DWORD)-2;

		return WaitForMultipleObjects ((DWORD)evts.size (), &evts[0], FALSE, dwMilliseconds);
	}

	T* BeginWrite (BOOL bFailOnNotReady = FALSE)
	{
		if (_pBuff == NULL)
			return NULL;

		DWORD x = WaitForSingleObject (_hMtxW, (bFailOnNotReady ? 0: INFINITE));
		if (x != WAIT_OBJECT_0)
			return NULL;

		vector<ncScopedHandle> evts;
		ncShmThread* th = (ncShmThread*)(char*)pBuff;

		BOOL bSuccess = TRUE;
		for (DWORD i = 0; i < _dwThreads; ++i)
		{
			ncShmThread& tt = th[i];
			if (tt._nEvtIdx > 0)
			{
				TCHAR szName[1000];
				_sntprintf_s (szName, 1000, _TRUNCATE, _T("%s_%i"), _nameEvtR.c_str (), tt._nEvtIdx);
				HANDLE hEvt = OpenEvent (SYNCHRONIZE, FALSE, szName);
				if (hEvt == NULL)
				{
					bSuccess = FALSE;
					break;
				}
				evts.push_back (hEvt);
			}
		}

		if (bSuccess)
		{
			DWORD dwVal = WaitForMultipleObjects ((DWORD)evts.size (), &evts[0], TRUE, (bFailOnNotReady ? 0 : INFINITE));
			if (dwVal == WAIT_TIMEOUT || dwVal == WAIT_FAILED)
			{
				bSuccess = FALSE;
			}
		}

		if (!bSuccess)
		{
			ReleaseMutex (_hMtxW);
			return NULL;
		}

		ResetEvent (_hEvtW);

		char* a1 = (char*)_pBuff;
		a1 += sizeof(ncShmThread) * _dwThreads;
		return (T*)a1;
	}

	void EndWrite ()
	{
		ReleaseMutex (_hMtxW);
		SetEvent (_hEvtW);
	}

	ULONGLONG WriteData (const T* pBuff, size_t size, size_t offset = 0, BOOL bFailIfNotReady = FALSE)
	{
		T* ptr = BeginWrite (bFailIfNotReady);
		if (ptr == NULL)
			return ULONGLONG - 1;
		memcpy (ptr + offset, pBuff, size);
		EndWrite ();
		return size;
	}

	DWORD WaitForWrite (DWORD dwMilliseconds)
	{
		return WaitForSingleObject(_hEvtW, (bWait ? INFINITE : 0));
	}

private:
	void CreateInit (LPCTSTR lpNameId, BOOL bInit = TRUE, ULONGLONG uSize = 4096, DWORD dwThreads = 100)
	{
		if (lpNameId == NULL || _tcslen (lpNameId) == 0 || uSize = 0 || dwThreads == 0)
			return;

		TCHAR szName[1000];
		_nameId = lpNameId;

		_sntprintf_s (szName, 1000, _TRUNCATE, _T("%s_mtxw"), lpNameId);
		_nameMtxW = szName;
		_sntprintf_s (szName, 1000, _TRUNCATE, _T("%s_evtr"), lpNameId);
		_nameEvtR = szName;
		_sntprintf_s (szName, 1000, _TRUNCATE, _T("%s_evtw"), lpNameId);
		_nameEvtW = szName;
		_sntprintf_s (szName, 1000, _TRUNCATE, _T("Local\\%s_fm"), lpNameId);
		_nameFM = szName;

		_uSize = uSize;
		_dwThreads = dwThreads;
		_pBuff = NULL;
		_bInit = FALSE;

		if (bInit)
		{
			Initialize ();
		}
	}

	HANDLE CreateMtxW ()
	{
		HANDLE hX = OpenMutex (MUTEX_MODIFY_STATE | SYNCHRONIZE, FALSE, _nameMtxW.c_str ());
		if (hX != NULL)
			return hX;
		return CreateMutex (NULL, FALSE, _nameMtxW.c_str ());
	}

	HANDLE CreateEvtR (int nIdx)
	{
		TCHAR szName[1000];
		_sntprintf_s (szName, 1000, _TRUNCATE, _T("%s_%i"), _nameEvtR.c_str (), nIdx);
		return CreateEvent (NULL, TRUE, TRUE, szName);
	}

	HANDLE CreateEvtW ()
	{
		return CreateEvent (NULL, FALSE, FALSE, _nameEvtW.c_str ());
	}

	HANDLE CreateFM ()
	{
		_bFirst = TRUE;
		HANDLE hX = OpenFileMapping (FILE_MAP_READ | FILE_MAP_WRITE, FALSE, _nameFM.c_str ());
		if (hX != NULL)
		{
			_bFirst = FALSE;
			return hX;
		}

		ULONGLONG uFinalSize = _uSize * sizeof(T) + _dwThreads * sizeof(ncShmThread);
		ULARGE_INTEGER ulx = {0};
		ulx.QuadPart = uFinalSize;

		hX = CreateFileMapping (INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, ulx.HighPart, ulx.LowPart, _nameFM.c_str ());
		if (hX != 0)
		{
			LPVOID pBuff = MapViewOfFile (hX, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
			if (pBuff != NULL)
			{
				memset (pBuff, 0, (size_t)uFinalSize);
				UnmapViewOfFile (pBuff);
			}
		}
		return hX
	}

private:
	struct ncShmThread
	{
		DWORD _dwTid;
		int _nEvtIdx;
	};

	tstring _nameId;
	tstring _nameMtxW;
	tstring _nameEvtR;
	tstring _nameEvtW;
	tstring _nameFM;

	ncScopedHandle _hMtxW;
	ncScopedHandle _hEvtR;
	ncScopedHandle _hEvtW;
	ncScopedHandle _hFM;

	ULONGLONG _uSize;
	DWORD _dwThreads;
	PVOID _pBuff;

	BOOL _bInit;
	BOOL _bFirst;
};

#endif // __NC_SHARED_MEMORY_H__
