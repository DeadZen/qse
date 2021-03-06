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

/* Copyright (c) 1996-1999 by Internet Software Consortium
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL INTERNET SOFTWARE
 * CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#include <qse/cmn/ipad.h>
#include <qse/cmn/hton.h>
#include <qse/cmn/str.h>
#include <qse/cmn/fmt.h>
#include "mem.h"

#if 0
const qse_ip4ad_t qse_ip4ad_any =
{
	0 /* 0.0.0.0 */
};

const qse_ip4ad_t qse_ip4ad_loopback =
{
#if defined(QSE_ENDIAN_BIG)
	0x7F000001u /* 127.0.0.1 */
#elif defined(QSE_ENDIAN_LITTLE)
	0x0100007Fu
#else
#	error Unknown endian
#endif
};

const qse_ip6ad_t qse_ip6ad_any =
{
	{ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } /* :: */
};

const qse_ip6ad_t qse_ip6ad_loopback =
{
	{ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 } /* ::1 */
};
#endif

static int str_to_ip4ad (int mbs, const void* str, qse_size_t len, qse_ip4ad_t* ipad)
{
	const void* end;
	int dots = 0, digits = 0;
	qse_uint32_t acc = 0, addr = 0;	
	qse_wchar_t c;

	end = (mbs? (const void*)((const qse_mchar_t*)str + len):
	            (const void*)((const qse_wchar_t*)str + len));

	do
	{
		if (str >= end)
		{
			if (dots < 3 || digits == 0) return -1;
			addr = (addr << 8) | acc;
			break;
		}

		if (mbs)
		{
			c = *(const qse_mchar_t*)str;
			str = (const qse_mchar_t*)str + 1;
		}
		else
		{
			c = *(const qse_wchar_t*)str;
			str = (const qse_wchar_t*)str + 1;
		}

		if (c >= QSE_WT('0') && c <= QSE_WT('9')) 
		{
			if (digits > 0 && acc == 0) return -1;
			acc = acc * 10 + (c - QSE_T('0'));
			if (acc > 255) return -1;
			digits++;
		}
		else if (c == QSE_WT('.')) 
		{
			if (dots >= 3 || digits == 0) return -1;
			addr = (addr << 8) | acc;
			dots++; acc = 0; digits = 0;
		}
		else return -1;
	}
	while (1);

	if (ipad != QSE_NULL) ipad->value = qse_hton32(addr);
	return 0;
}

int qse_mbstoip4ad (const qse_mchar_t* str, qse_ip4ad_t* ipad)
{
	return str_to_ip4ad (1, str, qse_mbslen(str), ipad);
}

int qse_wcstoip4ad (const qse_wchar_t* str, qse_ip4ad_t* ipad)
{
	return str_to_ip4ad (0, str, qse_wcslen(str), ipad);
}

int qse_mbsntoip4ad (const qse_mchar_t* str, qse_size_t len, qse_ip4ad_t* ipad)
{
	return str_to_ip4ad (1, str, len, ipad);
}

int qse_wcsntoip4ad (const qse_wchar_t* str, qse_size_t len, qse_ip4ad_t* ipad)
{
	return str_to_ip4ad (0, str, len, ipad);
}

#define __BTOA(type_t,b,p,end) \
	do { \
		type_t* sp = p; \
		do {  \
			if (p >= end) { \
				if (p == sp) break; \
				if (p - sp > 1) p[-2] = p[-1]; \
				p[-1] = (b % 10) + '0'; \
			} \
			else *p++ = (b % 10) + '0'; \
			b /= 10; \
		} while (b > 0); \
		if (p - sp > 1) { \
			type_t t = sp[0]; \
			sp[0] = p[-1]; \
			p[-1] = t; \
		} \
	} while (0);

#define __ADDDOT(p, end) \
	do { \
		if (p >= end) break; \
		*p++ = '.'; \
	} while (0)

qse_size_t qse_ip4adtombs (
	const qse_ip4ad_t* ipad, qse_mchar_t* buf, qse_size_t size)
{
	qse_byte_t b;
	qse_mchar_t* p, * end;
	qse_uint32_t ip;

	if (size <= 0) return 0;

	ip = ipad->value;

	p = buf;
	end = buf + size - 1;

#if defined(QSE_ENDIAN_BIG)
	b = (ip >> 24) & 0xFF; __BTOA (qse_mchar_t, b, p, end); __ADDDOT (p, end);
	b = (ip >> 16) & 0xFF; __BTOA (qse_mchar_t, b, p, end); __ADDDOT (p, end);
	b = (ip >>  8) & 0xFF; __BTOA (qse_mchar_t, b, p, end); __ADDDOT (p, end);
	b = (ip >>  0) & 0xFF; __BTOA (qse_mchar_t, b, p, end);
#elif defined(QSE_ENDIAN_LITTLE)
	b = (ip >>  0) & 0xFF; __BTOA (qse_mchar_t, b, p, end); __ADDDOT (p, end);
	b = (ip >>  8) & 0xFF; __BTOA (qse_mchar_t, b, p, end); __ADDDOT (p, end);
	b = (ip >> 16) & 0xFF; __BTOA (qse_mchar_t, b, p, end); __ADDDOT (p, end);
	b = (ip >> 24) & 0xFF; __BTOA (qse_mchar_t, b, p, end);
#else
#	error Unknown Endian
#endif

	*p = QSE_MT('\0');
	return p - buf;
}

qse_size_t qse_ip4adtowcs (
	const qse_ip4ad_t* ipad, qse_wchar_t* buf, qse_size_t size)
{
	qse_byte_t b;
	qse_wchar_t* p, * end;
	qse_uint32_t ip;

	if (size <= 0) return 0;

	ip = ipad->value;

	p = buf;
	end = buf + size - 1;

#if defined(QSE_ENDIAN_BIG)
	b = (ip >> 24) & 0xFF; __BTOA (qse_wchar_t, b, p, end); __ADDDOT (p, end);
	b = (ip >> 16) & 0xFF; __BTOA (qse_wchar_t, b, p, end); __ADDDOT (p, end);
	b = (ip >>  8) & 0xFF; __BTOA (qse_wchar_t, b, p, end); __ADDDOT (p, end);
	b = (ip >>  0) & 0xFF; __BTOA (qse_wchar_t, b, p, end);
#elif defined(QSE_ENDIAN_LITTLE)
	b = (ip >>  0) & 0xFF; __BTOA (qse_wchar_t, b, p, end); __ADDDOT (p, end);
	b = (ip >>  8) & 0xFF; __BTOA (qse_wchar_t, b, p, end); __ADDDOT (p, end);
	b = (ip >> 16) & 0xFF; __BTOA (qse_wchar_t, b, p, end); __ADDDOT (p, end);
	b = (ip >> 24) & 0xFF; __BTOA (qse_wchar_t, b, p, end);
#else
#	error Unknown Endian
#endif

	*p = QSE_WT('\0');
	return p - buf;
}

int qse_mbstoip6ad (const qse_mchar_t* src, qse_ip6ad_t* ipad)
{
	return qse_mbsntoip6ad (src, qse_mbslen(src), ipad);
}

int qse_mbsntoip6ad (const qse_mchar_t* src, qse_size_t len, qse_ip6ad_t* ipad)
{
	qse_ip6ad_t tmp;
	qse_uint8_t* tp, * endp, * colonp;
	const qse_mchar_t* curtok;
	qse_mchar_t ch;
	int saw_xdigit;
	unsigned int val;
	const qse_mchar_t* src_end;

	src_end = src + len;

	QSE_MEMSET (&tmp, 0, QSE_SIZEOF(tmp));
	tp = &tmp.value[0];
	endp = &tmp.value[QSE_COUNTOF(tmp.value)];
	colonp = QSE_NULL;

	/* Leading :: requires some special handling. */
	if (src < src_end && *src == QSE_MT(':'))
	{
		src++;
		if (src >= src_end || *src != QSE_MT(':')) return -1;
	}

	curtok = src;
	saw_xdigit = 0;
	val = 0;

	while (src < src_end)
	{
		int v1;

		ch = *src++;

		if (ch >= QSE_MT('0') && ch <= QSE_MT('9'))
			v1 = ch - QSE_MT('0');
		else if (ch >= QSE_MT('A') && ch <= QSE_MT('F'))
			v1 = ch - QSE_MT('A') + 10;
		else if (ch >= QSE_MT('a') && ch <= QSE_MT('f'))
			v1 = ch - QSE_MT('a') + 10;
		else v1 = -1;
		if (v1 >= 0)
		{
			val <<= 4;
			val |= v1;
			if (val > 0xffff) return -1;
			saw_xdigit = 1;
			continue;
		}

		if (ch == QSE_MT(':')) 
		{
			curtok = src;
			if (!saw_xdigit) 
			{
				if (colonp) return -1;
				colonp = tp;
				continue;
			}
			else if (src >= src_end)
			{
				/* a colon can't be the last character */
				return -1;
			}

			*tp++ = (qse_uint8_t)(val >> 8) & 0xff;
			*tp++ = (qse_uint8_t)val & 0xff;
			saw_xdigit = 0;
			val = 0;
			continue;
		}

		if (ch == QSE_MT('.') && ((tp + QSE_SIZEOF(qse_ip4ad_t)) <= endp) &&
		    qse_mbsntoip4ad(curtok, src_end - curtok, (qse_ip4ad_t*)tp) == 0) 
		{
			tp += QSE_SIZEOF(qse_ip4ad_t);
			saw_xdigit = 0;
			break; 
		}

		return -1;
	}

	if (saw_xdigit) 
	{
		if (tp + QSE_SIZEOF(qse_uint16_t) > endp) return -1;
		*tp++ = (qse_uint8_t)(val >> 8) & 0xff;
		*tp++ = (qse_uint8_t)val & 0xff;
	}
	if (colonp != QSE_NULL) 
	{
		/*
		 * Since some memmove()'s erroneously fail to handle
		 * overlapping regions, we'll do the shift by hand.
		 */
		qse_size_t n = tp - colonp;
		qse_size_t i;
 
		for (i = 1; i <= n; i++) 
		{
			endp[-i] = colonp[n - i];
			colonp[n - i] = 0;
		}
		tp = endp;
	}

	if (tp != endp) return -1;

	*ipad = tmp;
	return 0;
}


int qse_wcstoip6ad (const qse_wchar_t* src, qse_ip6ad_t* ipad)
{
	return qse_wcsntoip6ad (src, qse_wcslen(src), ipad);
}

int qse_wcsntoip6ad (const qse_wchar_t* src, qse_size_t len, qse_ip6ad_t* ipad)
{
	qse_ip6ad_t tmp;
	qse_uint8_t* tp, * endp, * colonp;
	const qse_wchar_t* curtok;
	qse_wchar_t ch;
	int saw_xdigit;
	unsigned int val;
	const qse_wchar_t* src_end;

	src_end = src + len;

	QSE_MEMSET (&tmp, 0, QSE_SIZEOF(tmp));
	tp = &tmp.value[0];
	endp = &tmp.value[QSE_COUNTOF(tmp.value)];
	colonp = QSE_NULL;

	/* Leading :: requires some special handling. */
	if (src < src_end && *src == QSE_WT(':'))
	{
		src++;
		if (src >= src_end || *src != QSE_WT(':')) return -1;
	}

	curtok = src;
	saw_xdigit = 0;
	val = 0;

	while (src < src_end)
	{
		int v1;

		ch = *src++;

		if (ch >= QSE_WT('0') && ch <= QSE_WT('9'))
			v1 = ch - QSE_WT('0');
		else if (ch >= QSE_WT('A') && ch <= QSE_WT('F'))
			v1 = ch - QSE_WT('A') + 10;
		else if (ch >= QSE_WT('a') && ch <= QSE_WT('f'))
			v1 = ch - QSE_WT('a') + 10;
		else v1 = -1;
		if (v1 >= 0)
		{
			val <<= 4;
			val |= v1;
			if (val > 0xffff) return -1;
			saw_xdigit = 1;
			continue;
		}

		if (ch == QSE_WT(':')) 
		{
			curtok = src;
			if (!saw_xdigit) 
			{
				if (colonp) return -1;
				colonp = tp;
				continue;
			}
			else if (src >= src_end)
			{
				/* a colon can't be the last character */
				return -1;
			}

			*tp++ = (qse_uint8_t)(val >> 8) & 0xff;
			*tp++ = (qse_uint8_t)val & 0xff;
			saw_xdigit = 0;
			val = 0;
			continue;
		}

		if (ch == QSE_WT('.') && ((tp + QSE_SIZEOF(qse_ip4ad_t)) <= endp) &&
		    qse_wcsntoip4ad(curtok, src_end - curtok, (qse_ip4ad_t*)tp) == 0) 
		{
			tp += QSE_SIZEOF(qse_ip4ad_t);
			saw_xdigit = 0;
			break; 
		}

		return -1;
	}

	if (saw_xdigit) 
	{
		if (tp + QSE_SIZEOF(qse_uint16_t) > endp) return -1;
		*tp++ = (qse_uint8_t)(val >> 8) & 0xff;
		*tp++ = (qse_uint8_t)val & 0xff;
	}
	if (colonp != QSE_NULL) 
	{
		/*
		 * Since some memmove()'s erroneously fail to handle
		 * overlapping regions, we'll do the shift by hand.
		 */
		qse_size_t n = tp - colonp;
		qse_size_t i;
 
		for (i = 1; i <= n; i++) 
		{
			endp[-i] = colonp[n - i];
			colonp[n - i] = 0;
		}
		tp = endp;
	}

	if (tp != endp) return -1;

	*ipad = tmp;
	return 0;
}

qse_size_t qse_ip6adtombs (
	const qse_ip6ad_t* ipad, qse_mchar_t* buf, qse_size_t size)
{
	/*
	 * Note that int32_t and int16_t need only be "at least" large enough
	 * to contain a value of the specified size.  On some systems, like
	 * Crays, there is no such thing as an integer variable with 16 bits.
	 * Keep this in mind if you think this function should have been coded
	 * to use pointer overlays.  All the world's not a VAX.
	 */

#define IP6ADDR_NWORDS (QSE_SIZEOF(ipad->value) / QSE_SIZEOF(qse_uint16_t))

	qse_mchar_t tmp[QSE_COUNTOF(QSE_MT("ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255"))], *tp;
	struct { int base, len; } best, cur;
	qse_uint16_t words[IP6ADDR_NWORDS];
	int i;

	if (size <= 0) return 0;

	/*
	 * Preprocess:
	 *	Copy the input (bytewise) array into a wordwise array.
	 *	Find the longest run of 0x00's in src[] for :: shorthanding.
	 */
	QSE_MEMSET (words, 0, QSE_SIZEOF(words));
	for (i = 0; i < QSE_SIZEOF(ipad->value); i++)
		words[i / 2] |= (ipad->value[i] << ((1 - (i % 2)) << 3));
	best.base = -1;
	cur.base = -1;

	for (i = 0; i < IP6ADDR_NWORDS; i++) 
	{
		if (words[i] == 0) 
		{
			if (cur.base == -1)
			{
				cur.base = i;
				cur.len = 1;
			}
			else
			{
				cur.len++;
			}
		}
		else 
		{
			if (cur.base != -1) 
			{
				if (best.base == -1 || cur.len > best.len) best = cur;
				cur.base = -1;
			}
		}
	}
	if (cur.base != -1) 
	{
		if (best.base == -1 || cur.len > best.len) best = cur;
	}
	if (best.base != -1 && best.len < 2) best.base = -1;

	/*
	 * Format the result.
	 */
	tp = tmp;
	for (i = 0; i < IP6ADDR_NWORDS; i++) 
	{
		/* Are we inside the best run of 0x00's? */
		if (best.base != -1 && i >= best.base &&
		    i < (best.base + best.len)) 
		{
			if (i == best.base) *tp++ = QSE_MT(':');
			continue;
		}

		/* Are we following an initial run of 0x00s or any real hex? */
		if (i != 0) *tp++ = QSE_MT(':');

		/* Is this address an encapsulated IPv4? ipv4-compatible or ipv4-mapped */
		if (i == 6 && best.base == 0 &&
		    (best.len == 6 || (best.len == 5 && words[5] == 0xffff))) 
		{
			qse_ip4ad_t ip4ad;
			QSE_MEMCPY (&ip4ad.value, ipad->value+12, QSE_SIZEOF(ip4ad.value));
			tp += qse_ip4adtombs (&ip4ad, tp, QSE_COUNTOF(tmp) - (tp - tmp));
			break;
		}

		tp += qse_fmtuintmaxtombs (
			tp, QSE_COUNTOF(tmp) - (tp - tmp), 
			words[i], 16, 0, QSE_WT('\0'), QSE_NULL);
	}

	/* Was it a trailing run of 0x00's? */
	if (best.base != -1 && 
	    (best.base + best.len) == IP6ADDR_NWORDS) *tp++ = QSE_MT(':');
	*tp++ = QSE_MT('\0');

	return qse_mbsxcpy (buf, size, tmp);

#undef IP6ADDR_NWORDS
}

qse_size_t qse_ip6adtowcs (
	const qse_ip6ad_t* ipad, qse_wchar_t* buf, qse_size_t size)
{
	/*
	 * Note that int32_t and int16_t need only be "at least" large enough
	 * to contain a value of the specified size.  On some systems, like
	 * Crays, there is no such thing as an integer variable with 16 bits.
	 * Keep this in mind if you think this function should have been coded
	 * to use pointer overlays.  All the world's not a VAX.
	 */

#define IP6ADDR_NWORDS (QSE_SIZEOF(ipad->value) / QSE_SIZEOF(qse_uint16_t))

	qse_wchar_t tmp[QSE_COUNTOF(QSE_MT("ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255"))], *tp;
	struct { int base, len; } best, cur;
	qse_uint16_t words[IP6ADDR_NWORDS];
	int i;

	if (size <= 0) return 0;

	/*
	 * Preprocess:
	 *	Copy the input (bytewise) array into a wordwise array.
	 *	Find the longest run of 0x00's in src[] for :: shorthanding.
	 */
	QSE_MEMSET (words, 0, QSE_SIZEOF(words));
	for (i = 0; i < QSE_SIZEOF(ipad->value); i++)
		words[i / 2] |= (ipad->value[i] << ((1 - (i % 2)) << 3));
	best.base = -1;
	cur.base = -1;

	for (i = 0; i < IP6ADDR_NWORDS; i++) 
	{
		if (words[i] == 0) 
		{
			if (cur.base == -1)
			{
				cur.base = i;
				cur.len = 1;
			}
			else
			{
				cur.len++;
			}
		}
		else 
		{
			if (cur.base != -1) 
			{
				if (best.base == -1 || cur.len > best.len) best = cur;
				cur.base = -1;
			}
		}
	}
	if (cur.base != -1) 
	{
		if (best.base == -1 || cur.len > best.len) best = cur;
	}
	if (best.base != -1 && best.len < 2) best.base = -1;

	/*
	 * Format the result.
	 */
	tp = tmp;
	for (i = 0; i < IP6ADDR_NWORDS; i++) 
	{
		/* Are we inside the best run of 0x00's? */
		if (best.base != -1 && i >= best.base &&
		    i < (best.base + best.len)) 
		{
			if (i == best.base) *tp++ = QSE_MT(':');
			continue;
		}

		/* Are we following an initial run of 0x00s or any real hex? */
		if (i != 0) *tp++ = QSE_MT(':');

		/* Is this address an encapsulated IPv4? ipv4-compatible or ipv4-mapped */
		if (i == 6 && best.base == 0 &&
		    (best.len == 6 || (best.len == 5 && words[5] == 0xffff))) 
		{
			qse_ip4ad_t ip4ad;
			QSE_MEMCPY (&ip4ad.value, ipad->value+12, QSE_SIZEOF(ip4ad.value));
			tp += qse_ip4adtowcs (&ip4ad, tp, QSE_COUNTOF(tmp) - (tp - tmp));
			break;
		}

		tp += qse_fmtuintmaxtowcs (
			tp, QSE_COUNTOF(tmp) - (tp - tmp), 
			words[i], 16, 0, QSE_WT('\0'), QSE_NULL);
	}

	/* Was it a trailing run of 0x00's? */
	if (best.base != -1 && 
	    (best.base + best.len) == IP6ADDR_NWORDS) *tp++ = QSE_MT(':');
	*tp++ = QSE_MT('\0');

	return qse_wcsxcpy (buf, size, tmp);

#undef IP6ADDR_NWORDS
}

int qse_prefixtoip4ad (int prefix, qse_ip4ad_t* ipad)
{
	static qse_uint32_t tab[] =
	{
		0x00000000,
		0x80000000, 0xC0000000, 0xE0000000, 0xF0000000,
		0xF8000000, 0xFC000000, 0xFE000000, 0xFF000000,
		0xFF800000, 0xFFC00000, 0xFFE00000, 0xFFF00000,
		0xFFF80000, 0xFFFC0000, 0xFFFE0000, 0xFFFF0000,
		0xFFFF8000, 0xFFFFC000, 0xFFFFE000, 0xFFFFF000,
		0xFFFFF800, 0xFFFFFC00, 0xFFFFFE00, 0xFFFFFF00,
		0xFFFFFF80, 0xFFFFFFC0, 0xFFFFFFE0, 0xFFFFFFF0,
		0xFFFFFFF8, 0xFFFFFFFC, 0xFFFFFFFE, 0xFFFFFFFF
	};

	if (prefix < 0 || prefix > QSE_SIZEOF(*ipad) * 8) return -1;
	ipad->value = qse_hton32(tab[prefix]);
	return 0;

	/*
	int p, k;
	qse_uint32_t mask = 0;

	for (k = 24; prefix > 0; k -= 8) 
	{
		p = (prefix >= 8)? 0: (8 - prefix);
		mask |= ((0xFF << p) & 0xFF) << k;
		prefix -= 8;
	}

	ipad->value = qse_hton32(mask);
	return 0;
	*/
}

int qse_prefixtoip6ad (int prefix, qse_ip6ad_t* ipad)
{
	int i;

	if (prefix < 0 || prefix > QSE_SIZEOF(*ipad) * 8) return -1;	

	QSE_MEMSET (ipad, 0, QSE_SIZEOF(*ipad));
	for (i = 0; ; i++)
	{
		if (prefix > 8) 
		{
			ipad->value[i] = 0xFF;
			prefix -= 8;
		}
		else
		{
			ipad->value[i] = 0xFF << (8 - prefix);
			break;
		}

	}

	return 0;
}
