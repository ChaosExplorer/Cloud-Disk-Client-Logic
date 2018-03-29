/***************************************************************************************************
ncCursorUtil.h:
	Copyright (c) Eisoo Software, Inc.(2004-2015), All rights reserved.

Purpose:
	Header file for cursor related utilities.

Author:
	Chaos

Creating Time:
	2015-02-25
***************************************************************************************************/

#ifndef __NC_CURSOR_UTILITIES_H__
#define __NC_CURSOR_UTILITIES_H__

#pragma once

class ncScopedCursor
{
public:
	ncScopedCursor ()
	{
		::SetCursor (LoadCursor (NULL, IDC_WAIT));
	}

	~ncScopedCursor ()
	{
		::SetCursor (LoadCursor (NULL, IDC_ARROW));
	}
};

#endif // __NC_CURSOR_UTILITIES_H__
