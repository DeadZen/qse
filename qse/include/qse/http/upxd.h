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

#ifndef _QSE_HTTP_UPXD_H_
#define _QSE_HTTP_UPXD_H_

#include <qse/types.h>
#include <qse/macros.h>
#include <qse/cmn/nwad.h>
#include <qse/cmn/time.h>

typedef struct qse_upxd_t qse_upxd_t;

typedef qse_intptr_t qse_upxd_hnd_t;

enum qse_upxd_errnum_t
{
	QSE_UPXD_ENOERR,
	QSE_UPXD_EOTHER,
	QSE_UPXD_ENOIMPL,
	QSE_UPXD_ESYSERR,
	QSE_UPXD_EINTERN,

	QSE_UPXD_ENOMEM,
	QSE_UPXD_EINVAL,
	QSE_UPXD_EACCES,
	QSE_UPXD_ENOENT,
	QSE_UPXD_EEXIST,
	QSE_UPXD_EINTR,
	QSE_UPXD_EAGAIN
};
typedef enum qse_upxd_errnum_t qse_upxd_errnum_t;

typedef struct qse_upxd_server_t qse_upxd_server_t;
typedef struct qse_upxd_session_t qse_upxd_session_t;

typedef struct qse_upxd_sock_t  qse_upxd_sock_t;
struct qse_upxd_sock_t
{
	qse_upxd_hnd_t         handle;
	qse_nwad_t        bind;
	const qse_char_t* dev;
	qse_nwad_t        from;
	qse_nwad_t        to;
};

struct qse_upxd_session_t
{
	/** the server that this session belongs to */
	qse_upxd_server_t* server; 
	
	/** client's address that initiated this session */
	qse_nwad_t client; 
	
	/* session configuration to be filled in upxd->cbs->config(). */
	struct
	{
		/** peer's address that the client wants to talk with */
		qse_nwad_t         peer;
		
		/** binding address for peer socket */
		qse_nwad_t         bind;

#define QSE_UPXD_SESSION_DEV_LEN  (31)
		/** binding device for peer socket */
		qse_char_t         dev[QSE_UPXD_SESSION_DEV_LEN + 1];

#define QSE_UPXD_SESSION_DORMANCY (30)
		/** session's idle-timeout */
		qse_ntime_t        dormancy;
	} config;
};

typedef int (*qse_upxd_muxcb_t) (
	qse_upxd_t* upxd,
	void*       mux,
	qse_upxd_hnd_t   handle,
	void*       cbarg
);

struct qse_upxd_cbs_t
{
	struct
	{
		int (*open) (qse_upxd_t* upxd, qse_upxd_sock_t* server);
		void (*close) (qse_upxd_t* upxd, qse_upxd_sock_t* server);

		qse_ssize_t (*recv) (
			qse_upxd_t* upxd, qse_upxd_sock_t* server,
			void* buf, qse_size_t bufsize);

		qse_ssize_t (*send) (
			qse_upxd_t* upxd, qse_upxd_sock_t* sock,
			const void* buf, qse_size_t bufsize);
	} sock;


	struct
	{
		int (*config) (qse_upxd_t* upxd, qse_upxd_session_t* session);
		void (*error) (qse_upxd_t* upxd, qse_upxd_session_t* session);
	} session;

	struct
	{
		void* (*open) (qse_upxd_t* upxd);
		void (*close) (qse_upxd_t* upxd, void* mux);
		int (*addhnd) (
			qse_upxd_t* upxd, void* mux, qse_upxd_hnd_t handle,
			qse_upxd_muxcb_t cbfun, void* cbarg);
		int (*delhnd) (qse_upxd_t* upxd, void* mux, qse_upxd_hnd_t handle);
		int (*poll) (qse_upxd_t* upxd, void* mux, qse_ntime_t timeout);
	} mux;
};

typedef struct qse_upxd_cbs_t qse_upxd_cbs_t;

#if defined(__cplusplus)
extern "C" {
#endif

QSE_EXPORT qse_upxd_t* qse_upxd_open (
	qse_mmgr_t* mmgr,   /**< memory manager */
	qse_size_t  xtnsize /**< extension size in bytes */
);

QSE_EXPORT void qse_upxd_close (
	qse_upxd_t* upxd
);

QSE_EXPORT qse_upxd_errnum_t qse_upxd_geterrnum (
	qse_upxd_t* upxd
);

QSE_EXPORT void qse_upxd_seterrnum (
	qse_upxd_t*       upxd,
	qse_upxd_errnum_t errnum
);

QSE_EXPORT qse_mmgr_t* qse_upxd_getmmgr (
	qse_upxd_t* upxd
); 

QSE_EXPORT void* qse_upxd_getxtn (
	qse_upxd_t* upxd
);

QSE_EXPORT qse_upxd_cbs_t* qse_upxd_getcbs (
	qse_upxd_t* upxd
);

QSE_EXPORT void qse_upxd_setcbs (
	qse_upxd_t*     upxd,
	qse_upxd_cbs_t* cbs
);

QSE_EXPORT void* qse_upxd_allocmem (
	qse_upxd_t* upxd,
	qse_size_t  size
);

QSE_EXPORT void* qse_upxd_reallocmem (
	qse_upxd_t* upxd,
	void*       ptr,
	qse_size_t  size
);

QSE_EXPORT void qse_upxd_freemem (
	qse_upxd_t* upxd,
	void*       ptr
);

QSE_EXPORT qse_upxd_server_t* qse_upxd_addserver (
	qse_upxd_t*       upxd,
	const qse_nwad_t* nwad,
	const qse_char_t* dev
);

QSE_EXPORT void qse_upxd_delserver (
	qse_upxd_t*        upxd,
	qse_upxd_server_t* server
);
	
QSE_EXPORT void* qse_upxd_getserverctx (
	qse_upxd_t*        upxd,
	qse_upxd_server_t* server
);

QSE_EXPORT void qse_upxd_setserverctx (
	qse_upxd_t*        upxd,
	qse_upxd_server_t* server,
	void*              ctx
);

QSE_EXPORT void qse_upxd_stop (
	qse_upxd_t* upxd
);

QSE_EXPORT int qse_upxd_loop (
	qse_upxd_t* upxd,
	qse_ntime_t timeout
);

QSE_EXPORT int qse_upxd_enableserver (
	qse_upxd_t*        upxd,
	qse_upxd_server_t* server
);

QSE_EXPORT int qse_upxd_disableserver (
	qse_upxd_t*        upxd,
	qse_upxd_server_t* server
);

QSE_EXPORT int qse_upxd_poll (
	qse_upxd_t*  upxd,
	qse_ntime_t  timeout
);

#if defined(__cplusplus)
}
#endif

#endif
