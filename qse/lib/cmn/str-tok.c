/*
 * $Id$
 *
    Copyright (c) 2006-2014 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <qse/cmn/str.h>
#include <qse/cmn/chr.h>

qse_mchar_t* qse_mbstok (
	const qse_mchar_t* s, const qse_mchar_t* delim, qse_mcstr_t* tok)
{
	const qse_mchar_t* p = s, *d;
	const qse_mchar_t* sp = QSE_NULL, * ep = QSE_NULL;
	qse_mchar_t c; 
	int delim_mode;

	/* skip preceding space characters */
	while (/* *p != QSE_MT('\0') && */ QSE_ISMSPACE(*p)) p++;

	if (delim == QSE_NULL) delim_mode = 0;
	else 
	{
		delim_mode = 1;
		for (d = delim; *d != QSE_MT('\0'); d++) 
			if (!QSE_ISMSPACE(*d)) delim_mode = 2;
	}

	if (delim_mode == 0) 
	{
		/* when QSE_NULL is given as "delim", it has an effect of 
		 * cutting preceding and trailing space characters off "s". */
		while ((c = *p) != QSE_MT('\0')) 
		{
			if (!QSE_ISMSPACE(c)) 
			{
				if (sp == QSE_NULL) sp = p;
				ep = p;
			}
			p++;
		}
	}
	else if (delim_mode == 1) 
	{
		while ((c = *p) != QSE_MT('\0')) 
		{
			if (QSE_ISMSPACE(c)) break;

			if (sp == QSE_NULL) sp = p;
			ep = p++;
		}
	}
	else /* if (delim_mode == 2) */
	{
		while ((c = *p) != QSE_MT('\0')) 
		{
			if (QSE_ISMSPACE(c)) 
			{
				p++;
				continue;
			}
			for (d = delim; *d; d++) 
			{
				if (c == *d) goto exit_loop;
			}
			if (sp == QSE_NULL) sp = p;
			ep = p++;
		}
	}

exit_loop:
	if (sp == QSE_NULL) 
	{
		tok->ptr = QSE_NULL;
		tok->len = (qse_size_t)0;
	}
	else 
	{
		tok->ptr = (qse_mchar_t*)sp;
		tok->len = ep - sp + 1;
	}
	return (c == QSE_MT('\0'))? QSE_NULL: ((qse_mchar_t*)++p);
}

qse_mchar_t* qse_mbsxtok (
	const qse_mchar_t* s, qse_size_t len,
	const qse_mchar_t* delim, qse_mcstr_t* tok)
{
	const qse_mchar_t* p = s, *d;
	const qse_mchar_t* end = s + len;	
	const qse_mchar_t* sp = QSE_NULL, * ep = QSE_NULL;
	qse_mchar_t c; 
	int delim_mode;

	/* skip preceding space characters */
	while (p < end && QSE_ISMSPACE(*p)) p++;

	if (delim == QSE_NULL) delim_mode = 0;
	else 
	{
		delim_mode = 1;
		for (d = delim; *d != QSE_MT('\0'); d++) 
			if (!QSE_ISMSPACE(*d)) delim_mode = 2;
	}

	if (delim_mode == 0) 
	{
		/* when QSE_NULL is given as "delim", it has an effect of 
		 * cutting preceding and trailing space characters off "s". */
		while (p < end) 
		{
			c = *p;
			if (!QSE_ISMSPACE(c)) 
			{
				if (sp == QSE_NULL) sp = p;
				ep = p;
			}
			p++;
		}
	}
	else if (delim_mode == 1) 
	{
		while (p < end) 
		{
			c = *p;
			if (QSE_ISMSPACE(c)) break;
			if (sp == QSE_NULL) sp = p;
			ep = p++;
		}
	}
	else 
	{ /* if (delim_mode == 2) { */
		while (p < end) 
		{
			c = *p;
			if (QSE_ISMSPACE(c)) 
			{
				p++;
				continue;
			}
			for (d = delim; *d; d++) 
			{
				if (c == *d) goto exit_loop;
			}
			if (sp == QSE_NULL) sp = p;
			ep = p++;
		}
	}

exit_loop:
	if (sp == QSE_NULL) 
	{
		tok->ptr = QSE_NULL;
		tok->len = (qse_size_t)0;
	}
	else 
	{
		tok->ptr = (qse_mchar_t*)sp;
		tok->len = ep - sp + 1;
	}

	return (p >= end)? QSE_NULL: ((qse_mchar_t*)++p);
}

qse_mchar_t* qse_mbsxntok (
	const qse_mchar_t* s, qse_size_t len,
	const qse_mchar_t* delim, qse_size_t delim_len, qse_mcstr_t* tok)
{
	const qse_mchar_t* p = s, *d;
	const qse_mchar_t* end = s + len;	
	const qse_mchar_t* sp = QSE_NULL, * ep = QSE_NULL;
	const qse_mchar_t* delim_end = delim + delim_len;
	qse_mchar_t c; 
	int delim_mode;

	/* skip preceding space characters */
	while (p < end && QSE_ISMSPACE(*p)) p++;

	if (delim == QSE_NULL) delim_mode = 0;
	else 
	{
		delim_mode = 1;
		for (d = delim; d < delim_end; d++) 
			if (!QSE_ISMSPACE(*d)) delim_mode = 2;
	}

	if (delim_mode == 0) 
	{ 
		/* when QSE_NULL is given as "delim", it has an effect of 
		 * cutting preceding and trailing space characters off "s". */
		while (p < end) 
		{
			c = *p;
			if (!QSE_ISMSPACE(c)) 
			{
				if (sp == QSE_NULL) sp = p;
				ep = p;
			}
			p++;
		}
	}
	else if (delim_mode == 1) 
	{
		while (p < end) 
		{
			c = *p;
			if (QSE_ISMSPACE(c)) break;
			if (sp == QSE_NULL) sp = p;
			ep = p++;
		}
	}
	else /* if (delim_mode == 2) */ 
	{
		while (p < end) 
		{
			c = *p;
			if (QSE_ISMSPACE(c)) 
			{
				p++;
				continue;
			}
			for (d = delim; d < delim_end; d++) 
			{
				if (c == *d) goto exit_loop;
			}
			if (sp == QSE_NULL) sp = p;
			ep = p++;
		}
	}

exit_loop:
	if (sp == QSE_NULL) 
	{
		tok->ptr = QSE_NULL;
		tok->len = (qse_size_t)0;
	}
	else 
	{
		tok->ptr = (qse_mchar_t*)sp;
		tok->len = ep - sp + 1;
	}

	return (p >= end)? QSE_NULL: ((qse_mchar_t*)++p);
}


qse_wchar_t* qse_wcstok (
	const qse_wchar_t* s, const qse_wchar_t* delim, qse_wcstr_t* tok)
{
	const qse_wchar_t* p = s, *d;
	const qse_wchar_t* sp = QSE_NULL, * ep = QSE_NULL;
	qse_wchar_t c; 
	int delim_mode;

	/* skip preceding space characters */
	while (/* *p != QSE_WT('\0') && */ QSE_ISWSPACE(*p)) p++;

	if (delim == QSE_NULL) delim_mode = 0;
	else 
	{
		delim_mode = 1;
		for (d = delim; *d != QSE_WT('\0'); d++) 
			if (!QSE_ISWSPACE(*d)) delim_mode = 2;
	}

	if (delim_mode == 0) 
	{
		/* when QSE_NULL is given as "delim", it has an effect of 
		 * cutting preceding and trailing space characters off "s". */
		while ((c = *p) != QSE_WT('\0')) 
		{
			if (!QSE_ISWSPACE(c)) 
			{
				if (sp == QSE_NULL) sp = p;
				ep = p;
			}
			p++;
		}
	}
	else if (delim_mode == 1) 
	{
		while ((c = *p) != QSE_WT('\0')) 
		{
			if (QSE_ISWSPACE(c)) break;

			if (sp == QSE_NULL) sp = p;
			ep = p++;
		}
	}
	else 
	{ /* if (delim_mode == 2) { */
		while ((c = *p) != QSE_WT('\0')) 
		{
			if (QSE_ISWSPACE(c)) 
			{
				p++;
				continue;
			}
			for (d = delim; *d; d++) 
			{
				if (c == *d) goto exit_loop;
			}
			if (sp == QSE_NULL) sp = p;
			ep = p++;
		}
	}

exit_loop:
	if (sp == QSE_NULL) 
	{
		tok->ptr = QSE_NULL;
		tok->len = (qse_size_t)0;
	}
	else 
	{
		tok->ptr = (qse_wchar_t*)sp;
		tok->len = ep - sp + 1;
	}
	return (c == QSE_WT('\0'))? QSE_NULL: ((qse_wchar_t*)++p);
}

qse_wchar_t* qse_wcsxtok (
	const qse_wchar_t* s, qse_size_t len,
	const qse_wchar_t* delim, qse_wcstr_t* tok)
{
	const qse_wchar_t* p = s, *d;
	const qse_wchar_t* end = s + len;	
	const qse_wchar_t* sp = QSE_NULL, * ep = QSE_NULL;
	qse_wchar_t c; 
	int delim_mode;

	/* skip preceding space characters */
	while (p < end && QSE_ISWSPACE(*p)) p++;

	if (delim == QSE_NULL) delim_mode = 0;
	else 
	{
		delim_mode = 1;
		for (d = delim; *d != QSE_WT('\0'); d++) 
			if (!QSE_ISWSPACE(*d)) delim_mode = 2;
	}

	if (delim_mode == 0) 
	{
		/* when QSE_NULL is given as "delim", it has an effect of 
		 * cutting preceding and trailing space characters off "s". */
		while (p < end) 
		{
			c = *p;
			if (!QSE_ISWSPACE(c)) 
			{
				if (sp == QSE_NULL) sp = p;
				ep = p;
			}
			p++;
		}
	}
	else if (delim_mode == 1) 
	{
		while (p < end) 
		{
			c = *p;
			if (QSE_ISWSPACE(c)) break;
			if (sp == QSE_NULL) sp = p;
			ep = p++;
		}
	}
	else 
	{ /* if (delim_mode == 2) { */
		while (p < end) 
		{
			c = *p;
			if (QSE_ISWSPACE(c)) 
			{
				p++;
				continue;
			}
			for (d = delim; *d; d++) 
			{
				if (c == *d) goto exit_loop;
			}
			if (sp == QSE_NULL) sp = p;
			ep = p++;
		}
	}

exit_loop:
	if (sp == QSE_NULL) 
	{
		tok->ptr = QSE_NULL;
		tok->len = (qse_size_t)0;
	}
	else 
	{
		tok->ptr = (qse_wchar_t*)sp;
		tok->len = ep - sp + 1;
	}

	return (p >= end)? QSE_NULL: ((qse_wchar_t*)++p);
}

qse_wchar_t* qse_wcsxntok (
	const qse_wchar_t* s, qse_size_t len,
	const qse_wchar_t* delim, qse_size_t delim_len, qse_wcstr_t* tok)
{
	const qse_wchar_t* p = s, *d;
	const qse_wchar_t* end = s + len;	
	const qse_wchar_t* sp = QSE_NULL, * ep = QSE_NULL;
	const qse_wchar_t* delim_end = delim + delim_len;
	qse_wchar_t c; 
	int delim_mode;

	/* skip preceding space characters */
	while (p < end && QSE_ISWSPACE(*p)) p++;

	if (delim == QSE_NULL) delim_mode = 0;
	else 
	{
		delim_mode = 1;
		for (d = delim; d < delim_end; d++) 
			if (!QSE_ISWSPACE(*d)) delim_mode = 2;
	}

	if (delim_mode == 0) 
	{ 
		/* when QSE_NULL is given as "delim", it has an effect of 
		 * cutting preceding and trailing space characters off "s". */
		while (p < end) 
		{
			c = *p;
			if (!QSE_ISWSPACE(c)) 
			{
				if (sp == QSE_NULL) sp = p;
				ep = p;
			}
			p++;
		}
	}
	else if (delim_mode == 1) 
	{
		while (p < end) 
		{
			c = *p;
			if (QSE_ISWSPACE(c)) break;
			if (sp == QSE_NULL) sp = p;
			ep = p++;
		}
	}
	else /* if (delim_mode == 2) */ 
	{
		while (p < end) 
		{
			c = *p;
			if (QSE_ISWSPACE(c)) 
			{
				p++;
				continue;
			}
			for (d = delim; d < delim_end; d++) 
			{
				if (c == *d) goto exit_loop;
			}
			if (sp == QSE_NULL) sp = p;
			ep = p++;
		}
	}

exit_loop:
	if (sp == QSE_NULL) 
	{
		tok->ptr = QSE_NULL;
		tok->len = (qse_size_t)0;
	}
	else 
	{
		tok->ptr = (qse_wchar_t*)sp;
		tok->len = ep - sp + 1;
	}

	return (p >= end)? QSE_NULL: ((qse_wchar_t*)++p);
}

