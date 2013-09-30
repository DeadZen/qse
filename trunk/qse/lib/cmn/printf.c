
#include <qse/cmn/chr.h>
#include <qse/cmn/str.h>
#include <qse/cmn/mbwc.h>
#include <stdarg.h>

/* number of bits in a byte */
#define NBBY    8               

/* Max number conversion buffer length: 
 * qse_intmax_t in base 2, plus NUL byte. */
#define MAXNBUF (QSE_SIZEOF(qse_intmax_t) * NBBY + 1)

enum
{
	LF_C = (1 << 0),
	LF_H = (1 << 1),
	LF_J = (1 << 2),
	LF_L = (1 << 3),
	LF_Q = (1 << 4),
	LF_T = (1 << 5),
	LF_Z = (1 << 6)
};

static struct
{
	qse_uint8_t flag; /* for single occurrence */
	qse_uint8_t dflag; /* for double occurrence */
} lm_tab[26] = 
{
	{ 0,    0 }, /* a */
	{ 0,    0 }, /* b */
	{ 0,    0 }, /* c */
	{ 0,    0 }, /* d */
	{ 0,    0 }, /* e */
	{ 0,    0 }, /* f */
	{ 0,    0 }, /* g */
	{ LF_H, LF_C }, /* h */
	{ 0,    0 }, /* i */
	{ LF_J, 0 }, /* j */
	{ 0,    0 }, /* k */
	{ LF_L, LF_Q }, /* l */
	{ 0,    0 }, /* m */
	{ 0,    0 }, /* n */
	{ 0,    0 }, /* o */
	{ 0,    0 }, /* p */
	{ LF_Q, 0 }, /* q */
	{ 0,    0 }, /* r */
	{ 0,    0 }, /* s */
	{ LF_T, 0 }, /* t */
	{ 0,    0 }, /* u */
	{ 0,    0 }, /* v */
	{ 0,    0 }, /* w */
	{ 0,    0 }, /* z */
	{ 0,    0 }, /* y */
	{ LF_Z, 0 }, /* z */
};

#include <stdio.h> /* TODO: remove dependency on this */

static void xputwchar (qse_wchar_t c, void *arg)
{
	qse_cmgr_t* cmgr;
	qse_mchar_t mbsbuf[QSE_MBLEN_MAX + 1];
	qse_size_t n;

	cmgr = qse_getdflcmgr ();
	n = cmgr->wctomb (c, mbsbuf, QSE_COUNTOF(mbsbuf));
	if (n <= 0 || n > QSE_COUNTOF(mbsbuf))
	{
		putchar ('?');
	}
	else
	{
		qse_size_t i;
		for (i = 0; i < n; i++) putchar (mbsbuf[i]);
	}
}

static void xputmchar (qse_mchar_t c, void *arg)
{
	putchar (c);
}

#undef char_t
#undef uchar_t
#undef ochar_t
#undef T
#undef OT
#undef toupper
#undef hex2ascii
#undef sprintn
#undef xprintf

#define char_t qse_mchar_t
#define uchar_t qse_mchar_t
#define ochar_t qse_wchar_t
#define T(x) QSE_MT(x)
#define OT(x) QSE_WT(x)
#define toupper QSE_TOUPPER
#define sprintn m_sprintn
#define xprintf qse_mxprintf 

static const qse_mchar_t m_hex2ascii[] = QSE_MT("0123456789abcdefghijklmnopqrstuvwxyz");
#define hex2ascii(hex)  (m_hex2ascii[hex])

#include "printf.h"

int qse_mprintf (const char_t *fmt, ...)
{
	va_list ap;
	int n;
	va_start (ap, fmt);
	n = qse_mxprintf (fmt, xputmchar, xputwchar, QSE_NULL, ap);
	va_end (ap);
	return n;
}

int qse_mvprintf (const char_t* fmt, va_list ap)
{
	return qse_mxprintf (fmt, xputmchar, xputwchar, QSE_NULL, ap);
}

/* ------------------------------------------------------------------ */

#undef char_t
#undef uchar_t
#undef ochar_t
#undef T
#undef OT
#undef toupper
#undef hex2ascii
#undef sprintn
#undef xprintf

#define char_t qse_wchar_t
#define uchar_t qse_wchar_t
#define ochar_t qse_mchar_t
#define T(x) QSE_WT(x)
#define OT(x) QSE_MT(x)
#define toupper QSE_TOWUPPER
#define sprintn w_sprintn
#define xprintf qse_wxprintf 

static const qse_wchar_t w_hex2ascii[] = QSE_WT("0123456789abcdefghijklmnopqrstuvwxyz");
#define hex2ascii(hex)  (w_hex2ascii[hex])

#include "printf.h"

int qse_wprintf (const char_t *fmt, ...)
{
	va_list ap;
	int n;
	va_start (ap, fmt);
	n = qse_wxprintf (fmt, xputwchar, xputmchar, QSE_NULL, ap);
	va_end (ap);
	return n;
}

int qse_wvprintf (const char_t* fmt, va_list ap)
{
	return qse_wxprintf (fmt, xputwchar, xputmchar, QSE_NULL, ap);
}