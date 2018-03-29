/***************************************************************************************************
Purpose:
	Header file for class ncDetector.

Author:
	Chaos

Creating Time:
	2016-8-19
***************************************************************************************************/

#ifndef __NC_DETECTOR_H__
#define __NC_DETECTOR_H__

#if PRAGMA_ONCE
	#pragma once
#endif

#include <docmgr/public/ncICacheMgr.h>
#include <syncmgr/private/ncISyncTask.h>

class ncIEACPClient;
class ncIEFSPClient;
class ncSyncPolicyMgr;
class ncSyncScheduler;
class ncChangeListener;

// ̽��ģʽ���塣
enum ncDetectMode
{
	NC_DM_DELAY,		// �ӳ�ģʽ��̽��ʱ�����ӳ����ص�Ŀ¼������������������ӳ����أ�
	NC_DM_POLICY,		// ����ģʽ��̽��ʱ����ӳ����ز��ԣ������ݲ��Զ�����������ж�Ӧ���أ�
	NC_DM_FULL,			// ����ģʽ��̽��ʱ�������ӳ����ص�Ŀ¼�������������������ȫ���أ�
};

class ncDetector 
	: public Thread
{
public:
	ncDetector (ncIEACPClient* eacpClient,
				ncIEFSPClient* efspClient,
				ncIDocMgr* docMgr,
				ncSyncPolicyMgr* policyMgr,
				ncSyncScheduler* scheduler,
				ncChangeListener* chgListener,
				String localSyncPath);
	virtual ~ncDetector ();

public:
	// ��ʼ̽�⣨����̽���̣߳���
	void Start ();

	// ֹͣ̽�⣨ֹͣ̽���̣߳���
	void Stop ();

	// ����̽�⣨�ڵ������߳���̽��ָ��Ŀ¼����
	void InlineDetect (const String& docId, const String& relPath, ncDetectMode mode);

	// ����̽�⣨�ڵ������߳���̽������ָ��·����
	void InlineDetectExStart (const String& parrentDocId, const String& relPath, const String& docId, bool isDir);
	void InlineDetectExEnd ();

	void run ();

protected:
	// ̽��ָ��Ŀ¼��
	bool DetectDir (const String& docId,
					const String& relPath,
					ncDetectMode mode,
					bool isDelay = false);

	// ��ȡĿ¼���ƶ����ĵ���Ϣ��
	bool GetServerSubInfos (const String& docId, DetectInfoMap& svrMap);

	// ��ȡĿ¼���ƶ�ָ�����ĵ���Ϣ��
	bool GetServerSubInfo (const String& docId, const String& subDocId, ncDetectInfo& subInfo, bool isDir);

	// ��ȡ�ĵ���������Ϣ��
	void GetRenameInfo (const String& svrName,
						const String& cacheName,
						map<String, String>& rnmDirs,
						map<String, String>& rnmFiles,
						bool isDir);

	// ��ȡĿ¼���ƶ����ĵ���Ϣ��
	bool CheckDir (const String& docId);

private:
	AtomicValue						_stop;			// �Ƿ�ֹͣ̽��
	AtomicValue						_inlCnt;		// ����̽�����

	nsCOMPtr<ncIEACPClient>			_eacpClient;	// EACP�ͻ��˶���
	nsCOMPtr<ncIEFSPClient>			_efspClient;	// EFSP�ͻ��˶���
	nsCOMPtr<ncIDocMgr>				_docMgr;		// �ĵ��������
	nsCOMPtr<ncICacheMgr>			_cacheMgr;		// ����������
	nsCOMPtr<ncSyncPolicyMgr>		_policyMgr;		// ͬ�����Թ������
	nsCOMPtr<ncSyncScheduler>		_scheduler;		// ͬ�����ȶ���
	nsCOMPtr<ncChangeListener>		_chgListener;	// �仯��������

	bool							_newView;		// ��ģʽ
	ncTaskSrc						_taskSrc;		// ������Դ
	String							_localSyncPath;	// ����ͬ��Ŀ¼
};

#endif // __NC_DETECTOR_H__
