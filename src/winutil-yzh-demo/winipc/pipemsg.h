/***************************************************************************************************
pipemsg.h:
	Copyright (c) Eisoo Software Inc.(2004-2012), All rights reserved

Purpose:
	Header file for class asIPCPipeMessage.

Author:
	Chaos

Creating Time:
	2012-6-28
***************************************************************************************************/

#ifndef __AS_IPC_PIPE_MESSAGE_H__
#define __AS_IPC_PIPE_MESSAGE_H__

#pragma once

class WINUTIL_DLLEXPORT asIPCPipeMessage
{
public:
	explicit asIPCPipeMessage ();
	virtual ~asIPCPipeMessage ();

public:
	/**
	 * 向管道写入/读取消息。
	 */
	void writeToPipe (HANDLE hPipe) const;
	void readFromPipe (HANDLE hPipe);

	/**
	 * 序列化消息内容。
	 */
	asIPCPipeMessage& asIPCPipeMessage::operator<< (LPCTSTR v)
	{
		UINT size = (UINT)(_tcslen (v) + 1) * sizeof(TCHAR);
		return writeBytes (reinterpret_cast<unsigned char*>((LPTSTR)v), size);
	}

	asIPCPipeMessage& asIPCPipeMessage::operator<< (int v)
	{
		return writeBytes (reinterpret_cast<unsigned char*>(&v), sizeof(v));
	}

	asIPCPipeMessage& asIPCPipeMessage::operator<< (UINT v)
	{
		return writeBytes (reinterpret_cast<unsigned char*>(&v), sizeof(v));
	}

	asIPCPipeMessage& asIPCPipeMessage::operator<< (int64 v)
	{
		return writeBytes (reinterpret_cast<unsigned char*>(&v), sizeof(v));
	}

	asIPCPipeMessage& asIPCPipeMessage::operator<< (uint64 v)
	{
		return writeBytes (reinterpret_cast<unsigned char*>(&v), sizeof(v));
	}

	/**
	 * 反序列化消息内容。
	 */
	asIPCPipeMessage& asIPCPipeMessage::operator>> (LPTSTR v)
	{
		return readBytes (reinterpret_cast<unsigned char*>(v));
	}

	asIPCPipeMessage& asIPCPipeMessage::operator>> (int& v)
	{
		return readBytes (reinterpret_cast<unsigned char*>(&v));
	}

	asIPCPipeMessage& asIPCPipeMessage::operator>> (UINT& v)
	{
		return readBytes (reinterpret_cast<unsigned char*>(&v));
	}

	asIPCPipeMessage& asIPCPipeMessage::operator>> (int64& v)
	{
		return readBytes (reinterpret_cast<unsigned char*>(&v));
	}

	asIPCPipeMessage& asIPCPipeMessage::operator>> (uint64& v)
	{
		return readBytes (reinterpret_cast<unsigned char*>(&v));
	}

	/**
	 * 清空消息内容。
	 */
	void clear ()
	{
		_offset = 0;
	}

protected:
	/**
	 * 写入/读取字节流。
	 */
	asIPCPipeMessage& writeBytes (const unsigned char* bytes, UINT size);
	asIPCPipeMessage& readBytes (unsigned char* bytes);

	/**
	 * 扩充缓存容量。
	 */
	void grow (size_t growSize = _initSize);

private:
	unsigned char*			_pBuf;				// 缓存地址指针
	size_t					_capacity;			// 缓存容量大小
	size_t					_offset;			// 缓存当前位移

	static const size_t		_initSize = 512;	// 初始缓存容量
	static const size_t		_sizeLen = 4;		// 长度占用的字节数

}; // End class asIPCPipeMessage

#endif // __AS_IPC_PIPE_MESSAGE_H__
