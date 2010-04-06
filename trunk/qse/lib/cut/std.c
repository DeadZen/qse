/*
 * $Id: std.c 306 2009-11-22 13:58:53Z baconevi $
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

#include "cut.h"
#include <qse/cut/std.h>
#include <qse/cmn/str.h>
#include <qse/cmn/sio.h>
#include "../cmn/mem.h"

struct xtn_t
{
	const qse_char_t* infile;
	const qse_char_t* outfile;
};

typedef struct xtn_t xtn_t;

qse_cut_t* qse_cut_openstd (qse_size_t xtnsize)
{
	qse_cut_t* cut;
	xtn_t* xtn;

	/* create an object */
	cut = qse_cut_open (
		QSE_MMGR_GETDFL(), QSE_SIZEOF(xtn_t) + xtnsize);
	if (cut == QSE_NULL) return QSE_NULL;

	/* initialize extension */
	xtn = (xtn_t*) QSE_XTN (cut);
	QSE_MEMSET (xtn, 0, QSE_SIZEOF(xtn_t));

	return cut;
}

void* qse_cut_getxtnstd (qse_cut_t* cut)
{
	return (void*)((xtn_t*)QSE_XTN(cut) + 1);
}

int qse_cut_compstd (qse_cut_t* cut, const qse_char_t* sptr)
{
	return qse_cut_comp (cut, sptr, qse_strlen(sptr));
}

static qse_ssize_t xin (
	qse_cut_t* cut, qse_cut_io_cmd_t cmd, qse_cut_io_arg_t* arg,
	qse_char_t* buf, qse_size_t len)
{
	qse_sio_t* sio;
	xtn_t* xtn = (xtn_t*) QSE_XTN (cut);

	switch (cmd)
	{
		case QSE_CUT_IO_OPEN:
		{
			/* main data stream */
			if (xtn->infile == QSE_NULL) sio = qse_sio_in;
			else
			{
				sio = qse_sio_open (
					cut->mmgr,
					0,
					xtn->infile,
					QSE_SIO_READ
				);
				if (sio == QSE_NULL)
				{
					/* set the error message explicitly
					 * as the file name is different from
					 * the standard console name (NULL) */
					qse_cstr_t ea;
					ea.ptr = xtn->infile;
					ea.len = qse_strlen (xtn->infile);
					qse_cut_seterrnum (cut, QSE_CUT_EIOFIL, &ea);
					return -1;
				}
			}

			if (sio == QSE_NULL) return -1;
			arg->handle = sio;
			return 1;
		}

		case QSE_CUT_IO_CLOSE:
		{
			sio = (qse_sio_t*)arg->handle;
			if (sio != qse_sio_in && sio != qse_sio_out && sio != qse_sio_err)
				qse_sio_close (sio);
			return 0;
		}

		case QSE_CUT_IO_READ:
		{
			qse_ssize_t n = qse_sio_getsn (arg->handle, buf, len);

			if (n == -1)
			{
				if (xtn->infile != QSE_NULL)
				{
					qse_cstr_t ea;
					ea.ptr = xtn->infile;
					ea.len = qse_strlen (xtn->infile);
					qse_cut_seterrnum (cut, QSE_CUT_EIOFIL, &ea);
				}
			}

			return n;
		}

		default:
			return -1;
	}
}

static qse_ssize_t xout (
	qse_cut_t* cut, qse_cut_io_cmd_t cmd, qse_cut_io_arg_t* arg,
	qse_char_t* dat, qse_size_t len)
{
	qse_sio_t* sio;
	xtn_t* xtn = (xtn_t*) QSE_XTN (cut);

	switch (cmd)
	{
		case QSE_CUT_IO_OPEN:
		{
			if (xtn->outfile == QSE_NULL) sio = qse_sio_out;
			else
			{
				sio = qse_sio_open (
					cut->mmgr,
					0,
					xtn->outfile,
					QSE_SIO_WRITE |
					QSE_SIO_CREATE |
					QSE_SIO_TRUNCATE
				);
				if (sio == QSE_NULL)
				{
					/* set the error message explicitly
					 * as the file name is different from
					 * the standard console name (NULL) */
					qse_cstr_t ea;
					ea.ptr = xtn->outfile;
					ea.len = qse_strlen (xtn->outfile);
					qse_cut_seterrnum (cut, QSE_CUT_EIOFIL, &ea);
					return -1;
				}
			}

			if (sio == QSE_NULL) return -1;
			arg->handle = sio;
			return 1;
		}

		case QSE_CUT_IO_CLOSE:
		{
			sio = (qse_sio_t*)arg->handle;
			qse_sio_flush (sio);
			if (sio != qse_sio_in && sio != qse_sio_out && sio != qse_sio_err)
				qse_sio_close (sio);
			return 0;
		}

		case QSE_CUT_IO_WRITE:
		{
			qse_ssize_t n = qse_sio_putsn (arg->handle, dat, len);

			if (n == -1)
			{
				if (xtn->infile != QSE_NULL)
				{
					qse_cstr_t ea;
					ea.ptr = xtn->infile;
					ea.len = qse_strlen (xtn->infile);
					qse_cut_seterrnum (cut, QSE_CUT_EIOFIL, &ea);
				}
			}

			return n;
		}
	
		default:
			return -1;
	}
}

int qse_cut_execstd (qse_cut_t* cut, const qse_char_t* infile, const qse_char_t* outfile)
{
	xtn_t* xtn = (xtn_t*) QSE_XTN (cut);
	xtn->infile = infile;
	xtn->outfile = outfile;
	return qse_cut_exec (cut, xin, xout);
}