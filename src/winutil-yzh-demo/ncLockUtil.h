/***************************************************************************************************
ncLockUtil.h:
	Copyright (c) Eisoo Software, Inc.(2004-2015), All rights reserved.

Purpose:
	Header file for lock related utilities.

Author:
	Chaos

Creating Time:
	2015-02-25
***************************************************************************************************/

#ifndef __NC_LOCK_UTILITIES_H__
#define __NC_LOCK_UTILITIES_H__

#pragma once

class ncThreadMutexLock
{
	ncThreadMutexLock (const ncThreadMutexLock&);
	void operator= (const ncThreadMutexLock&);

public:
	ncThreadMutexLock (void)
	{
		::InitializeCriticalSection (&_cs);
	}

	~ncThreadMutexLock (void)
	{
		::DeleteCriticalSection (&_cs);
	}

public:
	void lock (void)
	{
		::EnterCriticalSection (&_cs); 
	}

	void unlock (void)
	{
		::LeaveCriticalSection (&_cs);
	}
	
private:
	CRITICAL_SECTION _cs;
};

template<typename lock_t>
class ncScopedLock
{
	ncScopedLock (const ncScopedLock&);
	void operator= (const ncScopedLock&);

public:
	ncScopedLock (lock_t* lock) 
		: _lock(lock)
	{
		_lock->lock();
	}

	~ncScopedLock (void)
	{
		_lock->unlock();
	}

private:
	lock_t* _lock;
};

#endif // __NC_LOCK_UTILITIES_H__
