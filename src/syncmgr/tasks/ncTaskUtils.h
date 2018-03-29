/***************************************************************************************************
Purpose:
	Header file for class ncTaskUtils.

Author:
	Chaos

Created Time:
	2016-09-10

***************************************************************************************************/

#ifndef __NC_TASK_UTILS_H__
#define __NC_TASK_UTILS_H__

#if PRAGMA_ONCE
	#pragma once
#endif

#include <efshttpclient/public/ncIEFSPClient.h>
#include <syncmgr/private/ncISyncTask.h>
#include <syncmgr/public/ncISyncScheduler.h>
#include <docmgr/public/ncICacheMgr.h>
#include <docMgr/public/ncIADSMgr.h>

class ncTaskUtils
{
public:
	// ���Ŀ¼���Ƿ��б��޸ĵ��ļ���
	static bool HasEditedFilesInDir (const String & path);

	// ��鲢ɾ����ڵĲ㼶��Ŀ¼��
	static void CheckDelEntryParent (const String& localPath, const String& path, int docType);

	// �ϴ�����������Ŀ¼��
	static void UploadRenamedDir (ncTaskSrc taskSrc, const String& localPath, const String& path);

	// �ϴ������������ļ���
	static void UploadRenamedFile (ncTaskSrc taskSrc, const String& localPath, const String& path);

	// �������������Ŀ¼�Ĺ����仯��
	static void CheckDirChange (const String& localPath, const String& path);

	// ��������������ļ��Ĺ����仯��
	static void CheckRenamedFileChange (ncTaskSrc taskSrc, const String& localPath, const String& path);

	// ��鲢��ԭ���ܶ�ʧ������docId��������
	static bool RestoreDocIdChanged (const String & path, const String & docId, bool isDir = false);

	/**
	* ����ļ��Ƿ񱻶�ռ��
	* @param path : ����·��
	* @param dwLockedAccess : ��Ҫ��ѯ��Ȩ��
	* @param checkLasttime : ����϶�ռ��ʱ������λ���룩
	* @param status: ����״̬
	*/ 
	static bool IsFileOccupied (const String& path, DWORD dwLockedAccess, int checLastTime, ncTaskStatus& status);

	// ��Ӷ��ļ��Ļ����������
	static void ClearFileCache (ncTaskSrc taskSrc, const String& relPath);

	// ��ȡ����Ŀ¼·�� ����������Ŀ¼ģʽ��
	static String GetTopDirPath (const String& relPath);

	// �ϴ�֮ǰ����������������Ƿ��ļ����ܣ�����ȡ������Ϣ
	static bool GetFileLabelInfo (const String filePath, const String docId, String& securityInfo);

	// ��װ���������ñ�����Ϣ
	static void SetThirdSecurityInfo (ncIEFSPClient* efspClient, const String& appId, const String& docId, const String& securityInfo);
	static void SetThirdSecurityInfoEx (ncIEFSPFileOperation* efspFileOperation, const String& appId, const String& docId, const String& securityInfo);

	// �������������ܼ�����ȡ
	static void GetThirdSecurityLevel (map<int, String>csfMapEnum, String securityInfo, int& csf);

	// �ж��ܼ���Ϣ�Ƿ�Ϊ��
	static bool isSecurityInfoEmpty (const String& securityInfo);

	// ��ʱ�������״̬���
	static void TaskStatusCheck (ncTaskStatus& status);

	// ���������ƶ�
	static HRESULT MutiSHCopyMove (vector<String>& srcPaths, const String& dstPath, int wFunc, int fFlags);

	// �����ƶ�
	static HRESULT SHCopyMove (const String& srcPath, const String& dstPath, int wFunc, int fFlags);

	// ��Ŀ¼�µ�δͬ���ĵ������ƶ���Ŀ�ĵ�
	static void TransferUnsyncInDir (const String& localPath, const String& dirPath, const String& dstPath, bool isMove, bool needExpand = true);
};

#endif  //__NC_TASK_UTILS_H__
