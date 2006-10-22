/*
 * $Id: init.c,v 1.12 2006-10-22 13:10:46 bacon Exp $
 */

#include <sse/lsp/lsp.h>
#include <sse/lsp/prim.h>
#include <sse/bas/memory.h>
#include <sse/bas/assert.h>

static int __add_builtin_prims (sse_lsp_t* lsp);

sse_lsp_t* sse_lsp_open (sse_lsp_t* lsp, 
	sse_size_t mem_ubound, sse_size_t mem_ubound_inc)
{
	if (lsp == SSE_NULL) {
		lsp = (sse_lsp_t*)sse_malloc(sse_sizeof(sse_lsp_t));
		if (lsp == SSE_NULL) return lsp;
		lsp->__dynamic = sse_true;
	}
	else lsp->__dynamic = sse_false;

	if (sse_lsp_token_open(&lsp->token, 0) == SSE_NULL) {
		if (lsp->__dynamic) sse_free (lsp);
		return SSE_NULL;
	}

	lsp->errnum = SSE_LSP_ERR_NONE;
	lsp->opt_undef_symbol = 1;
	//lsp->opt_undef_symbol = 0;

	lsp->curc = SSE_CHAR_EOF;
	lsp->input_func = SSE_NULL;
	lsp->output_func = SSE_NULL;
	lsp->input_arg = SSE_NULL;
	lsp->output_arg = SSE_NULL;

	lsp->mem = sse_lsp_mem_new (mem_ubound, mem_ubound_inc);
	if (lsp->mem == SSE_NULL) {
		sse_lsp_token_close (&lsp->token);
		if (lsp->__dynamic) sse_free (lsp);
		return SSE_NULL;
	}

	if (__add_builtin_prims(lsp) == -1) {
		sse_lsp_mem_free (lsp->mem);
		sse_lsp_token_close (&lsp->token);
		if (lsp->__dynamic) sse_free (lsp);
		return SSE_NULL;
	}

	lsp->max_eval_depth = 0; // TODO: put restriction here....
	lsp->cur_eval_depth = 0;

	return lsp;
}

void sse_lsp_close (sse_lsp_t* lsp)
{
	sse_assert (lsp != SSE_NULL);
	sse_lsp_mem_free (lsp->mem);
	sse_lsp_token_close (&lsp->token);
	if (lsp->__dynamic) sse_free (lsp);
}

int sse_lsp_attach_input (sse_lsp_t* lsp, sse_lsp_io_t input, void* arg)
{
	if (sse_lsp_detach_input(lsp) == -1) return -1;

	sse_assert (lsp->input_func == SSE_NULL);

	if (input(SSE_LSP_IO_OPEN, arg, SSE_NULL, 0) == -1) {
		/* TODO: set error number */
		return -1;
	}

	lsp->input_func = input;
	lsp->input_arg = arg;
	lsp->curc = SSE_CHAR_EOF;
	return 0;
}

int sse_lsp_detach_input (sse_lsp_t* lsp)
{
	if (lsp->input_func != SSE_NULL) {
		if (lsp->input_func(SSE_LSP_IO_CLOSE, lsp->input_arg, SSE_NULL, 0) == -1) {
			/* TODO: set error number */
			return -1;
		}
		lsp->input_func = SSE_NULL;
		lsp->input_arg = SSE_NULL;
		lsp->curc = SSE_CHAR_EOF;
	}

	return 0;
}

int sse_lsp_attach_output (sse_lsp_t* lsp, sse_lsp_io_t output, void* arg)
{
	if (sse_lsp_detach_output(lsp) == -1) return -1;

	sse_assert (lsp->output_func == SSE_NULL);

	if (output(SSE_LSP_IO_OPEN, arg, SSE_NULL, 0) == -1) {
		/* TODO: set error number */
		return -1;
	}
	lsp->output_func = output;
	lsp->output_arg = arg;
	return 0;
}

int sse_lsp_detach_output (sse_lsp_t* lsp)
{
	if (lsp->output_func != SSE_NULL) {
		if (lsp->output_func(SSE_LSP_IO_CLOSE, lsp->output_arg, SSE_NULL, 0) == -1) {
			/* TODO: set error number */
			return -1;
		}
		lsp->output_func = SSE_NULL;
		lsp->output_arg = SSE_NULL;
	}

	return 0;
}

static int __add_builtin_prims (sse_lsp_t* lsp)
{

#define ADD_PRIM(mem,name,prim) \
	if (sse_lsp_add_prim(mem,name,prim) == -1) return -1;

	ADD_PRIM (lsp, SSE_TEXT("abort"), sse_lsp_prim_abort);
	ADD_PRIM (lsp, SSE_TEXT("eval"),  sse_lsp_prim_eval);
	ADD_PRIM (lsp, SSE_TEXT("prog1"), sse_lsp_prim_prog1);
	ADD_PRIM (lsp, SSE_TEXT("progn"), sse_lsp_prim_progn);
	ADD_PRIM (lsp, SSE_TEXT("gc"),    sse_lsp_prim_gc);

	ADD_PRIM (lsp, SSE_TEXT("cond"),  sse_lsp_prim_cond);
	ADD_PRIM (lsp, SSE_TEXT("if"),    sse_lsp_prim_if);
	ADD_PRIM (lsp, SSE_TEXT("while"), sse_lsp_prim_while);

	ADD_PRIM (lsp, SSE_TEXT("car"),   sse_lsp_prim_car);
	ADD_PRIM (lsp, SSE_TEXT("cdr"),   sse_lsp_prim_cdr);
	ADD_PRIM (lsp, SSE_TEXT("cons"),  sse_lsp_prim_cons);
	ADD_PRIM (lsp, SSE_TEXT("set"),   sse_lsp_prim_set);
	ADD_PRIM (lsp, SSE_TEXT("setq"),  sse_lsp_prim_setq);
	ADD_PRIM (lsp, SSE_TEXT("quote"), sse_lsp_prim_quote);
	ADD_PRIM (lsp, SSE_TEXT("defun"), sse_lsp_prim_defun);
	ADD_PRIM (lsp, SSE_TEXT("demac"), sse_lsp_prim_demac);
	ADD_PRIM (lsp, SSE_TEXT("let"),   sse_lsp_prim_let);
	ADD_PRIM (lsp, SSE_TEXT("let*"),  sse_lsp_prim_letx);

	ADD_PRIM (lsp, SSE_TEXT("="),     sse_lsp_prim_eq);
	ADD_PRIM (lsp, SSE_TEXT("/="),    sse_lsp_prim_ne);
	ADD_PRIM (lsp, SSE_TEXT(">"),     sse_lsp_prim_gt);
	ADD_PRIM (lsp, SSE_TEXT("<"),     sse_lsp_prim_lt);
	ADD_PRIM (lsp, SSE_TEXT(">="),    sse_lsp_prim_ge);
	ADD_PRIM (lsp, SSE_TEXT("<="),    sse_lsp_prim_le);

	ADD_PRIM (lsp, SSE_TEXT("+"),     sse_lsp_prim_plus);
	ADD_PRIM (lsp, SSE_TEXT("-"),     sse_lsp_prim_minus);
	ADD_PRIM (lsp, SSE_TEXT("*"),     sse_lsp_prim_multiply);
	ADD_PRIM (lsp, SSE_TEXT("/"),     sse_lsp_prim_divide);
	ADD_PRIM (lsp, SSE_TEXT("%"),     sse_lsp_prim_modulus);

	return 0;
}
