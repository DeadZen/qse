/*
 * $Id: err.c 214 2009-06-27 02:50:54Z hyunghwan.chung $
 *
   Copyright 2006-2009 Chung, Hyung-Hwan.

   Licenawk under the Apache License, Version 2.0 (the "License");
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

const qse_char_t* qse_awk_dflerrstr (qse_awk_t* awk, qse_awk_errnum_t errnum)
{
	static const qse_char_t* errstr[] =
 	{
		QSE_T("no error"),
		QSE_T("unknown error"),

		QSE_T("invalid parameter or data"),
		QSE_T("insufficient memory"),
		QSE_T("not supported"),
		QSE_T("operation not allowed"),
		QSE_T("no such device"),
		QSE_T("no space left on device"),
		QSE_T("too many open files"),
		QSE_T("too many links"),
		QSE_T("resource temporarily unavailable"),
		QSE_T("'${0}' not existing"),
		QSE_T("'${0}' already exists"),
		QSE_T("file or data too big"),
		QSE_T("system too busy"),
		QSE_T("is a directory"),
		QSE_T("IO error"),

		QSE_T("cannot open '${0}'"),
		QSE_T("cannot read '${0}'"),
		QSE_T("cannot write '${0}'"),
		QSE_T("cannot close '${0}'"),
		
		QSE_T("internal error that should never have happened"),
		QSE_T("general runtime error"),
		QSE_T("block nested too deeply"),
		QSE_T("expressio nested too deeply"),

		QSE_T("cannot open source input"),
		QSE_T("cannot close source input"),
		QSE_T("cannot read source input"),

		QSE_T("cannot open source output"),
		QSE_T("cannot close source output"),
		QSE_T("cannot write source output"),

		QSE_T("invalid character '${0}'"),
		QSE_T("invalid digit '${0}'"),
		QSE_T("cannot unget character"),

		QSE_T("unexpected end of source"),
		QSE_T("a comment not closed properly"),
		QSE_T("a string or a regular expression not closed"),
		QSE_T("unexpected end of a regular expression"),
		QSE_T("a left brace expected in place of '${0}'"),
		QSE_T("a left parenthesis expected in place of '${0}'"),
		QSE_T("a right parenthesis expected in place of '${0}'"),
		QSE_T("a right bracket expected in place of '${0}'"),
		QSE_T("a comma expected in place of '${0}'"),
		QSE_T("a semicolon expected in place of '${0}'"),
		QSE_T("a colon expected in place of '${0}'"),
		QSE_T("statement not ending with a semicolon"),
		QSE_T("'in' expected in place of '${0}'"),
		QSE_T("right-hand side of the 'in' operator not a variable"),
		QSE_T("invalid expression"),

		QSE_T("keyword 'function' expected in place of '${0}'"),
		QSE_T("keyword 'while' expected in place of '${0}'"),
		QSE_T("invalid assignment statement"),
		QSE_T("an identifier expected in place of '${0}'"),
		QSE_T("'${0}' not a valid function name"),
		QSE_T("BEGIN not followed by a left bracket on the same line"),
		QSE_T("END not followed by a left bracket on the same line"),
		QSE_T("duplicate BEGIN"),
		QSE_T("duplicate END"),
		QSE_T("keyword '${0}' redefined"),
		QSE_T("intrinsic function '${0}' redefined"),
		QSE_T("function '${0}' redefined"),
		QSE_T("global variable '${0}' redefined"),
		QSE_T("parameter '${0}' redefined"),
		QSE_T("variable '${0}' redefined"),
		QSE_T("duplicate parameter name '${0}'"),
		QSE_T("duplicate global variable '${0}'"),
		QSE_T("duplicate local variable '${0}'"),
		QSE_T("'${0}' not a valid parameter name"),
		QSE_T("'${0}' not a valid variable name"),
		QSE_T("variable name missing"),
		QSE_T("undefined identifier '${0}'"),
		QSE_T("l-value required"),
		QSE_T("too many global variables"),
		QSE_T("too many local variables"),
		QSE_T("too many parameters"),
		QSE_T("delete statement not followed by a normal variable"),
		QSE_T("reset statement not followed by a normal variable"),
		QSE_T("break statement outside a loop"),
		QSE_T("continue statement outside a loop"),
		QSE_T("next statement illegal in the BEGIN block"),
		QSE_T("next statement illegal in the END block"),
		QSE_T("nextfile statement illegal in the BEGIN block"),
		QSE_T("nextfile statement illegal in the END block"),
		QSE_T("printf not followed by any arguments"),
		QSE_T("both prefix and postfix increment/decrement operator present"),

		QSE_T("divide by zero"),
		QSE_T("invalid operand"),
		QSE_T("wrong position index"),
		QSE_T("too few arguments"),
		QSE_T("too many arguments"),
		QSE_T("function '${0}' not found"),
		QSE_T("variable not indexable"),
		QSE_T("variable '${0}' not deletable"),
		QSE_T("value not a map"),
		QSE_T("right-hand side of the 'in' operator not a map"),
		QSE_T("right-hand side of the 'in' operator not a map nor nil"),
		QSE_T("value not referenceable"),
		QSE_T("value not assignable"),
		QSE_T("an indexed value cannot be assigned a map"),
		QSE_T("a positional value cannot be assigned a map"),
		QSE_T("map '${0}' not assignable with a scalar"),
		QSE_T("cannot change a scalar value to a map"),
		QSE_T("a map is not allowed"),
		QSE_T("invalid value type"),
		QSE_T("delete statement called with a wrong target"),
		QSE_T("reset statement called with a wrong target"),
		QSE_T("next statement called from the BEGIN block"),
		QSE_T("next statement called from the END block"),
		QSE_T("nextfile statement called from the BEGIN block"),
		QSE_T("nextfile statement called from the END block"),
		QSE_T("wrong implementation of intrinsic function handler"),
		QSE_T("intrinsic function handler returned an error"),
		QSE_T("wrong implementation of user-defined io handler"),
		QSE_T("IO handler returned an error"),
		QSE_T("no such IO name found"),
		QSE_T("IO name empty"),
		QSE_T("IO name '${0}' containing a null character"),
		QSE_T("not sufficient arguments to formatting sequence"),
		QSE_T("recursion detected in format conversion"),
		QSE_T("invalid character in CONVFMT"),
		QSE_T("invalid character in OFMT"),

		QSE_T("recursion too deep in the regular expression"),
		QSE_T("a right parenthesis expected in the regular expression"),
		QSE_T("a right bracket expected in the regular expression"),
		QSE_T("a right brace expected in the regular expression"),
		QSE_T("unbalanced parenthesis in the regular expression"),
		QSE_T("invalid brace in the regular expression"),
		QSE_T("a colon expected in the regular expression"),
		QSE_T("invalid character range in the regular expression"),
		QSE_T("invalid character class in the regular expression"),
		QSE_T("invalid boundary range in the regular expression"),
		QSE_T("unexpected end of the regular expression"),
		QSE_T("garbage after the regular expression")
	};

	return (errnum >= 0 && errnum < QSE_COUNTOF(errstr))?
		errstr[errnum]: QSE_T("unknown error");
}

qse_awk_errstr_t qse_awk_geterrstr (qse_awk_t* awk)
{
	return awk->errstr;
}

void qse_awk_seterrstr (qse_awk_t* awk, qse_awk_errstr_t errstr)
{
	awk->errstr = errstr;
}

qse_awk_errnum_t qse_awk_geterrnum (qse_awk_t* awk)
{
	return awk->errinf.num;
}

qse_size_t qse_awk_geterrlin (qse_awk_t* awk)
{
	return awk->errinf.lin;
}

const qse_char_t* qse_awk_geterrmsg (qse_awk_t* awk)
{
	return (awk->errinf.msg[0] == QSE_T('\0'))?
		qse_awk_geterrstr(awk)(awk,awk->errinf.num): awk->errinf.msg;
}

void qse_awk_geterrinf (qse_awk_t* awk, qse_awk_errinf_t* errinf)
{
	QSE_MEMCPY (errinf, &awk->errinf, QSE_SIZEOF(*errinf));
	if (errinf->msg[0] == QSE_T('\0'))
	{
		qse_strxcpy (errinf->msg, QSE_COUNTOF(errinf->msg),
			qse_awk_geterrstr(awk)(awk,awk->errinf.num));
	}
}

void qse_awk_geterror (
	qse_awk_t* awk, qse_awk_errnum_t* errnum, 
	qse_size_t* errlin, const qse_char_t** errmsg)
{
	if (errnum != QSE_NULL) *errnum = awk->errinf.num;
	if (errlin != QSE_NULL) *errlin = awk->errinf.lin;
	if (errmsg != QSE_NULL) 
	{
		*errmsg = (awk->errinf.msg[0] == QSE_T('\0'))?
			qse_awk_geterrstr(awk)(awk,awk->errinf.num):
			awk->errinf.msg;
	}
}

void qse_awk_seterrnum (qse_awk_t* awk, qse_awk_errnum_t errnum)
{
	awk->errinf.num = errnum;
	awk->errinf.lin = 0;
	awk->errinf.msg[0] = QSE_T('\0');
}

void qse_awk_seterrinf (qse_awk_t* awk, const qse_awk_errinf_t* errinf)
{
	QSE_MEMCPY (&awk->errinf, errinf, QSE_SIZEOF(*errinf));
}

void qse_awk_seterror (
	qse_awk_t* awk, qse_awk_errnum_t errnum,
	qse_size_t errlin, const qse_cstr_t* errarg)
{
	const qse_char_t* errfmt;

	awk->errinf.num = errnum;
	awk->errinf.lin = errlin;

	errfmt = qse_awk_geterrstr(awk)(awk,errnum);
	QSE_ASSERT (errfmt != QSE_NULL);
	qse_strxfncpy (
		awk->errinf.msg, QSE_COUNTOF(awk->errinf.msg),
		errfmt, errarg
	);
}

qse_awk_errnum_t qse_awk_rtx_geterrnum (qse_awk_rtx_t* rtx)
{
	return rtx->errinf.num;
}

qse_size_t qse_awk_rtx_geterrlin (qse_awk_rtx_t* rtx)
{
	return rtx->errinf.lin;
}

const qse_char_t* qse_awk_rtx_geterrmsg (qse_awk_rtx_t* rtx)
{
	return (rtx->errinf.msg[0] == QSE_T('\0')) ?
		qse_awk_geterrstr(rtx->awk)(rtx->awk,rtx->errinf.num): rtx->errinf.msg;
}

void qse_awk_rtx_geterrinf (qse_awk_rtx_t* rtx, qse_awk_errinf_t* errinf)
{
	QSE_MEMCPY (errinf, &rtx->errinf, QSE_SIZEOF(*errinf));
	if (errinf->msg[0] == QSE_T('\0'))
	{
		qse_strxcpy (errinf->msg, QSE_COUNTOF(errinf->msg),
			qse_awk_geterrstr(rtx->awk)(rtx->awk,rtx->errinf.num));
	}
}

void qse_awk_rtx_geterror (
	qse_awk_rtx_t* rtx, qse_awk_errnum_t* errnum, 
	qse_size_t* errlin, const qse_char_t** errmsg)
{
	if (errnum != QSE_NULL) *errnum = rtx->errinf.num;
	if (errlin != QSE_NULL) *errlin = rtx->errinf.lin;
	if (errmsg != QSE_NULL) 
	{
		*errmsg = (rtx->errinf.msg[0] == QSE_T('\0'))?
			qse_awk_geterrstr(rtx->awk)(rtx->awk,rtx->errinf.num): rtx->errinf.msg;
	}
}

void qse_awk_rtx_seterrnum (qse_awk_rtx_t* rtx, qse_awk_errnum_t errnum)
{
	rtx->errinf.num = errnum;
	rtx->errinf.lin = 0;
	rtx->errinf.msg[0] = QSE_T('\0');
}

void qse_awk_rtx_seterrinf (qse_awk_rtx_t* rtx, const qse_awk_errinf_t* errinf)
{
	QSE_MEMCPY (&rtx->errinf, errinf, QSE_SIZEOF(*errinf));
}

void qse_awk_rtx_seterror (
	qse_awk_rtx_t* rtx, qse_awk_errnum_t errnum, 
	qse_size_t errlin, const qse_cstr_t* errarg)
{
	const qse_char_t* errfmt;

	rtx->errinf.num = errnum;
	rtx->errinf.lin = errlin;

	errfmt = qse_awk_geterrstr(rtx->awk)(rtx->awk,errnum);
	QSE_ASSERT (errfmt != QSE_NULL);
	qse_strxfncpy (
		rtx->errinf.msg, QSE_COUNTOF(rtx->errinf.msg),
		errfmt, errarg
	);
}
