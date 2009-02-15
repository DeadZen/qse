/*
 * $Id: str_bas.c 376 2008-09-24 07:18:50Z baconevi $
 *
   Copyright 2006-2009 Chung, Hyung-Hwan.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#include <qse/cmn/str.h>
#include <stdarg.h>
#include "chr.h"
#include "mem.h"

qse_size_t qse_strlen (const qse_char_t* str)
{
	const qse_char_t* p = str;
	while (*p != QSE_T('\0')) p++;
	return p - str;
}

qse_size_t qse_strbytes (const qse_char_t* str)
{
	const qse_char_t* p = str;
	while (*p != QSE_T('\0')) p++;
	return (p - str) * QSE_SIZEOF(qse_char_t);
}

qse_size_t qse_strcpy (qse_char_t* buf, const qse_char_t* str)
{
	qse_char_t* org = buf;
	while ((*buf++ = *str++) != QSE_T('\0'));
	return buf - org - 1;
}

qse_size_t qse_strxcpy (
	qse_char_t* buf, qse_size_t bsz, const qse_char_t* str)
{
	qse_char_t* p, * p2;

	p = buf; p2 = buf + bsz - 1;

	while (p < p2) 
	{
		if (*str == QSE_T('\0')) break;
		*p++ = *str++;
	}

	if (bsz > 0) *p = QSE_T('\0');
	return p - buf;
}

qse_size_t qse_strncpy (
	qse_char_t* buf, const qse_char_t* str, qse_size_t len)
{
	/*
	const qse_char_t* end = str + len;
	while (str < end) *buf++ = *str++;
	*buf = QSE_T('\0');
	return len;
	*/

	if (len > 0)
	{
		qse_size_t n = (len-1) >> 3; /* (len-1) / 8 */

		switch (len & 7) /* len % 8 */
		{
		repeat:
			case 0: *buf++ = *str++;
			case 7: *buf++ = *str++;
			case 6: *buf++ = *str++;
			case 5: *buf++ = *str++;
			case 4: *buf++ = *str++;
			case 3: *buf++ = *str++;
			case 2: *buf++ = *str++;
			case 1: *buf++ = *str++;
			        if (n <= 0) break;
			        n--;
			        goto repeat;
		}
	}

	*buf = QSE_T('\0');
	return len;
}

qse_size_t qse_strxncpy (
	qse_char_t* buf, qse_size_t bsz, const qse_char_t* str, qse_size_t len)
{
	qse_size_t n;

	if (bsz <= 0) return 0;
	if ((n = bsz - 1) > len) n = len;
	QSE_MEMCPY (buf, str, n * QSE_SIZEOF(qse_char_t));
	buf[n] = QSE_T('\0');

	return n;
}

qse_size_t qse_strfcpy (qse_char_t* buf, const qse_char_t* fmt, ...)
{
	qse_char_t* b = buf;
	const qse_char_t* f = fmt;

	while (*f != QSE_T('\0'))
	{
		if (*f == QSE_T('$'))
		{
			if (f[1] == QSE_T('{') && 
			    (f[2] >= QSE_T('0') && f[2] <= QSE_T('9')))
			{
				va_list ap;
				const qse_char_t* tmp;
				qse_size_t idx = 0;

				tmp = f;
				f += 2;

				do idx = idx * 10 + (*f++ - QSE_T('0'));
				while (*f >= QSE_T('0') && *f <= QSE_T('9'));
	
				if (*f != QSE_T('}'))
				{
					f = tmp;
					goto normal;
				}

				f++;
				
				/* TODO: some optimization in getting the argument */
				va_start (ap, fmt);

				do tmp = va_arg(ap,const qse_char_t*);
				while (idx-- > 0);

				va_end (ap);

				while (*tmp != QSE_T('\0')) *b++ = *tmp++;
				continue;
			}
			else if (f[1] == QSE_T('$')) f++;
		}

	normal:
		*b++ = *f++;
	}

	*b = QSE_T('\0');
	return b - buf;
}

qse_size_t qse_strxfcpy (
	qse_char_t* buf, qse_size_t bsz, const qse_char_t* fmt, ...)
{
	qse_char_t* b = buf;
	qse_char_t* end = buf + bsz - 1;
	const qse_char_t* f = fmt;

	if (bsz <= 0) return 0;

	while (*f != QSE_T('\0'))
	{
		if (*f == QSE_T('$'))
		{
			if (f[1] == QSE_T('{') && 
			    (f[2] >= QSE_T('0') && f[2] <= QSE_T('9')))
			{
				va_list ap;
				const qse_char_t* tmp;
				qse_size_t idx = 0;

				tmp = f;
				f += 2;

				do idx = idx * 10 + (*f++ - QSE_T('0'));
				while (*f >= QSE_T('0') && *f <= QSE_T('9'));
	
				if (*f != QSE_T('}'))
				{
					f = tmp;
					goto normal;
				}

				f++;
				
				/* TODO: some optimization in getting the argument */
				va_start (ap, fmt);

				do tmp = va_arg(ap,const qse_char_t*);
				while (idx-- > 0);

				va_end (ap);

				while (*tmp != QSE_T('\0')) 
				{
					if (b >= end) goto fini;
					*b++ = *tmp++;
				}
				continue;
			}
			else if (f[1] == QSE_T('$')) f++;
		}

	normal:
		if (b >= end) break;
		*b++ = *f++;
	}

fini:
	*b = QSE_T('\0');
	return b - buf;
}

qse_size_t qse_strxcat (qse_char_t* buf, qse_size_t bsz, const qse_char_t* str)
{
	qse_char_t* p, * p2;
	qse_size_t blen;

	blen = qse_strlen(buf);
	if (blen >= bsz) return blen; /* something wrong */

	p = buf + blen;
	p2 = buf + bsz - 1;

	while (p < p2) 
	{
		if (*str == QSE_T('\0')) break;
		*p++ = *str++;
	}

	if (bsz > 0) *p = QSE_T('\0');
	return p - buf;
}


qse_size_t qse_strxncat (
	qse_char_t* buf, qse_size_t bsz, const qse_char_t* str, qse_size_t len)
{
	qse_char_t* p, * p2;
	const qse_char_t* end;
	qse_size_t blen;

	blen = qse_strlen(buf);
	if (blen >= bsz) return blen; /* something wrong */

	p = buf + blen;
	p2 = buf + bsz - 1;

	end = str + len;

	while (p < p2) 
	{
		if (str >= end) break;
		*p++ = *str++;
	}

	if (bsz > 0) *p = QSE_T('\0');
	return p - buf;
}

int qse_strcmp (const qse_char_t* s1, const qse_char_t* s2)
{
	while (*s1 == *s2) 
	{
		if (*s1 == QSE_T('\0')) return 0;
		s1++, s2++;
	}

	return (*s1 > *s2)? 1: -1;
}

int qse_strxcmp (const qse_char_t* s1, qse_size_t len, const qse_char_t* s2)
{
	const qse_char_t* end = s1 + len;
	while (s1 < end && *s2 != QSE_T('\0') && *s1 == *s2) s1++, s2++;
	if (s1 == end && *s2 == QSE_T('\0')) return 0;
	if (*s1 == *s2) return (s1 < end)? 1: -1;
	return (*s1 > *s2)? 1: -1;
}

int qse_strxncmp (
	const qse_char_t* s1, qse_size_t len1, 
	const qse_char_t* s2, qse_size_t len2)
{
	qse_char_t c1, c2;
	const qse_char_t* end1 = s1 + len1;
	const qse_char_t* end2 = s2 + len2;

	while (s1 < end1)
	{
		c1 = *s1;
		if (s2 < end2) 
		{
			c2 = *s2;
			if (c1 > c2) return 1;
			if (c1 < c2) return -1;
		}
		else return 1;
		s1++; s2++;
	}

	return (s2 < end2)? -1: 0;
}

int qse_strcasecmp (
	const qse_char_t* s1, const qse_char_t* s2, qse_ccls_t* ccls)
{
	while (QSE_CCLS_TOUPPER(ccls,*s1) == QSE_CCLS_TOUPPER(ccls,*s2)) 
	{
		if (*s1 == QSE_C('\0')) return 0;
		s1++, s2++;
	}

	return (QSE_CCLS_TOUPPER(ccls,*s1) > QSE_CCLS_TOUPPER(ccls,*s2))? 1: -1;
}

int qse_strxncasecmp (
	const qse_char_t* s1, qse_size_t len1, 
	const qse_char_t* s2, qse_size_t len2, qse_ccls_t* ccls)
{
	qse_char_t c1, c2;
	const qse_char_t* end1 = s1 + len1;
	const qse_char_t* end2 = s2 + len2;

	while (s1 < end1)
	{
		c1 = QSE_CCLS_TOUPPER (ccls, *s1); 
		if (s2 < end2) 
		{
			c2 = QSE_CCLS_TOUPPER (ccls, *s2);
			if (c1 > c2) return 1;
			if (c1 < c2) return -1;
		}
		else return 1;
		s1++; s2++;
	}

	return (s2 < end2)? -1: 0;
}

qse_char_t* qse_strdup (const qse_char_t* str, qse_mmgr_t* mmgr)
{
	qse_char_t* tmp;

	tmp = (qse_char_t*) QSE_MALLOC (
		mmgr, (qse_strlen(str)+1)*QSE_SIZEOF(qse_char_t));
	if (tmp == QSE_NULL) return QSE_NULL;

	qse_strcpy (tmp, str);
	return tmp;
}

qse_char_t* qse_strxdup (
	const qse_char_t* str, qse_size_t len, qse_mmgr_t* mmgr)
{
	qse_char_t* tmp;

	tmp = (qse_char_t*) QSE_MALLOC (
		mmgr, (len+1)*QSE_SIZEOF(qse_char_t));
	if (tmp == QSE_NULL) return QSE_NULL;

	qse_strncpy (tmp, str, len);
	return tmp;
}

qse_char_t* qse_strxdup2 (
	const qse_char_t* str1, qse_size_t len1,
	const qse_char_t* str2, qse_size_t len2, qse_mmgr_t* mmgr)
{
	qse_char_t* tmp;

	tmp = (qse_char_t*) QSE_MALLOC (
		mmgr, (len1+len2+1) * QSE_SIZEOF(qse_char_t));
	if (tmp == QSE_NULL) return QSE_NULL;

	qse_strncpy (tmp, str1, len1);
	qse_strncpy (tmp + len1, str2, len2);
	return tmp;
}

qse_char_t* qse_strstr (const qse_char_t* str, const qse_char_t* sub)
{
	const qse_char_t* x, * y;

	y = sub;
	if (*y == QSE_T('\0')) return (qse_char_t*)str;

	while (*str != QSE_T('\0')) 
	{
		if (*str != *y) 
		{
			str++;
			continue;
		}

		x = str;
		while (1)
		{
			if (*y == QSE_T('\0')) return (qse_char_t*)str;
			if (*x++ != *y++) break;
		}

		y = sub;
		str++;
	}

	return QSE_NULL;
}

qse_char_t* qse_strxstr (
	const qse_char_t* str, qse_size_t size, const qse_char_t* sub)
{
	return qse_strxnstr (str, size, sub, qse_strlen(sub));
}

qse_char_t* qse_strxnstr (
	const qse_char_t* str, qse_size_t strsz, 
	const qse_char_t* sub, qse_size_t subsz)
{
	const qse_char_t* end, * subp;

	if (subsz == 0) return (qse_char_t*)str;
	if (strsz < subsz) return QSE_NULL;
	
	end = str + strsz - subsz;
	subp = sub + subsz;

	while (str <= end) 
	{
		const qse_char_t* x = str;
		const qse_char_t* y = sub;

		while (1)
		{
			if (y >= subp) return (qse_char_t*)str;
			if (*x != *y) break;
			x++; y++;
		}	

		str++;
	}
		
	return QSE_NULL;
}

qse_char_t* qse_strchr (const qse_char_t* str, qse_cint_t c)
{
	while (*str != QSE_T('\0')) 
	{
		if (*str == c) return (qse_char_t*)str;
		str++;
	}
	return QSE_NULL;
}

qse_char_t* qse_strxchr (const qse_char_t* str, qse_size_t len, qse_cint_t c)
{
	const qse_char_t* end = str + len;

	while (str < end) 
	{
		if (*str == c) return (qse_char_t*)str;
		str++;
	}

	return QSE_NULL;
}

qse_char_t* qse_strrchr (const qse_char_t* str, qse_cint_t c)
{
	const qse_char_t* end = str;

	while (*end != QSE_T('\0')) end++;

	while (end > str) 
	{
		if (*--end == c) return (qse_char_t*)end;
	}

	return QSE_NULL;
}

qse_char_t* qse_strxrchr (const qse_char_t* str, qse_size_t len, qse_cint_t c)
{
	const qse_char_t* end = str + len;

	while (end > str) 
	{
		if (*--end == c) return (qse_char_t*)end;
	}

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
