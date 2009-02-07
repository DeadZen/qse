/*
 * $Id$
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

#include "sed.h"
#include "../cmn/mem.h"

QSE_IMPLEMENT_COMMON_FUNCTIONS (sed)

qse_sed_t* qse_sed_open (qse_mmgr_t* mmgr, qse_size_t xtn)
{
	qse_sed_t* sed;

	if (mmgr == QSE_NULL) 
	{
		mmgr = QSE_MMGR_GETDFL();

		QSE_ASSERTX (mmgr != QSE_NULL,
			"Set the memory manager with QSE_MMGR_SETDFL()");

		if (mmgr == QSE_NULL) return QSE_NULL;
	}

	sed = (qse_sed_t*) QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(qse_sed_t) + xtn);
	if (sed == QSE_NULL) return QSE_NULL;

	if (qse_sed_init (sed, mmgr) == QSE_NULL)
	{
		QSE_MMGR_FREE (sed->mmgr, sed);
		return QSE_NULL;
	}

	return sed;
}

void qse_sed_close (qse_sed_t* sed)
{
	qse_sed_fini (sed);
	QSE_MMGR_FREE (sed->mmgr, sed);
}

qse_sed_t* qse_sed_init (qse_sed_t* sed, qse_mmgr_t* mmgr)
{
	QSE_MEMSET (sed, 0, sizeof(*sed));
	sed->mmgr = mmgr;

	return sed;
}

void qse_sed_fini (qse_sed_t* sed)
{
}

static void compile (qse_sed_t* sed, const qse_char_t* cp, qse_char_t seof)
{
	qse_char_t c;

	if ((c = *cp++) == seof) return QSE_NULL; /* // */

	do
	{
		if (c == QSE_T('\0') || c == QSE_T('\n'))
		{
			/* premature end of text */
			return QSE_NULL; /* TODO: return an error..*/
		}

		if (c == QSE_T('\\')
		{
			if (ep >= end)
			{
				/* too many characters */
				return QSE_NULL; /* TODO: return an error..*/
			}

			*ep++ = c;

			/* TODO: more escaped characters */
			if ((c = *cp++) == QSE_T('n') c = QSE_T('n');
		}

		if (ep >= end)
		{
			/* too many characters */
			return QSE_NULL; /* TODO: return an error..*/
		}

		*ep++ = c;
	}
	while ((c = *cp++) != seof);

	*ep = QSE_T('\0');
	regcomp (expbuf);
}

static const qse_char_t* address (
	qse_sed_t* sed, const qse_char_t* cp, qse_sed_a_t* a)
{
	qse_char_t c;

	if ((c = *cp) == QSE_T('$'))
	{
		a->type = QSE_SED_A_DOL;
		cp++;
	}
	else if (c == QSE_T('/'))
	{
		cp++;
		a->type = (a->u.rex = compile(sed, c))? A_RE: A_LAST;
	}
	else if (c >= QSE_T('0') && c <= QSE_T('9'))
	{
		qse_sed_line_t lno = 0;
		do
		{
			lno = lno * 10 + c - QSE_T('0');
			cp++;
		}
		while ((c = *cp) >= QSE_T('0') && c <= QSE_T('9'))

		/* line number 0 is illegal */
		if (lno == 0) return QSE_NULL;

		a->type = QSE_SED_A_LINE;
		a->u.line = lno;
	}
	else
	{
		a->type = QSE_SED_A_NONE;
	}

	return cp;
}

static void fcomp (const qse_char_t* str)
{
	const qse_char_t* cp = str;

	while (1)
	{
		/* TODO: should use ISSPACE()?  or is it enough to
		 *       check for a ' ' and '\t' because the input 'str'
		 *       is just a line 
		 */
		while (*cp == QSE_T(' ') || *cp == QSE_T('\t')) cp++;

		if (*cp == QSE_T('\0') || *cp == QSE_T('#')) break;

		if (*cp == QSE_T(';'))
		{
			cp++;
			continue;
		}

		cp = address (sed, cp/*, &rep->ad1*/);
	}

}
