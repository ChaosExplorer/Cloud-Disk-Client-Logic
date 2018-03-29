/**
*	@Created Time : 2016-6-29
*	@Author : Chaos
**/

#pragma once

class ServiceTool
{
public:
	ServiceTool();
	~ServiceTool();

public :
	
	static bool isServiceRunning(const CString& serviceName);

	static bool restartService(const CString& serviceName);
};

