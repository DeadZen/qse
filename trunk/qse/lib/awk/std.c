/*
 * $Id: std.c 246 2009-07-27 02:31:58Z hyunghwan.chung $
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

#include "awk.h"
#include <qse/awk/std.h>
#include <qse/cmn/sio.h>
#include <qse/cmn/pio.h>
#include <qse/cmn/str.h>
#include <qse/cmn/time.h>
#include <qse/cmn/stdio.h> /* TODO: remove dependency on qse_vsprintf */

#include <math.h>
#include <stdarg.h>
#include <stdlib.h>

typedef struct xtn_t
{
	struct
	{
		struct 
		{
			qse_awk_parsestd_type_t type;
			union
			{
				const qse_char_t* file; 
				const qse_char_t* cp;
				struct 
				{
					const qse_char_t* ptr;	
					const qse_char_t* end;	
				} cpl;
			} u;
			qse_sio_t* handle; /* the handle to an open file */
		} in;

		struct
		{
			qse_awk_parsestd_type_t type;
			union
			{
				const qse_char_t* file;
				qse_char_t*       cp;
				struct 
				{
					qse_xstr_t* osp;
					qse_char_t* ptr;	
					qse_char_t* end;	
				} cpl;
			} u;
			qse_sio_t*              handle;
		} out;

	} s;
} xtn_t;


typedef struct rxtn_t
{
	unsigned int seed;	

	struct
	{
		struct {
			const qse_char_t*const* files;
			qse_size_t index;
			qse_size_t count;
		} in; 

		struct 
		{
			const qse_char_t*const* files;
			qse_size_t index;
			qse_size_t count;
		} out;
	} c;  /* console */

} rxtn_t;

static qse_real_t custom_awk_pow (qse_awk_t* awk, qse_real_t x, qse_real_t y)
{
#if defined(HAVE_POWL)
	return powl (x, y);
#elif defined(HAVE_POW)
	return pow (x, y);
#elif defined(HAVE_POWF)
	return powf (x, y);
#else
	#error ### no pow function available ###
#endif
}

static int custom_awk_sprintf (
	qse_awk_t* awk, qse_char_t* buf, qse_size_t size, 
	const qse_char_t* fmt, ...)
{
	int n;

	va_list ap;
	va_start (ap, fmt);
	n = qse_vsprintf (buf, size, fmt, ap);
	va_end (ap);

	return n;
}

static int add_functions (qse_awk_t* awk);

qse_awk_t* qse_awk_openstd (qse_size_t xtnsize)
{
	qse_awk_t* awk;
	qse_awk_prm_t prm;
	xtn_t* xtn;

	prm.pow     = custom_awk_pow;
	prm.sprintf = custom_awk_sprintf;

	/* create an object */
	awk = qse_awk_open (
		QSE_MMGR_GETDFL(), QSE_SIZEOF(xtn_t) + xtnsize, &prm);
	if (awk == QSE_NULL) return QSE_NULL;

	/* initialize extension */
	xtn = (xtn_t*) QSE_XTN (awk);
	QSE_MEMSET (xtn, 0, QSE_SIZEOF(xtn_t));

	/* add intrinsic functions */
	if (add_functions (awk) == -1)
	{
		qse_awk_close (awk);
		return QSE_NULL;
	}

	return awk;
}

void* qse_awk_getxtnstd (qse_awk_t* awk)
{
	return (void*)((xtn_t*)QSE_XTN(awk) + 1);
}

/*** PARSESTD ***/

static qse_ssize_t sf_in_open (
	qse_awk_t* awk, qse_awk_sio_arg_t* arg, xtn_t* xtn)
{
	if (arg == QSE_NULL || arg->name == QSE_NULL)
	{
		switch (xtn->s.in.type)
		{
			case QSE_AWK_PARSESTD_FILE:
				if (xtn->s.in.u.file == QSE_NULL) return -1;

				if (xtn->s.in.u.file[0] == QSE_T('-') &&
				    xtn->s.in.u.file[1] == QSE_T('\0'))
				{
					/* special file name '-' */
					xtn->s.in.handle = qse_sio_in;
				}
				else
				{
					xtn->s.in.handle = qse_sio_open (
						awk->mmgr,
						0,
						xtn->s.in.u.file,
						QSE_SIO_READ
					);
					if (xtn->s.in.handle == QSE_NULL) 
					{
						qse_cstr_t ea;
						ea.ptr = xtn->s.in.u.file;
						ea.len = qse_strlen(ea.ptr);
						qse_awk_seterror (
							awk, QSE_AWK_EOPEN,
							0, &ea
						);
						return -1;
					}
				}
				return 1;

			case QSE_AWK_PARSESTD_STDIO:
				xtn->s.in.handle = qse_sio_in;
				return 1;

			case QSE_AWK_PARSESTD_CP:
			case QSE_AWK_PARSESTD_CPL:
				xtn->s.in.handle = QSE_NULL;
				return 1;
		}
	}
	else
	{
/* TODO: standard include path */
		arg->handle = qse_sio_open (
			awk->mmgr, 0, arg->name, QSE_SIO_READ
		);
		if (arg->handle == QSE_NULL) 
		{
			qse_cstr_t ea;
			ea.ptr = arg->name;
			ea.len = qse_strlen(ea.ptr);
			qse_awk_seterror (awk, QSE_AWK_EOPEN, 0, &ea);
			return -1;
		}

		return 1;
	}
}

static qse_ssize_t sf_in_close (
	qse_awk_t* awk, qse_awk_sio_arg_t* arg, xtn_t* xtn)
{
	if (arg == QSE_NULL || arg->name == QSE_NULL)
	{
		if (xtn->s.in.handle != QSE_NULL && 
		    xtn->s.in.handle != qse_sio_in &&
		    xtn->s.in.handle != qse_sio_out &&
		    xtn->s.in.handle != qse_sio_err) 
		{
			qse_sio_close (xtn->s.in.handle);
		}
	}
	else
	{
		qse_sio_close (arg->handle);
	}

	return 0;
}

static qse_ssize_t sf_in_read (
	qse_awk_t* awk, qse_awk_sio_arg_t* arg,
	qse_char_t* data, qse_size_t size, xtn_t* xtn)
{
	if (arg == QSE_NULL || arg->name == QSE_NULL)
	{
		switch (xtn->s.in.type)
		{
			case QSE_AWK_PARSESTD_FILE:
			case QSE_AWK_PARSESTD_STDIO:
			{
				qse_ssize_t n;

				QSE_ASSERT (xtn->s.in.handle != QSE_NULL);
				n = qse_sio_getsn (xtn->s.in.handle, data, size);
				if (n == -1)
				{
					qse_cstr_t ea;
					ea.ptr = xtn->s.in.u.file;
					ea.len = qse_strlen(ea.ptr);
					qse_awk_seterror (
						awk, QSE_AWK_EREAD, 0, &ea);
				}
				return n;
			}

			case QSE_AWK_PARSESTD_CP:
			{
				qse_size_t n = 0;
				while (n < size && *xtn->s.in.u.cp != QSE_T('\0'))
				{
					data[n++] = *xtn->s.in.u.cp++;
				}
				return n;
			}
		
			case QSE_AWK_PARSESTD_CPL:
			{
				qse_size_t n = 0;
				while (n < size && xtn->s.in.u.cpl.ptr < xtn->s.in.u.cpl.end)
				{
					data[n++] = *xtn->s.in.u.cpl.ptr++;
				}
				return n;
			}
		}
	}
	else
	{
		qse_ssize_t n;

		QSE_ASSERT (arg->handle != QSE_NULL);
		n = qse_sio_getsn (arg->handle, data, size);
		if (n == -1)
		{
			qse_cstr_t ea;
			ea.ptr = arg->name;
			ea.len = qse_strlen(ea.ptr);
			qse_awk_seterror (awk, QSE_AWK_EREAD, 0, &ea);
		}
		return n;
	}
}

static qse_ssize_t sf_in (
	qse_awk_t* awk, qse_awk_sio_cmd_t cmd, 
	qse_awk_sio_arg_t* arg, qse_char_t* data, qse_size_t size)
{
	xtn_t* xtn = QSE_XTN (awk);

	switch (cmd)
	{
		case QSE_AWK_SIO_OPEN:
			return sf_in_open (awk, arg, xtn);

		case QSE_AWK_SIO_CLOSE:
			return sf_in_close (awk, arg, xtn);

		case QSE_AWK_SIO_READ:
			return sf_in_read (awk, arg, data, size, xtn);

		default:
			return -1;
	}
}

static qse_ssize_t sf_out (
	qse_awk_t* awk, qse_awk_sio_cmd_t cmd, 
	qse_awk_sio_arg_t* arg, qse_char_t* data, qse_size_t size)
{
	xtn_t* xtn = QSE_XTN (awk);

	if (cmd == QSE_AWK_SIO_OPEN) 
	{
		switch (xtn->s.out.type)
		{
			case QSE_AWK_PARSESTD_FILE:
				if (xtn->s.out.u.file == QSE_NULL) return -1;

				if (xtn->s.out.u.file[0] == QSE_T('-') &&
				    xtn->s.out.u.file[1] == QSE_T('\0'))
				{
					/* special file name '-' */
					xtn->s.out.handle = qse_sio_out;
				}
				else
				{
					xtn->s.out.handle = qse_sio_open (
						awk->mmgr,
						0,
						xtn->s.out.u.file, 
						QSE_SIO_WRITE | 
						QSE_SIO_CREATE | 
						QSE_SIO_TRUNCATE
					);
					if (xtn->s.out.handle == QSE_NULL) 
					{
						qse_cstr_t ea;
						ea.ptr = xtn->s.out.u.file;
						ea.len = qse_strlen(ea.ptr);
						qse_awk_seterror (
							awk, QSE_AWK_EOPEN,
							0, &ea
						);
						return -1;
					}
				}
				return 1;

			case QSE_AWK_PARSESTD_STDIO:
				xtn->s.out.handle = qse_sio_out;
				return 1;

			case QSE_AWK_PARSESTD_CP:
			case QSE_AWK_PARSESTD_CPL:
				xtn->s.out.handle = QSE_NULL;
				return 1;
		}
	}
	else if (cmd == QSE_AWK_SIO_CLOSE) 
	{
		switch (xtn->s.out.type)
		{
			case QSE_AWK_PARSESTD_FILE:
			case QSE_AWK_PARSESTD_STDIO:

				qse_sio_flush (xtn->s.out.handle);
				if (xtn->s.out.handle != qse_sio_in &&
				    xtn->s.out.handle != qse_sio_out &&
				    xtn->s.out.handle != qse_sio_err) 
				{
					qse_sio_close (xtn->s.out.handle);
				}
				return 0;

			case QSE_AWK_PARSESTD_CP:
				*xtn->s.out.u.cp = QSE_T('\0');
				return 0;

			case QSE_AWK_PARSESTD_CPL:
				xtn->s.out.u.cpl.osp->len = 
					xtn->s.out.u.cpl.ptr -
					xtn->s.out.u.cpl.osp->ptr;
				return 0;
		}
	}
	else if (cmd == QSE_AWK_SIO_WRITE)
	{
		switch (xtn->s.out.type)
		{
			case QSE_AWK_PARSESTD_FILE:
			case QSE_AWK_PARSESTD_STDIO:
			{
				qse_ssize_t n;
				QSE_ASSERT (xtn->s.out.handle != QSE_NULL);
				n = qse_sio_putsn (xtn->s.out.handle, data, size);
				if (n == -1)
				{
					qse_cstr_t ea;
					ea.ptr = xtn->s.in.u.file;
					ea.len = qse_strlen(ea.ptr);
					qse_awk_seterror (
						awk, QSE_AWK_EWRITE, 0, &ea);
				}

				return n;
			}

			case QSE_AWK_PARSESTD_CP:
			{
				qse_size_t n = 0;
				while (n < size && *xtn->s.out.u.cp != QSE_T('\0'))
				{
					*xtn->s.out.u.cp++ = data[n++];
				}
				return n;
			}

			case QSE_AWK_PARSESTD_CPL:
			{
				qse_size_t n = 0;
				while (n < size && xtn->s.out.u.cpl.ptr < xtn->s.out.u.cpl.end)
				{
					*xtn->s.out.u.cpl.ptr++ = data[n++];
				}
				return n;
			}
		}
	}

	return -1;
}

int qse_awk_parsestd (
	qse_awk_t* awk, 
        const qse_awk_parsestd_in_t* in,
        qse_awk_parsestd_out_t* out)
{
	qse_awk_sio_t sio;
	xtn_t* xtn = (xtn_t*) QSE_XTN (awk);

	if (in == QSE_NULL)
	{
		/* the input is a must */
		qse_awk_seterrnum (awk, QSE_AWK_EINVAL);
		return -1;
	}

	switch (in->type)
	{
		case QSE_AWK_PARSESTD_FILE:
			xtn->s.in.u.file = in->u.file;
			break;

		case QSE_AWK_PARSESTD_CP:
			xtn->s.in.u.cp = in->u.cp;
			break;

		case QSE_AWK_PARSESTD_CPL:
			xtn->s.in.u.cpl.ptr = in->u.cpl.ptr;
			xtn->s.in.u.cpl.end = in->u.cpl.ptr + in->u.cpl.len;
			break;

		case QSE_AWK_PARSESTD_STDIO:
			/* nothing to do */
			break;

		default:
			qse_awk_seterrnum (awk, QSE_AWK_EINVAL);
			return -1;
	}
	xtn->s.in.type = in->type;
	xtn->s.in.handle = QSE_NULL;
        sio.in = sf_in;

	if (out == QSE_NULL) sio.out = QSE_NULL;
	else
	{
		switch (out->type)
		{
			case QSE_AWK_PARSESTD_FILE:
				xtn->s.out.u.file = out->u.file;
				break;
	
			case QSE_AWK_PARSESTD_CP:
				xtn->s.out.u.cp = out->u.cp;
				break;
	
			case QSE_AWK_PARSESTD_CPL:
				xtn->s.out.u.cpl.osp = &out->u.cpl;
				xtn->s.out.u.cpl.ptr = out->u.cpl.ptr;
				xtn->s.out.u.cpl.end = out->u.cpl.ptr + out->u.cpl.len;
				break;

			case QSE_AWK_PARSESTD_STDIO:
				/* nothing to do */
				break;
	
			default:
				qse_awk_seterrnum (awk, QSE_AWK_EINVAL);
				return -1;
		}
		xtn->s.out.type = out->type;
		xtn->s.out.handle = QSE_NULL;
		sio.out = sf_out;
	}

	return qse_awk_parse (awk, &sio);
}

/*** RTX_OPENSTD ***/
static qse_ssize_t awk_rio_pipe (
	qse_awk_rtx_t* rtx, qse_awk_rio_cmd_t cmd, qse_awk_rio_arg_t* riod,
	qse_char_t* data, qse_size_t size)
{
	switch (cmd)
	{
		case QSE_AWK_RIO_OPEN:
		{
			qse_pio_t* handle;
			int flags;

			if (riod->mode == QSE_AWK_RIO_PIPE_READ)
			{
				/* TODO: should we specify ERRTOOUT? */
				flags = QSE_PIO_READOUT | 
				        QSE_PIO_ERRTOOUT;
			}
			else if (riod->mode == QSE_AWK_RIO_PIPE_WRITE)
			{
				flags = QSE_PIO_WRITEIN;
			}
			else if (riod->mode == QSE_AWK_RIO_PIPE_RW)
			{
				flags = QSE_PIO_READOUT | 
				        QSE_PIO_ERRTOOUT |
				        QSE_PIO_WRITEIN;
			}
			else return -1; /* TODO: any way to set the error number? */

			handle = qse_pio_open (
				rtx->awk->mmgr,
				0, 
				riod->name, 
				flags|QSE_PIO_SHELL|QSE_PIO_TEXT
			);

			if (handle == QSE_NULL) return -1;
			riod->handle = (void*)handle;
			return 1;
		}

		case QSE_AWK_RIO_CLOSE:
		{
			qse_pio_close ((qse_pio_t*)riod->handle);
			riod->handle = QSE_NULL;
			return 0;
		}

		case QSE_AWK_RIO_READ:
		{
			return qse_pio_read (
				(qse_pio_t*)riod->handle,
				data,	
				size,
				QSE_PIO_OUT
			);
		}

		case QSE_AWK_RIO_WRITE:
		{
			return qse_pio_write (
				(qse_pio_t*)riod->handle,
				data,	
				size,
				QSE_PIO_IN
			);
		}

		case QSE_AWK_RIO_FLUSH:
		{
			/*if (riod->mode == QSE_AWK_RIO_PIPE_READ) return -1;*/
			return qse_pio_flush ((qse_pio_t*)riod->handle, QSE_PIO_IN);
		}

		case QSE_AWK_RIO_NEXT:
		{
			return -1;
		}
	}

	return -1;
}

static qse_ssize_t awk_rio_file (
	qse_awk_rtx_t* rtx, qse_awk_rio_cmd_t cmd, qse_awk_rio_arg_t* riod,
	qse_char_t* data, qse_size_t size)
{
	switch (cmd)
	{
		case QSE_AWK_RIO_OPEN:
		{
			qse_fio_t* handle;
			int flags;

			if (riod->mode == QSE_AWK_RIO_FILE_READ)
			{
				flags = QSE_FIO_READ;
			}
			else if (riod->mode == QSE_AWK_RIO_FILE_WRITE)
			{
				flags = QSE_FIO_WRITE |
				        QSE_FIO_CREATE |
				        QSE_FIO_TRUNCATE;
			}
			else if (riod->mode == QSE_AWK_RIO_FILE_APPEND)
			{
				flags = QSE_FIO_APPEND |
				        QSE_FIO_CREATE;
			}
			else return -1; /* TODO: any way to set the error number? */

			handle = qse_fio_open (
				rtx->awk->mmgr,
				0,
				riod->name, 
				flags | QSE_FIO_TEXT,
				QSE_FIO_RUSR | QSE_FIO_WUSR | 
				QSE_FIO_RGRP | QSE_FIO_ROTH
			);
			if (handle == QSE_NULL) 
			{
				qse_cstr_t errarg;

				errarg.ptr = riod->name;
				errarg.len = qse_strlen(riod->name);

				qse_awk_rtx_seterror (rtx, QSE_AWK_EOPEN, 0, &errarg);
				return -1;
			}

			riod->handle = (void*)handle;
			return 1;
		}

		case QSE_AWK_RIO_CLOSE:
		{
			qse_fio_close ((qse_fio_t*)riod->handle);
			riod->handle = QSE_NULL;
			return 0;
		}

		case QSE_AWK_RIO_READ:
		{
			return qse_fio_read (
				(qse_fio_t*)riod->handle,
				data,	
				size
			);
		}

		case QSE_AWK_RIO_WRITE:
		{
			return qse_fio_write (
				(qse_fio_t*)riod->handle,
				data,	
				size
			);
		}

		case QSE_AWK_RIO_FLUSH:
		{
			return qse_fio_flush ((qse_fio_t*)riod->handle);
		}

		case QSE_AWK_RIO_NEXT:
		{
			return -1;
		}

	}

	return -1;
}

static int open_rio_console (qse_awk_rtx_t* rtx, qse_awk_rio_arg_t* riod)
{
	rxtn_t* rxtn = (rxtn_t*) QSE_XTN (rtx);

	if (riod->mode == QSE_AWK_RIO_CONSOLE_READ)
	{
		if (rxtn->c.in.files == QSE_NULL)
		{
			/* if no input files is specified, 
			 * open the standard input */
			QSE_ASSERT (rxtn->c.in.index == 0);

			if (rxtn->c.in.count == 0)
			{
				riod->handle = qse_sio_in;
				rxtn->c.in.count++;
				return 1;
			}

			return 0;
		}
		else
		{
			/* a temporary variable sio is used here not to change 
			 * any fields of riod when the open operation fails */
			qse_sio_t* sio;
			const qse_char_t* file;
			qse_awk_val_t* argv;
			qse_map_t* map;
			qse_map_pair_t* pair;
			qse_char_t ibuf[128];
			qse_size_t ibuflen;
			qse_awk_val_t* v;
			qse_awk_rtx_valtostr_out_t out;

		nextfile:
			file = rxtn->c.in.files[rxtn->c.in.index];

			if (file == QSE_NULL)
			{
				/* no more input file */

				if (rxtn->c.in.count == 0)
				{
					/* all ARGVs are empty strings. 
					 * so no console files were opened.
					 * open the standard input here.
					 *
					 * 'BEGIN { ARGV[1]=""; ARGV[2]=""; }
					 *        { print $0; }' file1 file2
					 */
					riod->handle = qse_sio_in;
					rxtn->c.in.count++;
					return 1;
				}

				return 0;
			}

			/* handle special case when ARGV[x] has been altered.
			 * so from here down, the file name gotten from 
			 * rxtn->c.in.files is not important and is overridden 
			 * from ARGV.
			 * 'BEGIN { ARGV[1]="file3"; } 
			 *        { print $0; }' file1 file2
			 */
			argv = qse_awk_rtx_getgbl (rtx, QSE_AWK_GBL_ARGV);
			QSE_ASSERT (argv != QSE_NULL);
			QSE_ASSERT (argv->type == QSE_AWK_VAL_MAP);

			map = ((qse_awk_val_map_t*)argv)->map;
			QSE_ASSERT (map != QSE_NULL);
			
			ibuflen = qse_awk_longtostr (
				rtx->awk, rxtn->c.in.index + 1, 10, QSE_NULL,
				ibuf, QSE_COUNTOF(ibuf));

			pair = qse_map_search (map, ibuf, ibuflen);
			QSE_ASSERT (pair != QSE_NULL);

			v = QSE_MAP_VPTR(pair);
			QSE_ASSERT (v != QSE_NULL);

			out.type = QSE_AWK_RTX_VALTOSTR_CPLDUP;
			if (qse_awk_rtx_valtostr (rtx, v, &out) == QSE_NULL) return -1;

			if (out.u.cpldup.len == 0)
			{
				/* the name is empty */
				qse_awk_rtx_free (rtx, out.u.cpldup.ptr);
				rxtn->c.in.index++;
				goto nextfile;
			}

			if (qse_strlen(out.u.cpldup.ptr) < out.u.cpldup.len)
			{
				/* the name contains one or more '\0' */
				qse_cstr_t errarg;

				errarg.ptr = out.u.cpldup.ptr;
				/* use this length not to contains '\0'
				 * in an error message */
				errarg.len = qse_strlen(out.u.cpldup.ptr);

				qse_awk_rtx_seterror (
					rtx, QSE_AWK_EIONMNL, 0, &errarg);

				qse_awk_rtx_free (rtx, out.u.cpldup.ptr);
				return -1;
			}

			file = out.u.cpldup.ptr;

			if (file[0] == QSE_T('-') && file[1] == QSE_T('\0'))
			{
				/* special file name '-' */
				sio = qse_sio_in;
			}
			else
			{
				sio = qse_sio_open (
					rtx->awk->mmgr, 0, file, QSE_SIO_READ);
				if (sio == QSE_NULL)
				{
					qse_cstr_t errarg;

					errarg.ptr = file;
					errarg.len = qse_strlen(file);

					qse_awk_rtx_seterror (
						rtx, QSE_AWK_EOPEN, 0, &errarg);

					qse_awk_rtx_free (rtx, out.u.cpldup.ptr);
					return -1;
				}
			}

			if (qse_awk_rtx_setfilename (
				rtx, file, qse_strlen(file)) == -1)
			{
				if (sio != qse_sio_in) qse_sio_close (sio);
				qse_awk_rtx_free (rtx, out.u.cpldup.ptr);
				return -1;
			}

			qse_awk_rtx_free (rtx, out.u.cpldup.ptr);
			riod->handle = sio;

			/* increment the counter of files successfully opened */
			rxtn->c.in.count++;
			rxtn->c.in.index++;
			return 1;
		}

	}
	else if (riod->mode == QSE_AWK_RIO_CONSOLE_WRITE)
	{
		if (rxtn->c.out.files == QSE_NULL)
		{
			QSE_ASSERT (rxtn->c.out.index == 0);

			if (rxtn->c.out.count == 0)
			{
				riod->handle = qse_sio_out;
				rxtn->c.out.count++;
				return 1;
			}

			return 0;
		}
		else
		{
			/* a temporary variable sio is used here not to change 
			 * any fields of riod when the open operation fails */
			qse_sio_t* sio;
			const qse_char_t* file;

			file = rxtn->c.out.files[rxtn->c.out.index];

			if (file == QSE_NULL)
			{
				/* no more input file */
				return 0;
			}

			if (file[0] == QSE_T('-') && file[1] == QSE_T('\0'))
			{
				/* special file name '-' */
				sio = qse_sio_out;
			}
			else
			{
				sio = qse_sio_open (
					rtx->awk->mmgr, 0, file, QSE_SIO_READ);
				if (sio == QSE_NULL)
				{
					qse_cstr_t errarg;

					errarg.ptr = file;
					errarg.len = qse_strlen(file);

					qse_awk_rtx_seterror (
						rtx, QSE_AWK_EOPEN, 0, &errarg);
					return -1;
				}
			}
			
			if (qse_awk_rtx_setofilename (
				rtx, file, qse_strlen(file)) == -1)
			{
				qse_sio_close (sio);
				return -1;
			}

			riod->handle = sio;

			rxtn->c.out.index++;
			rxtn->c.out.count++;
			return 1;
		}

	}

	return -1;
}

static qse_ssize_t awk_rio_console (
	qse_awk_rtx_t* rtx, qse_awk_rio_cmd_t cmd, qse_awk_rio_arg_t* riod,
	qse_char_t* data, qse_size_t size)
{
	if (cmd == QSE_AWK_RIO_OPEN)
	{
		return open_rio_console (rtx, riod);
	}
	else if (cmd == QSE_AWK_RIO_CLOSE)
	{
		if (riod->handle != QSE_NULL &&
		    riod->handle != qse_sio_in && 
		    riod->handle != qse_sio_out && 
		    riod->handle != qse_sio_err)
		{
			qse_sio_close ((qse_sio_t*)riod->handle);
		}

		return 0;
	}
	else if (cmd == QSE_AWK_RIO_READ)
	{
		qse_ssize_t nn;

		while ((nn = qse_sio_getsn((qse_sio_t*)riod->handle,data,size)) == 0)
		{
			int n;
			qse_sio_t* sio = (qse_sio_t*)riod->handle;

			n = open_rio_console (rtx, riod);
			if (n == -1) return -1;

			if (n == 0) 
			{
				/* no more input console */
				return 0;
			}

			if (sio != QSE_NULL && 
			    sio != qse_sio_in && 
			    sio != qse_sio_out &&
			    sio != qse_sio_err) 
			{
				qse_sio_close (sio);
			}
		}

		return nn;
	}
	else if (cmd == QSE_AWK_RIO_WRITE)
	{
		return qse_sio_putsn (	
			(qse_sio_t*)riod->handle,
			data,
			size
		);
	}
	else if (cmd == QSE_AWK_RIO_FLUSH)
	{
		return qse_sio_flush ((qse_sio_t*)riod->handle);
	}
	else if (cmd == QSE_AWK_RIO_NEXT)
	{
		int n;
		qse_sio_t* sio = (qse_sio_t*)riod->handle;

		n = open_rio_console (rtx, riod);
		if (n == -1) return -1;

		if (n == 0) 
		{
			/* if there is no more file, keep the previous handle */
			return 0;
		}

		if (sio != QSE_NULL && 
		    sio != qse_sio_in && 
		    sio != qse_sio_out &&
		    sio != qse_sio_err) 
		{
			qse_sio_close (sio);
		}

		return n;
	}

	return -1;
}

qse_awk_rtx_t* qse_awk_rtx_openstd (
	qse_awk_t*              awk,
	qse_size_t              xtnsize,
	const qse_char_t*       id,
	const qse_char_t*const* icf,
	const qse_char_t*const* ocf)
{
	qse_awk_rtx_t* rtx;
	qse_awk_rio_t rio;
	rxtn_t* rxtn;
	qse_ntime_t now;

	const qse_char_t*const* p;
	qse_size_t argc = 0;
	qse_cstr_t argv[16];
	qse_cstr_t* argvp = QSE_NULL, * p2;

	rio.pipe    = awk_rio_pipe;
	rio.file    = awk_rio_file;
	rio.console = awk_rio_console;

	if (icf != QSE_NULL)
	{
		for (p = icf; *p != QSE_NULL; p++);
		argc = p - icf;
	}

	argc++; /* for id */

	if (argc < QSE_COUNTOF(argv)) argvp = argv;
	else
	{
		argvp = QSE_AWK_ALLOC (
			awk, QSE_SIZEOF(*argvp) * (argc + 1));
		if (argvp == QSE_NULL)
		{
			qse_awk_seterrnum (awk, QSE_AWK_ENOMEM);
			return QSE_NULL;
		}
	}

	p2 = argvp;

	p2->ptr = id;
	p2->len = qse_strlen(id);
	p2++;
	
	if (icf != QSE_NULL)
	{
		for (p = icf; *p != QSE_NULL; p++, p2++) 
		{
			p2->ptr = *p;
			p2->len = qse_strlen(*p);
		}
	}

	p2->ptr = QSE_NULL;
	p2->len = 0;

	rtx = qse_awk_rtx_open (
		awk, 
		QSE_SIZEOF(rxtn_t) + xtnsize,
		&rio,
		argvp
	);

	if (argvp != QSE_NULL && argvp != argv) QSE_AWK_FREE (awk, argvp);
	if (rtx == QSE_NULL) return QSE_NULL;

	rxtn = (rxtn_t*) QSE_XTN (rtx);
	QSE_MEMSET (rxtn, 0, QSE_SIZEOF(rxtn_t));

	if (qse_gettime (&now) == -1) rxtn->seed = 0;
	else rxtn->seed = (unsigned int) now;
	srand (rxtn->seed);

	rxtn->c.in.files = icf;
	rxtn->c.in.index = 0;
	rxtn->c.in.count = 0;
	rxtn->c.out.files = ocf;
	rxtn->c.out.index = 0;
	rxtn->c.out.count = 0;

	return rtx;
}

void* qse_awk_rtx_getxtnstd (qse_awk_rtx_t* rtx)
{
	return (void*)((rxtn_t*)QSE_XTN(rtx) + 1);
}

/*** EXTRA BUILTIN FUNCTIONS ***/
enum
{
	FNC_MATH_LD,
	FNC_MATH_D,
	FNC_MATH_F
};

static int fnc_math_1 (
	qse_awk_rtx_t* run, const qse_cstr_t* fnm,
	int type, void* f)
{
	qse_size_t nargs;
	qse_awk_val_t* a0;
	qse_long_t lv;
	qse_real_t rv;
	qse_awk_val_t* r;
	int n;

	nargs = qse_awk_rtx_getnargs (run);
	QSE_ASSERT (nargs == 1);

	a0 = qse_awk_rtx_getarg (run, 0);

	n = qse_awk_rtx_valtonum (run, a0, &lv, &rv);
	if (n == -1) return -1;
	if (n == 0) rv = (qse_real_t)lv;

	if (type == FNC_MATH_LD)
	{
		long double (*rf) (long double) = 
			(long double(*)(long double))f;
		r = qse_awk_rtx_makerealval (run, rf(rv));
	}
	else if (type == FNC_MATH_D)
	{
		double (*rf) (double) = (double(*)(double))f;
		r = qse_awk_rtx_makerealval (run, rf(rv));
	}
	else 
	{
		float (*rf) (float);
		QSE_ASSERT (type == FNC_MATH_F);
		rf = (float(*)(float))f;
		r = qse_awk_rtx_makerealval (run, rf(rv));
	}
	
	if (r == QSE_NULL)
	{
		qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM);
		return -1;
	}

	qse_awk_rtx_setretval (run, r);
	return 0;
}

static int fnc_math_2 (
	qse_awk_rtx_t* run, const qse_cstr_t* fnm, int type, void* f)
{
	qse_size_t nargs;
	qse_awk_val_t* a0, * a1;
	qse_long_t lv0, lv1;
	qse_real_t rv0, rv1;
	qse_awk_val_t* r;
	int n;

	nargs = qse_awk_rtx_getnargs (run);
	QSE_ASSERT (nargs == 2);

	a0 = qse_awk_rtx_getarg (run, 0);
	a1 = qse_awk_rtx_getarg (run, 1);

	n = qse_awk_rtx_valtonum (run, a0, &lv0, &rv0);
	if (n == -1) return -1;
	if (n == 0) rv0 = (qse_real_t)lv0;

	n = qse_awk_rtx_valtonum (run, a1, &lv1, &rv1);
	if (n == -1) return -1;
	if (n == 0) rv1 = (qse_real_t)lv1;

	if (type == FNC_MATH_LD)
	{
		long double (*rf) (long double,long double) = 
			(long double(*)(long double,long double))f;
		r = qse_awk_rtx_makerealval (run, rf(rv0,rv1));
	}
	else if (type == FNC_MATH_D)
	{
		double (*rf) (double,double) = (double(*)(double,double))f;
		r = qse_awk_rtx_makerealval (run, rf(rv0,rv1));
	}
	else 
	{
		float (*rf) (float,float);
		QSE_ASSERT (type == FNC_MATH_F);
		rf = (float(*)(float,float))f;
		r = qse_awk_rtx_makerealval (run, rf(rv0,rv1));
	}
	
	if (r == QSE_NULL)
	{
		qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM);
		return -1;
	}

	qse_awk_rtx_setretval (run, r);
	return 0;
}

static int fnc_sin (qse_awk_rtx_t* run, const qse_cstr_t* fnm)
{
	return fnc_math_1 (
		run, fnm,
	#if defined(HAVE_SINL)
		FNC_MATH_LD, (void*)sinl
	#elif defined(HAVE_SIN)
		FNC_MATH_D, (void*)sin
	#elif defined(HAVE_SINF)
		FNC_MATH_F, (void*)sinf
	#else
		#error ### no sin function available ###
	#endif
	);
}

static int fnc_cos (qse_awk_rtx_t* run, const qse_cstr_t* fnm)
{
	return fnc_math_1 (
		run, fnm,
	#if defined(HAVE_COSL)
		FNC_MATH_LD, (void*)cosl
	#elif defined(HAVE_COS)
		FNC_MATH_D, (void*)cos
	#elif defined(HAVE_COSF)
		FNC_MATH_F, (void*)cosf
	#else
		#error ### no cos function available ###
	#endif
	);
}

static int fnc_tan (qse_awk_rtx_t* run, const qse_cstr_t* fnm)
{
	return fnc_math_1 (
		run, fnm,
	#if defined(HAVE_TANL)
		FNC_MATH_LD, (void*)tanl
	#elif defined(HAVE_TAN)
		FNC_MATH_D, (void*)tan
	#elif defined(HAVE_TANF)
		FNC_MATH_F, (void*)tanf
	#else
		#error ### no tan function available ###
	#endif
	);
}

static int fnc_atan (qse_awk_rtx_t* run, const qse_cstr_t* fnm)
{
	return fnc_math_1 (
		run, fnm,
	#if defined(HAVE_ATANL)
		FNC_MATH_LD, (void*)atanl
	#elif defined(HAVE_ATAN)
		FNC_MATH_D, (void*)atan
	#elif defined(HAVE_ATANF)
		FNC_MATH_F, (void*)atanf
	#else
		#error ### no atan function available ###
	#endif
	);
}

static int fnc_atan2 (qse_awk_rtx_t* run, const qse_cstr_t* fnm)
{
	return fnc_math_2 (
		run, fnm,
	#if defined(HAVE_ATAN2L)
		FNC_MATH_LD, (void*)atan2l
	#elif defined(HAVE_ATAN2)
		FNC_MATH_D, (void*)atan2
	#elif defined(HAVE_ATAN2F)
		FNC_MATH_F, (void*)atan2f
	#else
		#error ### no atan2 function available ###
	#endif
	);
}

static int fnc_log (qse_awk_rtx_t* run, const qse_cstr_t* fnm)
{
	return fnc_math_1 (
		run, fnm,
	#if defined(HAVE_LOGL)
		FNC_MATH_LD, (void*)logl
	#elif defined(HAVE_LOG)
		FNC_MATH_D, (void*)log
	#elif defined(HAVE_LOGF)
		FNC_MATH_F, (void*)logf
	#else
		#error ### no log function available ###
	#endif
	);
}

static int fnc_exp (qse_awk_rtx_t* run, const qse_cstr_t* fnm)
{
	return fnc_math_1 (
		run, fnm,
	#if defined(HAVE_EXPL)
		FNC_MATH_LD, (void*)expl
	#elif defined(HAVE_EXP)
		FNC_MATH_D, (void*)exp
	#elif defined(HAVE_EXPF)
		FNC_MATH_F, (void*)expf
	#else
		#error ### no exp function available ###
	#endif
	);
}

static int fnc_sqrt (qse_awk_rtx_t* run, const qse_cstr_t* fnm)
{
	return fnc_math_1 (
		run, fnm, 
	#if defined(HAVE_SQRTL)
		FNC_MATH_LD, (void*)sqrtl
	#elif defined(HAVE_SQRT)
		FNC_MATH_D, (void*)sqrt
	#elif defined(HAVE_SQRTF)
		FNC_MATH_F, (void*)sqrtf
	#else
		#error ### no sqrt function available ###
	#endif
	);
}

static int fnc_int (qse_awk_rtx_t* run, const qse_cstr_t* fnm)
{
	qse_size_t nargs;
	qse_awk_val_t* a0;
	qse_long_t lv;
	qse_real_t rv;
	qse_awk_val_t* r;
	int n;

	nargs = qse_awk_rtx_getnargs (run);
	QSE_ASSERT (nargs == 1);

	a0 = qse_awk_rtx_getarg (run, 0);

	n = qse_awk_rtx_valtonum (run, a0, &lv, &rv);
	if (n == -1) return -1;
	if (n == 1) lv = (qse_long_t)rv;

	r = qse_awk_rtx_makeintval (run, lv);
	if (r == QSE_NULL)
	{
		qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM);
		return -1;
	}

	qse_awk_rtx_setretval (run, r);
	return 0;
}

static int fnc_rand (qse_awk_rtx_t* run, const qse_cstr_t* fnm)
{
	qse_awk_val_t* r;

	/*
	rxtn_t* rxtn;
	rxtn = (rxtn_t*) QSE_XTN (run);
	r = qse_awk_rtx_makerealval (
		run, (qse_real_t)(rand_r(rxtn->seed) % RAND_MAX) / RAND_MAX );
	*/
	r = qse_awk_rtx_makerealval (
		run, (qse_real_t)(rand() % RAND_MAX) / RAND_MAX);
	if (r == QSE_NULL)
	{
		qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM);
		return -1;
	}

	qse_awk_rtx_setretval (run, r);
	return 0;
}

static int fnc_srand (qse_awk_rtx_t* run, const qse_cstr_t* fnm)
{
	qse_size_t nargs;
	qse_awk_val_t* a0;
	qse_long_t lv;
	qse_real_t rv;
	qse_awk_val_t* r;
	int n;
	unsigned int prev;
	rxtn_t* rxtn;

	rxtn = (rxtn_t*) QSE_XTN (run);
	nargs = qse_awk_rtx_getnargs (run);
	QSE_ASSERT (nargs == 0 || nargs == 1);

	prev = rxtn->seed;

	if (nargs == 1)
	{
		a0 = qse_awk_rtx_getarg (run, 0);

		n = qse_awk_rtx_valtonum (run, a0, &lv, &rv);
		if (n <= -1) return -1;
		if (n >= 1) lv = (qse_long_t)rv;

		rxtn->seed = lv;
	}
	else
	{
		qse_ntime_t now;

		if (qse_gettime(&now) <= -1) rxtn->seed >>= 1;
		else rxtn->seed = (unsigned int)now;
	}

        srand (rxtn->seed);

	r = qse_awk_rtx_makeintval (run, prev);
	if (r == QSE_NULL)
	{
		qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM);
		return -1;
	}

	qse_awk_rtx_setretval (run, r);
	return 0;
}

static int fnc_system (qse_awk_rtx_t* run, const qse_cstr_t* fnm)
{
	qse_size_t nargs;
	qse_awk_val_t* v;
	qse_char_t* str, * ptr, * end;
	qse_size_t len;
	int n = 0;

	nargs = qse_awk_rtx_getnargs (run);
	QSE_ASSERT (nargs == 1);
	
	v = qse_awk_rtx_getarg (run, 0);
	if (v->type == QSE_AWK_VAL_STR)
	{
		str = ((qse_awk_val_str_t*)v)->ptr;
		len = ((qse_awk_val_str_t*)v)->len;
	}
	else
	{
		str = qse_awk_rtx_valtocpldup (run, v, &len);
		if (str == QSE_NULL) return -1;
	}

	/* the target name contains a null character.
	 * make system return -1 */
	ptr = str; end = str + len;
	while (ptr < end)
	{
		if (*ptr == QSE_T('\0')) 
		{
			n = -1;
			goto skip_system;
		}

		ptr++;
	}

#if defined(_WIN32)
	n = _tsystem (str);
#elif defined(QSE_CHAR_IS_MCHAR)
	n = system (str);
#else
	{
		char* mbs;
		qse_size_t mbl;

		mbs = (char*) qse_awk_alloc (run->awk, len*5+1);
		if (mbs == QSE_NULL) 
		{
			n = -1;
			goto skip_system;
		}

		/* at this point, the string is guaranteed to be 
		 * null-terminating. so qse_wcstombs() can be used to convert
		 * the string, not qse_wcsntombsn(). */

		mbl = len * 5;
		if (qse_wcstombs (str, mbs, &mbl) != len && mbl >= len * 5) 
		{
			/* not the entire string is converted.
			 * mbs is not null-terminated properly. */
			n = -1;
			goto skip_system_mbs;
		}

		mbs[mbl] = '\0';
		n = system (mbs);

	skip_system_mbs:
		qse_awk_free (run->awk, mbs);
	}
#endif

skip_system:
	if (v->type != QSE_AWK_VAL_STR) QSE_AWK_FREE (run->awk, str);

	v = qse_awk_rtx_makeintval (run, (qse_long_t)n);
	if (v == QSE_NULL)
	{
		/*qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM);*/
		return -1;
	}

	qse_awk_rtx_setretval (run, v);
	return 0;
}

#define ADDFNC(awk,name,min,max,fnc) \
        if (qse_awk_addfnc (\
		(awk), (name), qse_strlen(name), \
		0, (min), (max), QSE_NULL, (fnc)) == QSE_NULL) return -1;

static int add_functions (qse_awk_t* awk)
{
        ADDFNC (awk, QSE_T("sin"),        1, 1, fnc_sin);
        ADDFNC (awk, QSE_T("cos"),        1, 1, fnc_cos);
        ADDFNC (awk, QSE_T("tan"),        1, 1, fnc_tan);
        ADDFNC (awk, QSE_T("atan"),       1, 1, fnc_atan);
        ADDFNC (awk, QSE_T("atan2"),      2, 2, fnc_atan2);
        ADDFNC (awk, QSE_T("log"),        1, 1, fnc_log);
        ADDFNC (awk, QSE_T("exp"),        1, 1, fnc_exp);
        ADDFNC (awk, QSE_T("sqrt"),       1, 1, fnc_sqrt);
        ADDFNC (awk, QSE_T("int"),        1, 1, fnc_int);
        ADDFNC (awk, QSE_T("rand"),       0, 0, fnc_rand);
        ADDFNC (awk, QSE_T("srand"),      0, 1, fnc_srand);
        ADDFNC (awk, QSE_T("system"),     1, 1, fnc_system);
	return 0;
}