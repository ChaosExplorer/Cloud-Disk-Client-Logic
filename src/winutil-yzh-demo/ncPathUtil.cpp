/***************************************************************************************************
ncPathUtil.cpp:
	Copyright (c) Eisoo Software, Inc.(2004-2015), All rights reserved.

Purpose:
	Source file for path related utilities.

Author:
	Chaos

Created Time:
	2015-11-24
***************************************************************************************************/

#include <abprec.h>
#include "winutil.h"

// public static
BOOL ncPathUtil::IsParentPath (LPCTSTR lpPath1, LPCTSTR lpPath2, BOOL bIncludeSelf/* = FALSE*/)
{
	if (lpPath1 == NULL || lpPath2 == NULL)
		return FALSE;

	const TCHAR separator = _T('\\');

	tstring p1 (lpPath1);
	tstring p2 (lpPath2);

	if (p1[p1.length () - 1] != separator)
		p1 += separator;

	if (p2[p2.length () - 1] != separator)
		p2 += separator;

	if (p1.length () < p2.length ())
		return _tcsncicmp (p2.c_str (), p1.c_str (), p1.length ()) == 0;
	else
		return bIncludeSelf && _tcsicmp (p2.c_str (), p1.c_str ()) == 0;
}
