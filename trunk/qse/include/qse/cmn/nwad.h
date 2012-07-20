/*
 * $Id$
 * 
    Copyright 2006-2012 Chung, Hyung-Hwan.
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

#ifndef _QSE_CMN_NWAD_H_
#define _QSE_CMN_NWAD_H_

#include <qse/types.h>
#include <qse/macros.h>
#include <qse/cmn/ipad.h>

enum qse_nwad_type_t
{
	QSE_NWAD_IN4,
	QSE_NWAD_IN6
};
typedef enum qse_nwad_type_t qse_nwad_type_t;

typedef struct qse_nwad_t qse_nwad_t;

struct qse_nwad_t
{
	qse_nwad_type_t type;

	union
	{
		struct
		{
			qse_uint16_t port;
			qse_ipad4_t  addr;
		} in4;

		struct
		{
			qse_uint16_t port;
			qse_ipad6_t  addr;
			qse_uint32_t scope;
		} in6;	
	} u;	
};

enum qse_nwadtostr_flag_t
{
	QSE_NWADTOSTR_ADDR = (1 << 0),
#define QSE_NWADTOMBS_ADDR QSE_NWADTOSTR_ADDR
#define QSE_NWADTOWCS_ADDR QSE_NWADTOSTR_ADDR

	QSE_NWADTOSTR_PORT = (1 << 1),
#define QSE_NWADTOMBS_PORT QSE_NWADTOSTR_PORT
#define QSE_NWADTOWCS_PORT QSE_NWADTOSTR_PORT

	QSE_NWADTOSTR_ALL  = (QSE_NWADTOSTR_ADDR | QSE_NWADTOSTR_PORT)
#define QSE_NWADTOMBS_ALL  QSE_NWADTOSTR_ALL
#define QSE_NWADTOWCS_ALL  QSE_NWADTOSTR_ALL
};


#ifdef __cplusplus
extern "C" {
#endif

int qse_mbstonwad (
	const qse_mchar_t* mbs,
	qse_nwad_t*        nwad
);

int qse_mbsntonwad (
	const qse_mchar_t* mbs,
	qse_size_t         len,
	qse_nwad_t*        nwad
);

int qse_wcstonwad (
	const qse_wchar_t* wcs,
	qse_nwad_t*        nwad
);

int qse_wcsntonwad (
	const qse_wchar_t* wcs,
	qse_size_t         len,
	qse_nwad_t*        nwad
);

qse_size_t qse_nwadtombs (
	const qse_nwad_t* nwad,
	qse_mchar_t*      mbs,
	qse_size_t        len,
	int               flags
);

qse_size_t qse_nwadtowcs (
	const qse_nwad_t* nwad,
	qse_wchar_t*      wcs,
	qse_size_t        len,
	int               flags
);

#if defined(QSE_CHAR_IS_MCHAR)
#	define qse_strtonwad(ptr,nwad)           qse_mbstonwad(ptr,nwad)
#	define qse_strntonwad(ptr,len,nwad)      qse_mbsntonwad(ptr,len,nwad)
#	define qse_nwadtostr(nwad,ptr,len,flags) qse_nwadtombs(nwad,ptr,len,flags)
#else
#	define qse_strtonwad(ptr,nwad)           qse_wcstonwad(ptr,nwad)
#	define qse_strntonwad(ptr,len,nwad)      qse_wcsntonwad(ptr,len,nwad)
#	define qse_nwadtostr(nwad,ptr,len,flags) qse_nwadtowcs(nwad,ptr,len,flags)
#endif

#ifdef __cplusplus
}
#endif

#endif
