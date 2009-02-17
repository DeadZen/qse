/*
 * $Id: run.c 496 2008-12-15 09:56:48Z baconevi $
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

#include "awk.h"

#ifdef DEBUG_RUN
#include <qse/utl/stdio.h>
#endif

#define CMP_ERROR -99
#define DEF_BUF_CAPA 256
#define STACK_INCREMENT 512

#define IDXBUFSIZE 64

#define MMGR(run) ((run)->awk->mmgr)

#define STACK_AT(run,n) ((run)->stack[(run)->stack_base+(n)])
#define STACK_NARGS(run) (STACK_AT(run,3))
#define STACK_ARG(run,n) STACK_AT(run,3+1+(n))
#define STACK_LCL(run,n) STACK_AT(run,3+(qse_size_t)STACK_NARGS(run)+1+(n))
#define STACK_RETVAL(run) STACK_AT(run,2)
#define STACK_GBL(run,n) ((run)->stack[(n)])
#define STACK_RETVAL_GBL(run) ((run)->stack[(run)->awk->tree.ngbls+2])

enum exit_level_t
{
	EXIT_NONE,
	EXIT_BREAK,
	EXIT_CONTINUE,
	EXIT_FUNCTION,
	EXIT_NEXT,
	EXIT_GLOBAL,
	EXIT_ABORT
};

struct pafv
{
	qse_awk_val_t** args;
	qse_size_t      nargs;
};

#define DEFAULT_CONVFMT  QSE_T("%.6g")
#define DEFAULT_OFMT     QSE_T("%.6g")
#define DEFAULT_OFS      QSE_T(" ")
#define DEFAULT_ORS      QSE_T("\n")
#define DEFAULT_ORS_CRLF QSE_T("\r\n")
#define DEFAULT_SUBSEP   QSE_T("\034")

/* the index of a positional variable should be a positive interger
 * equal to or less than the maximum value of the type by which
 * the index is represented. but it has an extra check against the
 * maximum value of qse_size_t as the reference is represented
 * in a pointer variable of qse_awk_val_ref_t and sizeof(void*) is
 * equal to sizeof(qse_size_t). */
#define IS_VALID_POSIDX(idx) \
	((idx) >= 0 && \
	 (idx) < QSE_TYPE_MAX(qse_long_t) && \
	 (idx) < QSE_TYPE_MAX(qse_size_t))

static qse_size_t push_arg_from_vals (
	qse_awk_rtx_t* rtx, qse_awk_nde_call_t* call, void* data);

static int init_rtx (qse_awk_rtx_t* rtx, qse_awk_t* awk, qse_awk_rio_t* rio);
static void fini_rtx (qse_awk_rtx_t* rtx);

static int init_globals (qse_awk_rtx_t* rtx, const qse_cstr_t* arg);
static void fini_globals (qse_awk_rtx_t* rtx);

static int run_pattern_blocks  (qse_awk_rtx_t* run);
static int run_pattern_block_chain (
	qse_awk_rtx_t* run, qse_awk_chain_t* chain);
static int run_pattern_block (
	qse_awk_rtx_t* run, qse_awk_chain_t* chain, qse_size_t block_no);
static int run_block (qse_awk_rtx_t* run, qse_awk_nde_blk_t* nde);
static int run_block0 (qse_awk_rtx_t* run, qse_awk_nde_blk_t* nde);
static int run_statement (qse_awk_rtx_t* run, qse_awk_nde_t* nde);
static int run_if (qse_awk_rtx_t* run, qse_awk_nde_if_t* nde);
static int run_while (qse_awk_rtx_t* run, qse_awk_nde_while_t* nde);
static int run_for (qse_awk_rtx_t* run, qse_awk_nde_for_t* nde);
static int run_foreach (qse_awk_rtx_t* run, qse_awk_nde_foreach_t* nde);
static int run_break (qse_awk_rtx_t* run, qse_awk_nde_break_t* nde);
static int run_continue (qse_awk_rtx_t* run, qse_awk_nde_continue_t* nde);
static int run_return (qse_awk_rtx_t* run, qse_awk_nde_return_t* nde);
static int run_exit (qse_awk_rtx_t* run, qse_awk_nde_exit_t* nde);
static int run_next (qse_awk_rtx_t* run, qse_awk_nde_next_t* nde);
static int run_nextfile (qse_awk_rtx_t* run, qse_awk_nde_nextfile_t* nde);
static int run_delete (qse_awk_rtx_t* run, qse_awk_nde_delete_t* nde);
static int run_reset (qse_awk_rtx_t* run, qse_awk_nde_reset_t* nde);
static int run_print (qse_awk_rtx_t* run, qse_awk_nde_print_t* nde);
static int run_printf (qse_awk_rtx_t* run, qse_awk_nde_print_t* nde);

static int output_formatted (
	qse_awk_rtx_t* run, int out_type, const qse_char_t* dst, 
	const qse_char_t* fmt, qse_size_t fmt_len, qse_awk_nde_t* args);

static qse_awk_val_t* eval_expression (qse_awk_rtx_t* run, qse_awk_nde_t* nde);
static qse_awk_val_t* eval_expression0 (qse_awk_rtx_t* run, qse_awk_nde_t* nde);

static qse_awk_val_t* eval_group (qse_awk_rtx_t* run, qse_awk_nde_t* nde);

static qse_awk_val_t* eval_assignment (
	qse_awk_rtx_t* run, qse_awk_nde_t* nde);
static qse_awk_val_t* do_assignment (
	qse_awk_rtx_t* run, qse_awk_nde_t* var, qse_awk_val_t* val);
static qse_awk_val_t* do_assignment_scalar (
	qse_awk_rtx_t* run, qse_awk_nde_var_t* var, qse_awk_val_t* val);
static qse_awk_val_t* do_assignment_map (
	qse_awk_rtx_t* run, qse_awk_nde_var_t* var, qse_awk_val_t* val);
static qse_awk_val_t* do_assignment_pos (
	qse_awk_rtx_t* run, qse_awk_nde_pos_t* pos, qse_awk_val_t* val);

static qse_awk_val_t* eval_binary (
	qse_awk_rtx_t* run, qse_awk_nde_t* nde);
static qse_awk_val_t* eval_binop_lor (
	qse_awk_rtx_t* run, qse_awk_nde_t* left, qse_awk_nde_t* right);
static qse_awk_val_t* eval_binop_land (
	qse_awk_rtx_t* run, qse_awk_nde_t* left, qse_awk_nde_t* right);
static qse_awk_val_t* eval_binop_in (
	qse_awk_rtx_t* run, qse_awk_nde_t* left, qse_awk_nde_t* right);
static qse_awk_val_t* eval_binop_bor (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right);
static qse_awk_val_t* eval_binop_bxor (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right);
static qse_awk_val_t* eval_binop_band (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right);
static qse_awk_val_t* eval_binop_eq (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right);
static qse_awk_val_t* eval_binop_ne (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right);
static qse_awk_val_t* eval_binop_gt (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right);
static qse_awk_val_t* eval_binop_ge (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right);
static qse_awk_val_t* eval_binop_lt (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right);
static qse_awk_val_t* eval_binop_le (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right);
static qse_awk_val_t* eval_binop_lshift (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right);
static qse_awk_val_t* eval_binop_rshift (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right);
static qse_awk_val_t* eval_binop_plus (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right);
static qse_awk_val_t* eval_binop_minus (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right);
static qse_awk_val_t* eval_binop_mul (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right);
static qse_awk_val_t* eval_binop_div (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right);
static qse_awk_val_t* eval_binop_idiv (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right);
static qse_awk_val_t* eval_binop_mod (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right);
static qse_awk_val_t* eval_binop_exp (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right);
static qse_awk_val_t* eval_binop_concat (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right);
static qse_awk_val_t* eval_binop_ma (
	qse_awk_rtx_t* run, qse_awk_nde_t* left, qse_awk_nde_t* right);
static qse_awk_val_t* eval_binop_nm (
	qse_awk_rtx_t* run, qse_awk_nde_t* left, qse_awk_nde_t* right);
static qse_awk_val_t* eval_binop_match0 (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right,
	qse_size_t lline, qse_size_t rline, int ret);

static qse_awk_val_t* eval_unary (qse_awk_rtx_t* run, qse_awk_nde_t* nde);
static qse_awk_val_t* eval_incpre (qse_awk_rtx_t* run, qse_awk_nde_t* nde);
static qse_awk_val_t* eval_incpst (qse_awk_rtx_t* run, qse_awk_nde_t* nde);
static qse_awk_val_t* eval_cnd (qse_awk_rtx_t* run, qse_awk_nde_t* nde);

static qse_awk_val_t* eval_fun_ex (
	qse_awk_rtx_t* run, qse_awk_nde_t* nde, 
	void(*errhandler)(void*), void* eharg);

static qse_awk_val_t* eval_fnc (qse_awk_rtx_t* run, qse_awk_nde_t* nde);
static qse_awk_val_t* eval_fun (qse_awk_rtx_t* run, qse_awk_nde_t* nde);

static qse_awk_val_t* __eval_call (
	qse_awk_rtx_t* run,
	qse_awk_nde_t* nde, 
	const qse_char_t* fnc_arg_spec,
	qse_awk_fun_t* fun, 
	qse_size_t(*argpusher)(qse_awk_rtx_t*,qse_awk_nde_call_t*,void*),
	void* apdata,
	void(*errhandler)(void*),
	void* eharg);

static qse_awk_val_t* eval_call (
	qse_awk_rtx_t* run, qse_awk_nde_t* nde, 
	const qse_char_t* fnc_arg_spec, qse_awk_fun_t* fun,
	void(*errhandler)(void*), void* eharg);

static int get_reference (
	qse_awk_rtx_t* run, qse_awk_nde_t* nde, qse_awk_val_t*** ref);
static qse_awk_val_t** get_reference_indexed (
	qse_awk_rtx_t* run, qse_awk_nde_var_t* nde, qse_awk_val_t** val);

static qse_awk_val_t* eval_int (qse_awk_rtx_t* run, qse_awk_nde_t* nde);
static qse_awk_val_t* eval_real (qse_awk_rtx_t* run, qse_awk_nde_t* nde);
static qse_awk_val_t* eval_str (qse_awk_rtx_t* run, qse_awk_nde_t* nde);
static qse_awk_val_t* eval_rex (qse_awk_rtx_t* run, qse_awk_nde_t* nde);
static qse_awk_val_t* eval_named (qse_awk_rtx_t* run, qse_awk_nde_t* nde);
static qse_awk_val_t* eval_gbl (qse_awk_rtx_t* run, qse_awk_nde_t* nde);
static qse_awk_val_t* eval_lcl (qse_awk_rtx_t* run, qse_awk_nde_t* nde);
static qse_awk_val_t* eval_arg (qse_awk_rtx_t* run, qse_awk_nde_t* nde);
static qse_awk_val_t* eval_namedidx (qse_awk_rtx_t* run, qse_awk_nde_t* nde);
static qse_awk_val_t* eval_gblidx (qse_awk_rtx_t* run, qse_awk_nde_t* nde);
static qse_awk_val_t* eval_lclidx (qse_awk_rtx_t* run, qse_awk_nde_t* nde);
static qse_awk_val_t* eval_argidx (qse_awk_rtx_t* run, qse_awk_nde_t* nde);
static qse_awk_val_t* eval_pos (qse_awk_rtx_t* run, qse_awk_nde_t* nde);
static qse_awk_val_t* eval_getline (qse_awk_rtx_t* run, qse_awk_nde_t* nde);

static int __raw_push (qse_awk_rtx_t* run, void* val);
#define __raw_pop(run) \
	do { \
		QSE_ASSERT ((run)->stack_top > (run)->stack_base); \
		(run)->stack_top--; \
	} while (0)

static int read_record (qse_awk_rtx_t* run);
static int shorten_record (qse_awk_rtx_t* run, qse_size_t nflds);

static qse_char_t* idxnde_to_str (
	qse_awk_rtx_t* run, qse_awk_nde_t* nde, qse_char_t* buf, qse_size_t* len);

typedef qse_awk_val_t* (*binop_func_t) (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right);
typedef qse_awk_val_t* (*eval_expr_t) (qse_awk_rtx_t* run, qse_awk_nde_t* nde);

qse_size_t qse_awk_rtx_getnargs (qse_awk_rtx_t* run)
{
	return (qse_size_t) STACK_NARGS (run);
}

qse_awk_val_t* qse_awk_rtx_getarg (qse_awk_rtx_t* run, qse_size_t idx)
{
	return STACK_ARG (run, idx);
}

qse_awk_val_t* qse_awk_rtx_getgbl (qse_awk_rtx_t* run, int id)
{
	QSE_ASSERT (id >= 0 && id < (int)QSE_LDA_SIZE(run->awk->parse.gbls));
	return STACK_GBL (run, id);
}

/* internal function to set a value to a global variable.
 * this function can handle a few special global variables that
 * require special treatment. */
static int set_global (
	qse_awk_rtx_t* run, qse_size_t idx, 
	qse_awk_nde_var_t* var, qse_awk_val_t* val)
{
	qse_awk_val_t* old;
       
	old = STACK_GBL (run, idx);
	if (old->type == QSE_AWK_VAL_MAP)
	{	
		/* once a variable becomes a map,
		 * it cannot be changed to a scalar variable */

		if (var != QSE_NULL)
		{
			/* global variable */
			qse_cstr_t errarg;

			errarg.ptr = var->id.name.ptr; 
			errarg.len = var->id.name.len;

			qse_awk_rtx_seterror (run, 
				QSE_AWK_EMAPTOSCALAR, var->line, &errarg);
		}
		else
		{
			/* qse_awk_rtx_setgbl has been called */
			qse_cstr_t errarg;

			errarg.ptr = qse_awk_getgblname (
				run->awk, idx, &errarg.len);
			qse_awk_rtx_seterror (run, 
				QSE_AWK_EMAPTOSCALAR, 0, &errarg);
		}

		return -1;
	}

	/* builtin variables except ARGV cannot be assigned a map */
	if (val->type == QSE_AWK_VAL_MAP &&
	    (idx >= QSE_AWK_GBL_ARGC && idx <= QSE_AWK_GBL_SUBSEP) &&
	    idx != QSE_AWK_GBL_ARGV)
	{
		/* TODO: better error code */
		qse_awk_rtx_seterrnum (run, QSE_AWK_ESCALARTOMAP);
		return -1;
	}

	if (idx == QSE_AWK_GBL_CONVFMT)
	{
		qse_char_t* convfmt_ptr;
		qse_size_t convfmt_len, i;

		convfmt_ptr = qse_awk_rtx_valtostr (run, 
			val, QSE_AWK_VALTOSTR_CLEAR, QSE_NULL, &convfmt_len);
		if (convfmt_ptr == QSE_NULL) return  -1;

		for (i = 0; i < convfmt_len; i++)
		{
			if (convfmt_ptr[i] == QSE_T('\0'))
			{
				QSE_AWK_FREE (run->awk, convfmt_ptr);
				qse_awk_rtx_seterrnum (run, QSE_AWK_ECONVFMTCHR);
				return -1;
			}
		}

		if (run->gbl.convfmt.ptr != QSE_NULL)
			QSE_AWK_FREE (run->awk, run->gbl.convfmt.ptr);
		run->gbl.convfmt.ptr = convfmt_ptr;
		run->gbl.convfmt.len = convfmt_len;
	}
	else if (idx == QSE_AWK_GBL_FNR)
	{
		int n;
		qse_long_t lv;
		qse_real_t rv;

		n = qse_awk_rtx_valtonum (run, val, &lv, &rv);
		if (n == -1) return -1;
		if (n == 1) lv = (qse_long_t)rv;

		run->gbl.fnr = lv;
	}
	else if (idx == QSE_AWK_GBL_FS)
	{
		qse_char_t* fs_ptr;
		qse_size_t fs_len;

		if (val->type == QSE_AWK_VAL_STR)
		{
			fs_ptr = ((qse_awk_val_str_t*)val)->ptr;
			fs_len = ((qse_awk_val_str_t*)val)->len;
		}
		else
		{
			/* due to the expression evaluation rule, the 
			 * regular expression can not be an assigned value */
			QSE_ASSERT (val->type != QSE_AWK_VAL_REX);

			fs_ptr = qse_awk_rtx_valtostr (
				run, val, QSE_AWK_VALTOSTR_CLEAR, QSE_NULL, &fs_len);
			if (fs_ptr == QSE_NULL) return -1;
		}

		if (fs_len > 1)
		{
			void* rex;

			/* TODO: use safebuild */
			rex = QSE_AWK_BUILDREX (
				run->awk, fs_ptr, fs_len, &run->errnum);
			if (rex == QSE_NULL)
			{
				if (val->type != QSE_AWK_VAL_STR) 
					QSE_AWK_FREE (run->awk, fs_ptr);
				return -1;
			}

			if (run->gbl.fs != QSE_NULL) 
			{
				QSE_AWK_FREEREX (run->awk, run->gbl.fs);
			}
			run->gbl.fs = rex;
		}

		if (val->type != QSE_AWK_VAL_STR) QSE_AWK_FREE (run->awk, fs_ptr);
	}
	else if (idx == QSE_AWK_GBL_IGNORECASE)
	{
		if ((val->type == QSE_AWK_VAL_INT &&
		     ((qse_awk_val_int_t*)val)->val != 0) ||
		    (val->type == QSE_AWK_VAL_REAL &&
		     ((qse_awk_val_real_t*)val)->val != 0.0) ||
		    (val->type == QSE_AWK_VAL_STR &&
		     ((qse_awk_val_str_t*)val)->len != 0))
		{
			run->gbl.ignorecase = 1;
		}
		else
		{
			run->gbl.ignorecase = 0;
		}
	}
	else if (idx == QSE_AWK_GBL_NF)
	{
		int n;
		qse_long_t lv;
		qse_real_t rv;

		n = qse_awk_rtx_valtonum (run, val, &lv, &rv);
		if (n == -1) return -1;
		if (n == 1) lv = (qse_long_t)rv;

		if (lv < (qse_long_t)run->inrec.nflds)
		{
			if (shorten_record (run, (qse_size_t)lv) == -1)
			{
				/* adjust the error line */
				if (var != QSE_NULL) run->errlin = var->line;
				return -1;
			}
		}
	}
	else if (idx == QSE_AWK_GBL_NR)
	{
		int n;
		qse_long_t lv;
		qse_real_t rv;

		n = qse_awk_rtx_valtonum (run, val, &lv, &rv);
		if (n == -1) return -1;
		if (n == 1) lv = (qse_long_t)rv;

		run->gbl.nr = lv;
	}
	else if (idx == QSE_AWK_GBL_OFMT)
	{
		qse_char_t* ofmt_ptr;
		qse_size_t ofmt_len, i;

		ofmt_ptr = qse_awk_rtx_valtostr (
			run, val, QSE_AWK_VALTOSTR_CLEAR, QSE_NULL, &ofmt_len);
		if (ofmt_ptr == QSE_NULL) return  -1;

		for (i = 0; i < ofmt_len; i++)
		{
			if (ofmt_ptr[i] == QSE_T('\0'))
			{
				QSE_AWK_FREE (run->awk, ofmt_ptr);
				qse_awk_rtx_seterrnum (run, QSE_AWK_EOFMTCHR);
				return -1;
			}
		}

		if (run->gbl.ofmt.ptr != QSE_NULL)
			QSE_AWK_FREE (run->awk, run->gbl.ofmt.ptr);
		run->gbl.ofmt.ptr = ofmt_ptr;
		run->gbl.ofmt.len = ofmt_len;
	}
	else if (idx == QSE_AWK_GBL_OFS)
	{	
		qse_char_t* ofs_ptr;
		qse_size_t ofs_len;

		ofs_ptr = qse_awk_rtx_valtostr (
			run, val, QSE_AWK_VALTOSTR_CLEAR, QSE_NULL, &ofs_len);
		if (ofs_ptr == QSE_NULL) return  -1;

		if (run->gbl.ofs.ptr != QSE_NULL)
			QSE_AWK_FREE (run->awk, run->gbl.ofs.ptr);
		run->gbl.ofs.ptr = ofs_ptr;
		run->gbl.ofs.len = ofs_len;
	}
	else if (idx == QSE_AWK_GBL_ORS)
	{	
		qse_char_t* ors_ptr;
		qse_size_t ors_len;

		ors_ptr = qse_awk_rtx_valtostr (
			run, val, QSE_AWK_VALTOSTR_CLEAR, QSE_NULL, &ors_len);
		if (ors_ptr == QSE_NULL) return  -1;

		if (run->gbl.ors.ptr != QSE_NULL)
			QSE_AWK_FREE (run->awk, run->gbl.ors.ptr);
		run->gbl.ors.ptr = ors_ptr;
		run->gbl.ors.len = ors_len;
	}
	else if (idx == QSE_AWK_GBL_RS)
	{
		qse_char_t* rs_ptr;
		qse_size_t rs_len;

		if (val->type == QSE_AWK_VAL_STR)
		{
			rs_ptr = ((qse_awk_val_str_t*)val)->ptr;
			rs_len = ((qse_awk_val_str_t*)val)->len;
		}
		else
		{
			/* due to the expression evaluation rule, the 
			 * regular expression can not be an assigned value */
			QSE_ASSERT (val->type != QSE_AWK_VAL_REX);

			rs_ptr = qse_awk_rtx_valtostr (
				run, val, QSE_AWK_VALTOSTR_CLEAR, QSE_NULL, &rs_len);
			if (rs_ptr == QSE_NULL) return -1;
		}

		if (rs_len > 1)
		{
			void* rex;

			/* compile the regular expression */
			/* TODO: use safebuild */
			rex = QSE_AWK_BUILDREX (
				run->awk, rs_ptr, rs_len, &run->errnum);
			if (rex == QSE_NULL)
			{
				if (val->type != QSE_AWK_VAL_STR) 
					QSE_AWK_FREE (run->awk, rs_ptr);
				return -1;
			}

			if (run->gbl.rs != QSE_NULL) 
			{
				QSE_AWK_FREEREX (run->awk, run->gbl.rs);
			}
			run->gbl.rs = rex;
		}

		if (val->type != QSE_AWK_VAL_STR) QSE_AWK_FREE (run->awk, rs_ptr);
	}
	else if (idx == QSE_AWK_GBL_SUBSEP)
	{
		qse_char_t* subsep_ptr;
		qse_size_t subsep_len;

		subsep_ptr = qse_awk_rtx_valtostr (
			run, val, QSE_AWK_VALTOSTR_CLEAR, QSE_NULL, &subsep_len);
		if (subsep_ptr == QSE_NULL) return  -1;

		if (run->gbl.subsep.ptr != QSE_NULL)
			QSE_AWK_FREE (run->awk, run->gbl.subsep.ptr);
		run->gbl.subsep.ptr = subsep_ptr;
		run->gbl.subsep.len = subsep_len;
	}

	qse_awk_rtx_refdownval (run, old);
	STACK_GBL(run,idx) = val;
	qse_awk_rtx_refupval (run, val);

	return 0;
}

void qse_awk_rtx_setretval (qse_awk_rtx_t* run, qse_awk_val_t* val)
{
	qse_awk_rtx_refdownval (run, STACK_RETVAL(run));
	STACK_RETVAL(run) = val;
	/* should use the same trick as run_return */
	qse_awk_rtx_refupval (run, val); 
}

int qse_awk_rtx_setgbl (qse_awk_rtx_t* run, int id, qse_awk_val_t* val)
{
	QSE_ASSERT (id >= 0 && id < (int)QSE_LDA_SIZE(run->awk->parse.gbls));
	return set_global (run, (qse_size_t)id, QSE_NULL, val);
}

int qse_awk_rtx_setfilename (
	qse_awk_rtx_t* run, const qse_char_t* name, qse_size_t len)
{
	qse_awk_val_t* tmp;
	int n;

	if (len == 0) tmp = qse_awk_val_zls;
	else
	{
		tmp = qse_awk_rtx_makestrval (run, name, len);
		if (tmp == QSE_NULL) return -1;
	}

	qse_awk_rtx_refupval (run, tmp);
	n = qse_awk_rtx_setgbl (run, QSE_AWK_GBL_FILENAME, tmp);
	qse_awk_rtx_refdownval (run, tmp);

	return n;
}

int qse_awk_rtx_setofilename (
	qse_awk_rtx_t* run, const qse_char_t* name, qse_size_t len)
{
	qse_awk_val_t* tmp;
	int n;

	if (run->awk->option & QSE_AWK_NEXTOFILE)
	{
		if (len == 0) tmp = qse_awk_val_zls;
		else
		{
			tmp = qse_awk_rtx_makestrval (run, name, len);
			if (tmp == QSE_NULL) return -1;
		}

		qse_awk_rtx_refupval (run, tmp);
		n = qse_awk_rtx_setgbl (run, QSE_AWK_GBL_OFILENAME, tmp);
		qse_awk_rtx_refdownval (run, tmp);
	}
	else n = 0;

	return n;
}

void* qse_awk_rtx_getxtn (qse_awk_rtx_t* rtx)
{
	return QSE_XTN(rtx);
}

qse_awk_t* qse_awk_rtx_getawk (qse_awk_rtx_t* rtx)
{
	return rtx->awk;
}

qse_mmgr_t* qse_awk_rtx_getmmgr (qse_awk_rtx_t* rtx)
{
	return MMGR(rtx);
}

qse_map_t* qse_awk_rtx_getnvmap (qse_awk_rtx_t* rtx)
{
	return rtx->named;
}

qse_awk_rtx_t* qse_awk_rtx_open (
	qse_awk_t* awk, qse_size_t xtn, 
	qse_awk_rio_t* rio, const qse_cstr_t* arg)
{
	qse_awk_rtx_t* rtx;

        QSE_ASSERTX (awk->prm.pow != QSE_NULL, "Call qse_awk_setprm() first");
        QSE_ASSERTX (awk->prm.sprintf != QSE_NULL, "Call qse_awk_setprm() first");
        QSE_ASSERTX (awk->prm.isccls != QSE_NULL, "Call qse_awk_setprm() first");
        QSE_ASSERTX (awk->prm.toccls != QSE_NULL, "Call qse_awk_setprm() first");

	/* clear the awk error code */
	qse_awk_seterror (awk, QSE_AWK_ENOERR, 0, QSE_NULL);

	/* check if the code has ever been parsed */
	if (awk->tree.ngbls == 0 && 
	    awk->tree.begin == QSE_NULL &&
	    awk->tree.end == QSE_NULL &&
	    awk->tree.chain_size == 0 &&
	    qse_map_getsize(awk->tree.funs) == 0)
	{
		qse_awk_seterror (awk, QSE_AWK_ENOPER, 0, QSE_NULL);
		return QSE_NULL;
	}
	
	/* allocate the storage for the rtx object */
	rtx = (qse_awk_rtx_t*) QSE_AWK_ALLOC (
		awk, QSE_SIZEOF(qse_awk_rtx_t) + xtn);
	if (rtx == QSE_NULL)
	{
		/* if it fails, the failure is reported thru 
		 * the awk object */
		qse_awk_seterror (awk, QSE_AWK_ENOMEM, 0, QSE_NULL);
		return QSE_NULL;
	}


	/* initialize the run object */
	if (init_rtx (rtx, awk, rio) == -1) 
	{
		QSE_AWK_FREE (awk, rtx);
		return QSE_NULL;
	}

	/* clear the run error */
	rtx->errnum    = QSE_AWK_ENOERR;
	rtx->errlin    = 0;
	rtx->errmsg[0] = QSE_T('\0');

	if (init_globals (rtx, arg) == -1)
	{
		fini_rtx (rtx);
		QSE_AWK_FREE (awk, rtx);
		return QSE_NULL;
	}

	return rtx;
}

void qse_awk_rtx_close (qse_awk_rtx_t* rtx)
{
	fini_globals (rtx);
	fini_rtx (rtx);
	QSE_AWK_FREE (rtx->awk, rtx);
}

void qse_awk_rtx_stop (qse_awk_rtx_t* rtx)
{
	rtx->exit_level = EXIT_ABORT;
}

qse_bool_t qse_awk_rtx_shouldstop (qse_awk_rtx_t* rtx)
{
	return (rtx->exit_level == EXIT_ABORT || rtx->awk->stopall);
}

qse_awk_rcb_t* qse_awk_rtx_getrcb (qse_awk_rtx_t* rtx)
{
	return &rtx->rcb;
}

void qse_awk_rtx_setrcb (qse_awk_rtx_t* rtx, qse_awk_rcb_t* rcb)
{
	rtx->rcb = *rcb;
}

static void free_namedval (qse_map_t* map, void* dptr, qse_size_t dlen)
{
	qse_awk_rtx_refdownval (
		*(qse_awk_rtx_t**)QSE_XTN(map), dptr);
}

static void same_namedval (qse_map_t* map, void* dptr, qse_size_t dlen)
{
	qse_awk_rtx_refdownval_nofree (
		*(qse_awk_rtx_t**)QSE_XTN(map), dptr);
}

static int init_rtx (qse_awk_rtx_t* rtx, qse_awk_t* awk, qse_awk_rio_t* rio)
{
	/* zero out the runtime context excluding the extension */
	QSE_MEMSET (rtx, 0, QSE_SIZEOF(qse_awk_rtx_t));

	rtx->awk = awk;

	rtx->stack = QSE_NULL;
	rtx->stack_top = 0;
	rtx->stack_base = 0;
	rtx->stack_limit = 0;

	rtx->exit_level = EXIT_NONE;

	rtx->fcache_count = 0;
	/*rtx->scache32_count = 0;
	rtx->scache64_count = 0;*/
	rtx->vmgr.ichunk = QSE_NULL;
	rtx->vmgr.ifree = QSE_NULL;
	rtx->vmgr.rchunk = QSE_NULL;
	rtx->vmgr.rfree = QSE_NULL;

	rtx->errnum = QSE_AWK_ENOERR;
	rtx->errlin = 0;
	rtx->errmsg[0] = QSE_T('\0');

	rtx->inrec.buf_pos = 0;
	rtx->inrec.buf_len = 0;
	rtx->inrec.flds = QSE_NULL;
	rtx->inrec.nflds = 0;
	rtx->inrec.maxflds = 0;
	rtx->inrec.d0 = qse_awk_val_nil;
	if (qse_str_init (
		&rtx->inrec.line, MMGR(rtx), DEF_BUF_CAPA) == QSE_NULL)
	{
		qse_awk_seterror (awk, QSE_AWK_ENOMEM, 0, QSE_NULL);
		return -1;
	}

	if (qse_str_init (&rtx->format.out, MMGR(rtx), 256) == QSE_NULL)
	{
		qse_str_fini (&rtx->inrec.line);
		qse_awk_seterror (awk, QSE_AWK_ENOMEM, 0, QSE_NULL);
		return -1;
	}

	if (qse_str_init (&rtx->format.fmt, MMGR(rtx), 256) == QSE_NULL)
	{
		qse_str_fini (&rtx->format.out);
		qse_str_fini (&rtx->inrec.line);
		qse_awk_seterror (awk, QSE_AWK_ENOMEM, 0, QSE_NULL);
		return -1;
	}

	rtx->named = qse_map_open (
		MMGR(rtx), QSE_SIZEOF(rtx), 1024, 70);
	if (rtx->named == QSE_NULL)
	{
		qse_str_fini (&rtx->format.fmt);
		qse_str_fini (&rtx->format.out);
		qse_str_fini (&rtx->inrec.line);
		qse_awk_seterror (awk, QSE_AWK_ENOMEM, 0, QSE_NULL);
		return -1;
	}
	*(qse_awk_rtx_t**)QSE_XTN(rtx->named) = rtx;
	qse_map_setcopier (rtx->named, QSE_MAP_KEY, QSE_MAP_COPIER_INLINE);
	qse_map_setfreeer (rtx->named, QSE_MAP_VAL, free_namedval);
	qse_map_setkeeper (rtx->named, same_namedval);	
	qse_map_setscale (rtx->named, QSE_MAP_KEY, QSE_SIZEOF(qse_char_t));

	rtx->format.tmp.ptr = (qse_char_t*)
		QSE_AWK_ALLOC (rtx->awk, 4096*QSE_SIZEOF(qse_char_t*));
	if (rtx->format.tmp.ptr == QSE_NULL)
	{
		qse_map_close (rtx->named);
		qse_str_fini (&rtx->format.fmt);
		qse_str_fini (&rtx->format.out);
		qse_str_fini (&rtx->inrec.line);
		qse_awk_seterror (awk, QSE_AWK_ENOMEM, 0, QSE_NULL);
		return -1;
	}
	rtx->format.tmp.len = 4096;
	rtx->format.tmp.inc = 4096*2;

	if (rtx->awk->tree.chain_size > 0)
	{
		rtx->pattern_range_state = (qse_byte_t*) QSE_AWK_ALLOC (
			rtx->awk, rtx->awk->tree.chain_size*QSE_SIZEOF(qse_byte_t));
		if (rtx->pattern_range_state == QSE_NULL)
		{
			QSE_AWK_FREE (rtx->awk, rtx->format.tmp.ptr);
			qse_map_close (rtx->named);
			qse_str_fini (&rtx->format.fmt);
			qse_str_fini (&rtx->format.out);
			qse_str_fini (&rtx->inrec.line);
			qse_awk_seterror (awk, QSE_AWK_ENOMEM, 0, QSE_NULL);
			return -1;
		}

		QSE_MEMSET (
			rtx->pattern_range_state, 0, 
			rtx->awk->tree.chain_size * QSE_SIZEOF(qse_byte_t));
	}
	else rtx->pattern_range_state = QSE_NULL;

	if (rio != QSE_NULL)
	{
		rtx->rio.handler[QSE_AWK_RIO_PIPE] = rio->pipe;
		rtx->rio.handler[QSE_AWK_RIO_FILE] = rio->file;
		rtx->rio.handler[QSE_AWK_RIO_CONSOLE] = rio->console;
		rtx->rio.chain = QSE_NULL;
	}

	rtx->gbl.rs = QSE_NULL;
	rtx->gbl.fs = QSE_NULL;
	rtx->gbl.ignorecase = 0;

	rtx->depth.max.block = awk->run.depth.max.block;
	rtx->depth.max.expr = awk->run.depth.max.expr;
	rtx->depth.cur.block = 0; 
	rtx->depth.cur.expr = 0;

	return 0;
}

static void fini_rtx (qse_awk_rtx_t* rtx)
{
	if (rtx->pattern_range_state != QSE_NULL)
		QSE_AWK_FREE (rtx->awk, rtx->pattern_range_state);

	/* close all pending io's */
	/* TODO: what if this operation fails? */
	qse_awk_rtx_cleario (rtx);
	QSE_ASSERT (rtx->rio.chain == QSE_NULL);

	if (rtx->gbl.rs != QSE_NULL) 
	{
		QSE_AWK_FREE (rtx->awk, rtx->gbl.rs);
		rtx->gbl.rs = QSE_NULL;
	}
	if (rtx->gbl.fs != QSE_NULL)
	{
		QSE_AWK_FREE (rtx->awk, rtx->gbl.fs);
		rtx->gbl.fs = QSE_NULL;
	}

	if (rtx->gbl.convfmt.ptr != QSE_NULL &&
	    rtx->gbl.convfmt.ptr != DEFAULT_CONVFMT)
	{
		QSE_AWK_FREE (rtx->awk, rtx->gbl.convfmt.ptr);
		rtx->gbl.convfmt.ptr = QSE_NULL;
		rtx->gbl.convfmt.len = 0;
	}

	if (rtx->gbl.ofmt.ptr != QSE_NULL && 
	    rtx->gbl.ofmt.ptr != DEFAULT_OFMT)
	{
		QSE_AWK_FREE (rtx->awk, rtx->gbl.ofmt.ptr);
		rtx->gbl.ofmt.ptr = QSE_NULL;
		rtx->gbl.ofmt.len = 0;
	}

	if (rtx->gbl.ofs.ptr != QSE_NULL && 
	    rtx->gbl.ofs.ptr != DEFAULT_OFS)
	{
		QSE_AWK_FREE (rtx->awk, rtx->gbl.ofs.ptr);
		rtx->gbl.ofs.ptr = QSE_NULL;
		rtx->gbl.ofs.len = 0;
	}

	if (rtx->gbl.ors.ptr != QSE_NULL && 
	    rtx->gbl.ors.ptr != DEFAULT_ORS &&
	    rtx->gbl.ors.ptr != DEFAULT_ORS_CRLF)
	{
		QSE_AWK_FREE (rtx->awk, rtx->gbl.ors.ptr);
		rtx->gbl.ors.ptr = QSE_NULL;
		rtx->gbl.ors.len = 0;
	}

	if (rtx->gbl.subsep.ptr != QSE_NULL && 
	    rtx->gbl.subsep.ptr != DEFAULT_SUBSEP)
	{
		QSE_AWK_FREE (rtx->awk, rtx->gbl.subsep.ptr);
		rtx->gbl.subsep.ptr = QSE_NULL;
		rtx->gbl.subsep.len = 0;
	}

	QSE_AWK_FREE (rtx->awk, rtx->format.tmp.ptr);
	rtx->format.tmp.ptr = QSE_NULL;
	rtx->format.tmp.len = 0;
	qse_str_fini (&rtx->format.fmt);
	qse_str_fini (&rtx->format.out);

	/* destroy input record. qse_awk_rtx_clrrec should be called
	 * before the rtx stack has been destroyed because it may try
	 * to change the value to QSE_AWK_GBL_NF. */
	qse_awk_rtx_clrrec (rtx, QSE_FALSE);  
	if (rtx->inrec.flds != QSE_NULL) 
	{
		QSE_AWK_FREE (rtx->awk, rtx->inrec.flds);
		rtx->inrec.flds = QSE_NULL;
		rtx->inrec.maxflds = 0;
	}
	qse_str_fini (&rtx->inrec.line);

	/* destroy the stack if necessary */
	if (rtx->stack != QSE_NULL)
	{
		QSE_ASSERT (rtx->stack_top == 0);

		QSE_AWK_FREE (rtx->awk, rtx->stack);
		rtx->stack = QSE_NULL;
		rtx->stack_top = 0;
		rtx->stack_base = 0;
		rtx->stack_limit = 0;
	}

	/* destroy named variables */
	qse_map_close (rtx->named);

	/* destroy values in free list */
	while (rtx->fcache_count > 0)
	{
		qse_awk_val_ref_t* tmp = rtx->fcache[--rtx->fcache_count];
		qse_awk_rtx_freeval (rtx, (qse_awk_val_t*)tmp, QSE_FALSE);
	}

	/*while (rtx->scache32_count > 0)
	{
		qse_awk_val_str_t* tmp = rtx->scache32[--rtx->scache32_count];
		qse_awk_rtx_freeval (rtx, (qse_awk_val_t*)tmp, QSE_FALSE);
	}

	while (rtx->scache64_count > 0)
	{
		qse_awk_val_str_t* tmp = rtx->scache64[--rtx->scache64_count];
		qse_awk_rtx_freeval (rtx, (qse_awk_val_t*)tmp, QSE_FALSE);
	}*/

	qse_awk_rtx_freevalchunk (rtx, rtx->vmgr.ichunk);
	qse_awk_rtx_freevalchunk (rtx, rtx->vmgr.rchunk);
	rtx->vmgr.ichunk = QSE_NULL;
	rtx->vmgr.rchunk = QSE_NULL;
}

static int build_runarg (
	qse_awk_rtx_t* run, const qse_cstr_t* runarg, qse_size_t* nargs)
{
	const qse_cstr_t* p;
	qse_size_t argc;
	qse_awk_val_t* v_argc;
	qse_awk_val_t* v_argv;
	qse_awk_val_t* v_tmp;
	qse_char_t key[QSE_SIZEOF(qse_long_t)*8+2];
	qse_size_t key_len;

	v_argv = qse_awk_rtx_makemapval (run);
	if (v_argv == QSE_NULL) return -1;

	qse_awk_rtx_refupval (run, v_argv);

	if (runarg == QSE_NULL) argc = 0;
	else
	{
		for (argc = 0, p = runarg; p->ptr != QSE_NULL; argc++, p++)
		{
			v_tmp = qse_awk_rtx_makestrval (run, p->ptr, p->len);
			if (v_tmp == QSE_NULL)
			{
				qse_awk_rtx_refdownval (run, v_argv);
				return -1;
			}

			if (qse_awk_getoption(run->awk) & QSE_AWK_BASEONE)
			{
				key_len = qse_awk_longtostr (argc+1, 
					10, QSE_NULL, key, QSE_COUNTOF(key));
			}
			else
			{
				key_len = qse_awk_longtostr (argc, 
					10, QSE_NULL, key, QSE_COUNTOF(key));
			}
			QSE_ASSERT (key_len != (qse_size_t)-1);

			/* increment reference count of v_tmp in advance as if 
			 * it has successfully been assigned into ARGV. */
			qse_awk_rtx_refupval (run, v_tmp);

			if (qse_map_upsert (
				((qse_awk_val_map_t*)v_argv)->map,
				key, key_len, v_tmp, 0) == QSE_NULL)
			{
				/* if the assignment operation fails, decrements
				 * the reference of v_tmp to free it */
				qse_awk_rtx_refdownval (run, v_tmp);

				/* the values previously assigned into the
				 * map will be freeed when v_argv is freed */
				qse_awk_rtx_refdownval (run, v_argv);

				qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM);
				return -1;
			}
		}
	}

	v_argc = qse_awk_rtx_makeintval (run, (qse_long_t)argc);
	if (v_argc == QSE_NULL)
	{
		qse_awk_rtx_refdownval (run, v_argv);
		return -1;
	}

	qse_awk_rtx_refupval (run, v_argc);

	QSE_ASSERT (
		STACK_GBL(run,QSE_AWK_GBL_ARGC) == qse_awk_val_nil);

	if (qse_awk_rtx_setgbl (run, QSE_AWK_GBL_ARGC, v_argc) == -1) 
	{
		qse_awk_rtx_refdownval (run, v_argc);
		qse_awk_rtx_refdownval (run, v_argv);
		return -1;
	}

	if (qse_awk_rtx_setgbl (run, QSE_AWK_GBL_ARGV, v_argv) == -1)
	{
		/* ARGC is assigned nil when ARGV assignment has failed.
		 * However, this requires preconditions, as follows:
		 *  1. build_runarg should be called in a proper place
		 *     as it is not a generic-purpose routine.
		 *  2. ARGC should be nil before build_runarg is called 
		 * If the restoration fails, nothing can salvage it. */
		qse_awk_rtx_setgbl (run, QSE_AWK_GBL_ARGC, qse_awk_val_nil);
		qse_awk_rtx_refdownval (run, v_argc);
		qse_awk_rtx_refdownval (run, v_argv);
		return -1;
	}

	qse_awk_rtx_refdownval (run, v_argc);
	qse_awk_rtx_refdownval (run, v_argv);

	*nargs = argc;
	return 0;
}

static int update_fnr (qse_awk_rtx_t* rtx, qse_long_t fnr, qse_long_t nr)
{
	qse_awk_val_t* tmp1, * tmp2;

	tmp1 = qse_awk_rtx_makeintval (rtx, fnr);
	if (tmp1 == QSE_NULL) return -1;

	qse_awk_rtx_refupval (rtx, tmp1);

	if (nr == fnr) tmp2 = tmp1;
	else
	{
		tmp2 = qse_awk_rtx_makeintval (rtx, nr);
		if (tmp2 == QSE_NULL)
		{
			qse_awk_rtx_refdownval (rtx, tmp1);
			return -1;
		}

		qse_awk_rtx_refupval (rtx, tmp2);
	}

	if (qse_awk_rtx_setgbl (rtx, QSE_AWK_GBL_FNR, tmp1) == -1)
	{
		if (nr != fnr) qse_awk_rtx_refdownval (rtx, tmp2);
		qse_awk_rtx_refdownval (rtx, tmp1);
		return -1;
	}

	if (qse_awk_rtx_setgbl (rtx, QSE_AWK_GBL_NR, tmp2) == -1)
	{
		if (nr != fnr) qse_awk_rtx_refdownval (rtx, tmp2);
		qse_awk_rtx_refdownval (rtx, tmp1);
		return -1;
	}

	if (nr != fnr) qse_awk_rtx_refdownval (rtx, tmp2);
	qse_awk_rtx_refdownval (rtx, tmp1);
	return 0;
}

/* 
 * create global variables into the runtime stack
 * each variable is initialized to nil or zero.
 * ARGC and ARGV are built in this function
 */
static int prepare_globals (qse_awk_rtx_t* run, const qse_cstr_t* runarg)
{
	qse_size_t saved_stack_top;
	qse_size_t ngbls;
	qse_size_t nrunargs;

	saved_stack_top = run->stack_top;
	ngbls = run->awk->tree.ngbls;

	/* initialize all global variables to nil by push nils to the stack */
	while (ngbls > 0)
	{
		--ngbls;
		if (__raw_push(run,qse_awk_val_nil) == -1)
		{
			qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM);
			goto oops;
		}
	}	

	/* override NF to zero */
	if (qse_awk_rtx_setgbl (
		run, QSE_AWK_GBL_NF, qse_awk_val_zero) == -1) goto oops;

	/* override ARGC and ARGV if necessary */
	if (runarg && build_runarg (run, runarg, &nrunargs) == -1) goto oops;

	/* return success */
	return 0;

oops:
	/* restore the stack_top this way instead of calling __raw_pop()
	 * as many times as successful __raw_push(). it is ok because
	 * the values pushed so far are qse_awk_val_nils and qse_awk_val_zeros.
	 */
	run->stack_top = saved_stack_top;
	return -1;
}

/*
 * assign initial values to the global variables whose desired initial
 * values are not nil or zero. some are handled in prepare_globals () and 
 * update_fnr().
 */
static int defaultify_globals (qse_awk_rtx_t* rtx)
{
	struct gtab_t
	{
		int idx;
		const qse_char_t* str;
	} gtab[] =
	{
		{ QSE_AWK_GBL_CONVFMT,   DEFAULT_CONVFMT },
		{ QSE_AWK_GBL_FILENAME,  QSE_NULL },
		{ QSE_AWK_GBL_OFILENAME, QSE_NULL },
		{ QSE_AWK_GBL_OFMT,      DEFAULT_OFMT },
		{ QSE_AWK_GBL_OFS,       DEFAULT_OFS },
		{ QSE_AWK_GBL_ORS,       DEFAULT_ORS },
		{ QSE_AWK_GBL_SUBSEP,    DEFAULT_SUBSEP },
	};

	qse_awk_val_t* tmp;
	qse_size_t i, j;

	if (rtx->awk->option & QSE_AWK_CRLF)
	{
		/* ugly */
		gtab[5].str = DEFAULT_ORS_CRLF;
	}

	for (i = 0; i < QSE_COUNTOF(gtab); i++)
	{
		if (gtab[i].str == QSE_NULL || gtab[i].str[0] == QSE_T('\0'))
		{
			tmp = qse_awk_val_zls;
		}
		else 
		{
			tmp = qse_awk_rtx_makestrval0 (rtx, gtab[i].str);
			if (tmp == QSE_NULL) return -1;
		}
		
		qse_awk_rtx_refupval (rtx, tmp);

		QSE_ASSERT (STACK_GBL(rtx,gtab[i].idx) == qse_awk_val_nil);

		if (qse_awk_rtx_setgbl (rtx, gtab[i].idx, tmp) == -1)
		{
			for (j = 0; j < i; j++)
			{
				qse_awk_rtx_setgbl (
					rtx, gtab[i].idx, qse_awk_val_nil);
			}

			qse_awk_rtx_refdownval (rtx, tmp);
			return -1;
		}

		qse_awk_rtx_refdownval (rtx, tmp);
	}

	return 0;
}

static void refdown_globals (qse_awk_rtx_t* run, int pop)
{
	qse_size_t ngbls;
       
	ngbls = run->awk->tree.ngbls;
	while (ngbls > 0)
	{
		--ngbls;
		qse_awk_rtx_refdownval (run, STACK_GBL(run,ngbls));
		if (pop) __raw_pop (run);
		else STACK_GBL(run,ngbls) = qse_awk_val_nil;
	}
}

static int init_globals (qse_awk_rtx_t* rtx, const qse_cstr_t* arg)
{
	/* the stack must be clean when this function is invoked */
	QSE_ASSERTX (rtx->stack_base == 0, "stack not clean");
	QSE_ASSERTX (rtx->stack_top == 0, "stack not clean");

	if (prepare_globals (rtx, arg) == -1) return -1;
	if (update_fnr (rtx, 0, 0) == -1) goto oops;
	if (defaultify_globals (rtx) == -1) goto oops;
	return 0;

oops:
	refdown_globals (rtx, 1);
	return -1;
}

static void fini_globals (qse_awk_rtx_t* rtx)
{
	refdown_globals (rtx, 1);
}

struct capture_retval_data_t
{
	qse_awk_rtx_t* rtx;
	qse_awk_val_t* val;	
};

static void capture_retval_on_exit (void* arg)
{
	struct capture_retval_data_t* data;

	data = (struct capture_retval_data_t*)arg;
	data->val = STACK_RETVAL(data->rtx);
	qse_awk_rtx_refupval (data->rtx, data->val);
}


static int enter_stack_frame (qse_awk_rtx_t* run)
{
	qse_size_t saved_stack_top;

	/* remember the current stack top */
	saved_stack_top = run->stack_top;

	/* push the current stack base */
	if (__raw_push(run,(void*)run->stack_base) == -1) goto oops;

	/* push the current stack top before push the current stack base */
	if (__raw_push(run,(void*)saved_stack_top) == -1) goto oops;
	
	/* secure space for a return value */
	if (__raw_push(run,qse_awk_val_nil) == -1) goto oops;
	
	/* secure space for STACK_NARGS */
	if (__raw_push(run,qse_awk_val_nil) == -1) goto oops;

	/* let the stack top remembered be the base of a new stack frame */
	run->stack_base = saved_stack_top;
	return 0;

oops:
	/* restore the stack top in a cheesy(?) way. 
	 * it is ok to do so as the values pushed are
	 * nils and binary numbers. */
	run->stack_top = saved_stack_top;
	qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM);
	return -1;
}

static void exit_stack_frame (qse_awk_rtx_t* run)
{
	/* At this point, the current stack frame should have 
	 * the 4 entries pushed in enter_stack_frame(). */
	QSE_ASSERT ((run->stack_top-run->stack_base) == 4);

	run->stack_top = (qse_size_t)run->stack[run->stack_base+1];
	run->stack_base = (qse_size_t)run->stack[run->stack_base+0];
}

static int run_bpae_loop (qse_awk_rtx_t* rtx)
{
	qse_awk_nde_t* nde;
	qse_size_t nargs, i;
	qse_awk_val_t* v;
	int ret = 0;

	/* set nargs to zero */
	nargs = 0;
	STACK_NARGS(rtx) = (void*)nargs;

	/* call the callback */
	if (rtx->rcb.on_enter != QSE_NULL)
	{
		qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOERR);
		ret = rtx->rcb.on_enter (rtx, rtx->rcb.data);
		if (ret <= -1) 
		{
			if (rtx->errnum == QSE_AWK_ENOMEM)
				qse_awk_rtx_seterrnum (rtx, QSE_AWK_EUNKNOWN);
			ret = -1;
		}
	}

	/* execute the BEGIN block */
	for (nde = rtx->awk->tree.begin; 
	     ret == 0 && nde != QSE_NULL && rtx->exit_level < EXIT_GLOBAL;
	     nde = nde->next)
	{
		qse_awk_nde_blk_t* blk;

		blk = (qse_awk_nde_blk_t*)nde;
		QSE_ASSERT (blk->type == QSE_AWK_NDE_BLK);

		rtx->active_block = blk;
		rtx->exit_level = EXIT_NONE;
		if (run_block (rtx, blk) == -1) ret = -1;
	}

	if (ret == -1 && rtx->errnum == QSE_AWK_ENOERR) 
	{
		/* an error is returned with no error number set.
		 * this feature is used by eval_expression to
		 * abort the evaluation when exit() is executed 
		 * during function evaluation */
		ret = 0;
		rtx->errlin = 0;
		rtx->errmsg[0] = QSE_T('\0');
	}

	/* run pattern block loops */
	if (ret == 0 && 
	    (rtx->awk->tree.chain != QSE_NULL ||
	     rtx->awk->tree.end != QSE_NULL) && 
	     rtx->exit_level < EXIT_GLOBAL)
	{
		if (run_pattern_blocks(rtx) == -1) ret = -1;
	}

	if (ret == -1 && rtx->errnum == QSE_AWK_ENOERR)
	{
		/* an error is returned with no error number set.
		 * this feature is used by eval_expression to
		 * abort the evaluation when exit() is executed 
		 * during function evaluation */
		ret = 0;
		rtx->errlin = 0;
		rtx->errmsg[0] = QSE_T('\0');
	}

	/* execute END blocks. the first END block is executed if the 
	 * program is not explicitly aborted with qse_awk_rtx_stop().*/
	for (nde = rtx->awk->tree.end;
	     ret == 0 && nde != QSE_NULL && rtx->exit_level < EXIT_ABORT;
	     nde = nde->next) 
	{
		qse_awk_nde_blk_t* blk;

		blk = (qse_awk_nde_blk_t*)nde;
		QSE_ASSERT (blk->type == QSE_AWK_NDE_BLK);

		rtx->active_block = blk;
		rtx->exit_level = EXIT_NONE;
		if (run_block (rtx, blk) == -1) ret = -1;
		else if (rtx->exit_level >= EXIT_GLOBAL) 
		{
			/* once exit is called inside one of END blocks,
			 * subsequent END blocks must not be executed */
			break;
		}
	}

	if (ret == -1 && rtx->errnum == QSE_AWK_ENOERR)
	{
		/* an error is returned with no error number set.
		 * this feature is used by eval_expression to
		 * abort the evaluation when exit() is executed 
		 * during function evaluation */
		ret = 0;
		rtx->errlin = 0;
		rtx->errmsg[0] = QSE_T('\0');
	}

	/* derefrence all arguments. however, there should be no arguments 
	 * pushed to the stack as asserted below. we didn't push any arguments
	 * for BEGIN/pattern action/END block execution.*/
	nargs = (qse_size_t)STACK_NARGS(rtx);
	QSE_ASSERT (nargs == 0);
	for (i = 0; i < nargs; i++) 
		qse_awk_rtx_refdownval (rtx, STACK_ARG(rtx,i));

	/* get the return value in the current stack frame */
	v = STACK_RETVAL(rtx);

	if (rtx->rcb.on_exit != QSE_NULL)
	{
		/* we call the on_exit handler regardless of ret. 
		 * the return value passed is the global return value
		 * in the stack. */
		rtx->rcb.on_exit (rtx, v, rtx->rcb.data);
	}

	/* end the life of the global return value */
	qse_awk_rtx_refdownval (rtx, v);

	return ret;
}

/* start the BEGIN-pattern block-END loop */
int qse_awk_rtx_loop (qse_awk_rtx_t* rtx)
{
	int ret;

	rtx->exit_level = EXIT_NONE;

	ret = enter_stack_frame (rtx);
	if (ret == 0)
	{
		ret = run_bpae_loop (rtx);
		exit_stack_frame (rtx);
	}

	/* reset the exit level */
	rtx->exit_level = EXIT_NONE;
	return ret;
}

/* call an AWK function */
int qse_awk_rtx_call (
	qse_awk_rtx_t* rtx, const qse_char_t* name, 
	qse_awk_val_t** args, qse_size_t nargs)
{
	int ret = 0;
	qse_map_pair_t* pair;
	qse_awk_fun_t* fun;
	struct capture_retval_data_t crdata;
	qse_awk_val_t* v;
	struct pafv pafv = { args, nargs };
	qse_awk_nde_call_t call;

	if (rtx->exit_level >= EXIT_NEXT) 
	{
		/* cannot call the function again when exit() is called
		 * in an AWK program or qse_awk_rtx_stop() is invoked */
		qse_awk_rtx_seterror (rtx, QSE_AWK_ENOPER, 0, QSE_NULL);
		return -1;
	}
	/*rtx->exit_level = EXIT_NONE;*/

	/* forge a fake node containing a function call */
	QSE_MEMSET (&call, 0, QSE_SIZEOF(call));
	call.type = QSE_AWK_NDE_FUN;
	call.what.fun.name.ptr = (qse_char_t*)name;
	call.what.fun.name.len = qse_strlen(name);

	/* find the function */
	pair = qse_map_search (
		rtx->awk->tree.funs, 
		call.what.fun.name.ptr,
		call.what.fun.name.len
	);
	if (pair == QSE_NULL) 
	{
		qse_cstr_t errarg;

		errarg.ptr = call.what.fun.name.ptr;
		errarg.len = call.what.fun.name.len;

		qse_awk_rtx_seterror (rtx, QSE_AWK_EFUNNONE, 0, &errarg);
		return -1;
	}

	fun = (qse_awk_fun_t*)QSE_MAP_VPTR(pair);
	QSE_ASSERT (fun != QSE_NULL);

	/* check if the number of arguments given is more than expected */
	if (nargs > fun->nargs)
	{
		/* TODO: is this correct? what if i want to 
		 *       allow arbitarary numbers of arguments? */
		qse_awk_rtx_seterror (rtx, QSE_AWK_EARGTM, 0, QSE_NULL);
		return -1;
	}

	/* now that the function is found and ok, let's execute it */

	crdata.rtx = rtx;
	crdata.val = QSE_NULL;

	v = __eval_call (
		rtx, (qse_awk_nde_t*)&call, QSE_NULL, fun,
		push_arg_from_vals, (void*)&pafv,
		capture_retval_on_exit,
		&crdata
	);

	if (v == QSE_NULL) 
	{
		if (crdata.val == QSE_NULL) 
		{
			QSE_ASSERT (rtx->errnum != QSE_AWK_ENOERR);
			v = qse_awk_val_nil; /* defaults to nil */
			ret = -1;
		}
		else 
		{
			if (rtx->errnum == QSE_AWK_ENOERR)
			{
				/* exiting with exit() */
				v = crdata.val;
				/* no need to ref-up as it is done in 
				 * capture_retval_on_exit() */
			}
			else 
			{
				v = qse_awk_val_nil; /* defaults to nil */
				ret = -1;
			}
		}
	}
	else 
	{	
		/* the reference count of crdata.val is updated in 
		 * capture_retval_on_exit(). update reference count of 
		 * v here to balance it */
		qse_awk_rtx_refupval (rtx, v);
	}

	if (rtx->rcb.on_exit != QSE_NULL)
		rtx->rcb.on_exit (rtx, v, rtx->rcb.data);
	qse_awk_rtx_refdownval (rtx, v);

	return ret;
}

static int run_pattern_blocks (qse_awk_rtx_t* run)
{
	int n;

#define ADJUST_ERROR_LINE(run) \
	if (run->awk->tree.chain != QSE_NULL) \
	{ \
		if (run->awk->tree.chain->pattern != QSE_NULL) \
			run->errlin = run->awk->tree.chain->pattern->line; \
		else if (run->awk->tree.chain->action != QSE_NULL) \
			run->errlin = run->awk->tree.chain->action->line; \
	} \
	else if (run->awk->tree.end != QSE_NULL) \
	{ \
		run->errlin = run->awk->tree.end->line; \
	} 

	run->inrec.buf_pos = 0;
	run->inrec.buf_len = 0;
	run->inrec.eof = QSE_FALSE;

	/* run each pattern block */
	while (run->exit_level < EXIT_GLOBAL)
	{
		run->exit_level = EXIT_NONE;

		n = read_record (run);
		if (n == -1) 
		{
			ADJUST_ERROR_LINE (run);
			return -1; /* error */
		}
		if (n == 0) break; /* end of input */

		if (update_fnr (run, run->gbl.fnr+1, run->gbl.nr+1) == -1) 
		{
			ADJUST_ERROR_LINE (run);
			return -1;
		}

		if (run->awk->tree.chain != QSE_NULL)
		{
			if (run_pattern_block_chain (
				run, run->awk->tree.chain) == -1) return -1;
		}
	}

#undef ADJUST_ERROR_LINE
	return 0;
}

static int run_pattern_block_chain (qse_awk_rtx_t* run, qse_awk_chain_t* chain)
{
	qse_size_t block_no = 0;

	while (run->exit_level < EXIT_GLOBAL && chain != QSE_NULL)
	{
		if (run->exit_level == EXIT_NEXT)
		{
			run->exit_level = EXIT_NONE;
			break;
		}

		if (run_pattern_block (run, chain, block_no) == -1) return -1;

		chain = chain->next; 
		block_no++;
	}

	return 0;
}

static int run_pattern_block (
	qse_awk_rtx_t* run, qse_awk_chain_t* chain, qse_size_t block_no)
{
	qse_awk_nde_t* ptn;
	qse_awk_nde_blk_t* blk;

	ptn = chain->pattern;
	blk = (qse_awk_nde_blk_t*)chain->action;

	if (ptn == QSE_NULL)
	{
		/* just execute the block */
		run->active_block = blk;
		if (run_block (run, blk) == -1) return -1;
	}
	else
	{
		if (ptn->next == QSE_NULL)
		{
			/* pattern { ... } */
			qse_awk_val_t* v1;

			v1 = eval_expression (run, ptn);
			if (v1 == QSE_NULL) return -1;

			qse_awk_rtx_refupval (run, v1);

			if (qse_awk_rtx_valtobool (run, v1))
			{
				run->active_block = blk;
				if (run_block (run, blk) == -1) 
				{
					qse_awk_rtx_refdownval (run, v1);
					return -1;
				}
			}

			qse_awk_rtx_refdownval (run, v1);
		}
		else
		{
			/* pattern, pattern { ... } */
			QSE_ASSERT (ptn->next->next == QSE_NULL);
			QSE_ASSERT (run->pattern_range_state != QSE_NULL);

			if (run->pattern_range_state[block_no] == 0)
			{
				qse_awk_val_t* v1;

				v1 = eval_expression (run, ptn);
				if (v1 == QSE_NULL) return -1;
				qse_awk_rtx_refupval (run, v1);

				if (qse_awk_rtx_valtobool (run, v1))
				{
					run->active_block = blk;
					if (run_block (run, blk) == -1) 
					{
						qse_awk_rtx_refdownval (run, v1);
						return -1;
					}

					run->pattern_range_state[block_no] = 1;
				}

				qse_awk_rtx_refdownval (run, v1);
			}
			else if (run->pattern_range_state[block_no] == 1)
			{
				qse_awk_val_t* v2;

				v2 = eval_expression (run, ptn->next);
				if (v2 == QSE_NULL) return -1;
				qse_awk_rtx_refupval (run, v2);

				run->active_block = blk;
				if (run_block (run, blk) == -1) 
				{
					qse_awk_rtx_refdownval (run, v2);
					return -1;
				}

				if (qse_awk_rtx_valtobool (run, v2)) 
					run->pattern_range_state[block_no] = 0;

				qse_awk_rtx_refdownval (run, v2);
			}
		}
	}

	return 0;
}

static int run_block (qse_awk_rtx_t* run, qse_awk_nde_blk_t* nde)
{
	int n;

	if (run->depth.max.block > 0 &&
	    run->depth.cur.block >= run->depth.max.block)
	{
		qse_awk_rtx_seterror (
			run, QSE_AWK_EBLKNST, nde->line, QSE_NULL);
		return -1;;
	}

	run->depth.cur.block++;
	n = run_block0 (run, nde);
	run->depth.cur.block--;
	
	return n;
}

static int run_block0 (qse_awk_rtx_t* run, qse_awk_nde_blk_t* nde)
{
	qse_awk_nde_t* p;
	qse_size_t nlcls;
	qse_size_t saved_stack_top;
	int n = 0;

	if (nde == QSE_NULL)
	{
		/* blockless pattern - execute print $0*/
		qse_awk_rtx_refupval (run, run->inrec.d0);

		n = qse_awk_rtx_writeio_str (run, 
			QSE_AWK_OUT_CONSOLE, QSE_T(""),
			QSE_STR_PTR(&run->inrec.line),
			QSE_STR_LEN(&run->inrec.line));
		if (n == -1)
		{
			qse_awk_rtx_refdownval (run, run->inrec.d0);

			/* adjust the error line */
			run->errlin = nde->line;
			return -1;
		}

		n = qse_awk_rtx_writeio_str (
			run, QSE_AWK_OUT_CONSOLE, QSE_T(""),
			run->gbl.ors.ptr, run->gbl.ors.len);
		if (n == -1)
		{
			qse_awk_rtx_refdownval (run, run->inrec.d0);

			/* adjust the error line */
			run->errlin = nde->line;
			return -1;
		}

		qse_awk_rtx_refdownval (run, run->inrec.d0);
		return 0;
	}

	QSE_ASSERT (nde->type == QSE_AWK_NDE_BLK);

	p = nde->body;
	nlcls = nde->nlcls;

#ifdef DEBUG_RUN
	qse_dprintf (
		QSE_T("securing space for local variables nlcls = %d\n"), 
		(int)nlcls);
#endif

	saved_stack_top = run->stack_top;

	/* secure space for local variables */
	while (nlcls > 0)
	{
		--nlcls;
		if (__raw_push(run,qse_awk_val_nil) == -1)
		{
			/* restore stack top */
			run->stack_top = saved_stack_top;
			return -1;
		}

		/* refupval is not required for qse_awk_val_nil */
	}

#ifdef DEBUG_RUN
	qse_dprintf (QSE_T("executing block statements\n"));
#endif

	while (p != QSE_NULL && run->exit_level == EXIT_NONE)
	{
		if (run_statement (run, p) == -1) 
		{
			n = -1;
			break;
		}
		p = p->next;
	}

	/* pop off local variables */
#ifdef DEBUG_RUN
	qse_dprintf (QSE_T("popping off local variables\n"));
#endif
	nlcls = nde->nlcls;
	while (nlcls > 0)
	{
		--nlcls;
		qse_awk_rtx_refdownval (run, STACK_LCL(run,nlcls));
		__raw_pop (run);
	}

	return n;
}

#define ON_STATEMENT(rtx,nde) \
	if ((rtx)->awk->stopall) (rtx)->exit_level = EXIT_ABORT; \
	if ((rtx)->rcb.on_statement != QSE_NULL) \
	{ \
		(rtx)->rcb.on_statement ( \
			rtx, (nde)->line, (rtx)->rcb.data); \
	} 

static int run_statement (qse_awk_rtx_t* run, qse_awk_nde_t* nde)
{
	ON_STATEMENT (run, nde);

	switch (nde->type) 
	{
		case QSE_AWK_NDE_NULL:
		{
			/* do nothing */
			break;
		}

		case QSE_AWK_NDE_BLK:
		{
			if (run_block (run, 
				(qse_awk_nde_blk_t*)nde) == -1) return -1;
			break;
		}

		case QSE_AWK_NDE_IF:
		{
			if (run_if (run, 
				(qse_awk_nde_if_t*)nde) == -1) return -1;	
			break;
		}

		case QSE_AWK_NDE_WHILE:
		case QSE_AWK_NDE_DOWHILE:
		{
			if (run_while (run, 
				(qse_awk_nde_while_t*)nde) == -1) return -1;
			break;
		}

		case QSE_AWK_NDE_FOR:
		{
			if (run_for (run, 
				(qse_awk_nde_for_t*)nde) == -1) return -1;
			break;
		}

		case QSE_AWK_NDE_FOREACH:
		{
			if (run_foreach (run, 
				(qse_awk_nde_foreach_t*)nde) == -1) return -1;
			break;
		}

		case QSE_AWK_NDE_BREAK:
		{
			if (run_break (run, 
				(qse_awk_nde_break_t*)nde) == -1) return -1;
			break;
		}

		case QSE_AWK_NDE_CONTINUE:
		{
			if (run_continue (run, 
				(qse_awk_nde_continue_t*)nde) == -1) return -1;
			break;
		}

		case QSE_AWK_NDE_RETURN:
		{
			if (run_return (run, 
				(qse_awk_nde_return_t*)nde) == -1) return -1;
			break;
		}

		case QSE_AWK_NDE_EXIT:
		{
			if (run_exit (run, 
				(qse_awk_nde_exit_t*)nde) == -1) return -1;
			break;
		}

		case QSE_AWK_NDE_NEXT:
		{
			if (run_next (run, 
				(qse_awk_nde_next_t*)nde) == -1) return -1;
			break;
		}

		case QSE_AWK_NDE_NEXTFILE:
		{
			if (run_nextfile (run, 
				(qse_awk_nde_nextfile_t*)nde) == -1) return -1;
			break;
		}

		case QSE_AWK_NDE_DELETE:
		{
			if (run_delete (run, 
				(qse_awk_nde_delete_t*)nde) == -1) return -1;
			break;
		}

		case QSE_AWK_NDE_RESET:
		{
			if (run_reset (run, 
				(qse_awk_nde_reset_t*)nde) == -1) return -1;
			break;
		}


		case QSE_AWK_NDE_PRINT:
		{
			if (run_print (run, 
				(qse_awk_nde_print_t*)nde) == -1) return -1;
			break;
		}

		case QSE_AWK_NDE_PRINTF:
		{
			if (run_printf (run, 
				(qse_awk_nde_print_t*)nde) == -1) return -1;
			break;
		}

		default:
		{
			qse_awk_val_t* v;
			v = eval_expression (run, nde);
			if (v == QSE_NULL) return -1;

			/* destroy the value if not referenced */
			qse_awk_rtx_refupval (run, v);
			qse_awk_rtx_refdownval (run, v);

			break;
		}
	}

	return 0;
}

static int run_if (qse_awk_rtx_t* run, qse_awk_nde_if_t* nde)
{
	qse_awk_val_t* test;
	int n = 0;

	/* the test expression for the if statement cannot have 
	 * chained expressions. this should not be allowed by the
	 * parser first of all */
	QSE_ASSERT (nde->test->next == QSE_NULL);

	test = eval_expression (run, nde->test);
	if (test == QSE_NULL) return -1;

	qse_awk_rtx_refupval (run, test);
	if (qse_awk_rtx_valtobool (run, test))
	{
		n = run_statement (run, nde->then_part);
	}
	else if (nde->else_part != QSE_NULL)
	{
		n = run_statement (run, nde->else_part);
	}

	qse_awk_rtx_refdownval (run, test); /* TODO: is this correct?*/
	return n;
}

static int run_while (qse_awk_rtx_t* run, qse_awk_nde_while_t* nde)
{
	qse_awk_val_t* test;

	if (nde->type == QSE_AWK_NDE_WHILE)
	{
		/* no chained expressions are allowed for the test 
		 * expression of the while statement */
		QSE_ASSERT (nde->test->next == QSE_NULL);

		while (1)
		{
			ON_STATEMENT (run, nde->test);

			test = eval_expression (run, nde->test);
			if (test == QSE_NULL) return -1;

			qse_awk_rtx_refupval (run, test);

			if (qse_awk_rtx_valtobool (run, test))
			{
				if (run_statement(run,nde->body) == -1)
				{
					qse_awk_rtx_refdownval (run, test);
					return -1;
				}
			}
			else
			{
				qse_awk_rtx_refdownval (run, test);
				break;
			}

			qse_awk_rtx_refdownval (run, test);

			if (run->exit_level == EXIT_BREAK)
			{	
				run->exit_level = EXIT_NONE;
				break;
			}
			else if (run->exit_level == EXIT_CONTINUE)
			{
				run->exit_level = EXIT_NONE;
			}
			else if (run->exit_level != EXIT_NONE) break;

		}
	}
	else if (nde->type == QSE_AWK_NDE_DOWHILE)
	{
		/* no chained expressions are allowed for the test 
		 * expression of the while statement */
		QSE_ASSERT (nde->test->next == QSE_NULL);

		do
		{
			if (run_statement(run,nde->body) == -1) return -1;

			if (run->exit_level == EXIT_BREAK)
			{	
				run->exit_level = EXIT_NONE;
				break;
			}
			else if (run->exit_level == EXIT_CONTINUE)
			{
				run->exit_level = EXIT_NONE;
			}
			else if (run->exit_level != EXIT_NONE) break;

			ON_STATEMENT (run, nde->test);

			test = eval_expression (run, nde->test);
			if (test == QSE_NULL) return -1;

			qse_awk_rtx_refupval (run, test);

			if (!qse_awk_rtx_valtobool (run, test))
			{
				qse_awk_rtx_refdownval (run, test);
				break;
			}

			qse_awk_rtx_refdownval (run, test);
		}
		while (1);
	}

	return 0;
}

static int run_for (qse_awk_rtx_t* run, qse_awk_nde_for_t* nde)
{
	qse_awk_val_t* val;

	if (nde->init != QSE_NULL)
	{
		QSE_ASSERT (nde->init->next == QSE_NULL);

		ON_STATEMENT (run, nde->init);
		val = eval_expression(run,nde->init);
		if (val == QSE_NULL) return -1;

		qse_awk_rtx_refupval (run, val);
		qse_awk_rtx_refdownval (run, val);
	}

	while (1)
	{
		if (nde->test != QSE_NULL)
		{
			qse_awk_val_t* test;

			/* no chained expressions for the test expression of
			 * the for statement are allowed */
			QSE_ASSERT (nde->test->next == QSE_NULL);

			ON_STATEMENT (run, nde->test);
			test = eval_expression (run, nde->test);
			if (test == QSE_NULL) return -1;

			qse_awk_rtx_refupval (run, test);
			if (qse_awk_rtx_valtobool (run, test))
			{
				if (run_statement(run,nde->body) == -1)
				{
					qse_awk_rtx_refdownval (run, test);
					return -1;
				}
			}
			else
			{
				qse_awk_rtx_refdownval (run, test);
				break;
			}

			qse_awk_rtx_refdownval (run, test);
		}	
		else
		{
			if (run_statement(run,nde->body) == -1) return -1;
		}

		if (run->exit_level == EXIT_BREAK)
		{	
			run->exit_level = EXIT_NONE;
			break;
		}
		else if (run->exit_level == EXIT_CONTINUE)
		{
			run->exit_level = EXIT_NONE;
		}
		else if (run->exit_level != EXIT_NONE) break;

		if (nde->incr != QSE_NULL)
		{
			QSE_ASSERT (nde->incr->next == QSE_NULL);

			ON_STATEMENT (run, nde->incr);
			val = eval_expression (run, nde->incr);
			if (val == QSE_NULL) return -1;

			qse_awk_rtx_refupval (run, val);
			qse_awk_rtx_refdownval (run, val);
		}
	}

	return 0;
}

struct foreach_walker_t
{
	qse_awk_rtx_t* run;
	qse_awk_nde_t* var;
	qse_awk_nde_t* body;
	int ret;
};

static qse_map_walk_t walk_foreach (
	qse_map_t* map, qse_map_pair_t* pair, void* arg)
{
	struct foreach_walker_t* w = (struct foreach_walker_t*)arg;
	qse_awk_val_t* str;

	str = (qse_awk_val_t*) qse_awk_rtx_makestrval (
		w->run, QSE_MAP_KPTR(pair), QSE_MAP_KLEN(pair));
	if (str == QSE_NULL) 
	{
		/* adjust the error line */
		w->run->errlin = w->var->line;
		w->ret = -1;
		return QSE_MAP_WALK_STOP;
	}

	qse_awk_rtx_refupval (w->run, str);
	if (do_assignment (w->run, w->var, str) == QSE_NULL)
	{
		qse_awk_rtx_refdownval (w->run, str);
		w->ret = -1;
		return QSE_MAP_WALK_STOP;
	}

	if (run_statement (w->run, w->body) == -1)
	{
		qse_awk_rtx_refdownval (w->run, str);
		w->ret = -1;
		return QSE_MAP_WALK_STOP;
	}
	
	qse_awk_rtx_refdownval (w->run, str);
	return QSE_MAP_WALK_FORWARD;
}

static int run_foreach (qse_awk_rtx_t* run, qse_awk_nde_foreach_t* nde)
{
	qse_awk_nde_exp_t* test;
	qse_awk_val_t* rv;
	qse_map_t* map;
	struct foreach_walker_t walker;

	test = (qse_awk_nde_exp_t*)nde->test;
	QSE_ASSERT (
		test->type == QSE_AWK_NDE_EXP_BIN && 
		test->opcode == QSE_AWK_BINOP_IN);

	/* chained expressions should not be allowed 
	 * by the parser first of all */
	QSE_ASSERT (test->right->next == QSE_NULL); 

	rv = eval_expression (run, test->right);
	if (rv == QSE_NULL) return -1;

	qse_awk_rtx_refupval (run, rv);
	if (rv->type != QSE_AWK_VAL_MAP)
	{
		qse_awk_rtx_refdownval (run, rv);

		qse_awk_rtx_seterror (
			run, QSE_AWK_ENOTMAPIN, test->right->line, QSE_NULL);
		return -1;
	}
	map = ((qse_awk_val_map_t*)rv)->map;

	walker.run = run;
	walker.var = test->left;
	walker.body = nde->body;
	walker.ret = 0;
	qse_map_walk (map, walk_foreach, &walker);

	qse_awk_rtx_refdownval (run, rv);
	return walker.ret;
}

static int run_break (qse_awk_rtx_t* run, qse_awk_nde_break_t* nde)
{
	run->exit_level = EXIT_BREAK;
	return 0;
}

static int run_continue (qse_awk_rtx_t* run, qse_awk_nde_continue_t* nde)
{
	run->exit_level = EXIT_CONTINUE;
	return 0;
}

static int run_return (qse_awk_rtx_t* run, qse_awk_nde_return_t* nde)
{
	if (nde->val != QSE_NULL)
	{
		qse_awk_val_t* val;

		/* chained expressions should not be allowed 
		 * by the parser first of all */
		QSE_ASSERT (nde->val->next == QSE_NULL); 

		val = eval_expression (run, nde->val);
		if (val == QSE_NULL) return -1;

		if ((run->awk->option & QSE_AWK_MAPTOVAR) == 0)
		{
			if (val->type == QSE_AWK_VAL_MAP)
			{
				/* cannot return a map */
				qse_awk_rtx_refupval (run, val);
				qse_awk_rtx_refdownval (run, val);

				qse_awk_rtx_seterror (
					run, QSE_AWK_EMAPNOTALLOWED, 
					nde->line, QSE_NULL);
				return -1;
			}
		}

		qse_awk_rtx_refdownval (run, STACK_RETVAL(run));
		STACK_RETVAL(run) = val;

		/* NOTE: see eval_call() for the trick */
		qse_awk_rtx_refupval (run, val); 
	}
	
	run->exit_level = EXIT_FUNCTION;
	return 0;
}

static int run_exit (qse_awk_rtx_t* run, qse_awk_nde_exit_t* nde)
{
	if (nde->val != QSE_NULL)
	{
		qse_awk_val_t* val;

		/* chained expressions should not be allowed 
		 * by the parser first of all */
		QSE_ASSERT (nde->val->next == QSE_NULL); 

		val = eval_expression (run, nde->val);
		if (val == QSE_NULL) return -1;

		qse_awk_rtx_refdownval (run, STACK_RETVAL_GBL(run));
		STACK_RETVAL_GBL(run) = val; /* gbl return value */

		qse_awk_rtx_refupval (run, val);
	}

	run->exit_level = EXIT_GLOBAL;
	return 0;
}

static int run_next (qse_awk_rtx_t* run, qse_awk_nde_next_t* nde)
{
	/* the parser checks if next has been called in the begin/end
	 * block or whereever inappropriate. so the runtime doesn't 
	 * check that explicitly */
	if  (run->active_block == (qse_awk_nde_blk_t*)run->awk->tree.begin)
	{
		qse_awk_rtx_seterror (
			run, QSE_AWK_ERNEXTBEG, nde->line, QSE_NULL);
		return -1;
	}
	else if (run->active_block == (qse_awk_nde_blk_t*)run->awk->tree.end)
	{
		qse_awk_rtx_seterror (
			run, QSE_AWK_ERNEXTEND, nde->line, QSE_NULL);
		return -1;
	}

	run->exit_level = EXIT_NEXT;
	return 0;
}

static int run_nextinfile (qse_awk_rtx_t* run, qse_awk_nde_nextfile_t* nde)
{
	int n;

	/* normal nextfile statement */
	if  (run->active_block == (qse_awk_nde_blk_t*)run->awk->tree.begin)
	{
		qse_awk_rtx_seterror (
			run, QSE_AWK_ERNEXTFBEG, nde->line, QSE_NULL);
		return -1;
	}
	else if (run->active_block == (qse_awk_nde_blk_t*)run->awk->tree.end)
	{
		qse_awk_rtx_seterror (
			run, QSE_AWK_ERNEXTFEND, nde->line, QSE_NULL);
		return -1;
	}

	n = qse_awk_rtx_nextio_read (run, QSE_AWK_IN_CONSOLE, QSE_T(""));
	if (n == -1)
	{
		/* adjust the error line */
		run->errlin = nde->line;
		return -1;
	}

	if (n == 0)
	{
		/* no more input console */
		run->exit_level = EXIT_GLOBAL;
		return 0;
	}

	/* FNR resets to 0, NR remains the same */
	if (update_fnr (run, 0, run->gbl.nr) == -1) 
	{
		run->errlin = nde->line;
		return -1;
	}

	run->exit_level = EXIT_NEXT;
	return 0;

}

static int run_nextoutfile (qse_awk_rtx_t* run, qse_awk_nde_nextfile_t* nde)
{
	int n;

	/* nextofile can be called from BEGIN and END block unlike nextfile */

	n = qse_awk_rtx_nextio_write (run, QSE_AWK_OUT_CONSOLE, QSE_T(""));
	if (n == -1)
	{
		/* adjust the error line */
		run->errlin = nde->line;
		return -1;
	}

	if (n == 0)
	{
		/* should it terminate the program there is no more 
		 * output console? no. there will just be no more console 
		 * output */
		/*run->exit_level = EXIT_GLOBAL;*/
		return 0;
	}

	return 0;
}

static int run_nextfile (qse_awk_rtx_t* run, qse_awk_nde_nextfile_t* nde)
{
	return (nde->out)? 
		run_nextoutfile (run, nde): 
		run_nextinfile (run, nde);
}

static int run_delete (qse_awk_rtx_t* run, qse_awk_nde_delete_t* nde)
{
	qse_awk_nde_var_t* var;

	var = (qse_awk_nde_var_t*) nde->var;

	if (var->type == QSE_AWK_NDE_NAMED ||
	    var->type == QSE_AWK_NDE_NAMEDIDX)
	{
		qse_map_pair_t* pair;

		QSE_ASSERTX (
			(var->type == QSE_AWK_NDE_NAMED && var->idx == QSE_NULL) ||
			(var->type == QSE_AWK_NDE_NAMEDIDX && var->idx != QSE_NULL),
			"if a named variable has an index part and a named indexed variable doesn't have an index part, the program is definitely wrong");

		pair = qse_map_search (
			run->named, var->id.name.ptr, var->id.name.len);
		if (pair == QSE_NULL)
		{
			qse_awk_val_t* tmp;

			/* value not set for the named variable. 
			 * create a map and assign it to the variable */

			tmp = qse_awk_rtx_makemapval (run);
			if (tmp == QSE_NULL) 
			{
				/* adjust error line */
				run->errlin = nde->line;
				return -1;
			}

			pair = qse_map_upsert (run->named, 
				var->id.name.ptr, var->id.name.len, tmp, 0);
			if (pair == QSE_NULL)
			{
				qse_awk_rtx_refupval (run, tmp);
				qse_awk_rtx_refdownval (run, tmp);

				qse_awk_rtx_seterror (
					run, QSE_AWK_ENOMEM, var->line, QSE_NULL);
				return -1;
			}

			/* as this is the assignment, it needs to update
			 * the reference count of the target value. */
			qse_awk_rtx_refupval (run, tmp);
		}
		else
		{
			qse_awk_val_t* val;
			qse_map_t* map;

			val = (qse_awk_val_t*)QSE_MAP_VPTR(pair);
			QSE_ASSERT (val != QSE_NULL);

			if (val->type != QSE_AWK_VAL_MAP)
			{
				qse_cstr_t errarg;

				errarg.ptr = var->id.name.ptr; 
				errarg.len = var->id.name.len;

				qse_awk_rtx_seterror (
					run, QSE_AWK_ENOTDEL, var->line, &errarg);
				return -1;
			}

			map = ((qse_awk_val_map_t*)val)->map;
			if (var->type == QSE_AWK_NDE_NAMEDIDX)
			{
				qse_char_t* key;
				qse_size_t keylen;
				qse_awk_val_t* idx;
				qse_char_t buf[IDXBUFSIZE];

				QSE_ASSERT (var->idx != QSE_NULL);

				idx = eval_expression (run, var->idx);
				if (idx == QSE_NULL) return -1;

				qse_awk_rtx_refupval (run, idx);

				/* try with a fixed-size buffer */
				keylen = QSE_COUNTOF(buf);
				key = qse_awk_rtx_valtostr (
					run, idx, QSE_AWK_VALTOSTR_FIXED, 
					(qse_str_t*)buf, &keylen);
				if (key == QSE_NULL)
				{
					/* if it doesn't work, switch to dynamic mode */
					key = qse_awk_rtx_valtostr (
						run, idx, QSE_AWK_VALTOSTR_CLEAR,
						QSE_NULL, &keylen);
				}

				qse_awk_rtx_refdownval (run, idx);

				if (key == QSE_NULL) 
				{
					/* change the error line */
					run->errlin = var->line;
					return -1;
				}

				qse_map_delete (map, key, keylen);
				if (key != buf) QSE_AWK_FREE (run->awk, key);
			}
			else
			{
				qse_map_clear (map);
			}
		}
	}
	else if (var->type == QSE_AWK_NDE_GBL ||
	         var->type == QSE_AWK_NDE_LCL ||
	         var->type == QSE_AWK_NDE_ARG ||
	         var->type == QSE_AWK_NDE_GBLIDX ||
	         var->type == QSE_AWK_NDE_LCLIDX ||
	         var->type == QSE_AWK_NDE_ARGIDX)
	{
		qse_awk_val_t* val;

		if (var->type == QSE_AWK_NDE_GBL ||
		    var->type == QSE_AWK_NDE_GBLIDX)
			val = STACK_GBL (run,var->id.idxa);
		else if (var->type == QSE_AWK_NDE_LCL ||
		         var->type == QSE_AWK_NDE_LCLIDX)
			val = STACK_LCL (run,var->id.idxa);
		else val = STACK_ARG (run,var->id.idxa);

		QSE_ASSERT (val != QSE_NULL);

		if (val->type == QSE_AWK_VAL_NIL)
		{
			qse_awk_val_t* tmp;

			/* value not set.
			 * create a map and assign it to the variable */

			tmp = qse_awk_rtx_makemapval (run);
			if (tmp == QSE_NULL) 
			{
				/* adjust error line */
				run->errlin = nde->line;
				return -1;
			}

			/* no need to reduce the reference count of
			 * the previous value because it was nil. */
			if (var->type == QSE_AWK_NDE_GBL ||
			    var->type == QSE_AWK_NDE_GBLIDX)
			{
				if (qse_awk_rtx_setgbl (
					run, (int)var->id.idxa, tmp) == -1)
				{
					qse_awk_rtx_refupval (run, tmp);
					qse_awk_rtx_refdownval (run, tmp);

					run->errlin = var->line;
					return -1;
				}
			}
			else if (var->type == QSE_AWK_NDE_LCL ||
			         var->type == QSE_AWK_NDE_LCLIDX)
			{
				STACK_LCL(run,var->id.idxa) = tmp;
				qse_awk_rtx_refupval (run, tmp);
			}
			else 
			{
				STACK_ARG(run,var->id.idxa) = tmp;
				qse_awk_rtx_refupval (run, tmp);
			}
		}
		else
		{
			qse_map_t* map;

			if (val->type != QSE_AWK_VAL_MAP)
			{
				qse_cstr_t errarg;

				errarg.ptr = var->id.name.ptr; 
				errarg.len = var->id.name.len;

				qse_awk_rtx_seterror (
					run, QSE_AWK_ENOTDEL, var->line, &errarg);
				return -1;
			}

			map = ((qse_awk_val_map_t*)val)->map;
			if (var->type == QSE_AWK_NDE_GBLIDX ||
			    var->type == QSE_AWK_NDE_LCLIDX ||
			    var->type == QSE_AWK_NDE_ARGIDX)
			{
				qse_char_t* key;
				qse_size_t keylen;
				qse_awk_val_t* idx;
				qse_char_t buf[IDXBUFSIZE];

				QSE_ASSERT (var->idx != QSE_NULL);

				idx = eval_expression (run, var->idx);
				if (idx == QSE_NULL) return -1;

				qse_awk_rtx_refupval (run, idx);

				/* try with a fixed-size buffer */
				keylen = QSE_COUNTOF(buf);
				key = qse_awk_rtx_valtostr (
					run, idx, QSE_AWK_VALTOSTR_FIXED, 
					(qse_str_t*)buf, &keylen);
				if (key == QSE_NULL)
				{
					/* if it doesn't work, switch to dynamic mode */
					key = qse_awk_rtx_valtostr (
						run, idx, QSE_AWK_VALTOSTR_CLEAR,
						QSE_NULL, &keylen);
				}

				qse_awk_rtx_refdownval (run, idx);

				if (key == QSE_NULL)
				{
					run->errlin = var->line;
					return -1;
				}
				qse_map_delete (map, key, keylen);
				if (key != buf) QSE_AWK_FREE (run->awk, key);
			}
			else
			{
				qse_map_clear (map);
			}
		}
	}
	else
	{
		QSE_ASSERTX (
			!"should never happen - wrong target for delete",
			"the delete statement cannot be called with other nodes than the variables such as a named variable, a named indexed variable, etc");

		qse_awk_rtx_seterror (
			run, QSE_AWK_ERDELETE, var->line, QSE_NULL);
		return -1;
	}

	return 0;
}

static int run_reset (qse_awk_rtx_t* run, qse_awk_nde_reset_t* nde)
{
	qse_awk_nde_var_t* var;

	var = (qse_awk_nde_var_t*) nde->var;

	if (var->type == QSE_AWK_NDE_NAMED)
	{
		QSE_ASSERTX (
			var->type == QSE_AWK_NDE_NAMED && var->idx == QSE_NULL,
			"if a named variable has an index part, something is definitely wrong");

		/* a named variable can be reset if removed from a internal map 
		   to manage it */
		qse_map_delete (run->named, var->id.name.ptr, var->id.name.len);
	}
	else if (var->type == QSE_AWK_NDE_GBL ||
	         var->type == QSE_AWK_NDE_LCL ||
	         var->type == QSE_AWK_NDE_ARG)
	{
		qse_awk_val_t* val;

		if (var->type == QSE_AWK_NDE_GBL)
			val = STACK_GBL(run,var->id.idxa);
		else if (var->type == QSE_AWK_NDE_LCL)
			val = STACK_LCL(run,var->id.idxa);
		else val = STACK_ARG(run,var->id.idxa);

		QSE_ASSERT (val != QSE_NULL);

		if (val->type != QSE_AWK_VAL_NIL)
		{
			qse_awk_rtx_refdownval (run, val);
			if (var->type == QSE_AWK_NDE_GBL)
				STACK_GBL(run,var->id.idxa) = qse_awk_val_nil;
			else if (var->type == QSE_AWK_NDE_LCL)
				STACK_LCL(run,var->id.idxa) = qse_awk_val_nil;
			else
				STACK_ARG(run,var->id.idxa) = qse_awk_val_nil;
		}
	}
	else
	{
		QSE_ASSERTX (
			!"should never happen - wrong target for reset",
			"the reset statement can only be called with plain variables");

		qse_awk_rtx_seterror (
			run, QSE_AWK_ERRESET, var->line, QSE_NULL);
		return -1;
	}

	return 0;
}

static int run_print (qse_awk_rtx_t* run, qse_awk_nde_print_t* nde)
{
	qse_char_t* out = QSE_NULL;
	const qse_char_t* dst;
	qse_awk_val_t* v;
	int n;

	QSE_ASSERT (
		(nde->out_type == QSE_AWK_OUT_PIPE && nde->out != QSE_NULL) ||
		(nde->out_type == QSE_AWK_OUT_RWPIPE && nde->out != QSE_NULL) ||
		(nde->out_type == QSE_AWK_OUT_FILE && nde->out != QSE_NULL) ||
		(nde->out_type == QSE_AWK_OUT_APFILE && nde->out != QSE_NULL) ||
		(nde->out_type == QSE_AWK_OUT_CONSOLE && nde->out == QSE_NULL));

	/* check if destination has been specified. */
	if (nde->out != QSE_NULL)
	{
		qse_size_t len;

		/* if so, resolve the destination name */
		v = eval_expression (run, nde->out);
		if (v == QSE_NULL) return -1;

		qse_awk_rtx_refupval (run, v);
		out = qse_awk_rtx_valtostr (
			run, v, QSE_AWK_VALTOSTR_CLEAR, QSE_NULL, &len);
		if (out == QSE_NULL) 
		{
			qse_awk_rtx_refdownval (run, v);

			/* change the error line */
			run->errlin = nde->line;
			return -1;
		}
		qse_awk_rtx_refdownval (run, v);

		if (len <= 0) 
		{
			/* the destination name is empty */
			QSE_AWK_FREE (run->awk, out);
			qse_awk_rtx_seterror (
				run, QSE_AWK_EIONMEM, nde->line, QSE_NULL);
			return -1;
		}

		/* it needs to check if the destination name contains
		 * any invalid characters to the underlying system */
		while (len > 0)
		{
			if (out[--len] == QSE_T('\0'))
			{
				/* if so, it skips writing */
				QSE_AWK_FREE (run->awk, out);
				qse_awk_rtx_seterror (
					run, QSE_AWK_EIONMNL, nde->line, QSE_NULL);
				return -1;
			}
		}
	}

	/* transforms the destination to suit the usage with io */
	dst = (out == QSE_NULL)? QSE_T(""): out;

	/* check if print is followed by any arguments */
	if (nde->args == QSE_NULL)
	{
		/* if it doesn't have any arguments, print the entire 
		 * input record */
		n = qse_awk_rtx_writeio_str (
			run, nde->out_type, dst,
			QSE_STR_PTR(&run->inrec.line),
			QSE_STR_LEN(&run->inrec.line));
		if (n <= -1 /*&& run->errnum != QSE_AWK_EIOIMPL*/)
		{
			if (out != QSE_NULL) 
				QSE_AWK_FREE (run->awk, out);

			/* adjust the error line */
			run->errlin = nde->line;
			return -1;
		}
	}
	else
	{
		/* if it has any arguments, print the arguments separated by
		 * the value OFS */
		qse_awk_nde_t* head, * np;

		if (nde->args->type == QSE_AWK_NDE_GRP)
		{
			/* parenthesized print */
			QSE_ASSERT (nde->args->next == QSE_NULL);
			head = ((qse_awk_nde_grp_t*)nde->args)->body;
		}
		else head = nde->args;

		for (np = head; np != QSE_NULL; np = np->next)
		{
			if (np != head)
			{
				n = qse_awk_rtx_writeio_str (
					run, nde->out_type, dst, 
					run->gbl.ofs.ptr, 
					run->gbl.ofs.len);
				if (n <= -1 /*&& run->errnum != QSE_AWK_EIOIMPL*/) 
				{
					if (out != QSE_NULL)
						QSE_AWK_FREE (run->awk, out);

					/* adjust the error line */
					run->errlin = nde->line;
					return -1;
				}
			}

			v = eval_expression (run, np);
			if (v == QSE_NULL) 
			{
				if (out != QSE_NULL)
					QSE_AWK_FREE (run->awk, out);
				return -1;
			}
			qse_awk_rtx_refupval (run, v);

			n = qse_awk_rtx_writeio_val (run, nde->out_type, dst, v);
			if (n <= -1 /*&& run->errnum != QSE_AWK_EIOIMPL*/) 
			{
				if (out != QSE_NULL) 
					QSE_AWK_FREE (run->awk, out);

				qse_awk_rtx_refdownval (run, v);
				/* adjust the error line */
				run->errlin = nde->line;
				return -1;
			}

			qse_awk_rtx_refdownval (run, v);
		}
	}

	/* print the value ORS to terminate the operation */
	n = qse_awk_rtx_writeio_str (
		run, nde->out_type, dst, 
		run->gbl.ors.ptr, run->gbl.ors.len);
	if (n <= -1 /*&& run->errnum != QSE_AWK_EIOIMPL*/)
	{
		if (out != QSE_NULL) QSE_AWK_FREE (run->awk, out);

		/* change the error line */
		run->errlin = nde->line;
		return -1;
	}

	if (out != QSE_NULL) QSE_AWK_FREE (run->awk, out);

/*skip_write:*/
	return 0;
}

static int run_printf (qse_awk_rtx_t* run, qse_awk_nde_print_t* nde)
{
	qse_char_t* out = QSE_NULL;
	const qse_char_t* dst;
	qse_awk_val_t* v;
	qse_awk_nde_t* head;
	int n;

	QSE_ASSERT (
		(nde->out_type == QSE_AWK_OUT_PIPE && nde->out != QSE_NULL) ||
		(nde->out_type == QSE_AWK_OUT_RWPIPE && nde->out != QSE_NULL) ||
		(nde->out_type == QSE_AWK_OUT_FILE && nde->out != QSE_NULL) ||
		(nde->out_type == QSE_AWK_OUT_APFILE && nde->out != QSE_NULL) ||
		(nde->out_type == QSE_AWK_OUT_CONSOLE && nde->out == QSE_NULL));

	if (nde->out != QSE_NULL)
	{
		qse_size_t len;

		v = eval_expression (run, nde->out);
		if (v == QSE_NULL) return -1;

		qse_awk_rtx_refupval (run, v);
		out = qse_awk_rtx_valtostr (
			run, v, QSE_AWK_VALTOSTR_CLEAR, QSE_NULL, &len);
		if (out == QSE_NULL) 
		{
			qse_awk_rtx_refdownval (run, v);

			/* change the error line */
			run->errlin = nde->line;
			return -1;
		}
		qse_awk_rtx_refdownval (run, v);

		if (len <= 0) 
		{
			/* the output destination name is empty. */
			QSE_AWK_FREE (run->awk, out);
			qse_awk_rtx_seterror (
				run, QSE_AWK_EIONMEM, nde->line, QSE_NULL);
			return -1;
		}

		while (len > 0)
		{
			if (out[--len] == QSE_T('\0'))
			{
				/* the output destination name contains a null 
				 * character. */
				QSE_AWK_FREE (run->awk, out);
				qse_awk_rtx_seterror (
					run, QSE_AWK_EIONMNL, nde->line, QSE_NULL);
				return -1;
			}
		}
	}

	dst = (out == QSE_NULL)? QSE_T(""): out;

	QSE_ASSERTX (nde->args != QSE_NULL, 
		"a valid printf statement should have at least one argument. the parser must ensure this.");

	if (nde->args->type == QSE_AWK_NDE_GRP)
	{
		/* parenthesized print */
		QSE_ASSERT (nde->args->next == QSE_NULL);
		head = ((qse_awk_nde_grp_t*)nde->args)->body;
	}
	else head = nde->args;

	QSE_ASSERTX (head != QSE_NULL,
		"a valid printf statement should have at least one argument. the parser must ensure this.");

	v = eval_expression (run, head);
	if (v == QSE_NULL) 
	{
		if (out != QSE_NULL) QSE_AWK_FREE (run->awk, out);
		return -1;
	}

	qse_awk_rtx_refupval (run, v);
	if (v->type != QSE_AWK_VAL_STR)
	{
		/* the remaining arguments are ignored as the format cannot 
		 * contain any % characters */
		n = qse_awk_rtx_writeio_val (run, nde->out_type, dst, v);
		if (n <= -1 /*&& run->errnum != QSE_AWK_EIOIMPL*/)
		{
			if (out != QSE_NULL) QSE_AWK_FREE (run->awk, out);
			qse_awk_rtx_refdownval (run, v);

			/* change the error line */
			run->errlin = nde->line;
			return -1;
		}
	}
	else
	{
		/* perform the formatted output */
		if (output_formatted (
			run, nde->out_type, dst,
			((qse_awk_val_str_t*)v)->ptr,
			((qse_awk_val_str_t*)v)->len,
			head->next) == -1)
		{
			if (out != QSE_NULL) QSE_AWK_FREE (run->awk, out);
			qse_awk_rtx_refdownval (run, v);

			/* adjust the error line */
			run->errlin = nde->line;
			return -1;
		}
	}
	qse_awk_rtx_refdownval (run, v);

	if (out != QSE_NULL) QSE_AWK_FREE (run->awk, out);

/*skip_write:*/
	return 0;
}

static int output_formatted (
	qse_awk_rtx_t* run, int out_type, const qse_char_t* dst, 
	const qse_char_t* fmt, qse_size_t fmt_len, qse_awk_nde_t* args)
{
	qse_char_t* ptr;
	qse_size_t len;
	int n;

	ptr = qse_awk_rtx_format (run, 
		QSE_NULL, QSE_NULL, fmt, fmt_len, 0, args, &len);
	if (ptr == QSE_NULL) return -1;

	n = qse_awk_rtx_writeio_str (run, out_type, dst, ptr, len);
	if (n <= -1 /*&& run->errnum != QSE_AWK_EIOIMPL*/) return -1;

	return 0;
}

static qse_awk_val_t* eval_expression (qse_awk_rtx_t* run, qse_awk_nde_t* nde)
{
	qse_awk_val_t* v;
	int n, errnum;

#if 0
	if (run->exit_level >= EXIT_GLOBAL) 
	{
		/* returns QSE_NULL as if an error occurred but
		 * clears the error number. run_main will 
		 * detect this condition and treat it as a 
		 * non-error condition.*/
		run->errnum = QSE_AWK_ENOERR;
		return QSE_NULL;
	}
#endif

	v = eval_expression0 (run, nde);
	if (v == QSE_NULL) return QSE_NULL;

	if (v->type == QSE_AWK_VAL_REX)
	{
		qse_awk_rtx_refupval (run, v);

		if (run->inrec.d0->type == QSE_AWK_VAL_NIL)
		{
			/* the record has never been read. 
			 * probably, this functions has been triggered
			 * by the statements in the BEGIN block */
			n = QSE_AWK_ISEMPTYREX(run->awk,((qse_awk_val_rex_t*)v)->code)? 1: 0;
		}
		else
		{
			QSE_ASSERTX (
				run->inrec.d0->type == QSE_AWK_VAL_STR,
				"the internal value representing $0 should always be of the string type once it has been set/updated. it is nil initially.");

			n = QSE_AWK_MATCHREX (
				((qse_awk_rtx_t*)run)->awk, 
				((qse_awk_val_rex_t*)v)->code,
				((((qse_awk_rtx_t*)run)->gbl.ignorecase)? QSE_REX_IGNORECASE: 0),
				((qse_awk_val_str_t*)run->inrec.d0)->ptr,
				((qse_awk_val_str_t*)run->inrec.d0)->len,
				QSE_NULL, QSE_NULL, &errnum);
	
			if (n == -1) 
			{
				qse_awk_rtx_refdownval (run, v);

				/* matchrex should never set the error number
				 * whose message contains a formatting 
				 * character. otherwise, the following way of
				 * setting the error information may not work */
				qse_awk_rtx_seterror (
					run, errnum, nde->line, QSE_NULL);
				return QSE_NULL;
			}
		}

		qse_awk_rtx_refdownval (run, v);

		v = qse_awk_rtx_makeintval (run, (n != 0));
		if (v == QSE_NULL) 
		{
			/* adjust error line */
			run->errlin = nde->line;
			return QSE_NULL;
		}
	}

	return v;
}

static qse_awk_val_t* eval_expression0 (qse_awk_rtx_t* run, qse_awk_nde_t* nde)
{
	static eval_expr_t __evaluator[] =
	{
		/* the order of functions here should match the order
		 * of node types declared in tree.h */
		eval_group,
		eval_assignment,
		eval_binary,
		eval_unary,
		eval_incpre,
		eval_incpst,
		eval_cnd,
		eval_fnc,
		eval_fun,
		eval_int,
		eval_real,
		eval_str,
		eval_rex,
		eval_named,
		eval_gbl,
		eval_lcl,
		eval_arg,
		eval_namedidx,
		eval_gblidx,
		eval_lclidx,
		eval_argidx,
		eval_pos,
		eval_getline
	};

	qse_awk_val_t* v;

	QSE_ASSERT (nde->type >= QSE_AWK_NDE_GRP &&
		(nde->type - QSE_AWK_NDE_GRP) < QSE_COUNTOF(__evaluator));

	v = __evaluator[nde->type-QSE_AWK_NDE_GRP] (run, nde);

	if (v != QSE_NULL && run->exit_level >= EXIT_GLOBAL)
	{
		qse_awk_rtx_refupval (run, v);	
		qse_awk_rtx_refdownval (run, v);

		/* returns QSE_NULL as if an error occurred but
		 * clears the error number. run_main will 
		 * detect this condition and treat it as a 
		 * non-error condition.*/
		run->errnum = QSE_AWK_ENOERR;
		return QSE_NULL;
	}

	return v;
}

static qse_awk_val_t* eval_group (qse_awk_rtx_t* run, qse_awk_nde_t* nde)
{
	/* eval_binop_in evaluates the QSE_AWK_NDE_GRP specially.
	 * so this function should never be reached. */
	QSE_ASSERT (!"should never happen - NDE_GRP only for in");
	qse_awk_rtx_seterror (run, QSE_AWK_EINTERN, nde->line, QSE_NULL);
	return QSE_NULL;
}

static qse_awk_val_t* eval_assignment (qse_awk_rtx_t* run, qse_awk_nde_t* nde)
{
	qse_awk_val_t* val, * ret;
	qse_awk_nde_ass_t* ass = (qse_awk_nde_ass_t*)nde;

	QSE_ASSERT (ass->left != QSE_NULL);
	QSE_ASSERT (ass->right != QSE_NULL);

	QSE_ASSERT (ass->right->next == QSE_NULL);
	val = eval_expression (run, ass->right);
	if (val == QSE_NULL) return QSE_NULL;

	qse_awk_rtx_refupval (run, val);

	if (ass->opcode != QSE_AWK_ASSOP_NONE)
	{
		qse_awk_val_t* val2, * tmp;
		static binop_func_t binop_func[] =
		{
			QSE_NULL, /* QSE_AWK_ASSOP_NONE */
			eval_binop_plus,
			eval_binop_minus,
			eval_binop_mul,
			eval_binop_div,
			eval_binop_idiv,
			eval_binop_mod,
			eval_binop_exp,
			eval_binop_rshift,
			eval_binop_lshift,
			eval_binop_band,
			eval_binop_bxor,
			eval_binop_bor
		};

		QSE_ASSERT (ass->left->next == QSE_NULL);
		val2 = eval_expression (run, ass->left);
		if (val2 == QSE_NULL)
		{
			qse_awk_rtx_refdownval (run, val);
			return QSE_NULL;
		}

		qse_awk_rtx_refupval (run, val2);

		QSE_ASSERT (ass->opcode >= 0);
		QSE_ASSERT (ass->opcode < QSE_COUNTOF(binop_func));
		QSE_ASSERT (binop_func[ass->opcode] != QSE_NULL);

		tmp = binop_func[ass->opcode] (run, val2, val);
		if (tmp == QSE_NULL)
		{
			qse_awk_rtx_refdownval (run, val2);
			qse_awk_rtx_refdownval (run, val);
			return QSE_NULL;
		}

		qse_awk_rtx_refdownval (run, val2);
		qse_awk_rtx_refdownval (run, val);

		val = tmp;
		qse_awk_rtx_refupval (run, val);
	}

	ret = do_assignment (run, ass->left, val);
	qse_awk_rtx_refdownval (run, val);

	return ret;
}

static qse_awk_val_t* do_assignment (
	qse_awk_rtx_t* run, qse_awk_nde_t* var, qse_awk_val_t* val)
{
	qse_awk_val_t* ret;
	int errnum;

	if (var->type == QSE_AWK_NDE_NAMED ||
	    var->type == QSE_AWK_NDE_GBL ||
	    var->type == QSE_AWK_NDE_LCL ||
	    var->type == QSE_AWK_NDE_ARG) 
	{
		if ((run->awk->option & QSE_AWK_MAPTOVAR) == 0)
		{
			if (val->type == QSE_AWK_VAL_MAP)
			{
				errnum = QSE_AWK_ENOTASS;
				goto exit_on_error;
			}
		}

		ret = do_assignment_scalar (run, (qse_awk_nde_var_t*)var, val);
	}
	else if (var->type == QSE_AWK_NDE_NAMEDIDX ||
	         var->type == QSE_AWK_NDE_GBLIDX ||
	         var->type == QSE_AWK_NDE_LCLIDX ||
	         var->type == QSE_AWK_NDE_ARGIDX) 
	{
		if (val->type == QSE_AWK_VAL_MAP)
		{
			errnum = QSE_AWK_ENOTASS;
			goto exit_on_error;
		}

		ret = do_assignment_map (run, (qse_awk_nde_var_t*)var, val);
	}
	else if (var->type == QSE_AWK_NDE_POS)
	{
		if (val->type == QSE_AWK_VAL_MAP)
		{
			errnum = QSE_AWK_ENOTASS;
			goto exit_on_error;
		}
	

		ret = do_assignment_pos (run, (qse_awk_nde_pos_t*)var, val);
	}
	else
	{
		QSE_ASSERT (!"should never happen - invalid variable type");
		errnum = QSE_AWK_EINTERN;
		goto exit_on_error;
	}

	return ret;

exit_on_error:
	qse_awk_rtx_seterror (run, errnum, var->line, QSE_NULL);
	return QSE_NULL;
}

static qse_awk_val_t* do_assignment_scalar (
	qse_awk_rtx_t* run, qse_awk_nde_var_t* var, qse_awk_val_t* val)
{
	QSE_ASSERT (
		(var->type == QSE_AWK_NDE_NAMED ||
		 var->type == QSE_AWK_NDE_GBL ||
		 var->type == QSE_AWK_NDE_LCL ||
		 var->type == QSE_AWK_NDE_ARG) && var->idx == QSE_NULL);

	QSE_ASSERT (
		(run->awk->option & QSE_AWK_MAPTOVAR) ||
		val->type != QSE_AWK_VAL_MAP);

	if (var->type == QSE_AWK_NDE_NAMED) 
	{
		qse_map_pair_t* pair;

		pair = qse_map_search (
			run->named, var->id.name.ptr, var->id.name.len);
		if (pair != QSE_NULL && 
		    ((qse_awk_val_t*)QSE_MAP_VPTR(pair))->type == QSE_AWK_VAL_MAP)
		{
			/* once a variable becomes a map,
			 * it cannot be changed to a scalar variable */
			qse_cstr_t errarg;

			errarg.ptr = var->id.name.ptr; 
			errarg.len = var->id.name.len;

			qse_awk_rtx_seterror (run,
				QSE_AWK_EMAPTOSCALAR, var->line, &errarg);
			return QSE_NULL;
		}

		if (qse_map_upsert (run->named, 
			var->id.name.ptr, var->id.name.len, val, 0) == QSE_NULL)
		{
			qse_awk_rtx_seterror (
				run, QSE_AWK_ENOMEM, var->line, QSE_NULL);
			return QSE_NULL;
		}

		qse_awk_rtx_refupval (run, val);
	}
	else if (var->type == QSE_AWK_NDE_GBL) 
	{
		if (set_global (run, var->id.idxa, var, val) == -1) 
		{
			/* adjust error line */
			run->errlin = var->line;
			return QSE_NULL;
		}
	}
	else if (var->type == QSE_AWK_NDE_LCL) 
	{
		qse_awk_val_t* old = STACK_LCL(run,var->id.idxa);
		if (old->type == QSE_AWK_VAL_MAP)
		{	
			/* once the variable becomes a map,
			 * it cannot be changed to a scalar variable */
			qse_cstr_t errarg;

			errarg.ptr = var->id.name.ptr; 
			errarg.len = var->id.name.len;

			qse_awk_rtx_seterror (run,
				QSE_AWK_EMAPTOSCALAR, var->line, &errarg);
			return QSE_NULL;
		}

		qse_awk_rtx_refdownval (run, old);
		STACK_LCL(run,var->id.idxa) = val;
		qse_awk_rtx_refupval (run, val);
	}
	else /* if (var->type == QSE_AWK_NDE_ARG) */
	{
		qse_awk_val_t* old = STACK_ARG(run,var->id.idxa);
		if (old->type == QSE_AWK_VAL_MAP)
		{	
			/* once the variable becomes a map,
			 * it cannot be changed to a scalar variable */
			qse_cstr_t errarg;

			errarg.ptr = var->id.name.ptr; 
			errarg.len = var->id.name.len;

			qse_awk_rtx_seterror (run,
				QSE_AWK_EMAPTOSCALAR, var->line, &errarg);
			return QSE_NULL;
		}

		qse_awk_rtx_refdownval (run, old);
		STACK_ARG(run,var->id.idxa) = val;
		qse_awk_rtx_refupval (run, val);
	}

	return val;
}

static qse_awk_val_t* do_assignment_map (
	qse_awk_rtx_t* run, qse_awk_nde_var_t* var, qse_awk_val_t* val)
{
	qse_awk_val_map_t* map;
	qse_char_t* str;
	qse_size_t len;
	qse_char_t idxbuf[IDXBUFSIZE];

	QSE_ASSERT (
		(var->type == QSE_AWK_NDE_NAMEDIDX ||
		 var->type == QSE_AWK_NDE_GBLIDX ||
		 var->type == QSE_AWK_NDE_LCLIDX ||
		 var->type == QSE_AWK_NDE_ARGIDX) && var->idx != QSE_NULL);
	QSE_ASSERT (val->type != QSE_AWK_VAL_MAP);

	if (var->type == QSE_AWK_NDE_NAMEDIDX)
	{
		qse_map_pair_t* pair;
		pair = qse_map_search (
			run->named, var->id.name.ptr, var->id.name.len);
		map = (pair == QSE_NULL)? 
			(qse_awk_val_map_t*)qse_awk_val_nil: 
			(qse_awk_val_map_t*)QSE_MAP_VPTR(pair);
	}
	else
	{
		map = (var->type == QSE_AWK_NDE_GBLIDX)? 
		      	(qse_awk_val_map_t*)STACK_GBL(run,var->id.idxa):
		      (var->type == QSE_AWK_NDE_LCLIDX)? 
		      	(qse_awk_val_map_t*)STACK_LCL(run,var->id.idxa):
		      	(qse_awk_val_map_t*)STACK_ARG(run,var->id.idxa);
	}

	if (map->type == QSE_AWK_VAL_NIL)
	{
		/* the map is not initialized yet */
		qse_awk_val_t* tmp;

		tmp = qse_awk_rtx_makemapval (run);
		if (tmp == QSE_NULL) 
		{
			/* adjust error line */
			run->errlin = var->line;
			return QSE_NULL;
		}

		if (var->type == QSE_AWK_NDE_NAMEDIDX)
		{
			/* doesn't have to decrease the reference count 
			 * of the previous value here as it is done by 
			 * qse_map_upsert */
			if (qse_map_upsert (
				run->named,
				var->id.name.ptr,
				var->id.name.len, 
				tmp,
				0) == QSE_NULL)
			{
				qse_awk_rtx_refupval (run, tmp);
				qse_awk_rtx_refdownval (run, tmp);

				qse_awk_rtx_seterror (
					run, QSE_AWK_ENOMEM, var->line, QSE_NULL);
				return QSE_NULL;
			}

			qse_awk_rtx_refupval (run, tmp);
		}
		else if (var->type == QSE_AWK_NDE_GBLIDX)
		{
			qse_awk_rtx_refupval (run, tmp);
			if (qse_awk_rtx_setgbl (run, (int)var->id.idxa, tmp) == -1)
			{
				qse_awk_rtx_refdownval (run, tmp);

				/* change error line */
				run->errlin = var->line;
				return QSE_NULL;
			}
			qse_awk_rtx_refdownval (run, tmp);
		}
		else if (var->type == QSE_AWK_NDE_LCLIDX)
		{
			qse_awk_rtx_refdownval (run, (qse_awk_val_t*)map);
			STACK_LCL(run,var->id.idxa) = tmp;
			qse_awk_rtx_refupval (run, tmp);
		}
		else /* if (var->type == QSE_AWK_NDE_ARGIDX) */
		{
			qse_awk_rtx_refdownval (run, (qse_awk_val_t*)map);
			STACK_ARG(run,var->id.idxa) = tmp;
			qse_awk_rtx_refupval (run, tmp);
		}

		map = (qse_awk_val_map_t*) tmp;
	}
	else if (map->type != QSE_AWK_VAL_MAP)
	{
		/* variable assigned is not a map */
		qse_awk_rtx_seterror (
			run, QSE_AWK_ENOTIDX, var->line, QSE_NULL);
		return QSE_NULL;
	}

	len = QSE_COUNTOF(idxbuf);
	str = idxnde_to_str (run, var->idx, idxbuf, &len);
	if (str == QSE_NULL) 
	{
		str = idxnde_to_str (run, var->idx, QSE_NULL, &len);
		if (str == QSE_NULL) return QSE_NULL;
	}

#ifdef DEBUG_RUN
	qse_dprintf (QSE_T("**** index str=>%s, map->ref=%d, map->type=%d\n"), 
		str, (int)map->ref, (int)map->type);
#endif

	if (qse_map_upsert (map->map, str, len, val, 0) == QSE_NULL)
	{
		if (str != idxbuf) QSE_AWK_FREE (run->awk, str);
		qse_awk_rtx_seterror (run, QSE_AWK_ENOMEM, var->line, QSE_NULL);
		return QSE_NULL;
	}

	if (str != idxbuf) QSE_AWK_FREE (run->awk, str);
	qse_awk_rtx_refupval (run, val);
	return val;
}

static qse_awk_val_t* do_assignment_pos (
	qse_awk_rtx_t* run, qse_awk_nde_pos_t* pos, qse_awk_val_t* val)
{
	qse_awk_val_t* v;
	qse_long_t lv;
	qse_real_t rv;
	qse_char_t* str;
	qse_size_t len;
	int n;

	v = eval_expression (run, pos->val);
	if (v == QSE_NULL) return QSE_NULL;

	qse_awk_rtx_refupval (run, v);
	n = qse_awk_rtx_valtonum (run, v, &lv, &rv);
	qse_awk_rtx_refdownval (run, v);

	if (n == -1) 
	{
		qse_awk_rtx_seterror (
			run, QSE_AWK_EPOSIDX, pos->line, QSE_NULL);
		return QSE_NULL;
	}
	if (n == 1) lv = (qse_long_t)rv;
	if (!IS_VALID_POSIDX(lv)) 
	{
		qse_awk_rtx_seterror (
			run, QSE_AWK_EPOSIDX, pos->line, QSE_NULL);
		return QSE_NULL;
	}

	if (val->type == QSE_AWK_VAL_STR)
	{
		str = ((qse_awk_val_str_t*)val)->ptr;
		len = ((qse_awk_val_str_t*)val)->len;
	}
	else
	{
		str = qse_awk_rtx_valtostr (
			run, val, QSE_AWK_VALTOSTR_CLEAR, QSE_NULL, &len);
		if (str == QSE_NULL)
		{
			/* change error line */
			run->errlin = pos->line;
			return QSE_NULL;
		}
	}
	
	n = qse_awk_rtx_setrec (run, (qse_size_t)lv, str, len);

	if (val->type != QSE_AWK_VAL_STR) QSE_AWK_FREE (run->awk, str);

	if (n == -1) return QSE_NULL;
	return (lv == 0)? run->inrec.d0: run->inrec.flds[lv-1].val;
}

static qse_awk_val_t* eval_binary (qse_awk_rtx_t* run, qse_awk_nde_t* nde)
{
	static binop_func_t binop_func[] =
	{
		/* the order of the functions should be inline with
		 * the operator declaration in run.h */

		QSE_NULL, /* eval_binop_lor */
		QSE_NULL, /* eval_binop_land */
		QSE_NULL, /* eval_binop_in */

		eval_binop_bor,
		eval_binop_bxor,
		eval_binop_band,

		eval_binop_eq,
		eval_binop_ne,
		eval_binop_gt,
		eval_binop_ge,
		eval_binop_lt,
		eval_binop_le,

		eval_binop_lshift,
		eval_binop_rshift,
		
		eval_binop_plus,
		eval_binop_minus,
		eval_binop_mul,
		eval_binop_div,
		eval_binop_idiv,
		eval_binop_mod,
		eval_binop_exp,

		eval_binop_concat,
		QSE_NULL, /* eval_binop_ma */
		QSE_NULL  /* eval_binop_nm */
	};

	qse_awk_nde_exp_t* exp = (qse_awk_nde_exp_t*)nde;
	qse_awk_val_t* left, * right, * res;

	QSE_ASSERT (exp->type == QSE_AWK_NDE_EXP_BIN);

	if (exp->opcode == QSE_AWK_BINOP_LAND)
	{
		res = eval_binop_land (run, exp->left, exp->right);
	}
	else if (exp->opcode == QSE_AWK_BINOP_LOR)
	{
		res = eval_binop_lor (run, exp->left, exp->right);
	}
	else if (exp->opcode == QSE_AWK_BINOP_IN)
	{
		/* treat the in operator specially */
		res = eval_binop_in (run, exp->left, exp->right);
	}
	else if (exp->opcode == QSE_AWK_BINOP_NM)
	{
		res = eval_binop_nm (run, exp->left, exp->right);
	}
	else if (exp->opcode == QSE_AWK_BINOP_MA)
	{
		res = eval_binop_ma (run, exp->left, exp->right);
	}
	else
	{
		QSE_ASSERT (exp->left->next == QSE_NULL);
		left = eval_expression (run, exp->left);
		if (left == QSE_NULL) return QSE_NULL;

		qse_awk_rtx_refupval (run, left);

		QSE_ASSERT (exp->right->next == QSE_NULL);
		right = eval_expression (run, exp->right);
		if (right == QSE_NULL) 
		{
			qse_awk_rtx_refdownval (run, left);
			return QSE_NULL;
		}

		qse_awk_rtx_refupval (run, right);

		QSE_ASSERT (exp->opcode >= 0 && 
			exp->opcode < QSE_COUNTOF(binop_func));
		QSE_ASSERT (binop_func[exp->opcode] != QSE_NULL);

		res = binop_func[exp->opcode] (run, left, right);
		if (res == QSE_NULL)
		{
			/* change the error line */
			run->errlin = nde->line;
		}

		qse_awk_rtx_refdownval (run, left);
		qse_awk_rtx_refdownval (run, right);
	}

	return res;
}

static qse_awk_val_t* eval_binop_lor (
	qse_awk_rtx_t* run, qse_awk_nde_t* left, qse_awk_nde_t* right)
{
	/*
	qse_awk_val_t* res = QSE_NULL;

	res = qse_awk_rtx_makeintval (run, 
		qse_awk_rtx_valtobool(run left) || qse_awk_rtx_valtobool(run,right));
	if (res == QSE_NULL)
	{
		run->errlin = left->line;
		return QSE_NULL;
	}

	return res;
	*/

	/* short-circuit evaluation required special treatment */
	qse_awk_val_t* lv, * rv, * res;

	QSE_ASSERT (left->next == QSE_NULL);
	lv = eval_expression (run, left);
	if (lv == QSE_NULL) return QSE_NULL;

	qse_awk_rtx_refupval (run, lv);
	if (qse_awk_rtx_valtobool (run, lv)) 
	{
		res = qse_awk_val_one;
	}
	else
	{
		QSE_ASSERT (right->next == QSE_NULL);
		rv = eval_expression (run, right);
		if (rv == QSE_NULL)
		{
			qse_awk_rtx_refdownval (run, lv);
			return QSE_NULL;
		}
		qse_awk_rtx_refupval (run, rv);

		res = qse_awk_rtx_valtobool(run,rv)? qse_awk_val_one: qse_awk_val_zero;
		qse_awk_rtx_refdownval (run, rv);
	}

	qse_awk_rtx_refdownval (run, lv);

	return res;
}

static qse_awk_val_t* eval_binop_land (
	qse_awk_rtx_t* run, qse_awk_nde_t* left, qse_awk_nde_t* right)
{
	/*
	qse_awk_val_t* res = QSE_NULL;

	res = qse_awk_rtx_makeintval (run, 
		qse_awk_rtx_valtobool(run,left) && qse_awk_rtx_valtobool(run,right));
	if (res == QSE_NULL) 
	{
		run->errlin = left->line;
		return QSE_NULL;
	}

	return res;
	*/

	/* short-circuit evaluation required special treatment */
	qse_awk_val_t* lv, * rv, * res;

	QSE_ASSERT (left->next == QSE_NULL);
	lv = eval_expression (run, left);
	if (lv == QSE_NULL) return QSE_NULL;

	qse_awk_rtx_refupval (run, lv);
	if (!qse_awk_rtx_valtobool (run, lv)) 
	{
		res = qse_awk_val_zero;
	}
	else
	{
		QSE_ASSERT (right->next == QSE_NULL);
		rv = eval_expression (run, right);
		if (rv == QSE_NULL)
		{
			qse_awk_rtx_refdownval (run, lv);
			return QSE_NULL;
		}
		qse_awk_rtx_refupval (run, rv);

		res = qse_awk_rtx_valtobool(run,rv)? qse_awk_val_one: qse_awk_val_zero;
		qse_awk_rtx_refdownval (run, rv);
	}

	qse_awk_rtx_refdownval (run, lv);

	return res;
}

static qse_awk_val_t* eval_binop_in (
	qse_awk_rtx_t* run, qse_awk_nde_t* left, qse_awk_nde_t* right)
{
	qse_awk_val_t* rv;
	qse_char_t* str;
	qse_size_t len;
	qse_char_t idxbuf[IDXBUFSIZE];

	if (right->type != QSE_AWK_NDE_GBL &&
	    right->type != QSE_AWK_NDE_LCL &&
	    right->type != QSE_AWK_NDE_ARG &&
	    right->type != QSE_AWK_NDE_NAMED)
	{
		/* the compiler should have handled this case */
		QSE_ASSERT (
			!"should never happen - in needs a plain variable");

		qse_awk_rtx_seterror (
			run, QSE_AWK_EINTERN, right->line, QSE_NULL);
		return QSE_NULL;
	}

	/* evaluate the left-hand side of the operator */
	len = QSE_COUNTOF(idxbuf);
	str = (left->type == QSE_AWK_NDE_GRP)?
		idxnde_to_str (run, ((qse_awk_nde_grp_t*)left)->body, idxbuf, &len):
		idxnde_to_str (run, left, idxbuf, &len);
	if (str == QSE_NULL)
	{
		str = (left->type == QSE_AWK_NDE_GRP)?
			idxnde_to_str (run, ((qse_awk_nde_grp_t*)left)->body, QSE_NULL, &len):
			idxnde_to_str (run, left, QSE_NULL, &len);
		if (str == QSE_NULL) return QSE_NULL;
	}

	/* evaluate the right-hand side of the operator */
	QSE_ASSERT (right->next == QSE_NULL);
	rv = eval_expression (run, right);
	if (rv == QSE_NULL) 
	{
		if (str != idxbuf) QSE_AWK_FREE (run->awk, str);
		return QSE_NULL;
	}

	qse_awk_rtx_refupval (run, rv);

	if (rv->type == QSE_AWK_VAL_NIL)
	{
		if (str != idxbuf) QSE_AWK_FREE (run->awk, str);
		qse_awk_rtx_refdownval (run, rv);
		return qse_awk_val_zero;
	}
	else if (rv->type == QSE_AWK_VAL_MAP)
	{
		qse_awk_val_t* res;
		qse_map_t* map;

		map = ((qse_awk_val_map_t*)rv)->map;
		res = (qse_map_search (map, str, len) == QSE_NULL)? 
			qse_awk_val_zero: qse_awk_val_one;

		if (str != idxbuf) QSE_AWK_FREE (run->awk, str);
		qse_awk_rtx_refdownval (run, rv);
		return res;
	}

	/* need a map */
	if (str != idxbuf) QSE_AWK_FREE (run->awk, str);
	qse_awk_rtx_refdownval (run, rv);

	qse_awk_rtx_seterror (run, QSE_AWK_ENOTMAPNILIN, right->line, QSE_NULL);
	return QSE_NULL;
}

static qse_awk_val_t* eval_binop_bor (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right)
{
	int n1, n2, n3;
	qse_long_t l1, l2;
	qse_real_t r1, r2;

	n1 = qse_awk_rtx_valtonum (run, left, &l1, &r1);
	n2 = qse_awk_rtx_valtonum (run, right, &l2, &r2);

	if (n1 == -1 || n2 == -1) 
	{
		qse_awk_rtx_seterrnum (run, QSE_AWK_EOPERAND);
		return QSE_NULL;
	}

	n3 = n1 + (n2 << 1);
	QSE_ASSERT (n3 >= 0 && n3 <= 3);
	return (n3 == 0)? qse_awk_rtx_makeintval(run,(qse_long_t)l1|(qse_long_t)l2):
	       (n3 == 1)? qse_awk_rtx_makeintval(run,(qse_long_t)r1|(qse_long_t)l2):
	       (n3 == 2)? qse_awk_rtx_makeintval(run,(qse_long_t)l1|(qse_long_t)r2):
	                  qse_awk_rtx_makeintval(run,(qse_long_t)r1|(qse_long_t)r2);
}

static qse_awk_val_t* eval_binop_bxor (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right)
{
	int n1, n2, n3;
	qse_long_t l1, l2;
	qse_real_t r1, r2;

	n1 = qse_awk_rtx_valtonum (run, left, &l1, &r1);
	n2 = qse_awk_rtx_valtonum (run, right, &l2, &r2);

	if (n1 == -1 || n2 == -1)
	{
		qse_awk_rtx_seterrnum (run, QSE_AWK_EOPERAND);
		return QSE_NULL;
	}

	n3 = n1 + (n2 << 1);
	QSE_ASSERT (n3 >= 0 && n3 <= 3);
	return (n3 == 0)? qse_awk_rtx_makeintval(run,(qse_long_t)l1^(qse_long_t)l2):
	       (n3 == 1)? qse_awk_rtx_makeintval(run,(qse_long_t)r1^(qse_long_t)l2):
	       (n3 == 2)? qse_awk_rtx_makeintval(run,(qse_long_t)l1^(qse_long_t)r2):
	                  qse_awk_rtx_makeintval(run,(qse_long_t)r1^(qse_long_t)r2);
}

static qse_awk_val_t* eval_binop_band (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right)
{
	int n1, n2, n3;
	qse_long_t l1, l2;
	qse_real_t r1, r2;

	n1 = qse_awk_rtx_valtonum (run, left, &l1, &r1);
	n2 = qse_awk_rtx_valtonum (run, right, &l2, &r2);

	if (n1 == -1 || n2 == -1) 
	{
		qse_awk_rtx_seterrnum (run, QSE_AWK_EOPERAND);
		return QSE_NULL;
	}

	n3 = n1 + (n2 << 1);
	QSE_ASSERT (n3 >= 0 && n3 <= 3);
	return (n3 == 0)? qse_awk_rtx_makeintval(run,(qse_long_t)l1&(qse_long_t)l2):
	       (n3 == 1)? qse_awk_rtx_makeintval(run,(qse_long_t)r1&(qse_long_t)l2):
	       (n3 == 2)? qse_awk_rtx_makeintval(run,(qse_long_t)l1&(qse_long_t)r2):
	                  qse_awk_rtx_makeintval(run,(qse_long_t)r1&(qse_long_t)r2);
}

static int __cmp_nil_nil (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right)
{
	return 0;
}

static int __cmp_nil_int (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right)
{
	if (((qse_awk_val_int_t*)right)->val < 0) return 1;
	if (((qse_awk_val_int_t*)right)->val > 0) return -1;
	return 0;
}

static int __cmp_nil_real (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right)
{
	if (((qse_awk_val_real_t*)right)->val < 0) return 1;
	if (((qse_awk_val_real_t*)right)->val > 0) return -1;
	return 0;
}

static int __cmp_nil_str (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right)
{
	return (((qse_awk_val_str_t*)right)->len == 0)? 0: -1;
}

static int __cmp_int_nil (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right)
{
	if (((qse_awk_val_int_t*)left)->val > 0) return 1;
	if (((qse_awk_val_int_t*)left)->val < 0) return -1;
	return 0;
}

static int __cmp_int_int (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right)
{
	if (((qse_awk_val_int_t*)left)->val > 
	    ((qse_awk_val_int_t*)right)->val) return 1;
	if (((qse_awk_val_int_t*)left)->val < 
	    ((qse_awk_val_int_t*)right)->val) return -1;
	return 0;
}

static int __cmp_int_real (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right)
{
	if (((qse_awk_val_int_t*)left)->val > 
	    ((qse_awk_val_real_t*)right)->val) return 1;
	if (((qse_awk_val_int_t*)left)->val < 
	    ((qse_awk_val_real_t*)right)->val) return -1;
	return 0;
}

static int __cmp_int_str (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right)
{
	qse_char_t* str;
	qse_size_t len;
	qse_long_t r;
	qse_real_t rr;
	int n;

	r = qse_awk_strxtolong (run->awk, 
		((qse_awk_val_str_t*)right)->ptr,
		((qse_awk_val_str_t*)right)->len, 0, (const qse_char_t**)&str);
	if (str == ((qse_awk_val_str_t*)right)->ptr + 
		   ((qse_awk_val_str_t*)right)->len)
	{
		if (((qse_awk_val_int_t*)left)->val > r) return 1;
		if (((qse_awk_val_int_t*)left)->val < r) return -1;
		return 0;
	}
/* TODO: should i do this???  conversion to real and comparision... */
	else if (*str == QSE_T('.') || *str == QSE_T('E') || *str == QSE_T('e'))
	{
		rr = qse_awk_strxtoreal (run->awk,
			((qse_awk_val_str_t*)right)->ptr,
			((qse_awk_val_str_t*)right)->len, 
			(const qse_char_t**)&str);
		if (str == ((qse_awk_val_str_t*)right)->ptr + 
			   ((qse_awk_val_str_t*)right)->len)
		{
			if (((qse_awk_val_int_t*)left)->val > rr) return 1;
			if (((qse_awk_val_int_t*)left)->val < rr) return -1;
			return 0;
		}
	}

	str = qse_awk_rtx_valtostr (
		run, left, QSE_AWK_VALTOSTR_CLEAR, QSE_NULL, &len);
	if (str == QSE_NULL) return CMP_ERROR;

	if (run->gbl.ignorecase)
	{
		n = qse_strxncasecmp (
			str, len,
			((qse_awk_val_str_t*)right)->ptr, 
			((qse_awk_val_str_t*)right)->len,
			&run->awk->ccls);
	}
	else
	{
		n = qse_strxncmp (
			str, len,
			((qse_awk_val_str_t*)right)->ptr, 
			((qse_awk_val_str_t*)right)->len);
	}

	QSE_AWK_FREE (run->awk, str);
	return n;
}

static int __cmp_real_nil (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right)
{
	if (((qse_awk_val_real_t*)left)->val > 0) return 1;
	if (((qse_awk_val_real_t*)left)->val < 0) return -1;
	return 0;
}

static int __cmp_real_int (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right)
{
	if (((qse_awk_val_real_t*)left)->val > 
	    ((qse_awk_val_int_t*)right)->val) return 1;
	if (((qse_awk_val_real_t*)left)->val < 
	    ((qse_awk_val_int_t*)right)->val) return -1;
	return 0;
}

static int __cmp_real_real (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right)
{
	if (((qse_awk_val_real_t*)left)->val > 
	    ((qse_awk_val_real_t*)right)->val) return 1;
	if (((qse_awk_val_real_t*)left)->val < 
	    ((qse_awk_val_real_t*)right)->val) return -1;
	return 0;
}

static int __cmp_real_str (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right)
{
	qse_char_t* str;
	qse_size_t len;
	qse_real_t rr;
	int n;

	rr = qse_awk_strxtoreal (run->awk,
		((qse_awk_val_str_t*)right)->ptr,
		((qse_awk_val_str_t*)right)->len, 
		(const qse_char_t**)&str);
	if (str == ((qse_awk_val_str_t*)right)->ptr + 
		   ((qse_awk_val_str_t*)right)->len)
	{
		if (((qse_awk_val_real_t*)left)->val > rr) return 1;
		if (((qse_awk_val_real_t*)left)->val < rr) return -1;
		return 0;
	}

	str = qse_awk_rtx_valtostr (
		run, left, QSE_AWK_VALTOSTR_CLEAR, QSE_NULL, &len);
	if (str == QSE_NULL) return CMP_ERROR;

	if (run->gbl.ignorecase)
	{
		n = qse_strxncasecmp (
			str, len,
			((qse_awk_val_str_t*)right)->ptr, 
			((qse_awk_val_str_t*)right)->len,
			&run->awk->ccls);
	}
	else
	{
		n = qse_strxncmp (
			str, len,
			((qse_awk_val_str_t*)right)->ptr, 
			((qse_awk_val_str_t*)right)->len);
	}

	QSE_AWK_FREE (run->awk, str);
	return n;
}

static int __cmp_str_nil (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right)
{
	return (((qse_awk_val_str_t*)left)->len == 0)? 0: 1;
}

static int __cmp_str_int (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right)
{
	return -__cmp_int_str (run, right, left);
}

static int __cmp_str_real (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right)
{
	return -__cmp_real_str (run, right, left);
}

static int __cmp_str_str (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right)
{
	int n;
	qse_awk_val_str_t* ls, * rs;

	ls = (qse_awk_val_str_t*)left;
	rs = (qse_awk_val_str_t*)right;

	if (run->gbl.ignorecase)
	{
		n = qse_strxncasecmp (
			ls->ptr, ls->len, rs->ptr, rs->len,
			&run->awk->ccls);
	}
	else
	{
		n = qse_strxncmp (
			ls->ptr, ls->len, rs->ptr, rs->len);
	}

	return n;
}

static int __cmp_val (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right)
{
	typedef int (*cmp_val_t) (qse_awk_rtx_t*, qse_awk_val_t*, qse_awk_val_t*);

	static cmp_val_t func[] =
	{
		/* this table must be synchronized with 
		 * the QSE_AWK_VAL_XXX values in awk.h */
		__cmp_nil_nil,  __cmp_nil_int,  __cmp_nil_real,  __cmp_nil_str,
		__cmp_int_nil,  __cmp_int_int,  __cmp_int_real,  __cmp_int_str,
		__cmp_real_nil, __cmp_real_int, __cmp_real_real, __cmp_real_str,
		__cmp_str_nil,  __cmp_str_int,  __cmp_str_real,  __cmp_str_str,
	};

	if (left->type == QSE_AWK_VAL_MAP || right->type == QSE_AWK_VAL_MAP)
	{
		/* a map can't be compared againt other values */
		qse_awk_rtx_seterrnum (run, QSE_AWK_EOPERAND);
		return CMP_ERROR; 
	}

	QSE_ASSERT (
		left->type >= QSE_AWK_VAL_NIL &&
		left->type <= QSE_AWK_VAL_STR);
	QSE_ASSERT (
		right->type >= QSE_AWK_VAL_NIL &&
		right->type <= QSE_AWK_VAL_STR);

	return func[left->type*4+right->type] (run, left, right);
}

static qse_awk_val_t* eval_binop_eq (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right)
{
	int n = __cmp_val (run, left, right);
	if (n == CMP_ERROR) return QSE_NULL;
	return (n == 0)? qse_awk_val_one: qse_awk_val_zero;
}

static qse_awk_val_t* eval_binop_ne (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right)
{
	int n = __cmp_val (run, left, right);
	if (n == CMP_ERROR) return QSE_NULL;
	return (n != 0)? qse_awk_val_one: qse_awk_val_zero;
}

static qse_awk_val_t* eval_binop_gt (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right)
{
	int n = __cmp_val (run, left, right);
	if (n == CMP_ERROR) return QSE_NULL;
	return (n > 0)? qse_awk_val_one: qse_awk_val_zero;
}

static qse_awk_val_t* eval_binop_ge (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right)
{
	int n = __cmp_val (run, left, right);
	if (n == CMP_ERROR) return QSE_NULL;
	return (n >= 0)? qse_awk_val_one: qse_awk_val_zero;
}

static qse_awk_val_t* eval_binop_lt (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right)
{
	int n = __cmp_val (run, left, right);
	if (n == CMP_ERROR) return QSE_NULL;
	return (n < 0)? qse_awk_val_one: qse_awk_val_zero;
}

static qse_awk_val_t* eval_binop_le (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right)
{
	int n = __cmp_val (run, left, right);
	if (n == CMP_ERROR) return QSE_NULL;
	return (n <= 0)? qse_awk_val_one: qse_awk_val_zero;
}

static qse_awk_val_t* eval_binop_lshift (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right)
{
	int n1, n2, n3;
	qse_long_t l1, l2;
	qse_real_t r1, r2;

	n1 = qse_awk_rtx_valtonum (run, left, &l1, &r1);
	n2 = qse_awk_rtx_valtonum (run, right, &l2, &r2);

	if (n1 == -1 || n2 == -1)
	{
		qse_awk_rtx_seterrnum (run, QSE_AWK_EOPERAND);
		return QSE_NULL;
	}

	n3 = n1 + (n2 << 1);
	if (n3 == 0)
	{
		return qse_awk_rtx_makeintval (run, (qse_long_t)l1<<(qse_long_t)l2);
	}
	else
	{
		qse_awk_rtx_seterrnum (run, QSE_AWK_EOPERAND);
		return QSE_NULL;
	}
}

static qse_awk_val_t* eval_binop_rshift (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right)
{
	int n1, n2, n3;
	qse_long_t l1, l2;
	qse_real_t r1, r2;

	n1 = qse_awk_rtx_valtonum (run, left, &l1, &r1);
	n2 = qse_awk_rtx_valtonum (run, right, &l2, &r2);

	if (n1 == -1 || n2 == -1)
	{
		qse_awk_rtx_seterrnum (run, QSE_AWK_EOPERAND);
		return QSE_NULL;
	}

	n3 = n1 + (n2 << 1);
	if (n3 == 0)
	{
		return qse_awk_rtx_makeintval (
			run, (qse_long_t)l1>>(qse_long_t)l2);
	}
	else
	{
		qse_awk_rtx_seterrnum (run, QSE_AWK_EOPERAND);
		return QSE_NULL;
	}
}

static qse_awk_val_t* eval_binop_plus (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right)
{
	int n1, n2, n3;
	qse_long_t l1, l2;
	qse_real_t r1, r2;

	n1 = qse_awk_rtx_valtonum (run, left, &l1, &r1);
	n2 = qse_awk_rtx_valtonum (run, right, &l2, &r2);

	if (n1 == -1 || n2 == -1)
	{
		qse_awk_rtx_seterrnum (run, QSE_AWK_EOPERAND);
		return QSE_NULL;
	}
	/*
	n1  n2    n3
	0   0   = 0
	1   0   = 1
	0   1   = 2
	1   1   = 3
	*/
	n3 = n1 + (n2 << 1);
	QSE_ASSERT (n3 >= 0 && n3 <= 3);

	return (n3 == 0)? qse_awk_rtx_makeintval(run,(qse_long_t)l1+(qse_long_t)l2):
	       (n3 == 1)? qse_awk_rtx_makerealval(run,(qse_real_t)r1+(qse_real_t)l2):
	       (n3 == 2)? qse_awk_rtx_makerealval(run,(qse_real_t)l1+(qse_real_t)r2):
	                  qse_awk_rtx_makerealval(run,(qse_real_t)r1+(qse_real_t)r2);
}

static qse_awk_val_t* eval_binop_minus (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right)
{
	int n1, n2, n3;
	qse_long_t l1, l2;
	qse_real_t r1, r2;

	n1 = qse_awk_rtx_valtonum (run, left, &l1, &r1);
	n2 = qse_awk_rtx_valtonum (run, right, &l2, &r2);

	if (n1 == -1 || n2 == -1)
	{
		qse_awk_rtx_seterrnum (run, QSE_AWK_EOPERAND);
		return QSE_NULL;
	}

	n3 = n1 + (n2 << 1);
	QSE_ASSERT (n3 >= 0 && n3 <= 3);
	return (n3 == 0)? qse_awk_rtx_makeintval(run,(qse_long_t)l1-(qse_long_t)l2):
	       (n3 == 1)? qse_awk_rtx_makerealval(run,(qse_real_t)r1-(qse_real_t)l2):
	       (n3 == 2)? qse_awk_rtx_makerealval(run,(qse_real_t)l1-(qse_real_t)r2):
	                  qse_awk_rtx_makerealval(run,(qse_real_t)r1-(qse_real_t)r2);
}

static qse_awk_val_t* eval_binop_mul (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right)
{
	int n1, n2, n3;
	qse_long_t l1, l2;
	qse_real_t r1, r2;

	n1 = qse_awk_rtx_valtonum (run, left, &l1, &r1);
	n2 = qse_awk_rtx_valtonum (run, right, &l2, &r2);

	if (n1 == -1 || n2 == -1)
	{
		qse_awk_rtx_seterrnum (run, QSE_AWK_EOPERAND);
		return QSE_NULL;
	}

	n3 = n1 + (n2 << 1);
	QSE_ASSERT (n3 >= 0 && n3 <= 3);
	return (n3 == 0)? qse_awk_rtx_makeintval(run,(qse_long_t)l1*(qse_long_t)l2):
	       (n3 == 1)? qse_awk_rtx_makerealval(run,(qse_real_t)r1*(qse_real_t)l2):
	       (n3 == 2)? qse_awk_rtx_makerealval(run,(qse_real_t)l1*(qse_real_t)r2):
	                  qse_awk_rtx_makerealval(run,(qse_real_t)r1*(qse_real_t)r2);
}

static qse_awk_val_t* eval_binop_div (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right)
{
	int n1, n2, n3;
	qse_long_t l1, l2;
	qse_real_t r1, r2;
	qse_awk_val_t* res;

	n1 = qse_awk_rtx_valtonum (run, left, &l1, &r1);
	n2 = qse_awk_rtx_valtonum (run, right, &l2, &r2);

	if (n1 == -1 || n2 == -1) 
	{
		qse_awk_rtx_seterrnum (run, QSE_AWK_EOPERAND);
		return QSE_NULL;
	}

	n3 = n1 + (n2 << 1);
	if (n3 == 0)
	{
		if  (l2 == 0) 
		{
			qse_awk_rtx_seterrnum (run, QSE_AWK_EDIVBY0);
			return QSE_NULL;
		}

		if (((qse_long_t)l1 % (qse_long_t)l2) == 0)
		{
			res = qse_awk_rtx_makeintval (
				run, (qse_long_t)l1 / (qse_long_t)l2);
		}
		else
		{
			res = qse_awk_rtx_makerealval (
				run, (qse_real_t)l1 / (qse_real_t)l2);
		}
	}
	else if (n3 == 1)
	{
		res = qse_awk_rtx_makerealval (
			run, (qse_real_t)r1 / (qse_real_t)l2);
	}
	else if (n3 == 2)
	{
		res = qse_awk_rtx_makerealval (
			run, (qse_real_t)l1 / (qse_real_t)r2);
	}
	else
	{
		QSE_ASSERT (n3 == 3);
		res = qse_awk_rtx_makerealval (
			run, (qse_real_t)r1 / (qse_real_t)r2);
	}

	return res;
}

static qse_awk_val_t* eval_binop_idiv (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right)
{
	int n1, n2, n3;
	qse_long_t l1, l2;
	qse_real_t r1, r2, quo;
	qse_awk_val_t* res;

	n1 = qse_awk_rtx_valtonum (run, left, &l1, &r1);
	n2 = qse_awk_rtx_valtonum (run, right, &l2, &r2);

	if (n1 == -1 || n2 == -1) 
	{
		qse_awk_rtx_seterrnum (run, QSE_AWK_EOPERAND);
		return QSE_NULL;
	}

	n3 = n1 + (n2 << 1);
	if (n3 == 0)
	{
		if (l2 == 0) 
		{
			qse_awk_rtx_seterrnum (run, QSE_AWK_EDIVBY0);
			return QSE_NULL;
		}
		res = qse_awk_rtx_makeintval (
			run, (qse_long_t)l1 / (qse_long_t)l2);
	}
	else if (n3 == 1)
	{
		quo = (qse_real_t)r1 / (qse_real_t)l2;
		res = qse_awk_rtx_makeintval (run, (qse_long_t)quo);
	}
	else if (n3 == 2)
	{
		quo = (qse_real_t)l1 / (qse_real_t)r2;
		res = qse_awk_rtx_makeintval (run, (qse_long_t)quo);
	}
	else
	{
		QSE_ASSERT (n3 == 3);

		quo = (qse_real_t)r1 / (qse_real_t)r2;
		res = qse_awk_rtx_makeintval (run, (qse_long_t)quo);
	}

	return res;
}

static qse_awk_val_t* eval_binop_mod (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right)
{
	int n1, n2, n3;
	qse_long_t l1, l2;
	qse_real_t r1, r2;
	qse_awk_val_t* res;

	n1 = qse_awk_rtx_valtonum (run, left, &l1, &r1);
	n2 = qse_awk_rtx_valtonum (run, right, &l2, &r2);

	if (n1 == -1 || n2 == -1)
	{
		qse_awk_rtx_seterrnum (run, QSE_AWK_EOPERAND);
		return QSE_NULL;
	}

	n3 = n1 + (n2 << 1);
	if (n3 == 0)
	{
		if  (l2 == 0) 
		{
			qse_awk_rtx_seterrnum (run, QSE_AWK_EDIVBY0);
			return QSE_NULL;
		}
		res = qse_awk_rtx_makeintval (
			run, (qse_long_t)l1 % (qse_long_t)l2);
	}
	else 
	{
		qse_awk_rtx_seterrnum (run, QSE_AWK_EOPERAND);
		return QSE_NULL;
	}

	return res;
}

static qse_awk_val_t* eval_binop_exp (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right)
{
	int n1, n2, n3;
	qse_long_t l1, l2;
	qse_real_t r1, r2;
	qse_awk_val_t* res;

	QSE_ASSERTX (run->awk->prm.pow != QSE_NULL,
		"the pow function should be provided when the awk object is created to make the exponentiation work properly.");

	n1 = qse_awk_rtx_valtonum (run, left, &l1, &r1);
	n2 = qse_awk_rtx_valtonum (run, right, &l2, &r2);

	if (n1 == -1 || n2 == -1) 
	{
		qse_awk_rtx_seterrnum (run, QSE_AWK_EOPERAND);
		return QSE_NULL;
	}

	n3 = n1 + (n2 << 1);
	if (n3 == 0)
	{
		/* left - int, right - int */
		if (l2 >= 0)
		{
			qse_long_t v = 1;
			while (l2-- > 0) v *= l1;
			res = qse_awk_rtx_makeintval (run, v);
		}
		else if (l1 == 0)
		{
			qse_awk_rtx_seterrnum (run, QSE_AWK_EDIVBY0);
			return QSE_NULL;
		}
		else
		{
			qse_real_t v = 1.0;
			l2 *= -1;
			while (l2-- > 0) v /= l1;
			res = qse_awk_rtx_makerealval (run, v);
		}
	}
	else if (n3 == 1)
	{
		/* left - real, right - int */
		if (l2 >= 0)
		{
			qse_real_t v = 1.0;
			while (l2-- > 0) v *= r1;
			res = qse_awk_rtx_makerealval (run, v);
		}
		else if (r1 == 0.0)
		{
			qse_awk_rtx_seterrnum (run, QSE_AWK_EDIVBY0);
			return QSE_NULL;
		}
		else
		{
			qse_real_t v = 1.0;
			l2 *= -1;
			while (l2-- > 0) v /= r1;
			res = qse_awk_rtx_makerealval (run, v);
		}
	}
	else if (n3 == 2)
	{
		/* left - int, right - real */
		res = qse_awk_rtx_makerealval (run, 
			run->awk->prm.pow (
				run->awk, (qse_real_t)l1, (qse_real_t)r2
			)
		);
	}
	else
	{
		/* left - real, right - real */
		QSE_ASSERT (n3 == 3);
		res = qse_awk_rtx_makerealval (run,
			run->awk->prm.pow (
				run->awk, (qse_real_t)r1,(qse_real_t)r2
			)
		);
	}

	return res;
}

static qse_awk_val_t* eval_binop_concat (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right)
{
	qse_char_t* strl, * strr;
	qse_size_t strl_len, strr_len;
	qse_awk_val_t* res;

	strl = qse_awk_rtx_valtostr (
		run, left, QSE_AWK_VALTOSTR_CLEAR, QSE_NULL, &strl_len);
	if (strl == QSE_NULL) return QSE_NULL;

	strr = qse_awk_rtx_valtostr (
		run, right, QSE_AWK_VALTOSTR_CLEAR, QSE_NULL, &strr_len);
	if (strr == QSE_NULL) 
	{
		QSE_AWK_FREE (run->awk, strl);
		return QSE_NULL;
	}

	res = qse_awk_rtx_makestrval2 (run, strl, strl_len, strr, strr_len);

	QSE_AWK_FREE (run->awk, strl);
	QSE_AWK_FREE (run->awk, strr);

	return res;
}

static qse_awk_val_t* eval_binop_ma (
	qse_awk_rtx_t* run, qse_awk_nde_t* left, qse_awk_nde_t* right)
{
	qse_awk_val_t* lv, * rv, * res;

	QSE_ASSERT (left->next == QSE_NULL);
	QSE_ASSERT (right->next == QSE_NULL);

	lv = eval_expression (run, left);
	if (lv == QSE_NULL) return QSE_NULL;

	qse_awk_rtx_refupval (run, lv);

	rv = eval_expression0 (run, right);
	if (rv == QSE_NULL)
	{
		qse_awk_rtx_refdownval (run, lv);
		return QSE_NULL;
	}

	qse_awk_rtx_refupval (run, rv);

	res = eval_binop_match0 (run, lv, rv, left->line, right->line, 1);

	qse_awk_rtx_refdownval (run, rv);
	qse_awk_rtx_refdownval (run, lv);

	return res;
}

static qse_awk_val_t* eval_binop_nm (
	qse_awk_rtx_t* run, qse_awk_nde_t* left, qse_awk_nde_t* right)
{
	qse_awk_val_t* lv, * rv, * res;

	QSE_ASSERT (left->next == QSE_NULL);
	QSE_ASSERT (right->next == QSE_NULL);

	lv = eval_expression (run, left);
	if (lv == QSE_NULL) return QSE_NULL;

	qse_awk_rtx_refupval (run, lv);

	rv = eval_expression0 (run, right);
	if (rv == QSE_NULL)
	{
		qse_awk_rtx_refdownval (run, lv);
		return QSE_NULL;
	}

	qse_awk_rtx_refupval (run, rv);

	res = eval_binop_match0 (
		run, lv, rv, left->line, right->line, 0);

	qse_awk_rtx_refdownval (run, rv);
	qse_awk_rtx_refdownval (run, lv);

	return res;
}

static qse_awk_val_t* eval_binop_match0 (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right, 
	qse_size_t lline, qse_size_t rline, int ret)
{
	qse_awk_val_t* res;
	int n, errnum;
	qse_char_t* str;
	qse_size_t len;
	void* rex_code;

	if (right->type == QSE_AWK_VAL_REX)
	{
		rex_code = ((qse_awk_val_rex_t*)right)->code;
	}
	else if (right->type == QSE_AWK_VAL_STR)
	{
		rex_code = QSE_AWK_BUILDREX ( 
			run->awk,
			((qse_awk_val_str_t*)right)->ptr,
			((qse_awk_val_str_t*)right)->len, &errnum);
		if (rex_code == QSE_NULL)
		{
			qse_awk_rtx_seterror (run, errnum, rline, QSE_NULL);
			return QSE_NULL;
		}
	}
	else
	{
		str = qse_awk_rtx_valtostr (
			run, right, QSE_AWK_VALTOSTR_CLEAR, QSE_NULL, &len);
		if (str == QSE_NULL) return QSE_NULL;

		rex_code = QSE_AWK_BUILDREX (run->awk, str, len, &errnum);
		if (rex_code == QSE_NULL)
		{
			QSE_AWK_FREE (run->awk, str);

			qse_awk_rtx_seterror (run, errnum, rline, QSE_NULL);
			return QSE_NULL;
		}

		QSE_AWK_FREE (run->awk, str);
	}

	if (left->type == QSE_AWK_VAL_STR)
	{
		n = QSE_AWK_MATCHREX (
			run->awk, rex_code,
			((run->gbl.ignorecase)? QSE_REX_IGNORECASE: 0),
			((qse_awk_val_str_t*)left)->ptr,
			((qse_awk_val_str_t*)left)->len,
			QSE_NULL, QSE_NULL, &errnum);
		if (n == -1) 
		{
			if (right->type != QSE_AWK_VAL_REX) 
				QSE_AWK_FREE (run->awk, rex_code);

			qse_awk_rtx_seterror (run, errnum, lline, QSE_NULL);
			return QSE_NULL;
		}

		res = qse_awk_rtx_makeintval (run, (n == ret));
		if (res == QSE_NULL) 
		{
			if (right->type != QSE_AWK_VAL_REX) 
				QSE_AWK_FREE (run->awk, rex_code);

			/* adjust error line */
			run->errlin = lline;
			return QSE_NULL;
		}
	}
	else
	{
		str = qse_awk_rtx_valtostr (
			run, left, QSE_AWK_VALTOSTR_CLEAR, QSE_NULL, &len);
		if (str == QSE_NULL) 
		{
			if (right->type != QSE_AWK_VAL_REX) 
				QSE_AWK_FREE (run->awk, rex_code);
			return QSE_NULL;
		}

		n = QSE_AWK_MATCHREX (
			run->awk, rex_code, 
			((run->gbl.ignorecase)? QSE_REX_IGNORECASE: 0),
			str, len, QSE_NULL, QSE_NULL, &errnum);
		if (n == -1) 
		{
			QSE_AWK_FREE (run->awk, str);
			if (right->type != QSE_AWK_VAL_REX) 
				QSE_AWK_FREE (run->awk, rex_code);

			qse_awk_rtx_seterror (run, errnum, lline, QSE_NULL);
			return QSE_NULL;
		}

		res = qse_awk_rtx_makeintval (run, (n == ret));
		if (res == QSE_NULL) 
		{
			QSE_AWK_FREE (run->awk, str);
			if (right->type != QSE_AWK_VAL_REX) 
				QSE_AWK_FREE (run->awk, rex_code);

			/* adjust error line */
			run->errlin = lline;
			return QSE_NULL;
		}

		QSE_AWK_FREE (run->awk, str);
	}

	if (right->type != QSE_AWK_VAL_REX) QSE_AWK_FREE (run->awk, rex_code);
	return res;
}

static qse_awk_val_t* eval_unary (qse_awk_rtx_t* run, qse_awk_nde_t* nde)
{
	qse_awk_val_t* left, * res = QSE_NULL;
	qse_awk_nde_exp_t* exp = (qse_awk_nde_exp_t*)nde;
	int n;
	qse_long_t l;
	qse_real_t r;

	QSE_ASSERT (
		exp->type == QSE_AWK_NDE_EXP_UNR);
	QSE_ASSERT (
		exp->left != QSE_NULL && exp->right == QSE_NULL);
	QSE_ASSERT (
		exp->opcode == QSE_AWK_UNROP_PLUS ||
		exp->opcode == QSE_AWK_UNROP_MINUS ||
		exp->opcode == QSE_AWK_UNROP_LNOT ||
		exp->opcode == QSE_AWK_UNROP_BNOT);

	QSE_ASSERT (exp->left->next == QSE_NULL);
	left = eval_expression (run, exp->left);
	if (left == QSE_NULL) return QSE_NULL;

	qse_awk_rtx_refupval (run, left);

	if (exp->opcode == QSE_AWK_UNROP_MINUS)
	{
		n = qse_awk_rtx_valtonum (run, left, &l, &r);
		if (n == -1) goto exit_func;

		res = (n == 0)? qse_awk_rtx_makeintval (run, -l):
		                qse_awk_rtx_makerealval (run, -r);
	}
	else if (exp->opcode == QSE_AWK_UNROP_LNOT)
	{
		if (left->type == QSE_AWK_VAL_STR)
		{
			res = qse_awk_rtx_makeintval (
				run, !(((qse_awk_val_str_t*)left)->len > 0));
		}
		else
		{
			n = qse_awk_rtx_valtonum (run, left, &l, &r);
			if (n == -1) goto exit_func;

			res = (n == 0)? qse_awk_rtx_makeintval (run, !l):
			                qse_awk_rtx_makerealval (run, !r);
		}
	}
	else if (exp->opcode == QSE_AWK_UNROP_BNOT)
	{
		n = qse_awk_rtx_valtonum (run, left, &l, &r);
		if (n == -1) goto exit_func;

		if (n == 1) l = (qse_long_t)r;
		res = qse_awk_rtx_makeintval (run, ~l);
	}
	else if (exp->opcode == QSE_AWK_UNROP_PLUS) 
	{
		n = qse_awk_rtx_valtonum (run, left, &l, &r);
		if (n == -1) goto exit_func;

		res = (n == 0)? qse_awk_rtx_makeintval (run, l):
		                qse_awk_rtx_makerealval (run, r);
	}

exit_func:
	qse_awk_rtx_refdownval (run, left);
	if (res == QSE_NULL) run->errlin = nde->line;
	return res;
}

static qse_awk_val_t* eval_incpre (qse_awk_rtx_t* run, qse_awk_nde_t* nde)
{
	qse_awk_val_t* left, * res;
	qse_awk_nde_exp_t* exp = (qse_awk_nde_exp_t*)nde;

	QSE_ASSERT (exp->type == QSE_AWK_NDE_EXP_INCPRE);
	QSE_ASSERT (exp->left != QSE_NULL && exp->right == QSE_NULL);

	/* this way of checking if the l-value is assignable is
	 * ugly as it is dependent of the values defined in tree.h.
	 * but let's keep going this way for the time being. */
	if (exp->left->type < QSE_AWK_NDE_NAMED ||
	    /*exp->left->type > QSE_AWK_NDE_ARGIDX) XXX */
	    exp->left->type > QSE_AWK_NDE_POS)
	{
		qse_awk_rtx_seterror (
			run, QSE_AWK_EOPERAND, nde->line, QSE_NULL);
		return QSE_NULL;
	}

	QSE_ASSERT (exp->left->next == QSE_NULL);
	left = eval_expression (run, exp->left);
	if (left == QSE_NULL) return QSE_NULL;

	qse_awk_rtx_refupval (run, left);

	if (exp->opcode == QSE_AWK_INCOP_PLUS) 
	{
		if (left->type == QSE_AWK_VAL_INT)
		{
			qse_long_t r = ((qse_awk_val_int_t*)left)->val;
			res = qse_awk_rtx_makeintval (run, r + 1);
			if (res == QSE_NULL) 
			{
				qse_awk_rtx_refdownval (run, left);
				/* adjust error line */
				run->errlin = nde->line;
				return QSE_NULL;
			}
		}
		else if (left->type == QSE_AWK_VAL_REAL)
		{
			qse_real_t r = ((qse_awk_val_real_t*)left)->val;
			res = qse_awk_rtx_makerealval (run, r + 1.0);
			if (res == QSE_NULL) 
			{
				qse_awk_rtx_refdownval (run, left);
				/* adjust error line */
				run->errlin = nde->line;
				return QSE_NULL;
			}
		}
		else
		{
			qse_long_t v1;
			qse_real_t v2;
			int n;

			n = qse_awk_rtx_valtonum (run, left, &v1, &v2);
			if (n == -1)
			{
				qse_awk_rtx_refdownval (run, left);
				/*
				qse_awk_rtx_seterror (
					run, QSE_AWK_EOPERAND, nde->line,
					QSE_NULL);
				*/
				run->errlin = nde->line;
				return QSE_NULL;
			}

			if (n == 0) 
			{
				res = qse_awk_rtx_makeintval (run, v1 + 1);
			}
			else /* if (n == 1) */
			{
				QSE_ASSERT (n == 1);
				res = qse_awk_rtx_makerealval (run, v2 + 1.0);
			}

			if (res == QSE_NULL) 
			{
				qse_awk_rtx_refdownval (run, left);
				/* adjust error line */
				run->errlin = nde->line;
				return QSE_NULL;
			}
		}
	}
	else if (exp->opcode == QSE_AWK_INCOP_MINUS)
	{
		if (left->type == QSE_AWK_VAL_INT)
		{
			qse_long_t r = ((qse_awk_val_int_t*)left)->val;
			res = qse_awk_rtx_makeintval (run, r - 1);
			if (res == QSE_NULL) 
			{
				qse_awk_rtx_refdownval (run, left);
				/* adjust error line */
				run->errlin = nde->line;
				return QSE_NULL;
			}
		}
		else if (left->type == QSE_AWK_VAL_REAL)
		{
			qse_real_t r = ((qse_awk_val_real_t*)left)->val;
			res = qse_awk_rtx_makerealval (run, r - 1.0);
			if (res == QSE_NULL) 
			{
				qse_awk_rtx_refdownval (run, left);
				/* adjust error line */
				run->errlin = nde->line;
				return QSE_NULL;
			}
		}
		else
		{
			qse_long_t v1;
			qse_real_t v2;
			int n;

			n = qse_awk_rtx_valtonum (run, left, &v1, &v2);
			if (n == -1)
			{
				qse_awk_rtx_refdownval (run, left);
				/*
				qse_awk_rtx_seterror (
					run, QSE_AWK_EOPERAND, nde->line, 
					QSE_NULL);
				*/
				run->errlin = nde->line;
				return QSE_NULL;
			}

			if (n == 0) 
			{
				res = qse_awk_rtx_makeintval (run, v1 - 1);
			}
			else /* if (n == 1) */
			{
				QSE_ASSERT (n == 1);
				res = qse_awk_rtx_makerealval (run, v2 - 1.0);
			}

			if (res == QSE_NULL) 
			{
				qse_awk_rtx_refdownval (run, left);
				/* adjust error line */
				run->errlin = nde->line;
				return QSE_NULL;
			}
		}
	}
	else
	{
		QSE_ASSERT (
			!"should never happen - invalid opcode");
		qse_awk_rtx_refdownval (run, left);

		qse_awk_rtx_seterror (
			run, QSE_AWK_EINTERN, nde->line, QSE_NULL);
		return QSE_NULL;
	}

	if (do_assignment (run, exp->left, res) == QSE_NULL)
	{
		qse_awk_rtx_refdownval (run, left);
		return QSE_NULL;
	}

	qse_awk_rtx_refdownval (run, left);
	return res;
}

static qse_awk_val_t* eval_incpst (qse_awk_rtx_t* run, qse_awk_nde_t* nde)
{
	qse_awk_val_t* left, * res, * res2;
	qse_awk_nde_exp_t* exp = (qse_awk_nde_exp_t*)nde;

	QSE_ASSERT (exp->type == QSE_AWK_NDE_EXP_INCPST);
	QSE_ASSERT (exp->left != QSE_NULL && exp->right == QSE_NULL);

	/* this way of checking if the l-value is assignable is
	 * ugly as it is dependent of the values defined in tree.h.
	 * but let's keep going this way for the time being. */
	if (exp->left->type < QSE_AWK_NDE_NAMED ||
	    /*exp->left->type > QSE_AWK_NDE_ARGIDX) XXX */
	    exp->left->type > QSE_AWK_NDE_POS)
	{
		qse_awk_rtx_seterror (
			run, QSE_AWK_EOPERAND, nde->line, QSE_NULL);
		return QSE_NULL;
	}

	QSE_ASSERT (exp->left->next == QSE_NULL);
	left = eval_expression (run, exp->left);
	if (left == QSE_NULL) return QSE_NULL;

	qse_awk_rtx_refupval (run, left);

	if (exp->opcode == QSE_AWK_INCOP_PLUS) 
	{
		if (left->type == QSE_AWK_VAL_INT)
		{
			qse_long_t r = ((qse_awk_val_int_t*)left)->val;
			res = qse_awk_rtx_makeintval (run, r);
			if (res == QSE_NULL) 
			{
				qse_awk_rtx_refdownval (run, left);
				/* adjust error line */
				run->errlin = nde->line;
				return QSE_NULL;
			}

			res2 = qse_awk_rtx_makeintval (run, r + 1);
			if (res2 == QSE_NULL)
			{
				qse_awk_rtx_refdownval (run, left);
				qse_awk_rtx_freeval (run, res, QSE_TRUE);
				/* adjust error line */
				run->errlin = nde->line;
				return QSE_NULL;
			}
		}
		else if (left->type == QSE_AWK_VAL_REAL)
		{
			qse_real_t r = ((qse_awk_val_real_t*)left)->val;
			res = qse_awk_rtx_makerealval (run, r);
			if (res == QSE_NULL) 
			{
				qse_awk_rtx_refdownval (run, left);
				/* adjust error line */
				run->errlin = nde->line;
				return QSE_NULL;
			}

			res2 = qse_awk_rtx_makerealval (run, r + 1.0);
			if (res2 == QSE_NULL)
			{
				qse_awk_rtx_refdownval (run, left);
				qse_awk_rtx_freeval (run, res, QSE_TRUE);
				/* adjust error line */
				run->errlin = nde->line;
				return QSE_NULL;
			}
		}
		else
		{
			qse_long_t v1;
			qse_real_t v2;
			int n;

			n = qse_awk_rtx_valtonum (run, left, &v1, &v2);
			if (n == -1)
			{
				qse_awk_rtx_refdownval (run, left);
				/*
				qse_awk_rtx_seterror (
					run, QSE_AWK_EOPERAND, nde->line,
					QSE_NULL);
				*/
				run->errlin = nde->line;
				return QSE_NULL;
			}

			if (n == 0) 
			{
				res = qse_awk_rtx_makeintval (run, v1);
				if (res == QSE_NULL)
				{
					qse_awk_rtx_refdownval (run, left);
					/* adjust error line */
					run->errlin = nde->line;
					return QSE_NULL;
				}

				res2 = qse_awk_rtx_makeintval (run, v1 + 1);
				if (res2 == QSE_NULL)
				{
					qse_awk_rtx_refdownval (run, left);
					qse_awk_rtx_freeval (run, res, QSE_TRUE);
					/* adjust error line */
					run->errlin = nde->line;
					return QSE_NULL;
				}
			}
			else /* if (n == 1) */
			{
				QSE_ASSERT (n == 1);
				res = qse_awk_rtx_makerealval (run, v2);
				if (res == QSE_NULL)
				{
					qse_awk_rtx_refdownval (run, left);
					/* adjust error line */
					run->errlin = nde->line;
					return QSE_NULL;
				}

				res2 = qse_awk_rtx_makerealval (run, v2 + 1.0);
				if (res2 == QSE_NULL)
				{
					qse_awk_rtx_refdownval (run, left);
					qse_awk_rtx_freeval (run, res, QSE_TRUE);
					/* adjust error line */
					run->errlin = nde->line;
					return QSE_NULL;
				}
			}
		}
	}
	else if (exp->opcode == QSE_AWK_INCOP_MINUS)
	{
		if (left->type == QSE_AWK_VAL_INT)
		{
			qse_long_t r = ((qse_awk_val_int_t*)left)->val;
			res = qse_awk_rtx_makeintval (run, r);
			if (res == QSE_NULL) 
			{
				qse_awk_rtx_refdownval (run, left);
				/* adjust error line */
				run->errlin = nde->line;
				return QSE_NULL;
			}

			res2 = qse_awk_rtx_makeintval (run, r - 1);
			if (res2 == QSE_NULL)
			{
				qse_awk_rtx_refdownval (run, left);
				qse_awk_rtx_freeval (run, res, QSE_TRUE);
				/* adjust error line */
				run->errlin = nde->line;
				return QSE_NULL;
			}
		}
		else if (left->type == QSE_AWK_VAL_REAL)
		{
			qse_real_t r = ((qse_awk_val_real_t*)left)->val;
			res = qse_awk_rtx_makerealval (run, r);
			if (res == QSE_NULL) 
			{
				qse_awk_rtx_refdownval (run, left);
				/* adjust error line */
				run->errlin = nde->line;
				return QSE_NULL;
			}

			res2 = qse_awk_rtx_makerealval (run, r - 1.0);
			if (res2 == QSE_NULL)
			{
				qse_awk_rtx_refdownval (run, left);
				qse_awk_rtx_freeval (run, res, QSE_TRUE);
				/* adjust error line */
				run->errlin = nde->line;
				return QSE_NULL;
			}
		}
		else
		{
			qse_long_t v1;
			qse_real_t v2;
			int n;

			n = qse_awk_rtx_valtonum (run, left, &v1, &v2);
			if (n == -1)
			{
				qse_awk_rtx_refdownval (run, left);
				/*
				qse_awk_rtx_seterror (
					run, QSE_AWK_EOPERAND, nde->line, 
					QSE_NULL);
				*/
				run->errlin = nde->line;
				return QSE_NULL;
			}

			if (n == 0) 
			{
				res = qse_awk_rtx_makeintval (run, v1);
				if (res == QSE_NULL)
				{
					qse_awk_rtx_refdownval (run, left);
					/* adjust error line */
					run->errlin = nde->line;
					return QSE_NULL;
				}

				res2 = qse_awk_rtx_makeintval (run, v1 - 1);
				if (res2 == QSE_NULL)
				{
					qse_awk_rtx_refdownval (run, left);
					qse_awk_rtx_freeval (run, res, QSE_TRUE);
					/* adjust error line */
					run->errlin = nde->line;
					return QSE_NULL;
				}
			}
			else /* if (n == 1) */
			{
				QSE_ASSERT (n == 1);
				res = qse_awk_rtx_makerealval (run, v2);
				if (res == QSE_NULL)
				{
					qse_awk_rtx_refdownval (run, left);
					/* adjust error line */
					run->errlin = nde->line;
					return QSE_NULL;
				}

				res2 = qse_awk_rtx_makerealval (run, v2 - 1.0);
				if (res2 == QSE_NULL)
				{
					qse_awk_rtx_refdownval (run, left);
					qse_awk_rtx_freeval (run, res, QSE_TRUE);
					/* adjust error line */
					run->errlin = nde->line;
					return QSE_NULL;
				}
			}
		}
	}
	else
	{
		QSE_ASSERT (
			!"should never happen - invalid opcode");
		qse_awk_rtx_refdownval (run, left);

		qse_awk_rtx_seterror (
			run, QSE_AWK_EINTERN, nde->line, QSE_NULL);
		return QSE_NULL;
	}

	if (do_assignment (run, exp->left, res2) == QSE_NULL)
	{
		qse_awk_rtx_refdownval (run, left);
		return QSE_NULL;
	}

	qse_awk_rtx_refdownval (run, left);
	return res;
}

static qse_awk_val_t* eval_cnd (qse_awk_rtx_t* run, qse_awk_nde_t* nde)
{
	qse_awk_val_t* tv, * v;
	qse_awk_nde_cnd_t* cnd = (qse_awk_nde_cnd_t*)nde;

	QSE_ASSERT (cnd->test->next == QSE_NULL);

	tv = eval_expression (run, cnd->test);
	if (tv == QSE_NULL) return QSE_NULL;

	qse_awk_rtx_refupval (run, tv);

	QSE_ASSERT (
		cnd->left->next == QSE_NULL && 
		cnd->right->next == QSE_NULL);
	v = (qse_awk_rtx_valtobool (run, tv))?
		eval_expression (run, cnd->left):
		eval_expression (run, cnd->right);

	qse_awk_rtx_refdownval (run, tv);
	return v;
}

static qse_awk_val_t* eval_fnc (qse_awk_rtx_t* run, qse_awk_nde_t* nde)
{
	qse_awk_nde_call_t* call = (qse_awk_nde_call_t*)nde;

	/* intrinsic function */
	if (call->nargs < call->what.fnc.arg.min)
	{
		qse_awk_rtx_seterror (
			run, QSE_AWK_EARGTF, nde->line, QSE_NULL);
		return QSE_NULL;
	}

	if (call->nargs > call->what.fnc.arg.max)
	{
		qse_awk_rtx_seterror (
			run, QSE_AWK_EARGTM, nde->line, QSE_NULL);
		return QSE_NULL;
	}

	return eval_call (
		run, nde, call->what.fnc.arg.spec, 
		QSE_NULL, QSE_NULL, QSE_NULL);
}

static qse_awk_val_t* eval_fun_ex (
	qse_awk_rtx_t* run, qse_awk_nde_t* nde, void(*errhandler)(void*), void* eharg)
{
	qse_awk_nde_call_t* call = (qse_awk_nde_call_t*)nde;
	qse_awk_fun_t* fun;
	qse_map_pair_t* pair;

	pair = qse_map_search (run->awk->tree.funs, 
		call->what.fun.name.ptr, call->what.fun.name.len);
	if (pair == QSE_NULL) 
	{
		qse_cstr_t errarg;

		errarg.ptr = call->what.fun.name.ptr;
		errarg.len = call->what.fun.name.len,

		qse_awk_rtx_seterror (run, 
			QSE_AWK_EFUNNONE, nde->line, &errarg);
		return QSE_NULL;
	}

	fun = (qse_awk_fun_t*)QSE_MAP_VPTR(pair);
	QSE_ASSERT (fun != QSE_NULL);

	if (call->nargs > fun->nargs)
	{
		/* TODO: is this correct? what if i want to 
		 *       allow arbitarary numbers of arguments? */
		qse_awk_rtx_seterror (
			run, QSE_AWK_EARGTM, nde->line, QSE_NULL);
		return QSE_NULL;
	}

	return eval_call (run, nde, QSE_NULL, fun, errhandler, eharg);
}

static qse_awk_val_t* eval_fun (qse_awk_rtx_t* run, qse_awk_nde_t* nde)
{
	return eval_fun_ex (run, nde, QSE_NULL, QSE_NULL);
}

/* run->stack_base has not been set for this  
 * stack frame. so the STACK_ARG macro cannot be used as in 
 * qse_awk_rtx_refdownval (run, STACK_ARG(run,nargs));*/ 
#define UNWIND_RTX_STACK_ARG(rtx,nargs) \
	do { \
		while ((nargs) > 0) \
		{ \
			--(nargs); \
			qse_awk_rtx_refdownval ((rtx), \
				(rtx)->stack[(rtx)->stack_top-1]); \
			__raw_pop (rtx); \
		} \
	} while (0)

#define UNWIND_RTX_STACK_BASE(rtx) \
	do { \
		__raw_pop (rtx); /* nargs */ \
		__raw_pop (rtx); /* return */ \
		__raw_pop (rtx); /* prev stack top */ \
		__raw_pop (rtx); /* prev stack back */  \
	} while (0)

#define UNWIND_RTX_STACK(rtx,nargs) \
	do { \
		UNWIND_RTX_STACK_ARG(rtx,nargs); \
		UNWIND_RTX_STACK_BASE(rtx); \
	} while (0)

static qse_awk_val_t* __eval_call (
	qse_awk_rtx_t* run, 
	qse_awk_nde_t* nde, 
	const qse_char_t* fnc_arg_spec,
	qse_awk_fun_t* fun, 
	qse_size_t(*argpusher)(qse_awk_rtx_t*,qse_awk_nde_call_t*,void*), 
	void* apdata,
	void(*errhandler)(void*),
	void* eharg)
{
	qse_awk_nde_call_t* call = (qse_awk_nde_call_t*)nde;
	qse_size_t saved_stack_top;
	qse_size_t nargs, i;
	qse_awk_val_t* v;
	int n;

	/* 
	 * ---------------------
	 *  lcln               <- stack top
	 * ---------------------
	 *  ....
	 * ---------------------
	 *  lcl0               local variables are pushed by run_block
	 * =====================
	 *  argn                     
	 * ---------------------
	 *  ....
	 * ---------------------
	 *  arg1
	 * ---------------------
	 *  arg0 
	 * ---------------------
	 *  nargs 
	 * ---------------------
	 *  return value
	 * ---------------------
	 *  previous stack top
	 * ---------------------
	 *  previous stack base  <- stack base
	 * =====================
	 *  0 (nargs)            <- stack top
	 * ---------------------
	 *  return value
	 * ---------------------
	 *  previous stack top
	 * ---------------------
	 *  previous stack base  <- stack base
	 * =====================
	 *  gbln
	 * ---------------------
	 *  ....
	 * ---------------------
	 *  gbl0
	 * ---------------------
	 */

	QSE_ASSERT (
		QSE_SIZEOF(void*) >= QSE_SIZEOF(run->stack_top));
	QSE_ASSERT (
		QSE_SIZEOF(void*) >= QSE_SIZEOF(run->stack_base));

	saved_stack_top = run->stack_top;

#ifdef DEBUG_RUN
	qse_dprintf (QSE_T("setting up function stack frame top=%ld base=%ld\n"), 
		(long)run->stack_top, (long)run->stack_base);
#endif
	if (__raw_push(run,(void*)run->stack_base) == -1) 
	{
		qse_awk_rtx_seterror (
			run, QSE_AWK_ENOMEM, nde->line, QSE_NULL);
		return QSE_NULL;
	}

	if (__raw_push(run,(void*)saved_stack_top) == -1) 
	{
		__raw_pop (run);
		qse_awk_rtx_seterror (
			run, QSE_AWK_ENOMEM, nde->line, QSE_NULL);
		return QSE_NULL;
	}

	/* secure space for a return value. */
	if (__raw_push(run,qse_awk_val_nil) == -1)
	{
		__raw_pop (run);
		__raw_pop (run);
		qse_awk_rtx_seterror (
			run, QSE_AWK_ENOMEM, nde->line, QSE_NULL);
		return QSE_NULL;
	}

	/* secure space for nargs */
	if (__raw_push(run,qse_awk_val_nil) == -1)
	{
		__raw_pop (run);
		__raw_pop (run);
		__raw_pop (run);
		qse_awk_rtx_seterror (
			run, QSE_AWK_ENOMEM, nde->line, QSE_NULL);
		return QSE_NULL;
	}

	/* push all arguments onto the stack */
	nargs = argpusher (run, call, apdata);
	if (nargs == (qse_size_t)-1)
	{
		UNWIND_RTX_STACK_BASE (run);
		return QSE_NULL;
	}

	QSE_ASSERT (nargs == call->nargs);

	if (fun != QSE_NULL)
	{
		/* extra step for normal awk functions */

		while (nargs < fun->nargs)
		{
			/* push as many nils as the number of missing actual arguments */
			if (__raw_push(run,qse_awk_val_nil) == -1)
			{
				UNWIND_RTX_STACK (run, nargs);
				qse_awk_rtx_seterror (
					run, QSE_AWK_ENOMEM, nde->line,
					QSE_NULL);
				return QSE_NULL;
			}

			nargs++;
		}
	}

	run->stack_base = saved_stack_top;
	STACK_NARGS(run) = (void*)nargs;
	
#ifdef DEBUG_RUN
	qse_dprintf (QSE_T("running function body\n"));
#endif

	if (fun != QSE_NULL)
	{
		/* normal awk function */
		QSE_ASSERT (fun->body->type == QSE_AWK_NDE_BLK);
		n = run_block(run,(qse_awk_nde_blk_t*)fun->body);
	}
	else
	{
		n = 0;

		/* intrinsic function */
		QSE_ASSERT (
			call->nargs >= call->what.fnc.arg.min &&
			call->nargs <= call->what.fnc.arg.max);

		if (call->what.fnc.handler != QSE_NULL)
		{
			run->errnum = QSE_AWK_ENOERR;

			/* NOTE: oname is used when the handler is invoked.
			 *       name might be differnt from oname if 
			 *       qse_awk_setword has been used */
			n = call->what.fnc.handler (
				run,
				call->what.fnc.oname.ptr, 
				call->what.fnc.oname.len);

			if (n <= -1)
			{
				if (run->errnum == QSE_AWK_ENOERR)
				{
					/* the handler has not set the error.
					 * fix it */ 
					qse_awk_rtx_seterror (
						run, QSE_AWK_EFNCIMPL, 
						nde->line, QSE_NULL);
				}
				else
				{
					/* adjust the error line */
					run->errlin = nde->line;	
				}

				/* correct the return code just in case */
				if (n < -1) n = -1;
			}
		}
	}

	/* refdown args in the run.stack */
	nargs = (qse_size_t)STACK_NARGS(run);
#ifdef DEBUG_RUN
	qse_dprintf (QSE_T("block run complete nargs = %d\n"), (int)nargs); 
#endif
	for (i = 0; i < nargs; i++)
	{
		qse_awk_rtx_refdownval (run, STACK_ARG(run,i));
	}

#ifdef DEBUG_RUN
	qse_dprintf (QSE_T("got return value\n"));
#endif

	v = STACK_RETVAL(run);
	if (n == -1)
	{
		if (errhandler != QSE_NULL) 
		{
			/* capture_retval_on_exit takes advantage of 
			 * this handler to retrieve the return value
			 * when exit() is used to terminate the program. */
			errhandler (eharg);
		}

		/* if the earlier operations failed and this function
		 * has to return a error, the return value is just
		 * destroyed and replaced by nil */
		qse_awk_rtx_refdownval (run, v);
		STACK_RETVAL(run) = qse_awk_val_nil;
	}
	else
	{	
		/* this trick has been mentioned in run_return.
		 * adjust the reference count of the return value.
		 * the value must not be freed even if the reference count
		 * reached zero because its reference has been incremented 
		 * in run_return or directly by qse_awk_rtx_setretval()
		 * regardless of its reference count. */
		qse_awk_rtx_refdownval_nofree (run, v);
	}

	run->stack_top =  (qse_size_t)run->stack[run->stack_base+1];
	run->stack_base = (qse_size_t)run->stack[run->stack_base+0];

	if (run->exit_level == EXIT_FUNCTION) run->exit_level = EXIT_NONE;

#ifdef DEBUG_RUN
	qse_dprintf (QSE_T("returning from function top=%ld, base=%ld\n"),
		(long)run->stack_top, (long)run->stack_base); 
#endif
	return (n == -1)? QSE_NULL: v;
}

static qse_size_t push_arg_from_vals (qse_awk_rtx_t* rtx, qse_awk_nde_call_t* call, void* data)
{
	struct pafv* pafv = (struct pafv*)data;
	qse_size_t nargs = 0;

	for (nargs = 0; nargs < pafv->nargs; nargs++)
	{
		if (__raw_push (rtx, pafv->args[nargs]) == -1) 
		{
			/* ugly - v needs to be freed if it doesn't have
			 * any reference. but its reference has not been 
			 * updated yet as it is carried out after the 
			 * successful stack push. so it adds up a reference 
			 * and dereferences it */
			qse_awk_rtx_refupval (rtx, pafv->args[nargs]);
			qse_awk_rtx_refdownval (rtx, pafv->args[nargs]);

			UNWIND_RTX_STACK_ARG (rtx, nargs);
			qse_awk_rtx_seterror (
				rtx, QSE_AWK_ENOMEM, call->line, QSE_NULL);
			return (qse_size_t)-1;
		}
	}

	return nargs; 
}

static qse_size_t push_arg_from_nde (qse_awk_rtx_t* rtx, qse_awk_nde_call_t* call, void* data)
{
	qse_awk_nde_t* p;
	qse_awk_val_t* v;
	qse_size_t nargs;
	const qse_char_t* fnc_arg_spec = (const qse_char_t*)data;

	for (p = call->args, nargs = 0; p != QSE_NULL; p = p->next, nargs++)
	{
		QSE_ASSERT (
			fnc_arg_spec == QSE_NULL ||
			(fnc_arg_spec != QSE_NULL && 
			 qse_strlen(fnc_arg_spec) > nargs));

		if (fnc_arg_spec != QSE_NULL && 
		    fnc_arg_spec[nargs] == QSE_T('r'))
		{
			qse_awk_val_t** ref;
			      
			if (get_reference (rtx, p, &ref) == -1)
			{
				UNWIND_RTX_STACK_ARG (rtx, nargs);
				return (qse_size_t)-1;
			}

			/* p->type-QSE_AWK_NDE_NAMED assumes that the
			 * derived value matches QSE_AWK_VAL_REF_XXX */
			v = qse_awk_rtx_makerefval (
				rtx, p->type-QSE_AWK_NDE_NAMED, ref);
		}
		else if (fnc_arg_spec != QSE_NULL && 
		         fnc_arg_spec[nargs] == QSE_T('x'))
		{
			/* a regular expression is passed to 
			 * the function as it is */
			v = eval_expression0 (rtx, p);
		}
		else
		{
			v = eval_expression (rtx, p);
		}
		if (v == QSE_NULL)
		{
			UNWIND_RTX_STACK_ARG (rtx, nargs);
			return (qse_size_t)-1;
		}

		if (__raw_push(rtx,v) == -1) 
		{
			/* ugly - v needs to be freed if it doesn't have
			 * any reference. but its reference has not been 
			 * updated yet as it is carried out after the 
			 * successful stack push. so it adds up a reference 
			 * and dereferences it */
			qse_awk_rtx_refupval (rtx, v);
			qse_awk_rtx_refdownval (rtx, v);

			UNWIND_RTX_STACK_ARG (rtx, nargs);
			qse_awk_rtx_seterror (
				rtx, QSE_AWK_ENOMEM, call->line, QSE_NULL);
			return (qse_size_t)-1;
		}

		qse_awk_rtx_refupval (rtx, v);
	}

	return nargs;
}

static qse_awk_val_t* eval_call (
	qse_awk_rtx_t* rtx, qse_awk_nde_t* nde, 
	const qse_char_t* fnc_arg_spec, qse_awk_fun_t* fun, 
	void(*errhandler)(void*), void* eharg)
{
	return __eval_call (
		rtx, nde, fnc_arg_spec, fun, 
		push_arg_from_nde, (void*)fnc_arg_spec,
		errhandler, eharg);
}

static int get_reference (
	qse_awk_rtx_t* run, qse_awk_nde_t* nde, qse_awk_val_t*** ref)
{
	qse_awk_nde_var_t* tgt = (qse_awk_nde_var_t*)nde;
	qse_awk_val_t** tmp;

	/* refer to eval_indexed for application of a similar concept */

	if (nde->type == QSE_AWK_NDE_NAMED)
	{
		qse_map_pair_t* pair;

		pair = qse_map_search (
			run->named, tgt->id.name.ptr, tgt->id.name.len);
		if (pair == QSE_NULL)
		{
			/* it is bad that the named variable has to be
			 * created in the function named "__get_refernce".
			 * would there be any better ways to avoid this? */
			pair = qse_map_upsert (
				run->named, tgt->id.name.ptr,
				tgt->id.name.len, qse_awk_val_nil, 0);
			if (pair == QSE_NULL) 
			{
				qse_awk_rtx_seterror (
					run, QSE_AWK_ENOMEM, nde->line,
					QSE_NULL);
				return -1;
			}
		}

		*ref = (qse_awk_val_t**)&QSE_MAP_VPTR(pair);
		return 0;
	}

	if (nde->type == QSE_AWK_NDE_GBL)
	{
		*ref = (qse_awk_val_t**)&STACK_GBL(run,tgt->id.idxa);
		return 0;
	}

	if (nde->type == QSE_AWK_NDE_LCL)
	{
		*ref = (qse_awk_val_t**)&STACK_LCL(run,tgt->id.idxa);
		return 0;
	}

	if (nde->type == QSE_AWK_NDE_ARG)
	{
		*ref = (qse_awk_val_t**)&STACK_ARG(run,tgt->id.idxa);
		return 0;
	}

	if (nde->type == QSE_AWK_NDE_NAMEDIDX)
	{
		qse_map_pair_t* pair;

		pair = qse_map_search (
			run->named, tgt->id.name.ptr, tgt->id.name.len);
		if (pair == QSE_NULL)
		{
			pair = qse_map_upsert (
				run->named, tgt->id.name.ptr,
				tgt->id.name.len, qse_awk_val_nil, 0);
			if (pair == QSE_NULL) 
			{
				qse_awk_rtx_seterror (
					run, QSE_AWK_ENOMEM, nde->line,
					QSE_NULL);
				return -1;
			}
		}

		tmp = get_reference_indexed (
			run, tgt, (qse_awk_val_t**)&QSE_MAP_VPTR(pair));
		if (tmp == QSE_NULL) return -1;
		*ref = tmp;
	}

	if (nde->type == QSE_AWK_NDE_GBLIDX)
	{
		tmp = get_reference_indexed (run, tgt, 
			(qse_awk_val_t**)&STACK_GBL(run,tgt->id.idxa));
		if (tmp == QSE_NULL) return -1;
		*ref = tmp;
	}

	if (nde->type == QSE_AWK_NDE_LCLIDX)
	{
		tmp = get_reference_indexed (run, tgt, 
			(qse_awk_val_t**)&STACK_LCL(run,tgt->id.idxa));
		if (tmp == QSE_NULL) return -1;
		*ref = tmp;
	}

	if (nde->type == QSE_AWK_NDE_ARGIDX)
	{
		tmp = get_reference_indexed (run, tgt, 
			(qse_awk_val_t**)&STACK_ARG(run,tgt->id.idxa));
		if (tmp == QSE_NULL) return -1;
		*ref = tmp;
	}

	if (nde->type == QSE_AWK_NDE_POS)
	{
		int n;
		qse_long_t lv;
		qse_real_t rv;
		qse_awk_val_t* v;

		/* the position number is returned for the positional 
		 * variable unlike other reference types. */
		v = eval_expression (run, ((qse_awk_nde_pos_t*)nde)->val);
		if (v == QSE_NULL) return -1;

		qse_awk_rtx_refupval (run, v);
		n = qse_awk_rtx_valtonum (run, v, &lv, &rv);
		qse_awk_rtx_refdownval (run, v);

		if (n == -1) 
		{
			qse_awk_rtx_seterror (
				run, QSE_AWK_EPOSIDX, nde->line, QSE_NULL);
			return -1;
		}
		if (n == 1) lv = (qse_long_t)rv;
		if (!IS_VALID_POSIDX(lv)) 
		{
			qse_awk_rtx_seterror (
				run, QSE_AWK_EPOSIDX, nde->line, QSE_NULL);
			return -1;
		}

		*ref = (qse_awk_val_t**)((qse_size_t)lv);
		return 0;
	}

	qse_awk_rtx_seterror (run, QSE_AWK_ENOTREF, nde->line, QSE_NULL);
	return -1;
}

static qse_awk_val_t** get_reference_indexed (
	qse_awk_rtx_t* run, qse_awk_nde_var_t* nde, qse_awk_val_t** val)
{
	qse_map_pair_t* pair;
	qse_char_t* str;
	qse_size_t len;
	qse_char_t idxbuf[IDXBUFSIZE];

	QSE_ASSERT (val != QSE_NULL);

	if ((*val)->type == QSE_AWK_VAL_NIL)
	{
		qse_awk_val_t* tmp;

		tmp = qse_awk_rtx_makemapval (run);
		if (tmp == QSE_NULL)
		{
			run->errlin = nde->line;
			return QSE_NULL;
		}

		qse_awk_rtx_refdownval (run, *val);
		*val = tmp;
		qse_awk_rtx_refupval (run, (qse_awk_val_t*)*val);
	}
	else if ((*val)->type != QSE_AWK_VAL_MAP) 
	{
		qse_awk_rtx_seterror (
			run, QSE_AWK_ENOTMAP, nde->line, QSE_NULL);
		return QSE_NULL;
	}

	QSE_ASSERT (nde->idx != QSE_NULL);

	len = QSE_COUNTOF(idxbuf);
	str = idxnde_to_str (run, nde->idx, idxbuf, &len);
	if (str == QSE_NULL)
	{
		str = idxnde_to_str (run, nde->idx, QSE_NULL, &len);
		if (str == QSE_NULL) return QSE_NULL;
	}

	pair = qse_map_search ((*(qse_awk_val_map_t**)val)->map, str, len);
	if (pair == QSE_NULL)
	{
		pair = qse_map_upsert (
			(*(qse_awk_val_map_t**)val)->map, 
			str, len, qse_awk_val_nil, 0);
		if (pair == QSE_NULL)
		{
			if (str != idxbuf) QSE_AWK_FREE (run->awk, str);
			qse_awk_rtx_seterror (
				run, QSE_AWK_ENOMEM, nde->line, QSE_NULL);
			return QSE_NULL;
		}

		qse_awk_rtx_refupval (run, QSE_MAP_VPTR(pair));
	}

	if (str != idxbuf) QSE_AWK_FREE (run->awk, str);
	return (qse_awk_val_t**)&QSE_MAP_VPTR(pair);
}

static qse_awk_val_t* eval_int (qse_awk_rtx_t* run, qse_awk_nde_t* nde)
{
	qse_awk_val_t* val;

	val = qse_awk_rtx_makeintval (run, ((qse_awk_nde_int_t*)nde)->val);
	if (val == QSE_NULL)
	{
		run->errlin = nde->line;
		return QSE_NULL;
	}
	((qse_awk_val_int_t*)val)->nde = nde;

	return val;
}

static qse_awk_val_t* eval_real (qse_awk_rtx_t* run, qse_awk_nde_t* nde)
{
	qse_awk_val_t* val;

	val = qse_awk_rtx_makerealval (run, ((qse_awk_nde_real_t*)nde)->val);
	if (val == QSE_NULL) 
	{
		run->errlin = nde->line;
		return QSE_NULL;
	}
	((qse_awk_val_real_t*)val)->nde = nde;

	return val;
}

static qse_awk_val_t* eval_str (qse_awk_rtx_t* run, qse_awk_nde_t* nde)
{
	qse_awk_val_t* val;

	val = qse_awk_rtx_makestrval (run,
		((qse_awk_nde_str_t*)nde)->ptr,
		((qse_awk_nde_str_t*)nde)->len);
	if (val == QSE_NULL)
	{
		run->errlin = nde->line;
		return QSE_NULL;
	}

	return val;
}

static qse_awk_val_t* eval_rex (qse_awk_rtx_t* run, qse_awk_nde_t* nde)
{
	qse_awk_val_t* val;

	val = qse_awk_rtx_makerexval (run,
		((qse_awk_nde_rex_t*)nde)->ptr,
		((qse_awk_nde_rex_t*)nde)->len,
		((qse_awk_nde_rex_t*)nde)->code);
	if (val == QSE_NULL) 
	{
		run->errlin = nde->line;
		return QSE_NULL;
	}

	return val;
}

static qse_awk_val_t* eval_named (qse_awk_rtx_t* run, qse_awk_nde_t* nde)
{
	qse_map_pair_t* pair;
		       
	pair = qse_map_search (run->named, 
		((qse_awk_nde_var_t*)nde)->id.name.ptr, 
		((qse_awk_nde_var_t*)nde)->id.name.len);

	return (pair == QSE_NULL)? qse_awk_val_nil: QSE_MAP_VPTR(pair);
}

static qse_awk_val_t* eval_gbl (qse_awk_rtx_t* run, qse_awk_nde_t* nde)
{
	return STACK_GBL(run,((qse_awk_nde_var_t*)nde)->id.idxa);
}

static qse_awk_val_t* eval_lcl (qse_awk_rtx_t* run, qse_awk_nde_t* nde)
{
	return STACK_LCL(run,((qse_awk_nde_var_t*)nde)->id.idxa);
}

static qse_awk_val_t* eval_arg (qse_awk_rtx_t* run, qse_awk_nde_t* nde)
{
	return STACK_ARG(run,((qse_awk_nde_var_t*)nde)->id.idxa);
}

static qse_awk_val_t* eval_indexed (
	qse_awk_rtx_t* run, qse_awk_nde_var_t* nde, qse_awk_val_t** val)
{
	qse_map_pair_t* pair;
	qse_char_t* str;
	qse_size_t len;
	qse_char_t idxbuf[IDXBUFSIZE];

	QSE_ASSERT (val != QSE_NULL);

	if ((*val)->type == QSE_AWK_VAL_NIL)
	{
		qse_awk_val_t* tmp;

		tmp = qse_awk_rtx_makemapval (run);
		if (tmp == QSE_NULL)
		{
			run->errlin = nde->line;
			return QSE_NULL;
		}

		qse_awk_rtx_refdownval (run, *val);
		*val = tmp;
		qse_awk_rtx_refupval (run, (qse_awk_val_t*)*val);
	}
	else if ((*val)->type != QSE_AWK_VAL_MAP) 
	{
		qse_awk_rtx_seterror (
			run, QSE_AWK_ENOTMAP, nde->line, QSE_NULL);
		return QSE_NULL;
	}

	QSE_ASSERT (nde->idx != QSE_NULL);

	len = QSE_COUNTOF(idxbuf);
	str = idxnde_to_str (run, nde->idx, idxbuf, &len);
	if (str == QSE_NULL) 
	{
		str = idxnde_to_str (run, nde->idx, QSE_NULL, &len);
		if (str == QSE_NULL) return QSE_NULL;
	}

	pair = qse_map_search ((*(qse_awk_val_map_t**)val)->map, str, len);
	if (str != idxbuf) QSE_AWK_FREE (run->awk, str);

	return (pair == QSE_NULL)? qse_awk_val_nil: 
	                           (qse_awk_val_t*)QSE_MAP_VPTR(pair);
}

static qse_awk_val_t* eval_namedidx (qse_awk_rtx_t* run, qse_awk_nde_t* nde)
{
	qse_awk_nde_var_t* tgt = (qse_awk_nde_var_t*)nde;
	qse_map_pair_t* pair;

	pair = qse_map_search (run->named, tgt->id.name.ptr, tgt->id.name.len);
	if (pair == QSE_NULL)
	{
		pair = qse_map_upsert (run->named, 
			tgt->id.name.ptr, tgt->id.name.len, qse_awk_val_nil, 0);
		if (pair == QSE_NULL) 
		{
			qse_awk_rtx_seterror (
				run, QSE_AWK_ENOMEM, nde->line, QSE_NULL);
			return QSE_NULL;
		}

		qse_awk_rtx_refupval (run, QSE_MAP_VPTR(pair)); 
	}

	return eval_indexed (run, tgt, (qse_awk_val_t**)&QSE_MAP_VPTR(pair));
}

static qse_awk_val_t* eval_gblidx (qse_awk_rtx_t* run, qse_awk_nde_t* nde)
{
	return eval_indexed (run, (qse_awk_nde_var_t*)nde, 
		(qse_awk_val_t**)&STACK_GBL(run,((qse_awk_nde_var_t*)nde)->id.idxa));
}

static qse_awk_val_t* eval_lclidx (qse_awk_rtx_t* run, qse_awk_nde_t* nde)
{
	return eval_indexed (run, (qse_awk_nde_var_t*)nde, 
		(qse_awk_val_t**)&STACK_LCL(run,((qse_awk_nde_var_t*)nde)->id.idxa));
}

static qse_awk_val_t* eval_argidx (qse_awk_rtx_t* run, qse_awk_nde_t* nde)
{
	return eval_indexed (run, (qse_awk_nde_var_t*)nde,
		(qse_awk_val_t**)&STACK_ARG(run,((qse_awk_nde_var_t*)nde)->id.idxa));
}

static qse_awk_val_t* eval_pos (qse_awk_rtx_t* run, qse_awk_nde_t* nde)
{
	qse_awk_nde_pos_t* pos = (qse_awk_nde_pos_t*)nde;
	qse_awk_val_t* v;
	qse_long_t lv;
	qse_real_t rv;
	int n;

	v = eval_expression (run, pos->val);
	if (v == QSE_NULL) return QSE_NULL;

	qse_awk_rtx_refupval (run, v);
	n = qse_awk_rtx_valtonum (run, v, &lv, &rv);
	qse_awk_rtx_refdownval (run, v);
	if (n == -1) 
	{
		qse_awk_rtx_seterror (
			run, QSE_AWK_EPOSIDX, nde->line, QSE_NULL);
		return QSE_NULL;
	}
	if (n == 1) lv = (qse_long_t)rv;

	if (lv < 0)
	{
		qse_awk_rtx_seterror (
			run, QSE_AWK_EPOSIDX, nde->line, QSE_NULL);
		return QSE_NULL;
	}
	if (lv == 0) v = run->inrec.d0;
	else if (lv > 0 && lv <= (qse_long_t)run->inrec.nflds) 
		v = run->inrec.flds[lv-1].val;
	else v = qse_awk_val_zls; /*qse_awk_val_nil;*/

	return v;
}

static qse_awk_val_t* eval_getline (qse_awk_rtx_t* run, qse_awk_nde_t* nde)
{
	qse_awk_nde_getline_t* p;
	qse_awk_val_t* v, * res;
	qse_char_t* in = QSE_NULL;
	const qse_char_t* dst;
	qse_str_t buf;
	int n;

	p = (qse_awk_nde_getline_t*)nde;

	QSE_ASSERT (
		(p->in_type == QSE_AWK_IN_PIPE && p->in != QSE_NULL) ||
		(p->in_type == QSE_AWK_IN_RWPIPE && p->in != QSE_NULL) ||
		(p->in_type == QSE_AWK_IN_FILE && p->in != QSE_NULL) ||
		(p->in_type == QSE_AWK_IN_CONSOLE && p->in == QSE_NULL));

	if (p->in != QSE_NULL)
	{
		qse_size_t len;

		v = eval_expression (run, p->in);
		if (v == QSE_NULL) return QSE_NULL;

		/* TODO: distinction between v->type == QSE_AWK_VAL_STR 
		 *       and v->type != QSE_AWK_VAL_STR
		 *       if you use the buffer the v directly when
		 *       v->type == QSE_AWK_VAL_STR, qse_awk_rtx_refdownval(v)
		 *       should not be called immediately below */
		qse_awk_rtx_refupval (run, v);
		in = qse_awk_rtx_valtostr (
			run, v, QSE_AWK_VALTOSTR_CLEAR, QSE_NULL, &len);
		if (in == QSE_NULL) 
		{
			qse_awk_rtx_refdownval (run, v);
			return QSE_NULL;
		}
		qse_awk_rtx_refdownval (run, v);

		if (len <= 0) 
		{
			/* the input source name is empty.
			 * make getline return -1 */
			QSE_AWK_FREE (run->awk, in);
			n = -1;
			goto skip_read;
		}

		while (len > 0)
		{
			if (in[--len] == QSE_T('\0'))
			{
				/* the input source name contains a null 
				 * character. make getline return -1 */
				QSE_AWK_FREE (run->awk, in);
				n = -1;
				goto skip_read;
			}
		}
	}

	dst = (in == QSE_NULL)? QSE_T(""): in;

	/* TODO: optimize the line buffer management */
	if (qse_str_init (&buf, MMGR(run), DEF_BUF_CAPA) == QSE_NULL)
	{
		if (in != QSE_NULL) QSE_AWK_FREE (run->awk, in);
		qse_awk_rtx_seterror (
			run, QSE_AWK_ENOMEM, nde->line, QSE_NULL);
		return QSE_NULL;
	}

	n = qse_awk_rtx_readio (run, p->in_type, dst, &buf);
	if (in != QSE_NULL) QSE_AWK_FREE (run->awk, in);

	if (n <= -1) 
	{
		/* make getline return -1 */
		n = -1;
	}

	if (n > 0)
	{
		if (p->var == QSE_NULL)
		{
			/* set $0 with the input value */
			if (qse_awk_rtx_setrec (run, 0,
				QSE_STR_PTR(&buf),
				QSE_STR_LEN(&buf)) == -1)
			{
				qse_str_fini (&buf);
				return QSE_NULL;
			}

			qse_str_fini (&buf);
		}
		else
		{
			qse_awk_val_t* v;

			v = qse_awk_rtx_makestrval (run, 
				QSE_STR_PTR(&buf), QSE_STR_LEN(&buf));
			qse_str_fini (&buf);
			if (v == QSE_NULL)
			{
				run->errlin = nde->line;
				return QSE_NULL;
			}

			qse_awk_rtx_refupval (run, v);
			if (do_assignment(run, p->var, v) == QSE_NULL)
			{
				qse_awk_rtx_refdownval (run, v);
				return QSE_NULL;
			}
			qse_awk_rtx_refdownval (run, v);
		}
	}
	else
	{
		qse_str_fini (&buf);
	}
	
skip_read:
	res = qse_awk_rtx_makeintval (run, n);
	if (res == QSE_NULL) run->errlin = nde->line;
	return res;
}

static int __raw_push (qse_awk_rtx_t* run, void* val)
{
	if (run->stack_top >= run->stack_limit)
	{
		void** tmp;
		qse_size_t n;
	       
		n = run->stack_limit + STACK_INCREMENT;

		if (MMGR(run)->realloc != QSE_NULL)
		{
			tmp = (void**) QSE_AWK_REALLOC (
				run->awk, run->stack, n * QSE_SIZEOF(void*)); 
			if (tmp == QSE_NULL) return -1;
		}
		else
		{
			tmp = (void**) QSE_AWK_ALLOC (
				run->awk, n * QSE_SIZEOF(void*));
			if (tmp == QSE_NULL) return -1;
			if (run->stack != QSE_NULL)
			{
				QSE_MEMCPY (
					tmp, run->stack, 
					run->stack_limit * QSE_SIZEOF(void*)); 
				QSE_AWK_FREE (run->awk, run->stack);
			}
		}
		run->stack = tmp;
		run->stack_limit = n;
	}

	run->stack[run->stack_top++] = val;
	return 0;
}

static int read_record (qse_awk_rtx_t* run)
{
	qse_ssize_t n;

	if (qse_awk_rtx_clrrec (run, QSE_FALSE) == -1) return -1;

	n = qse_awk_rtx_readio (
		run, QSE_AWK_IN_CONSOLE, QSE_T(""), &run->inrec.line);
	if (n <= -1) 
	{
		qse_awk_rtx_clrrec (run, QSE_FALSE);
		return -1;
	}

#ifdef DEBUG_RUN
	qse_dprintf (QSE_T("record len = %d str=[%.*s]\n"), 
			(int)QSE_STR_LEN(&run->inrec.line),
			(int)QSE_STR_LEN(&run->inrec.line),
			QSE_STR_PTR(&run->inrec.line));
#endif
	if (n == 0) 
	{
		QSE_ASSERT (QSE_STR_LEN(&run->inrec.line) == 0);
		return 0;
	}

	if (qse_awk_rtx_setrec (run, 0, 
		QSE_STR_PTR(&run->inrec.line), 
		QSE_STR_LEN(&run->inrec.line)) == -1) return -1;

	return 1;
}

static int shorten_record (qse_awk_rtx_t* run, qse_size_t nflds)
{
	qse_awk_val_t* v;
	qse_char_t* ofs_free = QSE_NULL, * ofs;
	qse_size_t ofs_len, i;
	qse_str_t tmp;

	QSE_ASSERT (nflds <= run->inrec.nflds);

	if (nflds > 1)
	{
		v = STACK_GBL(run, QSE_AWK_GBL_OFS);
		qse_awk_rtx_refupval (run, v);

		if (v->type == QSE_AWK_VAL_NIL)
		{
			/* OFS not set */
			ofs = QSE_T(" ");
			ofs_len = 1;
		}
		else if (v->type == QSE_AWK_VAL_STR)
		{
			ofs = ((qse_awk_val_str_t*)v)->ptr;
			ofs_len = ((qse_awk_val_str_t*)v)->len;
		}
		else
		{
			ofs = qse_awk_rtx_valtostr (
				run, v, QSE_AWK_VALTOSTR_CLEAR, 
				QSE_NULL, &ofs_len);
			if (ofs == QSE_NULL) return -1;

			ofs_free = ofs;
		}
	}

	if (qse_str_init (
		&tmp, MMGR(run), QSE_STR_LEN(&run->inrec.line)) == QSE_NULL)
	{
		qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM);
		return -1;
	}

	for (i = 0; i < nflds; i++)
	{
		if (i > 0 && qse_str_ncat(&tmp,ofs,ofs_len) == (qse_size_t)-1)
		{
			if (ofs_free != QSE_NULL) 
				QSE_AWK_FREE (run->awk, ofs_free);
			if (nflds > 1) qse_awk_rtx_refdownval (run, v);
			qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM);
			return -1;
		}

		if (qse_str_ncat (&tmp, 
			run->inrec.flds[i].ptr, 
			run->inrec.flds[i].len) == (qse_size_t)-1)
		{
			if (ofs_free != QSE_NULL) 
				QSE_AWK_FREE (run->awk, ofs_free);
			if (nflds > 1) qse_awk_rtx_refdownval (run, v);

			qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM);
			return -1;
		}
	}

	if (ofs_free != QSE_NULL) QSE_AWK_FREE (run->awk, ofs_free);
	if (nflds > 1) qse_awk_rtx_refdownval (run, v);

	v = (qse_awk_val_t*) qse_awk_rtx_makestrval (
		run, QSE_STR_PTR(&tmp), QSE_STR_LEN(&tmp));
	if (v == QSE_NULL) return -1;

	qse_awk_rtx_refdownval (run, run->inrec.d0);
	run->inrec.d0 = v;
	qse_awk_rtx_refupval (run, run->inrec.d0);

	qse_str_swap (&tmp, &run->inrec.line);
	qse_str_fini (&tmp);

	for (i = nflds; i < run->inrec.nflds; i++)
	{
		qse_awk_rtx_refdownval (run, run->inrec.flds[i].val);
	}

	run->inrec.nflds = nflds;
	return 0;
}

static qse_char_t* idxnde_to_str (
	qse_awk_rtx_t* run, qse_awk_nde_t* nde, qse_char_t* buf, qse_size_t* len)
{
	qse_char_t* str;
	qse_awk_val_t* idx;

	QSE_ASSERT (nde != QSE_NULL);

	if (nde->next == QSE_NULL)
	{
		/* single node index */
		idx = eval_expression (run, nde);
		if (idx == QSE_NULL) return QSE_NULL;

		qse_awk_rtx_refupval (run, idx);

		str = QSE_NULL;

		if (buf != QSE_NULL)
		{
			/* try with a fixed-size buffer */
			str = qse_awk_rtx_valtostr (
				run, idx, QSE_AWK_VALTOSTR_FIXED, (qse_str_t*)buf, len);
		}

		if (str == QSE_NULL)
		{
			/* if it doen't work, switch to the dynamic mode */
			str = qse_awk_rtx_valtostr (
				run, idx, QSE_AWK_VALTOSTR_CLEAR, QSE_NULL, len);

			if (str == QSE_NULL) 
			{
				qse_awk_rtx_refdownval (run, idx);
				/* change error line */
				run->errlin = nde->line;
				return QSE_NULL;
			}
		}

		qse_awk_rtx_refdownval (run, idx);
	}
	else
	{
		/* multidimensional index */
		qse_str_t idxstr;
		qse_xstr_t tmp;

		if (qse_str_init (&idxstr, MMGR(run), DEF_BUF_CAPA) == QSE_NULL) 
		{
			qse_awk_rtx_seterror (
				run, QSE_AWK_ENOMEM, nde->line, QSE_NULL);
			return QSE_NULL;
		}

		while (nde != QSE_NULL)
		{
			idx = eval_expression (run, nde);
			if (idx == QSE_NULL) 
			{
				qse_str_fini (&idxstr);
				return QSE_NULL;
			}

			qse_awk_rtx_refupval (run, idx);

			if (QSE_STR_LEN(&idxstr) > 0 &&
			    qse_str_ncat (&idxstr, 
			    	run->gbl.subsep.ptr, 
			    	run->gbl.subsep.len) == (qse_size_t)-1)
			{
				qse_awk_rtx_refdownval (run, idx);
				qse_str_fini (&idxstr);

				qse_awk_rtx_seterror (
					run, QSE_AWK_ENOMEM, nde->line, QSE_NULL);

				return QSE_NULL;
			}

			if (qse_awk_rtx_valtostr (
				run, idx, 0, &idxstr, QSE_NULL) == QSE_NULL)
			{
				qse_awk_rtx_refdownval (run, idx);
				qse_str_fini (&idxstr);
				return QSE_NULL;
			}

			qse_awk_rtx_refdownval (run, idx);
			nde = nde->next;
		}

		qse_str_yield (&idxstr, &tmp, 0);
		str = tmp.ptr;
		*len = tmp.len;

		qse_str_fini (&idxstr);
	}

	return str;
}

qse_char_t* qse_awk_rtx_format (
	qse_awk_rtx_t* run, qse_str_t* out, qse_str_t* fbu,
	const qse_char_t* fmt, qse_size_t fmt_len, 
	qse_size_t nargs_on_stack, qse_awk_nde_t* args, qse_size_t* len)
{
	qse_size_t i, j;
	qse_size_t stack_arg_idx = 1;
	qse_awk_val_t* val;

#define OUT_CHAR(c) \
	do { \
		if (qse_str_ccat (out, (c)) == -1) \
		{ \
			qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM); \
			return QSE_NULL; \
		} \
	} while (0)

#define FMT_CHAR(c) \
	do { \
		if (qse_str_ccat (fbu, (c)) == -1) \
		{ \
			qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM); \
			return QSE_NULL; \
		} \
	} while (0)

#define GROW(buf) \
	do { \
		if ((buf)->ptr != QSE_NULL) \
		{ \
			QSE_AWK_FREE (run->awk, (buf)->ptr); \
			(buf)->ptr = QSE_NULL; \
		} \
		(buf)->len += (buf)->inc; \
		(buf)->ptr = (qse_char_t*)QSE_AWK_ALLOC ( \
			run->awk, (buf)->len * QSE_SIZEOF(qse_char_t)); \
		if ((buf)->ptr == QSE_NULL) (buf)->len = 0; \
	} while (0) 

	QSE_ASSERTX (run->format.tmp.ptr != QSE_NULL,
		"run->format.tmp.ptr should have been assigned a pointer to a block of memory before this function has been called");

	if (nargs_on_stack == (qse_size_t)-1) 
	{
		val = (qse_awk_val_t*)args;
		nargs_on_stack = 2;
	}
	else 
	{
		val = QSE_NULL;
	}

	if (out == QSE_NULL) out = &run->format.out;
	if (fbu == QSE_NULL) fbu = &run->format.fmt;

	qse_str_clear (out);
	qse_str_clear (fbu);

	for (i = 0; i < fmt_len; i++)
	{
		qse_long_t width = -1, prec = -1;
		qse_bool_t minus = QSE_FALSE;

		if (QSE_STR_LEN(fbu) == 0)
		{
			if (fmt[i] == QSE_T('%')) FMT_CHAR (fmt[i]);
			else OUT_CHAR (fmt[i]);
			continue;
		}

		while (i < fmt_len &&
		       (fmt[i] == QSE_T(' ') || fmt[i] == QSE_T('#') ||
		        fmt[i] == QSE_T('0') || fmt[i] == QSE_T('+') ||
		        fmt[i] == QSE_T('-')))
		{
			if (fmt[i] == QSE_T('-')) minus = QSE_TRUE;
			FMT_CHAR (fmt[i]); i++;
		}

		if (i < fmt_len && fmt[i] == QSE_T('*'))
		{
			qse_awk_val_t* v;
			qse_real_t r;
			qse_char_t* p;
			int n;

			if (args == QSE_NULL)
			{
				if (stack_arg_idx >= nargs_on_stack)
				{
					qse_awk_rtx_seterrnum (run, QSE_AWK_EFMTARG);
					return QSE_NULL;
				}
				v = qse_awk_rtx_getarg (run, stack_arg_idx);
			}
			else
			{
				if (val != QSE_NULL) 
				{
					if (stack_arg_idx >= nargs_on_stack)
					{
						qse_awk_rtx_seterrnum (run, QSE_AWK_EFMTARG);
						return QSE_NULL;
					}
					v = val;
				}
				else 
				{
					v = eval_expression (run, args);
					if (v == QSE_NULL) return QSE_NULL;
				}
			}

			qse_awk_rtx_refupval (run, v);
			n = qse_awk_rtx_valtonum (run, v, &width, &r);
			qse_awk_rtx_refdownval (run, v);

			if (n == -1) return QSE_NULL; 
			if (n == 1) width = (qse_long_t)r;

			do
			{
				n = run->awk->prm.sprintf (
					run->awk,
					run->format.tmp.ptr, 
					run->format.tmp.len,
				#if QSE_SIZEOF_LONG_LONG > 0
					QSE_T("%lld"), (long long)width
				#elif QSE_SIZEOF___INT64 > 0
					QSE_T("%I64d"), (__int64)width
				#elif QSE_SIZEOF_LONG > 0
					QSE_T("%ld"), (long)width
				#elif QSE_SIZEOF_INT > 0
					QSE_T("%d"), (int)width
				#else
					#error unsupported size	
				#endif
					);
				if (n == -1)
				{
					GROW (&run->format.tmp);
					if (run->format.tmp.ptr == QSE_NULL)
					{
						qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM);
						return QSE_NULL;
					}

					continue;
				}

				break;
			}
			while (1);

			p = run->format.tmp.ptr;
			while (*p != QSE_T('\0'))
			{
				FMT_CHAR (*p);
				p++;
			}

			if (args == QSE_NULL || val != QSE_NULL) stack_arg_idx++;
			else args = args->next;
			i++;
		}
		else
		{
			if (i < fmt_len && QSE_AWK_ISDIGIT(run->awk, fmt[i]))
			{
				width = 0;
				do
				{
					width = width * 10 + fmt[i] - QSE_T('0');
					FMT_CHAR (fmt[i]); i++;
				}
				while (i < fmt_len && QSE_AWK_ISDIGIT(run->awk, fmt[i]));
			}
		}

		if (i < fmt_len && fmt[i] == QSE_T('.'))
		{
			prec = 0;
			FMT_CHAR (fmt[i]); i++;
		}

		if (i < fmt_len && fmt[i] == QSE_T('*'))
		{
			qse_awk_val_t* v;
			qse_real_t r;
			qse_char_t* p;
			int n;

			if (args == QSE_NULL)
			{
				if (stack_arg_idx >= nargs_on_stack)
				{
					qse_awk_rtx_seterrnum (run, QSE_AWK_EFMTARG);
					return QSE_NULL;
				}
				v = qse_awk_rtx_getarg (run, stack_arg_idx);
			}
			else
			{
				if (val != QSE_NULL) 
				{
					if (stack_arg_idx >= nargs_on_stack)
					{
						qse_awk_rtx_seterrnum (run, QSE_AWK_EFMTARG);
						return QSE_NULL;
					}
					v = val;
				}
				else 
				{
					v = eval_expression (run, args);
					if (v == QSE_NULL) return QSE_NULL;
				}
			}

			qse_awk_rtx_refupval (run, v);
			n = qse_awk_rtx_valtonum (run, v, &prec, &r);
			qse_awk_rtx_refdownval (run, v);

			if (n == -1) return QSE_NULL; 
			if (n == 1) prec = (qse_long_t)r;

			do
			{
				n = run->awk->prm.sprintf (
					run->awk,
					run->format.tmp.ptr, 
					run->format.tmp.len,
				#if QSE_SIZEOF_LONG_LONG > 0
					QSE_T("%lld"), (long long)prec
				#elif QSE_SIZEOF___INT64 > 0
					QSE_T("%I64d"), (__int64)prec
				#elif QSE_SIZEOF_LONG > 0
					QSE_T("%ld"), (long)prec
				#elif QSE_SIZEOF_INT > 0
					QSE_T("%d"), (int)prec
				#endif
					);
				if (n == -1)
				{
					GROW (&run->format.tmp);
					if (run->format.tmp.ptr == QSE_NULL)
					{
						qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM);
						return QSE_NULL;
					}

					continue;
				}

				break;
			}
			while (1);

			p = run->format.tmp.ptr;
			while (*p != QSE_T('\0'))
			{
				FMT_CHAR (*p);
				p++;
			}

			if (args == QSE_NULL || val != QSE_NULL) stack_arg_idx++;
			else args = args->next;
			i++;
		}
		else
		{
			if (i < fmt_len && QSE_AWK_ISDIGIT(run->awk, fmt[i]))
			{
				prec = 0;
				do
				{
					prec = prec * 10 + fmt[i] - QSE_T('0');
					FMT_CHAR (fmt[i]); i++;
				}
				while (i < fmt_len && QSE_AWK_ISDIGIT(run->awk, fmt[i]));
			}
		}

		if (i >= fmt_len) break;

		if (fmt[i] == QSE_T('d') || fmt[i] == QSE_T('i') || 
		    fmt[i] == QSE_T('x') || fmt[i] == QSE_T('X') ||
		    fmt[i] == QSE_T('o'))
		{
			qse_awk_val_t* v;
			qse_long_t l;
			qse_real_t r;
			qse_char_t* p;
			int n;

		#if QSE_SIZEOF_LONG_LONG > 0
			FMT_CHAR (QSE_T('l'));
			FMT_CHAR (QSE_T('l'));
			FMT_CHAR (fmt[i]);
		#elif QSE_SIZEOF___INT64 > 0
			FMT_CHAR (QSE_T('I'));
			FMT_CHAR (QSE_T('6'));
			FMT_CHAR (QSE_T('4'));
			FMT_CHAR (fmt[i]);
		#elif QSE_SIZEOF_LONG > 0
			FMT_CHAR (QSE_T('l'));
			FMT_CHAR (fmt[i]);
		#elif QSE_SIZEOF_INT > 0
			FMT_CHAR (fmt[i]);
		#else
			#error unsupported integer size
		#endif	

			if (args == QSE_NULL)
			{
				if (stack_arg_idx >= nargs_on_stack)
				{
					qse_awk_rtx_seterrnum (run, QSE_AWK_EFMTARG);
					return QSE_NULL;
				}
				v = qse_awk_rtx_getarg (run, stack_arg_idx);
			}
			else
			{
				if (val != QSE_NULL) 
				{
					if (stack_arg_idx >= nargs_on_stack)
					{
						qse_awk_rtx_seterrnum (run, QSE_AWK_EFMTARG);
						return QSE_NULL;
					}
					v = val;
				}
				else 
				{
					v = eval_expression (run, args);
					if (v == QSE_NULL) return QSE_NULL;
				}
			}

			qse_awk_rtx_refupval (run, v);
			n = qse_awk_rtx_valtonum (run, v, &l, &r);
			qse_awk_rtx_refdownval (run, v);

			if (n == -1) return QSE_NULL; 
			if (n == 1) l = (qse_long_t)r;

			do
			{
				n = run->awk->prm.sprintf (
					run->awk,
					run->format.tmp.ptr, 
					run->format.tmp.len,
					QSE_STR_PTR(fbu),
				#if QSE_SIZEOF_LONG_LONG > 0
					(long long)l
				#elif QSE_SIZEOF___INT64 > 0
					(__int64)l
				#elif QSE_SIZEOF_LONG > 0
					(long)l
				#elif QSE_SIZEOF_INT > 0
					(int)l
				#endif
					);
					
				if (n == -1)
				{
					GROW (&run->format.tmp);
					if (run->format.tmp.ptr == QSE_NULL)
					{
						qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM);
						return QSE_NULL;
					}

					continue;
				}

				break;
			}
			while (1);

			p = run->format.tmp.ptr;
			while (*p != QSE_T('\0'))
			{
				OUT_CHAR (*p);
				p++;
			}
		}
		else if (fmt[i] == QSE_T('e') || fmt[i] == QSE_T('E') ||
		         fmt[i] == QSE_T('g') || fmt[i] == QSE_T('G') ||
		         fmt[i] == QSE_T('f'))
		{
			qse_awk_val_t* v;
			qse_long_t l;
			qse_real_t r;
			qse_char_t* p;
			int n;
	
			FMT_CHAR (QSE_T('L'));
			FMT_CHAR (fmt[i]);

			if (args == QSE_NULL)
			{
				if (stack_arg_idx >= nargs_on_stack)
				{
					qse_awk_rtx_seterrnum (run, QSE_AWK_EFMTARG);
					return QSE_NULL;
				}
				v = qse_awk_rtx_getarg (run, stack_arg_idx);
			}
			else
			{
				if (val != QSE_NULL) 
				{
					if (stack_arg_idx >= nargs_on_stack)
					{
						qse_awk_rtx_seterrnum (run, QSE_AWK_EFMTARG);
						return QSE_NULL;
					}
					v = val;
				}
				else 
				{
					v = eval_expression (run, args);
					if (v == QSE_NULL) return QSE_NULL;
				}
			}

			qse_awk_rtx_refupval (run, v);
			n = qse_awk_rtx_valtonum (run, v, &l, &r);
			qse_awk_rtx_refdownval (run, v);

			if (n == -1) return QSE_NULL;
			if (n == 0) r = (qse_real_t)l;

			do
			{
				n = run->awk->prm.sprintf (
					run->awk,
					run->format.tmp.ptr, 
					run->format.tmp.len,
					QSE_STR_PTR(fbu),
				#if defined(__MINGW32__)
					(double)r
				#else
					(long double)r
				#endif
				);
					
				if (n == -1)
				{
					GROW (&run->format.tmp);
					if (run->format.tmp.ptr == QSE_NULL)
					{
						qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM);
						return QSE_NULL;
					}

					continue;
				}

				break;
			}
			while (1);

			p = run->format.tmp.ptr;
			while (*p != QSE_T('\0'))
			{
				OUT_CHAR (*p);
				p++;
			}
		}
		else if (fmt[i] == QSE_T('c')) 
		{
			qse_char_t ch;
			qse_size_t ch_len;
			qse_awk_val_t* v;

			if (args == QSE_NULL)
			{
				if (stack_arg_idx >= nargs_on_stack)
				{
					qse_awk_rtx_seterrnum (run, QSE_AWK_EFMTARG);
					return QSE_NULL;
				}
				v = qse_awk_rtx_getarg (run, stack_arg_idx);
			}
			else
			{
				if (val != QSE_NULL) 
				{
					if (stack_arg_idx >= nargs_on_stack)
					{
						qse_awk_rtx_seterrnum (run, QSE_AWK_EFMTARG);
						return QSE_NULL;
					}
					v = val;
				}
				else 
				{
					v = eval_expression (run, args);
					if (v == QSE_NULL) return QSE_NULL;
				}
			}

			qse_awk_rtx_refupval (run, v);
			if (v->type == QSE_AWK_VAL_NIL)
			{
				ch = QSE_T('\0');
				ch_len = 0;
			}
			else if (v->type == QSE_AWK_VAL_INT)
			{
				ch = (qse_char_t)((qse_awk_val_int_t*)v)->val;
				ch_len = 1;
			}
			else if (v->type == QSE_AWK_VAL_REAL)
			{
				ch = (qse_char_t)((qse_awk_val_real_t*)v)->val;
				ch_len = 1;
			}
			else if (v->type == QSE_AWK_VAL_STR)
			{
				ch_len = ((qse_awk_val_str_t*)v)->len;
				if (ch_len > 0) 
				{
					ch = ((qse_awk_val_str_t*)v)->ptr[0];
					ch_len = 1;
				}
				else ch = QSE_T('\0');
			}
			else
			{
				qse_awk_rtx_refdownval (run, v);
				qse_awk_rtx_seterrnum (run, QSE_AWK_EVALTYPE);
				return QSE_NULL;
			}

			if (prec == -1 || prec == 0 || prec > (qse_long_t)ch_len) prec = (qse_long_t)ch_len;
			if (prec > width) width = prec;

			if (!minus)
			{
				while (width > prec) 
				{
					if (qse_str_ccat (out, QSE_T(' ')) == -1) 
					{ 
						qse_awk_rtx_refdownval (run, v);
						qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM);
						return QSE_NULL; 
					} 
					width--;
				}
			}

			if (prec > 0)
			{
				if (qse_str_ccat (out, ch) == -1) 
				{ 
					qse_awk_rtx_refdownval (run, v);
					qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM);
					return QSE_NULL; 
				} 
			}

			if (minus)
			{
				while (width > prec) 
				{
					if (qse_str_ccat (out, QSE_T(' ')) == -1) 
					{ 
						qse_awk_rtx_refdownval (run, v);
						qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM);
						return QSE_NULL; 
					} 
					width--;
				}
			}

			qse_awk_rtx_refdownval (run, v);
		}
		else if (fmt[i] == QSE_T('s')) 
		{
			qse_char_t* str, * str_free = QSE_NULL;
			qse_size_t str_len;
			qse_long_t k;
			qse_awk_val_t* v;

			if (args == QSE_NULL)
			{
				if (stack_arg_idx >= nargs_on_stack)
				{
					qse_awk_rtx_seterrnum (run, QSE_AWK_EFMTARG);
					return QSE_NULL;
				}
				v = qse_awk_rtx_getarg (run, stack_arg_idx);
			}
			else
			{
				if (val != QSE_NULL) 
				{
					if (stack_arg_idx >= nargs_on_stack)
					{
						qse_awk_rtx_seterrnum (run, QSE_AWK_EFMTARG);
						return QSE_NULL;
					}
					v = val;
				}
				else 
				{
					v = eval_expression (run, args);
					if (v == QSE_NULL) return QSE_NULL;
				}
			}

			qse_awk_rtx_refupval (run, v);
			if (v->type == QSE_AWK_VAL_NIL)
			{
				str = QSE_T("");
				str_len = 0;
			}
			else if (v->type == QSE_AWK_VAL_STR)
			{
				str = ((qse_awk_val_str_t*)v)->ptr;
				str_len = ((qse_awk_val_str_t*)v)->len;
			}
			else
			{
				if (v == val)
				{
					qse_awk_rtx_refdownval (run, v);
					qse_awk_rtx_seterrnum (run, QSE_AWK_EFMTCNV);
					return QSE_NULL;
				}
	
				str = qse_awk_rtx_valtostr (run, v, 
					QSE_AWK_VALTOSTR_CLEAR, QSE_NULL, &str_len);
				if (str == QSE_NULL)
				{
					qse_awk_rtx_refdownval (run, v);
					return QSE_NULL;
				}

				str_free = str;
			}

			if (prec == -1 || prec > (qse_long_t)str_len ) prec = (qse_long_t)str_len;
			if (prec > width) width = prec;

			if (!minus)
			{
				while (width > prec) 
				{
					if (qse_str_ccat (out, QSE_T(' ')) == -1) 
					{ 
						if (str_free != QSE_NULL) 
							QSE_AWK_FREE (run->awk, str_free);
						qse_awk_rtx_refdownval (run, v);
						qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM);
						return QSE_NULL; 
					} 
					width--;
				}
			}

			for (k = 0; k < prec; k++) 
			{
				if (qse_str_ccat (out, str[k]) == -1) 
				{ 
					if (str_free != QSE_NULL) 
						QSE_AWK_FREE (run->awk, str_free);
					qse_awk_rtx_refdownval (run, v);
					qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM);
					return QSE_NULL; 
				} 
			}

			if (str_free != QSE_NULL) QSE_AWK_FREE (run->awk, str_free);

			if (minus)
			{
				while (width > prec) 
				{
					if (qse_str_ccat (out, QSE_T(' ')) == -1) 
					{ 
						qse_awk_rtx_refdownval (run, v);
						qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM);
						return QSE_NULL; 
					} 
					width--;
				}
			}

			qse_awk_rtx_refdownval (run, v);
		}
		else /*if (fmt[i] == QSE_T('%'))*/
		{
			for (j = 0; j < QSE_STR_LEN(fbu); j++)
				OUT_CHAR (QSE_STR_CHAR(fbu,j));
			OUT_CHAR (fmt[i]);
		}

		if (args == QSE_NULL || val != QSE_NULL) stack_arg_idx++;
		else args = args->next;
		qse_str_clear (fbu);
	}

	/* flush uncompleted formatting sequence */
	for (j = 0; j < QSE_STR_LEN(fbu); j++)
		OUT_CHAR (QSE_STR_CHAR(fbu,j));

	*len = QSE_STR_LEN(out);
	return QSE_STR_PTR(out);
}

