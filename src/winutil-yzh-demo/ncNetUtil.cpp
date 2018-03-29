/***************************************************************************************************
ncNetUtil.cpp:
	Copyright (c) Eisoo Software, Inc.(2004-2015), All rights reserved.

Purpose:
	Source file for network related utilities.

Author:
	Chaos

Creating Time:
	2015-07-15
***************************************************************************************************/

#include <abprec.h>
#include "winutil.h"

#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")

// public static
tstring ncNetUtil::GetMacAddress (TCHAR separator/* = _T('')*/)
{
	tstring macAddr;

	PIP_ADAPTER_INFO pAdapterInfo = (IP_ADAPTER_INFO*)malloc (sizeof(IP_ADAPTER_INFO));
	ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
	
	if (GetAdaptersInfo (pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
		free (pAdapterInfo);
		pAdapterInfo = (IP_ADAPTER_INFO*)malloc (ulOutBufLen);
	}

	if (GetAdaptersInfo (pAdapterInfo, &ulOutBufLen) == ERROR_SUCCESS) {
		PIP_ADAPTER_INFO pAdapters = pAdapterInfo;
		while (pAdapters != NULL){
			// 验证IP 子网掩码 网络表入口 以及网关
			if (pAdapters->IpAddressList.IpAddress.String == "0.0.0.0"
				|| pAdapters->IpAddressList.IpMask.String == "0.0.0.0"
				|| pAdapters->IpAddressList.Context == 0
				|| pAdapters->GatewayList.IpAddress.String == "0.0.0.0") {
					pAdapters = pAdapters->Next;
					continue;
			}
			else {
				TCHAR szBuf[3];
				for (UINT i = 0; i < pAdapterInfo->AddressLength; i++) {
					_stprintf (szBuf, _T("%.2X"), (int)pAdapterInfo->Address[i]);
					macAddr.append (szBuf);

					if (i != pAdapterInfo->AddressLength - 1 && separator != _T(''))
						macAddr += separator;
				}
				break;
			}
		}
	}
	free (pAdapterInfo);

	if (macAddr.empty ())
		macAddr = _T("ff-ff-ff-ff-ff-ff");

	return macAddr;
}
