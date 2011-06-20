/*
 * $Id: str_dynw.c 462 2011-05-18 14:36:40Z hyunghwan.chung $
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
    License along with QSE. If not, see <http://www.gnu.org/licenses/>.
 */

#include <qse/cmn/str.h>
#include "mem.h"

QSE_IMPLEMENT_COMMON_FUNCTIONS (wcs)

qse_wcs_t* qse_wcs_open (qse_mmgr_t* mmgr, qse_size_t ext, qse_size_t capa)
{
	qse_wcs_t* str;

	if (mmgr == QSE_NULL) 
	{
		mmgr = QSE_MMGR_GETDFL();

		QSE_ASSERTX (mmgr != QSE_NULL,
			"Set the memory manager with QSE_MMGR_SETDFL()");

		if (mmgr == QSE_NULL) return QSE_NULL;
	}

	str = (qse_wcs_t*) QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(qse_wcs_t) + ext);
	if (str == QSE_NULL) return QSE_NULL;

	if (qse_wcs_init (str, mmgr, capa) == QSE_NULL)
	{
		QSE_MMGR_FREE (mmgr, str);
		return QSE_NULL;
	}

	return str;
}

void qse_wcs_close (qse_wcs_t* str)
{
	qse_wcs_fini (str);
	QSE_MMGR_FREE (str->mmgr, str);
}

qse_wcs_t* qse_wcs_init (qse_wcs_t* str, qse_mmgr_t* mmgr, qse_size_t capa)
{
	if (mmgr == QSE_NULL) mmgr = QSE_MMGR_GETDFL();

	QSE_MEMSET (str, 0, QSE_SIZEOF(qse_wcs_t));

	str->mmgr = mmgr;
	str->sizer = QSE_NULL;

	if (capa == 0) str->val.ptr = QSE_NULL;
	else
	{
		str->val.ptr = (qse_wchar_t*) QSE_MMGR_ALLOC (
			mmgr, QSE_SIZEOF(qse_wchar_t) * (capa + 1));
		if (str->val.ptr == QSE_NULL) return QSE_NULL;
		str->val.ptr[0] = QSE_WT('\0');
	}

	str->val.len = 0;
	str->capa = capa;

	return str;
}

void qse_wcs_fini (qse_wcs_t* str)
{
	if (str->val.ptr != QSE_NULL) QSE_MMGR_FREE (str->mmgr, str->val.ptr);
}

int qse_wcs_yield (qse_wcs_t* str, qse_wxstr_t* buf, qse_size_t new_capa)
{
	qse_wchar_t* tmp;

	if (new_capa == 0) tmp = QSE_NULL;
	else
	{
		tmp = (qse_wchar_t*) QSE_MMGR_ALLOC (
			str->mmgr, QSE_SIZEOF(qse_wchar_t) * (new_capa + 1));
		if (tmp == QSE_NULL) return -1;
		tmp[0] = QSE_WT('\0');
	}

	if (buf != QSE_NULL)
	{
		buf->ptr = str->val.ptr;
		buf->len = str->val.len;
	}

	str->val.ptr = tmp;
	str->val.len = 0;
	str->capa = new_capa;

	return 0;
}

qse_wcs_sizer_t qse_wcs_getsizer (qse_wcs_t* str)
{
	return str->sizer;	
}

void qse_wcs_setsizer (qse_wcs_t* str, qse_wcs_sizer_t sizer)
{
	str->sizer = sizer;
}

qse_size_t qse_wcs_getcapa (qse_wcs_t* str)
{
	return str->capa;
}

qse_size_t qse_wcs_setcapa (qse_wcs_t* str, qse_size_t capa)
{
	qse_wchar_t* tmp;

	if (capa == str->capa) return capa;

	if (str->mmgr->realloc != QSE_NULL && str->val.ptr != QSE_NULL)
	{
		tmp = (qse_wchar_t*) QSE_MMGR_REALLOC (
			str->mmgr, str->val.ptr, 
			QSE_SIZEOF(qse_wchar_t)*(capa+1));
		if (tmp == QSE_NULL) return (qse_size_t)-1;
	}
	else
	{
		tmp = (qse_wchar_t*) QSE_MMGR_ALLOC (
			str->mmgr, QSE_SIZEOF(qse_wchar_t)*(capa+1));
		if (tmp == QSE_NULL) return (qse_size_t)-1;

		if (str->val.ptr != QSE_NULL)
		{
			qse_size_t ncopy = (str->val.len <= capa)? str->val.len: capa;
			QSE_MEMCPY (tmp, str->val.ptr, 
				QSE_SIZEOF(qse_wchar_t)*(ncopy+1));
			QSE_MMGR_FREE (str->mmgr, str->val.ptr);
		}
	}

	if (capa < str->val.len)
	{
		str->val.len = capa;
		tmp[capa] = QSE_WT('\0');
	}

	str->capa = capa;
	str->val.ptr = tmp;

	return str->capa;
}

qse_size_t qse_wcs_getlen (qse_wcs_t* str)
{
	return QSE_WCS_LEN (str);
}

qse_size_t qse_wcs_setlen (qse_wcs_t* str, qse_size_t len)
{
	if (len == str->val.len) return len;
	if (len < str->val.len) 
	{
		str->val.len = len;
		str->val.ptr[len] = QSE_WT('\0');	
		return len;
	}

	if (len > str->capa)
	{
		if (qse_wcs_setcapa (str, len) == (qse_size_t)-1) 
			return (qse_size_t)-1;
	}

	while (str->val.len < len) str->val.ptr[str->val.len++] = QSE_WT(' ');
	return str->val.len;
}

void qse_wcs_clear (qse_wcs_t* str)
{
	str->val.len = 0;
	if (str->val.ptr != QSE_NULL)
	{
		QSE_ASSERT (str->capa >= 1);
		str->val.ptr[0] = QSE_WT('\0');
	}
}

void qse_wcs_swap (qse_wcs_t* str, qse_wcs_t* str1)
{
	qse_wcs_t tmp;

	tmp.val.ptr = str->val.ptr;
	tmp.val.len = str->val.len;
	tmp.capa = str->capa;
	tmp.mmgr = str->mmgr;

	str->val.ptr = str1->val.ptr;
	str->val.len = str1->val.len;
	str->capa = str1->capa;
	str->mmgr = str1->mmgr;

	str1->val.ptr = tmp.val.ptr;
	str1->val.len = tmp.val.len;
	str1->capa = tmp.capa;
	str1->mmgr = tmp.mmgr;
}

qse_size_t qse_wcs_cpy (qse_wcs_t* str, const qse_wchar_t* s)
{
	/* TODO: improve it */
	return qse_wcs_ncpy (str, s, qse_wcslen(s));
}

qse_size_t qse_wcs_ncpy (qse_wcs_t* str, const qse_wchar_t* s, qse_size_t len)
{
	if (len > str->capa || str->val.ptr == QSE_NULL) 
	{
		qse_wchar_t* buf;

		buf = (qse_wchar_t*) QSE_MMGR_ALLOC (
			str->mmgr, QSE_SIZEOF(qse_wchar_t) * (len + 1));
		if (buf == QSE_NULL) return (qse_size_t)-1;

		if (str->val.ptr != QSE_NULL) QSE_MMGR_FREE (str->mmgr, str->val.ptr);
		str->capa = len;
		str->val.ptr = buf;
	}

	str->val.len = qse_wcsncpy (str->val.ptr, s, len);
	str->val.ptr[str->val.len] = QSE_WT('\0');
	return str->val.len;
}

qse_size_t qse_wcs_cat (qse_wcs_t* str, const qse_wchar_t* s)
{
	/* TODO: improve it */
	return qse_wcs_ncat (str, s, qse_wcslen(s));
}

qse_size_t qse_wcs_ncat (qse_wcs_t* str, const qse_wchar_t* s, qse_size_t len)
{
	if (len > str->capa - str->val.len) 
	{
		qse_size_t ncapa, mincapa;

		/* let the minimum capacity be as large as 
		 * to fit in the new substring */
		mincapa = str->val.len + len;

		if (str->sizer == QSE_NULL)
		{
			/* increase the capacity by the length to add */
			ncapa = mincapa;
			/* if the new capacity is less than the double,
			 * just double it */
			if (ncapa < str->capa * 2) ncapa = str->capa * 2;
		}
		else
		{
			/* let the user determine the new capacity.
			 * pass the minimum capacity required as a hint */
			ncapa = str->sizer (str, mincapa);
			/* if no change in capacity, return current length */
			if (ncapa == str->capa) return str->val.len;
		}

		/* change the capacity */
		do
		{
			if (qse_wcs_setcapa (str, ncapa) != (qse_size_t)-1) break;
			if (ncapa <= mincapa) return (qse_size_t)-1;
			ncapa--;
		}
		while (1);
	}

	if (len > str->capa - str->val.len) 
	{
		/* copy as many characters as the number of cells available.
		 * if the capacity has been decreased, len is adjusted here */
		len = str->capa - str->val.len;
	}

	if (len > 0)
	{
		QSE_MEMCPY (&str->val.ptr[str->val.len], s, len*QSE_SIZEOF(*s));
		str->val.len += len;
		str->val.ptr[str->val.len] = QSE_WT('\0');
	}

	return str->val.len;
}

qse_size_t qse_wcs_ccat (qse_wcs_t* str, qse_wchar_t c)
{
	return qse_wcs_ncat (str, &c, 1);
}

qse_size_t qse_wcs_nccat (qse_wcs_t* str, qse_wchar_t c, qse_size_t len)
{
	while (len > 0)
	{
		if (qse_wcs_ncat (str, &c, 1) == (qse_size_t)-1) 
		{
			return (qse_size_t)-1;
		}

		len--;
	}
	return str->val.len;
}

qse_size_t qse_wcs_del (qse_wcs_t* str, qse_size_t index, qse_size_t size)
{
	if (str->val.ptr != QSE_NULL && index < str->val.len && size > 0)
	{
		qse_size_t nidx = index + size;
		if (nidx >= str->val.len)
		{
			str->val.ptr[index] = QSE_WT('\0');
			str->val.len = index;
		}
		else
		{
			qse_wcsncpy (
				&str->val.ptr[index], &str->val.ptr[nidx],
				str->val.len - nidx);
			str->val.len -= size;
		}
	}

	return str->val.len;
}

qse_size_t qse_wcs_trm (qse_wcs_t* str)
{
	if (str->val.ptr != QSE_NULL)
	{
		str->val.len = qse_wcsxtrm (str->val.ptr, str->val.len);
	}

	return str->val.len;
}

qse_size_t qse_wcs_pac (qse_wcs_t* str)
{
	if (str->val.ptr != QSE_NULL)
	{
		str->val.len = qse_wcsxpac (str->val.ptr, str->val.len);
	}

	return str->val.len;
}

