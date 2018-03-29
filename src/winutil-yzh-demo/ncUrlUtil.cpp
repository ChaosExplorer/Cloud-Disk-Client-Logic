/***************************************************************************************************
ncUrlUtil.cpp:
	Copyright (c) Eisoo Software, Inc.(2004-2015), All rights reserved.

Purpose:
	Source file for url related utilities.

Author:
	Chaos

Created Time:
	2015-05-23
***************************************************************************************************/

#include <abprec.h>
#include <atlbase.h>
#include "winutil.h"

//
// Converts a hex character to its integer value.
//
static inline char FromHex (char ch)
{
	return isdigit (ch) ? ch - '0' : tolower (ch) - 'a' + 10;
}

//
// Converts an integer value to its hex character.
//
static inline char ToHex (char ch)
{
	static char hex[] = "0123456789abcdef";
	return hex[ch & 0xF];
}

// public static
tstring ncUrlUtil::EncodeUrl (LPCTSTR lpUrl)
{
	if (lpUrl == NULL)
		return _T("");

	CT2A str (lpUrl, CP_UTF8);
	char* pstr = str;
	char* buf = new char[strlen (str) * 3 + 1];
	char* pbuf = buf;

	while (*pstr) {
		// 注意不要使用isalnum函数来判断是否为数字或字母，因为该函数依赖于系统的locale区域语言选项。
		if ((*pstr >= '0' && *pstr <= '9') || (*pstr >= 'a' && *pstr <= 'z') || (*pstr >= 'A' && *pstr <= 'Z') ||
			*pstr == '-' || *pstr == '_' || *pstr == '.' || *pstr == '*') {
			*pbuf++ = *pstr;
		}
		else {
			*pbuf++ = '%';
			*pbuf++ = ToHex (*pstr >> 4);
			*pbuf++ = ToHex (*pstr & 0xF);
		}
		pstr++;
	}
	*pbuf = '\0';

	CA2CT encode (buf, CP_UTF8);

	delete buf;
	return (LPCTSTR)encode;
}

// public static
tstring ncUrlUtil::DecodeUrl (LPCTSTR lpUrl)
{
	if (lpUrl == NULL)
		return _T("");

	CT2A str (lpUrl, CP_UTF8);
	char* pstr = str;
	char* buf = new char[strlen (str) + 1];
	char* pbuf = buf;

	while (*pstr) {
		if (*pstr != '%') {
			*pbuf++ = *pstr;
		}
		else if (pstr[1] && pstr[2]) {
			*pbuf++ = FromHex (pstr[1]) << 4 | FromHex (pstr[2]);
			pstr += 2;
		}
		pstr++;
	}
	*pbuf = '\0';

	CA2CT decode (buf, CP_UTF8);

	delete buf;
	return (LPCTSTR)decode;
}

// public static
void ncUrlUtil::CopyUrl (LPCTSTR lpUrl)
{
	if (lpUrl == NULL)
		return;

	tstring url (lpUrl);
	size_t index = url.find (_T("://"));

	// Get href elements from url...
	CT2A hrefHead (url.substr (0, index + 3).c_str ());
	CT2A hrefBody (EncodeUrl (url.substr (index + 3).c_str ()).c_str ());
	CT2A hrefText (lpUrl, CP_UTF8);

	// Create temporary buffer for HTML header...
	char* buf = new char[400 + strlen (hrefHead) + strlen (hrefBody) + strlen (hrefText)];

	// Create a template string for the HTML header...
	strcpy (buf,
			"Version:0.9\r\n"
			"StartHTML:00000000\r\n"
			"EndHTML:00000000\r\n"
			"StartFragment:00000000\r\n"
			"EndFragment:00000000\r\n"
			"<html><body>\r\n"
			"<!--StartFragment -->\r\n");

	// Append the HTML...
	strcat (buf, "<a href=\"");
	strcat (buf, hrefHead);
	strcat (buf, hrefBody);
	strcat (buf, "\">");
	strcat (buf, hrefText);
	strcat (buf, "</a>");
	strcat (buf, "\r\n");

	// Finish up the HTML format...
	strcat (buf,
			"<!--EndFragment-->\r\n"
			"</body>\r\n"
			"</html>");

	// Now go back, calculate all the lengths, and write out the
	// necessary header information. Note, sprintf() truncates the
	// string when you overwrite it so you follow up with code to
	// replace the 0 appended at the end with a '\r'...
	char* ptr = strstr (buf, "StartHTML");
	sprintf (ptr+10, "%08u", strstr (buf, "<html>") - buf);
	*(ptr+10+8) = '\r';

	ptr = strstr (buf, "EndHTML");
	sprintf (ptr+8, "%08u", strlen (buf));
	*(ptr+8+8) = '\r';

	ptr = strstr (buf, "StartFragment");
	sprintf (ptr+14, "%08u", strstr (buf, "<!--StartFrag") - buf);
	*(ptr+14+8) = '\r';

	ptr = strstr (buf, "EndFragment");
	sprintf (ptr+12, "%08u", strstr (buf, "<!--EndFrag") - buf);
	*(ptr+12+8) = '\r';

	// Open the clipboard...
	if (OpenClipboard (NULL)) {
		// Empty what's in there...
		EmptyClipboard ();

		// Register HTML clipboard format...
		static int cfid = 0;
		if(!cfid) cfid = RegisterClipboardFormat(_T("HTML Format"));

		// Set html data to clipboard...
		HGLOBAL hHtml = GlobalAlloc (GMEM_MOVEABLE|GMEM_DDESHARE, strlen (buf) + 1);
		char* pHtml = (char*)GlobalLock (hHtml);
		strcpy (pHtml, buf);
		GlobalUnlock (hHtml);
		::SetClipboardData (cfid, hHtml);
		GlobalFree (hHtml);

		// Set text data to clipboard...
		HGLOBAL hText = GlobalAlloc (GMEM_MOVEABLE|GMEM_DDESHARE, (_tcslen (lpUrl) + 1) * sizeof(TCHAR));
		TCHAR* pText = (TCHAR*)GlobalLock (hText);
		_tcscpy (pText, lpUrl);
		GlobalUnlock (hText);
		::SetClipboardData (CF_UNICODETEXT, hText);
		GlobalFree (hText);

		// Close the clipboard...
		CloseClipboard ();
	}

	// Clean up...
	delete[] buf;
}
