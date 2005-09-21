/*
 * $Id: prim.h,v 1.5 2005-09-21 11:52:36 bacon Exp $
 */

#ifndef _XP_LSP_PRIM_H_
#define _XP_LSP_PRIM_H_

#include <xp/lsp/types.h>
#include <xp/lsp/lsp.h>

#ifdef __cplusplus
extern "C" {
#endif

xp_lsp_obj_t* xp_lsp_prim_abort (xp_lsp_t* lsp, xp_lsp_obj_t* args);
xp_lsp_obj_t* xp_lsp_prim_eval  (xp_lsp_t* lsp, xp_lsp_obj_t* args);
xp_lsp_obj_t* xp_lsp_prim_prog1 (xp_lsp_t* lsp, xp_lsp_obj_t* args);
xp_lsp_obj_t* xp_lsp_prim_progn (xp_lsp_t* lsp, xp_lsp_obj_t* args);
xp_lsp_obj_t* xp_lsp_prim_gc    (xp_lsp_t* lsp, xp_lsp_obj_t* args);
xp_lsp_obj_t* xp_lsp_prim_cond  (xp_lsp_t* lsp, xp_lsp_obj_t* args);
xp_lsp_obj_t* xp_lsp_prim_if    (xp_lsp_t* lsp, xp_lsp_obj_t* args);
xp_lsp_obj_t* xp_lsp_prim_while (xp_lsp_t* lsp, xp_lsp_obj_t* args);

xp_lsp_obj_t* xp_lsp_prim_car   (xp_lsp_t* lsp, xp_lsp_obj_t* args);
xp_lsp_obj_t* xp_lsp_prim_cdr   (xp_lsp_t* lsp, xp_lsp_obj_t* args);
xp_lsp_obj_t* xp_lsp_prim_cons  (xp_lsp_t* lsp, xp_lsp_obj_t* args);
xp_lsp_obj_t* xp_lsp_prim_set   (xp_lsp_t* lsp, xp_lsp_obj_t* args);
xp_lsp_obj_t* xp_lsp_prim_setq  (xp_lsp_t* lsp, xp_lsp_obj_t* args);
xp_lsp_obj_t* xp_lsp_prim_quote (xp_lsp_t* lsp, xp_lsp_obj_t* args);
xp_lsp_obj_t* xp_lsp_prim_defun (xp_lsp_t* lsp, xp_lsp_obj_t* args);
xp_lsp_obj_t* xp_lsp_prim_demac (xp_lsp_t* lsp, xp_lsp_obj_t* args);
xp_lsp_obj_t* xp_lsp_prim_let   (xp_lsp_t* lsp, xp_lsp_obj_t* args);
xp_lsp_obj_t* xp_lsp_prim_letx  (xp_lsp_t* lsp, xp_lsp_obj_t* args);

/*---------------------
       prim_compar.c 
  ---------------------*/
xp_lsp_obj_t* xp_lsp_prim_eq (xp_lsp_t* lsp, xp_lsp_obj_t* args);
xp_lsp_obj_t* xp_lsp_prim_ne (xp_lsp_t* lsp, xp_lsp_obj_t* args);
xp_lsp_obj_t* xp_lsp_prim_gt (xp_lsp_t* lsp, xp_lsp_obj_t* args);
xp_lsp_obj_t* xp_lsp_prim_lt (xp_lsp_t* lsp, xp_lsp_obj_t* args);
xp_lsp_obj_t* xp_lsp_prim_ge (xp_lsp_t* lsp, xp_lsp_obj_t* args);
xp_lsp_obj_t* xp_lsp_prim_le (xp_lsp_t* lsp, xp_lsp_obj_t* args);

/*---------------------
       prim_math.c 
  ---------------------*/
xp_lsp_obj_t* xp_lsp_prim_plus     (xp_lsp_t* lsp, xp_lsp_obj_t* args);
xp_lsp_obj_t* xp_lsp_prim_minus    (xp_lsp_t* lsp, xp_lsp_obj_t* args);
xp_lsp_obj_t* xp_lsp_prim_multiply (xp_lsp_t* lsp, xp_lsp_obj_t* args);
xp_lsp_obj_t* xp_lsp_prim_divide   (xp_lsp_t* lsp, xp_lsp_obj_t* args);
xp_lsp_obj_t* xp_lsp_prim_modulus  (xp_lsp_t* lsp, xp_lsp_obj_t* args);

#ifdef __cplusplus
}
#endif

#define XP_LSP_PRIM_CHECK_ARG_COUNT(lsp,args,min,max) \
{ \
	xp_size_t count; \
	if (xp_lsp_probe_args(lsp->mem, args, &count) == -1) { \
		lsp->errnum = XP_LSP_ERR_BAD_ARG; \
		return XP_NULL; \
	} \
	if (count < min) { \
		lsp->errnum = XP_LSP_ERR_TOO_FEW_ARGS; \
		return XP_NULL; \
	} \
	if (count > max) { \
		lsp->errnum = XP_LSP_ERR_TOO_MANY_ARGS; \
		return XP_NULL; \
	} \
}

#define XP_LSP_PRIM_MAX_ARG_COUNT ((xp_size_t)~(xp_size_t)0)

#endif
