/*
 * $Id: htrd.h 223 2008-06-26 06:44:41Z baconevi $
 *
    Copyright 2006-2011 Chung, Hyung-Hwan.
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
    License along with QSE. If not, see <htrd://www.gnu.org/licenses/>.
 */

#ifndef _QSE_NET_HTTPD_H_
#define _QSE_NET_HTTPD_H_

#include <qse/types.h>
#include <qse/macros.h>

typedef struct qse_httpd_t qse_httpd_t;

enum qse_httpd_errnum_t
{
	QSE_HTTPD_ENOERR,
	QSE_HTTPD_ENOMEM,
	QSE_HTTPD_EINVAL,
	QSE_HTTPD_ESOCKET,
	QSE_HTTPD_EINTERN,
	QSE_HTTPD_ECOMCBS
};
typedef enum qse_httpd_errnum_t qse_httpd_errnum_t;

typedef struct qse_httpd_comcbs_t qse_httpd_comcbs_t;
struct qse_httpd_comcbs_t
{
	int (*open_listeners) (qse_httpd_t* httpd);
	int (*close_listeners) (qse_httpd_t* httpd);
};

#ifdef __cplusplus
extern "C" {
#endif

QSE_DEFINE_COMMON_FUNCTIONS (httpd)

/**
 * The qse_httpd_open() function creates a httpd processor.
 */
qse_httpd_t* qse_httpd_open (
	qse_mmgr_t* mmgr,   /**< memory manager */
	qse_size_t  xtnsize /**< extension size in bytes */
);

/**
 * The qse_httpd_close() function destroys a httpd processor.
 */
void qse_httpd_close (
	qse_httpd_t* httpd 
);

void qse_httpd_setcomcbs (
	qse_httpd_t* httpd,
	qse_httpd_comcbs_t* comcbs
);

int qse_httpd_loop (
	qse_httpd_t* httpd
);

/**
 * The qse_httpd_stop() function requests to stop qse_httpd_loop()
 */
void qse_httpd_stop (
	qse_httpd_t* httpd
);


int qse_httpd_addlisteners (
	qse_httpd_t*      httpd,
	const qse_char_t* uri
);

#ifdef __cplusplus
}
#endif

#endif