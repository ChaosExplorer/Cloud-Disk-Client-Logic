/***************************************************************************************************
pipemsg.cpp:
	Copyright (c) Eisoo Software Inc.(2004-2012), All rights reserved

Purpose:
	Source file for class asIPCPipeMessage.

Author:
	Chaos

Creating Time:
	2012-6-29
***************************************************************************************************/

#include <abprec.h>
#include "winipc.h"

/////////////////////////////////////////////////////////////////////////////
// class asIPCPipeMessage

// public ctor
asIPCPipeMessage::asIPCPipeMessage ()
	: _pBuf (0)
	, _capacity (_initSize)
	, _offset (0)
{
	WINIPC_TRACE (_T("asIPCPipeMessage ctor: 0x%p"), this);

	_pBuf = new unsigned char[_capacity];
}

// public dtor
asIPCPipeMessage::~asIPCPipeMessage ()
{
	WINIPC_TRACE (_T("asIPCPipeMessage dtor: 0x%p"), this);

	delete _pBuf;
}

// public
void
asIPCPipeMessage::writeToPipe (HANDLE hPipe) const
{
	WINIPC_TRACE (_T("begin - writeToPipe: %p, hPipe: %p"), this, hPipe);

	DWORD dwWritten;
	if (!::WriteFile (hPipe, _pBuf, (DWORD)_offset, &dwWritten, NULL) || dwWritten != _offset)
		asThrowIPCException (WINIPC_WRITE_PIPE_ERROR);

	WINIPC_TRACE (_T("end - writeToPipe: %p, hPipe: %p"), this, hPipe);
}

// public
void
asIPCPipeMessage::readFromPipe (HANDLE hPipe)
{
	WINIPC_TRACE (_T("begin - readFromPipe: %p, hPipe: %p"), this, hPipe);

	DWORD dwRead;
	if (!::ReadFile (hPipe, _pBuf, (DWORD)_capacity, &dwRead, NULL) || dwRead == 0)
		asThrowIPCException (WINIPC_READ_PIPE_ERROR);

	// ���ܵ����Ƿ���ʣ�����ݣ����������仺����������ȡʣ�����ݡ�
	if (dwRead == _capacity) {
		LARGE_INTEGER li;

		if (::GetFileSizeEx (hPipe, &li) && li.LowPart != 0) {
			_offset = dwRead;
			grow (li.LowPart);

			if (!::ReadFile (hPipe, _pBuf + _offset, li.LowPart, &dwRead, NULL) || dwRead == 0) {
				asThrowIPCException (WINIPC_READ_PIPE_ERROR);
			}
		}
	}

	// ��ȡ��ɺ����û���ƫ��λ�á�
	_offset = 0;

	WINIPC_TRACE (_T("end - readFromPipe: %p, hPipe: %p"), this, hPipe);
}

// protected
asIPCPipeMessage&
asIPCPipeMessage::writeBytes (const unsigned char* bytes, UINT size)
{
	size_t writeSize = size + _sizeLen;

	// ����������������������仺��������
	if (_offset + writeSize > _capacity)
		grow (writeSize + _initSize);

	// д��˴����ݵĳ��ȡ�
	memcpy (_pBuf + _offset, reinterpret_cast<unsigned char*>(&size), _sizeLen);
	_offset += _sizeLen;

	// д��˴����ݵ����ݡ�
	memcpy (_pBuf + _offset, bytes, size);
	_offset += size;

	return (*this);
}

// protected
asIPCPipeMessage&
asIPCPipeMessage::readBytes (unsigned char* bytes)
{
	// ��ȡ�˴����ݵĳ��ȡ�
	UINT size;
	memcpy (reinterpret_cast<unsigned char*>(&size), _pBuf + _offset, _sizeLen);
	_offset += _sizeLen;

	// ��ȡ�˴����ݵ����ݡ�
	memcpy (bytes, _pBuf + _offset, size);
	_offset += size;

	return (*this);
}

// protected
void
asIPCPipeMessage::grow (size_t growSize/* = _initSize*/)
{
	// �������ڴ档
	unsigned char* pBuf = new unsigned char[_capacity + growSize];

	// ����ԭ���ݵ����ڴ档
	memcpy (pBuf, _pBuf, _offset);

	// ɾ��ԭ�ڴ档
	delete _pBuf;

	// ���»�����Ϣ��
	_pBuf = pBuf;
	_capacity += growSize;
}
