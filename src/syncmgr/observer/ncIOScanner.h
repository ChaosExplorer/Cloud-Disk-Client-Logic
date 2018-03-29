/***************************************************************************************************
Purpose:
	Header file for class ncIOScanner.

Author:
	Chaos

Created Time:
	2016-8-19

***************************************************************************************************/

#ifndef __NC_IO_SCANNER_H__
#define __NC_IO_SCANNER_H__

#if PRAGMA_ONCE
	#pragma once
#endif

#include <docmgr/public/ncIDocMgr.h>
#include <docmgr/public/ncICacheMgr.h>
#include <docmgr/public/ncIRelayMgr.h>
#include <syncmgr/private/ncISyncTask.h>

class ncIDocMgr;
class ncICacheMgr;
class ncIPathMgr;
class ncIADSMgr;
class ncIRelayMgr;
class ncSyncScheduler;
class ncIChangeListener;

class ncIOScanner 
	: public Thread
{
public:
	ncIOScanner (ncIDocMgr* docMgr,
				 ncSyncScheduler* scheduler,
				 ncIChangeListener* chgListener,
				 String localSyncPath);
	virtual ~ncIOScanner ();

public:
	// ��ʼɨ�裨����ɨ���̣߳���
	void Start ();

	// ֹͣɨ�裨ֹͣɨ���̣߳���
	void Stop ();

	// ����ɨ�裨�ڵ������߳���ɨ��ָ��Ŀ¼����
	void InlineScan (const String& path);

	void run ();

protected:
	// ɨ��ָ��Ŀ¼��
	void ScanDir (const String& path);

	// ��ȡĿ¼�ı������ĵ���Ϣ��
	bool GetLocalSubInfos (const String& path,
						   ScanInfoMap& docIdInfos,
						   set<String>& noDocIdFiles,
						   set<String>& noDocIdDirs,
						   set<String>& orgnDirs);

	void ScanNonius ();

	// ɨ����Delay�����������ֱ��غ����ݿ�һ��
	void ReviseDelayADS (const String& path);

	void ScanUnsync ();

private:
	AtomicValue						_stop;			// �Ƿ�ֹͣɨ��
	AtomicValue						_inlCnt;		// ����ɨ�����

	nsCOMPtr<ncIDocMgr>				_docMgr;		// �ĵ��������
	nsCOMPtr<ncIPathMgr>			_pathMgr;		// ·���������
	nsCOMPtr<ncIADSMgr>				_adsMgr;		// �������������
	nsCOMPtr<ncICacheMgr>			_cacheMgr;		// ����������
	nsCOMPtr<ncIRelayMgr>			_relayMgr;		// �ϵ������������
	nsCOMPtr<ncIChangeListener>		_chgListener;	// �仯��������
	nsCOMPtr<ncSyncScheduler>		_scheduler;		// ͬ�����ȶ���

	bool							_newView;		// ��ģʽ
	ncTaskSrc						_taskSrc;		// ������Դ
	String							_localSyncPath;	// ����ͬ��Ŀ¼
};

#endif // __NC_IO_SCANNER_H__
