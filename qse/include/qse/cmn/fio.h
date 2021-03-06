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

#ifndef _QSE_CMN_FIO_H_
#define _QSE_CMN_FIO_H_

/** @file
 * This file defines a simple file I/O interface.
 */

#include <qse/types.h>
#include <qse/macros.h>

enum qse_fio_flag_t
{
	/* (1 << 0) to (1 << 7) reserved for qse_sio_flag_t. 
	 * see <qse/cmn/sio.h>. nerver use this value. */
	QSE_FIO_RESERVED      = 0xFF,

	/** treat the file name pointer as a handle pointer */
	QSE_FIO_HANDLE        = (1 << 8),

	/** treate the file name pointer as a pointer to file name
	 *  template to use when making a temporary file name */
	QSE_FIO_TEMPORARY     = (1 << 9),

	/** don't close an I/O handle in qse_fio_fini() and qse_fio_close() */
	QSE_FIO_NOCLOSE       = (1 << 10),

	/** treat the path name as a multi-byte string */
	QSE_FIO_MBSPATH       = (1 << 11), 

	/* normal open flags */
	QSE_FIO_READ          = (1 << 14),
	QSE_FIO_WRITE         = (1 << 15),
	QSE_FIO_APPEND        = (1 << 16),

	QSE_FIO_CREATE        = (1 << 17),
	QSE_FIO_TRUNCATE      = (1 << 18),
	QSE_FIO_EXCLUSIVE     = (1 << 19),
	QSE_FIO_SYNC          = (1 << 20),
	
	/* do not follow a symbolic link, only on a supported platform */
	QSE_FIO_NOFOLLOW      = (1 << 23),

	/* for WIN32 only. harmless(no effect) when used on other platforms */
	QSE_FIO_NOSHREAD      = (1 << 24),
	QSE_FIO_NOSHWRITE     = (1 << 25),
	QSE_FIO_NOSHDELETE    = (1 << 26),

	/* hints to OS. harmless(no effect) when used on unsupported platforms */
	QSE_FIO_RANDOM        = (1 << 27), /* hint that access be random */
	QSE_FIO_SEQUENTIAL    = (1 << 28)  /* hint that access is sequential */
};

enum qse_fio_errnum_t
{
	QSE_FIO_ENOERR = 0, /**< no error */
	QSE_FIO_EOTHER,     /**< other error */
	QSE_FIO_ENOIMPL,    /**< not implemented */
	QSE_FIO_ESYSERR,    /**< subsystem(system call) error */
	QSE_FIO_EINTERN,    /**< internal error */

	QSE_FIO_ENOMEM,     /**< out of memory */
	QSE_FIO_EINVAL,     /**< invalid parameter */
	QSE_FIO_EACCES,     /**< access denied */
	QSE_FIO_ENOENT,     /**< no such file */
	QSE_FIO_EEXIST,     /**< already exist */
	QSE_FIO_EINTR,      /**< interrupted */
	QSE_FIO_EPIPE,      /**< broken pipe */
	QSE_FIO_EAGAIN      /**< resource not available temporarily */
};
typedef enum qse_fio_errnum_t qse_fio_errnum_t;

enum qse_fio_std_t
{
	QSE_FIO_STDIN  = 0,
	QSE_FIO_STDOUT = 1,
	QSE_FIO_STDERR = 2
};
typedef enum qse_fio_std_t qse_fio_std_t;

/* seek origin */
enum qse_fio_ori_t
{
	QSE_FIO_BEGIN   = 0,
	QSE_FIO_CURRENT = 1,
	QSE_FIO_END     = 2
};
/* file origin for seek */
typedef enum qse_fio_ori_t qse_fio_ori_t;

enum qse_fio_mode_t
{
	QSE_FIO_SUID = 04000, /* set UID */
	QSE_FIO_SGID = 02000, /* set GID */
	QSE_FIO_SVTX = 01000, /* sticky bit */
	QSE_FIO_RUSR = 00400, /* can be read by owner */
	QSE_FIO_WUSR = 00200, /* can be written by owner */
	QSE_FIO_XUSR = 00100, /* can be executed by owner */
	QSE_FIO_RGRP = 00040, /* can be read by group */
	QSE_FIO_WGRP = 00020, /* can be written by group */
	QSE_FIO_XGRP = 00010, /* can be executed by group */
	QSE_FIO_ROTH = 00004, /* can be read by others */
	QSE_FIO_WOTH = 00002, /* can be written by others */
	QSE_FIO_XOTH = 00001  /* can be executed by others */
};

#if defined(_WIN32)
	/* <winnt.h> => typedef PVOID HANDLE; */
	typedef void* qse_fio_hnd_t;
#elif defined(__OS2__)
	/* <os2def.h> => typedef LHANDLE HFILE;
	                 typedef unsigned long LHANDLE; */
	typedef unsigned long qse_fio_hnd_t;
#elif defined(__DOS__)
	typedef int qse_fio_hnd_t;
#elif defined(vms) || defined(__vms)
	typedef void* qse_fio_hnd_t; /* struct FAB*, struct RAB* */
#else
	typedef int qse_fio_hnd_t;
#endif

/* file offset */
typedef qse_foff_t qse_fio_off_t;

typedef struct qse_fio_t qse_fio_t;
typedef struct qse_fio_lck_t qse_fio_lck_t;

struct qse_fio_t
{
	qse_mmgr_t*      mmgr;
	qse_fio_errnum_t errnum;
	qse_fio_hnd_t    handle;
	int              status; 
};

struct qse_fio_lck_t
{
	int            type;   /* READ, WRITE */
	qse_fio_off_t  offset; /* starting offset */
	qse_fio_off_t  length; /* length */
	qse_fio_ori_t  origin; /* origin */
};

#define QSE_FIO_HANDLE(fio) ((fio)->handle)

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * The qse_fio_open() function opens a file.
 * To open a file, you should set the flags with at least one of
 * QSE_FIO_READ, QSE_FIO_WRITE, QSE_FIO_APPEND.
 *
 * If the #QSE_FIO_HANDLE flag is set, the @a path parameter is interpreted
 * as a pointer to qse_fio_hnd_t.
 *
 * If the #QSE_FIO_TEMPORARY flag is set, the @a path parameter is 
 * interpreted as a path name template and an actual file name to open
 * is internally generated using the template. The @a path parameter 
 * is filled with the last actual path name attempted when the function
 * returns. So, you must not pass a constant string to the @a path 
 * parameter when #QSE_FIO_TEMPORARY is set.
 */
QSE_EXPORT qse_fio_t* qse_fio_open (
	qse_mmgr_t*       mmgr,
	qse_size_t        ext,
	const qse_char_t* path,
	int               flags,
	int               mode
);

/**
 * The qse_fio_close() function closes a file.
 */
QSE_EXPORT void qse_fio_close (
	qse_fio_t* fio
);

/**
 * The qse_fio_close() function opens a file into @a fio.
 */
QSE_EXPORT int qse_fio_init (
	qse_fio_t*        fio,
	qse_mmgr_t*       mmgr,
	const qse_char_t* path,
	int               flags,
	int               mode
);

/**
 * The qse_fio_close() function finalizes a file by closing the handle 
 * stored in @a fio.
 */
QSE_EXPORT void qse_fio_fini (
	qse_fio_t* fio
);

QSE_EXPORT qse_mmgr_t* qse_fio_getmmgr (
	qse_fio_t* fio
);

QSE_EXPORT void* qse_fio_getxtn (
	qse_fio_t* fio
);

QSE_EXPORT qse_fio_errnum_t qse_fio_geterrnum (
	const qse_fio_t* fio
);

/**
 * The qse_fio_gethandle() function returns the native file handle.
 */
QSE_EXPORT qse_fio_hnd_t qse_fio_gethandle (
	const qse_fio_t* fio
);


/**
 * The qse_fio_seek() function changes the current file position.
 */
QSE_EXPORT qse_fio_off_t qse_fio_seek (
	qse_fio_t*    fio,
	qse_fio_off_t offset,
	qse_fio_ori_t origin
);

/**
 * The qse_fio_truncate() function truncates a file to @a size.
 */
QSE_EXPORT int qse_fio_truncate (
	qse_fio_t*    fio,
	qse_fio_off_t size
);

/**
 * The qse_fio_read() function reads data.
 */
QSE_EXPORT qse_ssize_t qse_fio_read (
	qse_fio_t*  fio,
	void*       buf,
	qse_size_t  size
);

/**
 * The qse_fio_write() function writes data.
 */
QSE_EXPORT qse_ssize_t qse_fio_write (
	qse_fio_t*  fio,
	const void* data,
	qse_size_t  size
);

/**
 * The qse_fio_chmod() function changes the file mode.
 *
 * @note
 * On _WIN32, this function is implemented on the best-effort basis and 
 * returns an error on the following conditions:
 * - The file size is 0.
 * - The file is opened without #QSE_FIO_READ.
 */
QSE_EXPORT int qse_fio_chmod (
	qse_fio_t* fio,
	int        mode
);

/**
 * The qse_fio_sync() function synchronizes file contents into storage media
 * It is useful in determining the media error, without which qse_fio_close() 
 * may succeed despite such an error.
 */
QSE_EXPORT int qse_fio_sync (
	qse_fio_t* fio
);


/* TODO: qse_fio_lock, qse_fio_unlock */
QSE_EXPORT int qse_fio_lock ( 
	qse_fio_t*     fio, 
	qse_fio_lck_t* lck,
	int            flags
);

QSE_EXPORT int qse_fio_unlock (
	qse_fio_t*     fio,
	qse_fio_lck_t* lck,
	int            flags
);

QSE_EXPORT int qse_getstdfiohandle (
	qse_fio_std_t  std,
	qse_fio_hnd_t* hnd
);

#if defined(__cplusplus)
}
#endif

#endif
