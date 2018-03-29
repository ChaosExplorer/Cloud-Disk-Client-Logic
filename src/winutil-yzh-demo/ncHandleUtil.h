/***************************************************************************************************
ncHandleUtil.h:
	Copyright (c) Eisoo Software, Inc.(2004-2015), All rights reserved.

Purpose:
	Header file for handle related utilities.

Author:
	Chaos

Creating Time:
	2015-02-25
***************************************************************************************************/

#ifndef __NC_HANDLE_UTILITIES_H__
#define __NC_HANDLE_UTILITIES_H__

#pragma once

//
// ncScopedHandle
//
class ncScopedHandle
{
	ncScopedHandle (const ncScopedHandle&);
	void operator= (const ncScopedHandle&);

public:
	ncScopedHandle (HANDLE handle)
		: _handle (handle)
	{
	}

	~ncScopedHandle ()
	{
		if (_handle != NULL && _handle != INVALID_HANDLE_VALUE)
			CloseHandle(_handle);
	}

	operator HANDLE () const
	{
		return _handle;
	}

	PHANDLE operator& ()
	{
		return &_handle;
	}

	void operator= (HANDLE handle)
	{
		if (_handle != NULL && _handle != INVALID_HANDLE_VALUE)
			CloseHandle (_handle);
		_handle = handle;
	}

private:
	HANDLE _handle;
};

//
// ncHandleUtil
//
struct ncFileHandle
{
	HANDLE			_handle;
	tstring			_path;
	DWORD			_dwPid;

	ncFileHandle (HANDLE handle, const tstring& path, DWORD dwPid)
		: _handle (handle)
		, _path (path)
		, _dwPid (dwPid)
	{
	}
};

class WINUTIL_DLLEXPORT ncHandleUtil
{
public:
	static BOOL GetHandlePath (HANDLE handle, tstring& path);

	static BOOL FindFileHandle (LPCTSTR lpName, vector<ncFileHandle>& handles);

	static BOOL CloseHandleEx (HANDLE handle, DWORD dwPid);

protected:
	static PSYSTEM_HANDLE_INFORMATION GetSystemHandleInfo ();

	static BOOL IsBlockingHandle (HANDLE handle);
};

#endif // __NC_HANDLE_UTILITIES_H__
