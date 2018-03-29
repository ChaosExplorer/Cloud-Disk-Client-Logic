/**
*	@Created Time : 2016-6-29
*	@Author : Chaos
**/


// ProgressBar.cpp : 实现文件
//

#include "stdafx.h"
#include "BugReporter.h"
#include "ProgressBar.h"
#include "afxdialogex.h"


// ProgressBar 对话框

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
	// 释放了对自动化对象的最后一个引用后，将调用
	// OnFinalRelease。  基类将自动
	// 删除该对象。  在调用该基类之前，请添加您的
	// 对象所需的附加清理代码。

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

// 注意: 我们添加 IID_IProgressBar 支持
//  以支持来自 VBA 的类型安全绑定。  此 IID 必须同附加到 .IDL 文件中的
//  调度接口的 GUID 匹配。

// {EE73FAE0-080E-49C6-B263-D33182376E5E}
static const IID IID_IProgressBar =
{ 0xEE73FAE0, 0x80E, 0x49C6, { 0xB2, 0x63, 0xD3, 0x31, 0x82, 0x37, 0x6E, 0x5E } };

BEGIN_INTERFACE_MAP(ProgressBar, CDialogEx)
	INTERFACE_PART(ProgressBar, IID_IProgressBar, Dispatch)
END_INTERFACE_MAP()


// ProgressBar 消息处理程序
