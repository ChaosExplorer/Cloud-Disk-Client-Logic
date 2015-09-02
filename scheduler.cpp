/***************************************************************************************************
scheduler.cpp:
	Copyright (c) Eisoo Software, Inc.(2013), All rights reserved.

Purpose:
	scheduler 组件公共定义源文件。

Author:
	yi.zhihui@eisoo.com

Created Time:
	2013-5-21
***************************************************************************************************/

#include <abprec.h>
#include "scheduler.h"

//
// Definitions
//
IResourceLoader* schedulerLoader = 0;

//
// UMM memory pools
//
NC_DEFINE_UMM_ALLOCATOR (ncSchedulerAlloc);

//
// Functions
//
void ncCreateSchedulerUMMAllocators (void)
{
	//
	// 创建临时数据内存池，不限制大小，内存不足时抛错。
	//
	if (ncSchedulerAlloc == 0) {
		NC_CREATE_UMM_ALLOACTOR (ncModulePool::getInstance (),
								 ncSchedulerAlloc,
								 UMM_ERROR);
	}
}

void ncDestroySchedulerUMMAllocators (void)
{
	NC_DESTROY_UMM_ALLOCATOR (ncModulePool::getInstance (), ncSchedulerAlloc);
}

void ncCreateSchedulerMoResourceLoader (const AppSettings *appSettings,
										const AppContext *appCtx)
{
	if (schedulerLoader == 0) {
		schedulerLoader = new MoResourceLoader (::getResourceFileName (LIB_NAME,
													appSettings,
													appCtx,
													AB_RESOURCE_MO_EXT_NAME));
	}
}

void ncDestroySchedulerMoResourceLoader (void)
{
	delete schedulerLoader;
	schedulerLoader = 0;
}
