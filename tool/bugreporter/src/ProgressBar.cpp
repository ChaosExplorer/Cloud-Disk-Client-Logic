/**
*	@Created Time : 2016-6-29
*	@Author : Chaos
**/


// ProgressBar.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "BugReporter.h"
#include "ProgressBar.h"
#include "afxdialogex.h"


// ProgressBar �Ի���

IMPLEMENT_DYNAMIC(ProgressBar, CDialogEx)

ProgressBar::ProgressBar(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_DIALOG1, pParent)
{

	EnableAutomation();

}

ProgressBar::~ProgressBar()
{
}

void ProgressBar::OnFinalRelease()
{
	// �ͷ��˶��Զ�����������һ�����ú󣬽�����
	// OnFinalRelease��  ���ཫ�Զ�
	// ɾ���ö���  �ڵ��øû���֮ǰ�����������
	// ��������ĸ���������롣

	CDialogEx::OnFinalRelease();
}

void ProgressBar::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(ProgressBar, CDialogEx)
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(ProgressBar, CDialogEx)
END_DISPATCH_MAP()

// ע��: ������� IID_IProgressBar ֧��
//  ��֧������ VBA �����Ͱ�ȫ�󶨡�  �� IID ����ͬ���ӵ� .IDL �ļ��е�
//  ���Ƚӿڵ� GUID ƥ�䡣

// {EE73FAE0-080E-49C6-B263-D33182376E5E}
static const IID IID_IProgressBar =
{ 0xEE73FAE0, 0x80E, 0x49C6, { 0xB2, 0x63, 0xD3, 0x31, 0x82, 0x37, 0x6E, 0x5E } };

BEGIN_INTERFACE_MAP(ProgressBar, CDialogEx)
	INTERFACE_PART(ProgressBar, IID_IProgressBar, Dispatch)
END_INTERFACE_MAP()


// ProgressBar ��Ϣ�������
