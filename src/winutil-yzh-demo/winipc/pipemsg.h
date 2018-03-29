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
	 * ��ܵ�д��/��ȡ��Ϣ��
	 */
	void writeToPipe (HANDLE hPipe) const;
	void readFromPipe (HANDLE hPipe);

	/**
	 * ���л���Ϣ���ݡ�
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
	 * �����л���Ϣ���ݡ�
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
	 * �����Ϣ���ݡ�
	 */
	void clear ()
	{
		_offset = 0;
	}

protected:
	/**
	 * д��/��ȡ�ֽ�����
	 */
	asIPCPipeMessage& writeBytes (const unsigned char* bytes, UINT size);
	asIPCPipeMessage& readBytes (unsigned char* bytes);

	/**
	 * ���仺��������
	 */
	void grow (size_t growSize = _initSize);

private:
	unsigned char*			_pBuf;				// �����ַָ��
	size_t					_capacity;			// ����������С
	size_t					_offset;			// ���浱ǰλ��

	static const size_t		_initSize = 512;	// ��ʼ��������
	static const size_t		_sizeLen = 4;		// ����ռ�õ��ֽ���

}; // End class asIPCPipeMessage

#endif // __AS_IPC_PIPE_MESSAGE_H__
