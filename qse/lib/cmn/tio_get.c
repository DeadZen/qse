/*
 * $Id: tio_get.c,v 1.8 2005/12/26 07:41:48 bacon Exp $
 */

#include <qse/cmn/tio.h>
#include "mem.h"

#define STATUS_GETC_EILSEQ  (1 << 0)

qse_ssize_t qse_tio_getc (qse_tio_t* tio, qse_char_t* c)
{
	qse_size_t left = 0;
	qse_ssize_t n;
	qse_char_t curc;
#ifndef QSE_CHAR_IS_MCHAR
	qse_size_t seqlen;
#endif

	/* TODO: more efficient way to check this?
	 *       maybe better to use QSE_ASSERT 
	 * QSE_ASSERT (tio->input_func != QSE_NULL);
	 */
	if (tio->input_func == QSE_NULL) 
	{
		tio->errnum = QSE_TIO_ENOINF;
		return -1;
	}

	if (tio->input_status & STATUS_GETC_EILSEQ) 
	{
		tio->input_status &= ~STATUS_GETC_EILSEQ;
		tio->errnum = QSE_TIO_EILSEQ;
		return -1;
	}

	if (tio->inbuf_curp >= tio->inbuf_len) 
	{
	getc_conv:
		n = tio->input_func (
			QSE_TIO_IO_DATA, tio->input_arg,
			&tio->inbuf[left], QSE_COUNTOF(tio->inbuf) - left);
		if (n == 0) 
		{
			if (tio->inbuf_curp < tio->inbuf_len)
			{
				/* gargage left in the buffer */
				tio->errnum = QSE_TIO_EICSEQ;
				return -1;
			}

			return 0;
		}
		if (n <= -1) 
		{
			tio->errnum = QSE_TIO_EINPUT;
			return -1;
		}

		tio->inbuf_curp = 0;
		tio->inbuf_len = (qse_size_t)n + left;	
	}

#ifdef QSE_CHAR_IS_MCHAR
	curc = tio->inbuf[tio->inbuf_curp++];
#else
	left = tio->inbuf_len - tio->inbuf_curp;

#if 0
	seqlen = qse_mblen (tio->inbuf[tio->inbuf_curp], left);
	if (seqlen == 0) 
	{
		/* illegal sequence */
		tio->inbuf_curp++;  /* skip one byte */
		tio->errnum = QSE_TIO_EILSEQ;
		return -1;
	}

	if (seqlen > left) 
	{
		/* incomplete sequence */
		if (tio->inbuf_curp > 0)
		{
			QSE_MEMCPY (tio->inbuf, &tio->inbuf[tio->inbuf_curp], left);
			tio->inbuf_curp = 0;
			tio->inbuf_len = left;
		}
		goto getc_conv;
	}
	
	n = qse_mbtowc (&tio->inbuf[tio->inbuf_curp], seqlen, &curc);
	if (n == 0) 
	{
		/* illegal sequence */
		tio->inbuf_curp++; /* skip one byte */
		tio->errnum = QSE_TIO_EILSEQ;
		return -1;
	}
	if (n > seqlen)
	{
		/* incomplete sequence - 
		 *  this check might not be needed because qse_mblen has
		 *  checked it. would QSE_ASSERT (n <= seqlen) be enough? */

		if (tio->inbuf_curp > 0)
		{
			QSE_MEMCPY (tio->inbuf, &tio->inbuf[tio->inbuf_curp], left);
			tio->inbuf_curp = 0;
			tio->inbuf_len = left;
		}
		goto getc_conv;
	}
#endif

	n = qse_mbtowc (&tio->inbuf[tio->inbuf_curp], left, &curc);
	if (n == 0) 
	{
		/* illegal sequence */
		tio->inbuf_curp++; /* skip one byte */
		tio->errnum = QSE_TIO_EILSEQ;
		return -1;
	}
	if (n > left)
	{
		/* incomplete sequence */
		if (tio->inbuf_curp > 0)
		{
			QSE_MEMCPY (tio->inbuf, &tio->inbuf[tio->inbuf_curp], left);
			tio->inbuf_curp = 0;
			tio->inbuf_len = left;
		}
		goto getc_conv;
	}

	tio->inbuf_curp += n;
#endif

	*c = curc;
	return 1;
}

qse_ssize_t qse_tio_gets (qse_tio_t* tio, qse_char_t* buf, qse_size_t size)
{
	qse_ssize_t n;

	if (size <= 0) return 0;
	n = qse_tio_getsx (tio, buf, size - 1);
	if (n == -1) return -1;
	buf[n] = QSE_T('\0');
	return n;
}

qse_ssize_t qse_tio_getsx (qse_tio_t* tio, qse_char_t* buf, qse_size_t size)
{
	qse_ssize_t n;
	qse_char_t* p, * end, c;

	if (size <= 0) return 0;

	p = buf; end = buf + size;
	while (p < end) 
	{
		n = qse_tio_getc (tio, &c);
		if (n == -1) 
		{
			if (p > buf && tio->errnum == QSE_TIO_EILSEQ) 
			{
				tio->input_status |= STATUS_GETC_EILSEQ;
				break;
			}
			return -1;
		}
		if (n == 0) break;
		*p++ = c;

		/* TODO: support a different line breaker */
		if (c == QSE_T('\n')) break;
	}

	return p - buf;
}

qse_ssize_t qse_tio_getstr (qse_tio_t* tio, qse_str_t* buf)
{
	qse_ssize_t n;
	qse_char_t c;

	qse_str_clear (buf);

	for (;;) 
	{
		n = qse_tio_getc (tio, &c);
		if (n == -1) 
		{
			if (QSE_STR_LEN(buf) > 0 && tio->errnum == QSE_TIO_EILSEQ) 
			{
				tio->input_status |= STATUS_GETC_EILSEQ;
				break;
			}
			return -1;
		}
		if (n == 0) break;

		if (qse_str_ccat(buf, c) == (qse_size_t)-1) 
		{
			tio->errnum = QSE_TIO_ENOMEM;
			return -1;
		}

		/* TODO: support a different line breaker */
		if (c == QSE_T('\n')) break;
	}

	return QSE_STR_LEN(buf);
}