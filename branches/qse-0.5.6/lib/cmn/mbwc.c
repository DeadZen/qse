/*
 * $Id$
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

#include <qse/cmn/mbwc.h>
#include <qse/cmn/slmb.h>
#include <qse/cmn/utf8.h>
#include <qse/cmn/cp949.h>
#include <qse/cmn/cp950.h>
#include <qse/cmn/str.h>

/* TODO: there is no guarantee that slwc is a unicode charater or vice versa.
 *       the ctype handling functions should be made wide-character
 *       dependent. 
 */

/* TODO: binary cmgr -> simply expands a byte to wchar and vice versa. */

static qse_cmgr_t builtin_cmgr[] =
{
	{
		qse_slmbtoslwc,
		qse_slwctoslmb
	},

	{
		qse_utf8touc,
		qse_uctoutf8
	},

	{
		qse_cp949touc,
		qse_uctocp949
	},

	{
		qse_cp950touc,
		qse_uctocp950
	}
};

qse_cmgr_t* qse_slmbcmgr = &builtin_cmgr[0];
qse_cmgr_t* qse_utf8cmgr = &builtin_cmgr[1];
qse_cmgr_t* qse_cp949cmgr = &builtin_cmgr[2];
qse_cmgr_t* qse_cp950cmgr = &builtin_cmgr[3];

static qse_cmgr_t* dfl_cmgr = &builtin_cmgr[0];
static qse_cmgr_finder_t cmgr_finder = QSE_NULL;

qse_cmgr_t* qse_getdflcmgr (void)
{
	return dfl_cmgr;
}

void qse_setdflcmgr (qse_cmgr_t* cmgr)
{
	dfl_cmgr = (cmgr? cmgr: &builtin_cmgr[0]);
}

qse_cmgr_t* qse_findcmgr (const qse_char_t* name)
{
	if (name)
	{
		if (cmgr_finder)
		{
			qse_cmgr_t* cmgr;
			cmgr = cmgr_finder (name);
			if (cmgr) return cmgr;
		}

		if (qse_strcmp(name, QSE_T("")) == 0) return dfl_cmgr;
		if (qse_strcmp(name, QSE_T("utf8")) == 0) return qse_utf8cmgr;	
		if (qse_strcmp(name, QSE_T("cp949")) == 0) return qse_cp949cmgr;	
		if (qse_strcmp(name, QSE_T("cp950")) == 0) return qse_cp950cmgr;	
		if (qse_strcmp(name, QSE_T("slmb")) == 0) return qse_slmbcmgr;	
	}
	return QSE_NULL;
}

void qse_setcmgrfinder (qse_cmgr_finder_t finder)
{
	cmgr_finder = finder;
}

qse_cmgr_finder_t qse_getcmgrfinder (void)
{
	return cmgr_finder;
}

/* string conversion function using default character conversion manager */

int qse_mbstowcs (
	const qse_mchar_t* mbs, qse_size_t* mbslen,
	qse_wchar_t* wcs, qse_size_t* wcslen)
{
	return qse_mbstowcswithcmgr (mbs, mbslen, wcs, wcslen, dfl_cmgr);
}

int qse_mbstowcsall (
	const qse_mchar_t* mbs, qse_size_t* mbslen,
	qse_wchar_t* wcs, qse_size_t* wcslen)
{
	return qse_mbstowcsallwithcmgr (mbs, mbslen, wcs, wcslen, dfl_cmgr);
}

int qse_mbsntowcsn (
	const qse_mchar_t* mbs, qse_size_t* mbslen,
	qse_wchar_t* wcs, qse_size_t* wcslen)
{
	return qse_mbsntowcsnwithcmgr (mbs, mbslen, wcs, wcslen, dfl_cmgr);
}

int qse_mbsntowcsnall (
	const qse_mchar_t* mbs, qse_size_t* mbslen,
	qse_wchar_t* wcs, qse_size_t* wcslen)
{
	return qse_mbsntowcsnallwithcmgr (mbs, mbslen, wcs, wcslen, dfl_cmgr);
}

int qse_mbsntowcsnupto (
	const qse_mchar_t* mbs, qse_size_t* mbslen,
	qse_wchar_t* wcs, qse_size_t* wcslen, qse_wchar_t stopper)
{
	return qse_mbsntowcsnuptowithcmgr (
		mbs, mbslen, wcs, wcslen, stopper, dfl_cmgr);
}

qse_wchar_t* qse_mbstowcsdup (const qse_mchar_t* mbs, qse_mmgr_t* mmgr)
{
	return qse_mbstowcsdupwithcmgr (mbs, mmgr, dfl_cmgr);
}

qse_wchar_t* qse_mbstowcsalldup (const qse_mchar_t* mbs, qse_mmgr_t* mmgr)
{
	return qse_mbstowcsalldupwithcmgr (mbs, mmgr, dfl_cmgr);
}

qse_wchar_t* qse_mbsatowcsdup (const qse_mchar_t* mbs[], qse_mmgr_t* mmgr)
{
	return qse_mbsatowcsdupwithcmgr (mbs, mmgr, dfl_cmgr);
}

qse_wchar_t* qse_mbsatowcsalldup (const qse_mchar_t* mbs[], qse_mmgr_t* mmgr)
{
	return qse_mbsatowcsalldupwithcmgr (mbs, mmgr, dfl_cmgr);
}

/* -------------------------------------------------------------- */

int qse_wcstombs (
	const qse_wchar_t* wcs, qse_size_t* wcslen,
	qse_mchar_t* mbs, qse_size_t* mbslen)
{
	return qse_wcstombswithcmgr (wcs, wcslen, mbs, mbslen, dfl_cmgr);
}

int qse_wcsntombsn (
	const qse_wchar_t* wcs, qse_size_t* wcslen,
	qse_mchar_t* mbs, qse_size_t* mbslen)
{
	return qse_wcsntombsnwithcmgr (wcs, wcslen, mbs, mbslen, dfl_cmgr);
}

qse_mchar_t* qse_wcstombsdup (const qse_wchar_t* wcs, qse_mmgr_t* mmgr)
{
	return qse_wcstombsdupwithcmgr (wcs, mmgr, dfl_cmgr);
}

qse_mchar_t* qse_wcsatombsdup (const qse_wchar_t* wcs[], qse_mmgr_t* mmgr)
{
	return qse_wcsatombsdupwithcmgr (wcs, mmgr, dfl_cmgr);
}
