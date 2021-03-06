#include <qse/awk/stdawk.h>
#include <qse/cmn/sio.h>
#include <qse/cmn/main.h>
#include "awk00.h"

static const qse_char_t* src = QSE_T(
	"function init() { a = 20; return a; }"
	"function main() { return ++a; }"
	"function fini() { print \"a in fini() =>\", ++a; return a; }"
);

static const qse_char_t* fnc[] = 
{
	QSE_T("init"),
	QSE_T("main"),
	QSE_T("main"),
	QSE_T("main"),
	QSE_T("main"),
	QSE_T("fini"),
};

static int awk_main (int argc, qse_char_t* argv[])
{
	qse_awk_t* awk = QSE_NULL;
	qse_awk_rtx_t* rtx = QSE_NULL;
	qse_awk_parsestd_t psin[2];
	int ret = -1, i, opt;

	/* create an awk object */
	awk = qse_awk_openstd (0, QSE_NULL);
	if (awk == QSE_NULL)  
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: cannot open awk\n"));
		goto oops;
	}

	/* get the awk's trait */
	qse_awk_getopt (awk, QSE_AWK_TRAIT, &opt);
	/* change the trait value to disallow BEGIN, END, pattern-action blocks */
	opt &= ~QSE_AWK_PABLOCK;
	/* update the trait */
	qse_awk_setopt (awk, QSE_AWK_TRAIT, &opt);

	/* prepare a script to parse */
	psin[0].type = QSE_AWK_PARSESTD_STR;
	psin[0].u.str.ptr = src;
	psin[0].u.str.len = qse_strlen(src);
	psin[1].type = QSE_AWK_PARSESTD_NULL;

	/* parse a script */
	if (qse_awk_parsestd (awk, psin, QSE_NULL) <= -1)
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: %s\n"), 
			qse_awk_geterrmsg(awk));
		goto oops;
	}

	/* create a runtime context */
	rtx = qse_awk_rtx_openstd (
		awk,
		0,
		QSE_T("awk03"),
		QSE_NULL, /* stdin */
		QSE_NULL, /* stdout */
		QSE_NULL  /* default cmgr */
	);
	if (rtx == QSE_NULL) 
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: %s\n"), qse_awk_geterrmsg(awk));
		goto oops;
	}
	
	/* call init() initially, followed by 4 calls to main(), 
	 * and a final call to fini() */
	for (i = 0; i < QSE_COUNTOF(fnc); i++)
	{
		qse_awk_val_t* v;
		qse_char_t* str;	
		qse_size_t len;

		/* call the function */
		v = qse_awk_rtx_call (rtx, fnc[i], QSE_NULL, 0);
		if (v == QSE_NULL)
		{
			qse_fprintf (QSE_STDERR, QSE_T("ERROR: %s\n"), qse_awk_rtx_geterrmsg(rtx));
			goto oops;
		}

		/* convert the return value to a string with duplication */
		str = qse_awk_rtx_valtostrdup (rtx, v, &len);

		/* clear the return value */
		qse_awk_rtx_refdownval (rtx, v);

		if (str == QSE_NULL)
		{
			qse_fprintf (QSE_STDERR, QSE_T("ERROR: %s\n"), qse_awk_rtx_geterrmsg(rtx));
			goto oops;
		}

		/* print the return value */
		qse_printf (QSE_T("return: [%.*s]\n"), (int)len, str);

		/* destroy the duplicated string */
		qse_awk_rtx_freemem (rtx, str);
	}	

	ret = 0;

oops:
	/* destroy a runtime context */
	if (rtx) qse_awk_rtx_close (rtx);

	/* destroy the awk object */
	if (awk) qse_awk_close (awk);

	return ret;
}

int qse_main (int argc, qse_achar_t* argv[])
{
	int x;
	init_awk_sample_locale ();
	qse_openstdsios ();
	x = qse_runmain (argc, argv, awk_main);
	qse_closestdsios ();
	return x;
}
