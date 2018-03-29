/**
*	@Created Time : 2016-6-29
*	@Author : Chaos
**/


#include "stdafx.h"
#include "ServiceTool.h"
#include "Winsvc.h"
#include "Windows.h"

ServiceTool::ServiceTool()
{
}


ServiceTool::~ServiceTool()
{
}

bool ServiceTool::isServiceRunning(const CString & serviceName)
{
	bool result = false;

	try {
		SC_HANDLE hSC = OpenSCManager(NULL, NULL, GENERIC_EXECUTE);
		if (NULL == hSC) {
			return false;
		}

		SC_HANDLE hSvc = OpenService( hSC, serviceName,    
            SERVICE_START | SERVICE_QUERY_STATUS | SERVICE_STOP);    
        if( hSvc == NULL)    
        {    
            CloseServiceHandle( hSC);    
            return false;
        }    
  
        SERVICE_STATUS status;    
        if(QueryServiceStatus( hSvc, &status) == FALSE)    
        {    
            CloseServiceHandle( hSvc);    
            CloseServiceHandle( hSC);    
            return false;
        }    
  
		if (status.dwCurrentState == SERVICE_RUNNING)
		{
			result = true;
		}
		
		CloseServiceHandle( hSvc);    
		CloseServiceHandle( hSC);    
	}
	catch (...) {

	}
	

	return result;
}

bool ServiceTool::restartService(const CString & serviceName)
{
	try
	{
		SC_HANDLE hSC = OpenSCManager( NULL,     
            NULL, GENERIC_EXECUTE);    
        if( hSC == NULL)    
        {    
            return false;
        }    
  
        SC_HANDLE hSvc = OpenService( hSC, serviceName,    
            SERVICE_START | SERVICE_QUERY_STATUS | SERVICE_STOP);    
        if( hSvc == NULL)    
        {    
            CloseServiceHandle( hSC);    
            return false;    
        }    
  
        SERVICE_STATUS status;    
        if( QueryServiceStatus( hSvc, &status) == FALSE)    
        {    
            CloseServiceHandle( hSvc);    
            CloseServiceHandle( hSC);    
            return false;    
        }    
  
        if( status.dwCurrentState == SERVICE_RUNNING)    
        {    
            if( ControlService( hSvc,     
                SERVICE_CONTROL_STOP, &status) == FALSE)    
            {    
                CloseServiceHandle( hSvc);    
                CloseServiceHandle( hSC);    
                return false;   
            }    
  
            while( QueryServiceStatus(hSvc, &status) == TRUE)    
            {    
                Sleep( status.dwWaitHint);    
				if (status.dwCurrentState == SERVICE_STOPPED)
					break;
            }
        }
		
		if (status.dwCurrentState != SERVICE_RUNNING)
		{
			StartService(hSvc, 0, NULL);
			CloseServiceHandle( hSvc);    
			CloseServiceHandle( hSC);    
			return true; 
}
		
	}
	catch (...) {
		MessageBox(NULL, _T("Restart service failed"), _T("Error Occured"), MB_OK);
	}

	return false;
}
