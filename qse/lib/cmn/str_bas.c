/*
 * $Id: str_bas.c 421 2011-03-29 15:37:19Z hyunghwan.chung $
 *
    Copyright 2006-2009 Chung, Hyung-Hwan.
    This file is part of QSE.

    QSE is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as 
    published by the Free Software Foundation, either version 3 of 
    the License, or (at your option) any later version.

    QSE is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public 
    License along with QSE. If not, see <http://www.gnu.org/licenses/>.
 */

#include <qse/cmn/str.h>
#include <qse/cmn/chr.h>
#include "mem.h"

qse_size_t qse_mbslen (const qse_mchar_t* mbs)
{
	const qse_mchar_t* p = mbs;
	while (*p != QSE_MT('\0')) p++;
	return p - mbs;
}

qse_size_t qse_wcslen (const qse_wchar_t* wcs)
{
	const qse_wchar_t* p = wcs;
	while (*p != QSE_WT('\0')) p++;
	return p - wcs;
}

qse_size_t qse_strbytes (const qse_char_t* str)
{
	const qse_char_t* p = str;
	while (*p != QSE_T('\0')) p++;
	return (p - str) * QSE_SIZEOF(qse_char_t);
}

const qse_char_t* qse_strxword (
	const qse_char_t* str, qse_size_t len, const qse_char_t* word)
{
	/* find a full word in a string */

	const qse_char_t* ptr = str;
	const qse_char_t* end = str + len;
	const qse_char_t* s;

	do
	{
		while (ptr < end && QSE_ISSPACE(*ptr)) ptr++;
		if (ptr >= end) return QSE_NULL;

		s = ptr;
		while (ptr < end && !QSE_ISSPACE(*ptr)) ptr++;

		if (qse_strxcmp (s, ptr-s, word) == 0) return s;
	}
	while (ptr < end);

	return QSE_NULL;
}

const qse_char_t* qse_strxcaseword (
	const qse_char_t* str, qse_size_t len, const qse_char_t* word)
{
	const qse_char_t* ptr = str;
	const qse_char_t* end = str + len;
	const qse_char_t* s;

	do
	{
		while (ptr < end && QSE_ISSPACE(*ptr)) ptr++;
		if (ptr >= end) return QSE_NULL;

		s = ptr;
		while (ptr < end && !QSE_ISSPACE(*ptr)) ptr++;

		if (qse_strxcasecmp (s, ptr-s, word) == 0) return s;
	}
	while (ptr < end);

	return QSE_NULL;
}


qse_char_t* qse_strbeg (const qse_char_t* str, const qse_char_t* sub)
{
	while (*sub != QSE_T('\0'))
	{
		if (*str != *sub) return QSE_NULL;
		str++; sub++;
	}

	/* returns the pointer to the next character of the match */
	return (qse_char_t*)str;
}

qse_char_t* qse_strxbeg (
	const qse_char_t* str, qse_size_t len, const qse_char_t* sub)
{
	const qse_char_t* end = str + len;

	while (*sub != QSE_T('\0'))
	{
		if (str >= end || *str != *sub) return QSE_NULL;
		str++; sub++;
	}

	/* returns the pointer to the next character of the match */
	return (qse_char_t*)str;
}

qse_char_t* qse_strnbeg (
	const qse_char_t* str, const qse_char_t* sub, qse_size_t len)
{
	const qse_char_t* end = sub + len;
		
	while (sub < end)
	{
		if (*str == QSE_T('\0') || *str != *sub) return QSE_NULL;
		str++; sub++;
	}

	/* returns the pointer to the next character of the match */
	return (qse_char_t*)str;
}

qse_char_t* qse_strxnbeg (
	const qse_char_t* str, qse_size_t len1, 
	const qse_char_t* sub, qse_size_t len2)
{
	const qse_char_t* end1, * end2;

	if (len2 > len1) return QSE_NULL;

	end1 = str + len1;
	end2 = sub + len2;

	while (sub < end2)
	{
		if (str >= end1 || *str != *sub) return QSE_NULL;
		str++; sub++;
	}

	/* returns the pointer to the next character of the match */
	return (qse_char_t*)str;
}

qse_char_t* qse_strend (const qse_char_t* str, const qse_char_t* sub)
{
	return qse_strxnend (str, qse_strlen(str), sub, qse_strlen(sub));
}

qse_char_t* qse_strxend (
	const qse_char_t* str, qse_size_t len, const qse_char_t* sub)
{
	return qse_strxnend (str, len, sub, qse_strlen(sub));
}

qse_char_t* qse_strnend (
	const qse_char_t* str, const qse_char_t* sub, qse_size_t len)
{
	return qse_strxnend (str, qse_strlen(str), sub, len);
}

qse_char_t* qse_strxnend (
	const qse_char_t* str, qse_size_t len1, 
	const qse_char_t* sub, qse_size_t len2)
{
	const qse_char_t* end1, * end2;

	if (len2 > len1) return QSE_NULL;

	end1 = str + len1;
	end2 = sub + len2;

	while (end2 > sub)
	{
		if (end1 <= str) return QSE_NULL;
		if (*(--end1) != *(--end2)) return QSE_NULL;
	}
	
	/* returns the pointer to the match start */
	return (qse_char_t*)end1;
}
