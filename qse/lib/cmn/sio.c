/*
 * $Id: sio.c,v 1.30 2006/01/15 06:51:35 bacon Ease $
 */

#include <qse/cmn/sio.h>
#include "mem.h"

static qse_ssize_t __sio_input (int cmd, void* arg, void* buf, qse_size_t size);
static qse_ssize_t __sio_output (int cmd, void* arg, void* buf, qse_size_t size);

#ifdef _WIN32
	#include <windows.h>
#endif

static qse_sio_t __sio_in = 
{
	QSE_NULL, /* mmgr */

	/* fio */
	{
		QSE_NULL,
	#ifdef _WIN32
		(HANDLE)STD_INPUT_HANDLE,
	#else
		0,
	#endif
	},

	/* tio */
	{
		QSE_NULL,
		0,

		__sio_input,
		__sio_output,
		&__sio_in,	
		&__sio_in,

		0,
		0,
		0,
		0,

		{ 0 },
		{ 0 }
	}
};

static qse_sio_t __sio_out = 
{
	QSE_NULL, /* mmgr */

	/* fio */
	{
		QSE_NULL,
	#ifdef _WIN32
		(HANDLE)STD_OUTPUT_HANDLE,
	#else
		1
	#endif
	},

	/* tio */
	{
		QSE_NULL,
		0,

		__sio_input,
		__sio_output,
		&__sio_out,	
		&__sio_out,

		0,
		0,
		0,
		0,

		{ 0 },
		{ 0 }
	}
};

static qse_sio_t __sio_err = 
{
	QSE_NULL, /* mmgr */

	/* fio */
	{
		QSE_NULL,
	#ifdef _WIN32
		(HANDLE)STD_ERROR_HANDLE,
	#else
		2
	#endif
	},

	/* tio */
	{
		QSE_NULL,
		0,

		__sio_input,
		__sio_output,
		&__sio_err,	
		&__sio_err,

		0,
		0,
		0,
		0,

		{ 0 },
		{ 0 }
	}
};

qse_sio_t* qse_sio_in = &__sio_in;
qse_sio_t* qse_sio_out = &__sio_out;
qse_sio_t* qse_sio_err = &__sio_err;

qse_sio_t* qse_sio_open (
	qse_mmgr_t* mmgr, qse_size_t ext, const qse_char_t* file, int flags)
{
	qse_sio_t* sio;

	if (mmgr == QSE_NULL)
	{
		mmgr = QSE_MMGR_GETDFL();

		QSE_ASSERTX (mmgr != QSE_NULL,
			"Set the memory manager with QSE_MMGR_SETDFL()");

		if (mmgr == QSE_NULL) return QSE_NULL;
	}

	sio = QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(qse_sio_t) + ext);
	if (sio == QSE_NULL) return QSE_NULL;

	if (qse_sio_init (sio, mmgr, file, flags) == QSE_NULL)
	{
		QSE_MMGR_FREE (mmgr, sio);
		return QSE_NULL;
	}

	return sio;
}

void qse_sio_close (qse_sio_t* sio)
{
	qse_sio_fini (sio);
	QSE_MMGR_FREE (sio->mmgr, sio);
}

qse_sio_t* qse_sio_init (
	qse_sio_t* sio, qse_mmgr_t* mmgr, const qse_char_t* file, int flags)
{
	QSE_MEMSET (sio, 0, QSE_SIZEOF(*sio));
	sio->mmgr = mmgr;

	if (qse_fio_init (&sio->fio, mmgr, file, flags, 0644) == QSE_NULL) 
	{
		return QSE_NULL;
	}

	if (qse_tio_init(&sio->tio, mmgr) == QSE_NULL) 
	{
		qse_fio_fini (&sio->fio);
		return QSE_NULL;
	}

	if (qse_tio_attachin(&sio->tio, __sio_input, sio) == -1 ||
	    qse_tio_attachout(&sio->tio, __sio_output, sio) == -1) 
	{
		qse_tio_fini (&sio->tio);	
		qse_fio_fini (&sio->fio);
		return QSE_NULL;
	}

	return sio;
}

void qse_sio_fini (qse_sio_t* sio)
{
	/*if (qse_sio_flush (sio) == -1) return -1;*/
	qse_sio_flush (sio);
	qse_tio_fini (&sio->tio);
	qse_fio_fini (&sio->fio);

	if (sio == qse_sio_in) qse_sio_in = QSE_NULL;
	else if (sio == qse_sio_out) qse_sio_out = QSE_NULL;
	else if (sio == qse_sio_err) qse_sio_err = QSE_NULL;
}

qse_sio_hnd_t qse_sio_gethandle (qse_sio_t* sio)
{
	/*return qse_fio_gethandle (&sio->fio);*/
	return QSE_FIO_HANDLE(&sio->fio);
}

qse_ssize_t qse_sio_flush (qse_sio_t* sio)
{
	return qse_tio_flush (&sio->tio);
}

void qse_sio_purge (qse_sio_t* sio)
{
	qse_tio_purge (&sio->tio);
}

qse_ssize_t qse_sio_getc (qse_sio_t* sio, qse_char_t* c)
{
	return qse_tio_getc (&sio->tio, c);
}

qse_ssize_t qse_sio_gets (qse_sio_t* sio, qse_char_t* buf, qse_size_t size)
{
	return qse_tio_gets (&sio->tio, buf, size);
}

qse_ssize_t qse_sio_getsx (qse_sio_t* sio, qse_char_t* buf, qse_size_t size)
{
	return qse_tio_getsx (&sio->tio, buf, size);
}

qse_ssize_t qse_sio_getstr (qse_sio_t* sio, qse_str_t* buf)
{
	return qse_tio_getstr (&sio->tio, buf);
}

qse_ssize_t qse_sio_putc (qse_sio_t* sio, qse_char_t c)
{
	return qse_tio_putc (&sio->tio, c);
}

qse_ssize_t qse_sio_puts (qse_sio_t* sio, const qse_char_t* str)
{
	return qse_tio_puts (&sio->tio, str);
}

qse_ssize_t qse_sio_putsx (qse_sio_t* sio, const qse_char_t* str, qse_size_t size)
{
	return qse_tio_putsx (&sio->tio, str, size);
}

#if 0
qse_ssize_t qse_sio_putsn (qse_sio_t* sio, ...)
{
	qse_ssize_t n;
	qse_va_list ap;

	qse_va_start (ap, sio);
	n = qse_tio_putsv (&sio->tio, ap);
	qse_va_end (ap);

	return n;
}

qse_ssize_t qse_sio_putsxn (qse_sio_t* sio, ...)
{
	qse_ssize_t n;
	qse_va_list ap;

	qse_va_start (ap, sio);
	n = qse_tio_putsxv (&sio->tio, ap);
	qse_va_end (ap);

	return n;
}

qse_ssize_t qse_sio_putsv (qse_sio_t* sio, qse_va_list ap)
{
	return qse_tio_putsv (&sio->tio, ap);
}

qse_ssize_t qse_sio_putsxv (qse_sio_t* sio, qse_va_list ap)
{
	return qse_tio_putsxv (&sio->tio, ap);
}

int qse_sio_getpos (qse_sio_t* sio, qse_siopos_t* pos)
{
	qse_off_t off;

	off = qse_fio_seek (&sio->fio, 0, QSE_FIO_SEEK_CURRENT);
	if (off == (qse_off_t)-1) return -1;

	*pos = off;
	return 0;
}

int qse_sio_setpos (qse_sio_t* sio, qse_siopos_t pos)
{
	if (qse_sio_flush(sio) == -1) return -1;
	return (qse_fio_seek (&sio->fio,
		pos, QSE_FIO_SEEK_BEGIN) == (qse_off_t)-1)? -1: 0;
}

int qse_sio_rewind (qse_sio_t* sio)
{
	if (qse_sio_flush(sio) == -1) return -1;
	return (qse_fio_seek (&sio->fio, 
		0, QSE_FIO_SEEK_BEGIN) == (qse_off_t)-1)? -1: 0;
}

int qse_sio_movetoend (qse_sio_t* sio)
{
	if (qse_sio_flush(sio) == -1) return -1;
	return (qse_fio_seek (&sio->fio, 
		0, QSE_FIO_SEEK_END) == (qse_off_t)-1)? -1: 0;
}
#endif

static qse_ssize_t __sio_input (int cmd, void* arg, void* buf, qse_size_t size)
{
	qse_sio_t* sio = (qse_sio_t*)arg;

	QSE_ASSERT (sio != QSE_NULL);

	if (cmd == QSE_TIO_IO_DATA) 
	{
	#ifdef _WIN32
		/* TODO: I hate this way of adjusting the handle value
		 *       Is there any good ways to do it statically? */
		HANDLE h = sio->fio.handle;
		if (h == (HANDLE)STD_INPUT_HANDLE ||
		    h == (HANDLE)STD_OUTPUT_HANDLE ||
		    h == (HANDLE)STD_ERROR_HANDLE)
		{
			h = GetStdHandle((DWORD)h);
			if (h != INVALID_HANDLE_VALUE && h != NULL) 
			{
				sio->fio.handle = h; 
			}
		}
	#endif
		return qse_fio_read (&sio->fio, buf, size);
	}

	return 0;
}

static qse_ssize_t __sio_output (int cmd, void* arg, void* buf, qse_size_t size)
{
	qse_sio_t* sio = (qse_sio_t*)arg;

	QSE_ASSERT (sio != QSE_NULL);

	if (cmd == QSE_TIO_IO_DATA) 
	{
	#ifdef _WIN32
		/* TODO: I hate this way of adjusting the handle value
		 *       Is there any good ways to do it statically? */
		HANDLE h = sio->fio.handle;
		if (h == (HANDLE)STD_INPUT_HANDLE ||
		    h == (HANDLE)STD_OUTPUT_HANDLE ||
		    h == (HANDLE)STD_ERROR_HANDLE)
		{
			h = GetStdHandle((DWORD)h);
			if (h != INVALID_HANDLE_VALUE && h != NULL) 
			{
				sio->fio.handle = h; 
			}
		}
	#endif
		return qse_fio_write (&sio->fio, buf, size);
	}

	return 0;
}
