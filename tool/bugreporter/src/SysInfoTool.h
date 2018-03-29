/**
*	@Created Time : 2016-6-29
*	@Author : Chaos
**/

#pragma once

#include<string>
using namespace std;

// ����ϵͳ�汾���塣
enum ncOSVersion
{
	OSV_UNKNOWN,
	OSV_WINXP,
	OSV_WIN2003,
	OSV_WINVISTA,
	OSV_WIN2008,
	OSV_WIN7,
	OSV_WIN2008_R2,
	OSV_WIN8,
	OSV_WIN2012,
	OSV_WIN8_1,
	OSV_WIN2012_R2,
	OSV_WIN10,
};



class SysInfoTool
{
public:
	SysInfoTool();
	~SysInfoTool();

public:
	// ������ϵͳ�Ƿ�Ϊ32λ��
	static BOOL Is32BitOS ();

	// ��ȡ����ϵͳ�汾��
	static ncOSVersion GetOSVersion ();


	string getSystemInfo();

	void exportEventLog(const CString& outputPath);

	string OSString[12] =
	{
	"UNKNOWN",
	"WINXP",
	"WIN2003",
	"WINVISTA",
	"WIN2008",
	"WIN7",
	"WIN2008_R2",
	"WIN8",
	"WIN2012",
	"WIN8_1",
	"WIN2012_R2",
	"WIN10",
	};
};

