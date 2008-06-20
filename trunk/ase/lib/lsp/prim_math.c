/*
 * $Id: prim_math.c 215 2008-06-19 10:27:37Z baconevi $
 *
 * {License}
 */

#include "lsp_i.h"

ase_lsp_obj_t* ase_lsp_prim_plus (ase_lsp_t* lsp, ase_lsp_obj_t* args)
{
	ase_lsp_obj_t* body, * tmp;
	ase_long_t ival = 0;
	ase_real_t rval = .0;
	ase_bool_t realnum = ASE_FALSE;

	ASE_ASSERT (ASE_LSP_TYPE(args) == ASE_LSP_OBJ_CONS);

	body = args;
	while (ASE_LSP_TYPE(body) == ASE_LSP_OBJ_CONS) 
	{
		tmp = ase_lsp_eval (lsp, ASE_LSP_CAR(body));
		if (tmp == ASE_NULL) return ASE_NULL;

		if (ASE_LSP_TYPE(tmp) == ASE_LSP_OBJ_INT) 
		{
			if (body == args) 
			{
				ASE_ASSERT (realnum == ASE_FALSE);
				ival = ASE_LSP_IVAL(tmp);
			}
			else 
			{
				if (!realnum) 
					ival = ival + ASE_LSP_IVAL(tmp);
				else
					rval = rval + ASE_LSP_IVAL(tmp);
			}
		}
		else if (ASE_LSP_TYPE(tmp) == ASE_LSP_OBJ_REAL) 
		{
			if (body == args) 
			{
				ASE_ASSERT (realnum == ASE_FALSE);
				realnum = ASE_TRUE;
				rval = ASE_LSP_RVAL(tmp);
			}
			else 
			{
				if (!realnum) 
				{
					realnum = ASE_TRUE;
					rval = (ase_real_t)ival;
				}
				rval = rval + ASE_LSP_RVAL(tmp);
			}
		}
		else 
		{
			ase_lsp_seterror (lsp, ASE_LSP_EVALBAD, ASE_NULL, 0);
			return ASE_NULL;
		}


		body = ASE_LSP_CDR(body);
	}

	ASE_ASSERT (body == lsp->mem->nil);

	tmp = (realnum)?
		ase_lsp_makerealobj (lsp->mem, rval):
		ase_lsp_makeintobj (lsp->mem, ival);
	if (tmp == ASE_NULL) return ASE_NULL;

	return tmp;
}

ase_lsp_obj_t* ase_lsp_prim_minus (ase_lsp_t* lsp, ase_lsp_obj_t* args)
{
	ase_lsp_obj_t* body, * tmp;
	ase_long_t ival = 0;
	ase_real_t rval = .0;
	ase_bool_t realnum = ASE_FALSE;

	ASE_ASSERT (ASE_LSP_TYPE(args) == ASE_LSP_OBJ_CONS);

	body = args;
	while (ASE_LSP_TYPE(body) == ASE_LSP_OBJ_CONS) 
	{
		tmp = ase_lsp_eval (lsp, ASE_LSP_CAR(body));
		if (tmp == ASE_NULL) return ASE_NULL;


		if (ASE_LSP_TYPE(tmp) == ASE_LSP_OBJ_INT) 
		{
			if (body == args) 
			{
				ASE_ASSERT (realnum == ASE_FALSE);
				ival = ASE_LSP_IVAL(tmp);
			}
			else 
			{
				if (!realnum) 
					ival = ival - ASE_LSP_IVAL(tmp);
				else
					rval = rval - ASE_LSP_IVAL(tmp);
			}
		}
		else if (ASE_LSP_TYPE(tmp) == ASE_LSP_OBJ_REAL) 
		{
			if (body == args) 
			{
				ASE_ASSERT (realnum == ASE_FALSE);
				realnum = ASE_TRUE;
				rval = ASE_LSP_RVAL(tmp);
			}
			else 
			{
				if (!realnum) 
				{
					realnum = ASE_TRUE;
					rval = (ase_real_t)ival;
				}
				rval = rval - ASE_LSP_RVAL(tmp);
			}
		}
		else 
		{
			ase_lsp_seterror (lsp, ASE_LSP_EVALBAD, ASE_NULL, 0);
			return ASE_NULL;
		}


		body = ASE_LSP_CDR(body);
	}

	ASE_ASSERT (body == lsp->mem->nil);

	tmp = (realnum)?
		ase_lsp_makerealobj (lsp->mem, rval):
		ase_lsp_makeintobj (lsp->mem, ival);
	if (tmp == ASE_NULL) return ASE_NULL;

	return tmp;
}

ase_lsp_obj_t* ase_lsp_prim_mul (ase_lsp_t* lsp, ase_lsp_obj_t* args)
{
	ase_lsp_obj_t* body, * tmp;
	ase_long_t ival = 0;
	ase_real_t rval = .0;
	ase_bool_t realnum = ASE_FALSE;

	ASE_ASSERT (ASE_LSP_TYPE(args) == ASE_LSP_OBJ_CONS);

	body = args;
	while (ASE_LSP_TYPE(body) == ASE_LSP_OBJ_CONS) 
	{
		tmp = ase_lsp_eval (lsp, ASE_LSP_CAR(body));
		if (tmp == ASE_NULL) return ASE_NULL;

		if (ASE_LSP_TYPE(tmp) == ASE_LSP_OBJ_INT) 
		{
			if (body == args) 
			{
				ASE_ASSERT (realnum == ASE_FALSE);
				ival = ASE_LSP_IVAL(tmp);
			}
			else 
			{
				if (!realnum) 
					ival = ival * ASE_LSP_IVAL(tmp);
				else
					rval = rval * ASE_LSP_IVAL(tmp);
			}
		}
		else if (ASE_LSP_TYPE(tmp) == ASE_LSP_OBJ_REAL) 
		{
			if (body == args) 
			{
				ASE_ASSERT (realnum == ASE_FALSE);
				realnum = ASE_TRUE;
				rval = ASE_LSP_RVAL(tmp);
			}
			else 
			{
				if (!realnum) 
				{
					realnum = ASE_TRUE;
					rval = (ase_real_t)ival;
				}
				rval = rval * ASE_LSP_RVAL(tmp);
			}
		}
		else 
		{
			ase_lsp_seterror (lsp, ASE_LSP_EVALBAD, ASE_NULL, 0);
			return ASE_NULL;
		}


		body = ASE_LSP_CDR(body);
	}

	ASE_ASSERT (body == lsp->mem->nil);

	tmp = (realnum)?
		ase_lsp_makerealobj (lsp->mem, rval):
		ase_lsp_makeintobj (lsp->mem, ival);
	if (tmp == ASE_NULL) return ASE_NULL;

	return tmp;
}

ase_lsp_obj_t* ase_lsp_prim_div (ase_lsp_t* lsp, ase_lsp_obj_t* args)
{
	ase_lsp_obj_t* body, * tmp;
	ase_long_t ival = 0;
	ase_real_t rval = .0;
	ase_bool_t realnum = ASE_FALSE;

	ASE_ASSERT (ASE_LSP_TYPE(args) == ASE_LSP_OBJ_CONS);

	body = args;
	while (ASE_LSP_TYPE(body) == ASE_LSP_OBJ_CONS) 
	{
		tmp = ase_lsp_eval (lsp, ASE_LSP_CAR(body));
		if (tmp == ASE_NULL) return ASE_NULL;

		if (ASE_LSP_TYPE(tmp) == ASE_LSP_OBJ_INT) 
		{
			if (body == args) 
			{
				ASE_ASSERT (realnum == ASE_FALSE);
				ival = ASE_LSP_IVAL(tmp);
			}
			else 
			{
				if (!realnum) 
				{
					if (ASE_LSP_IVAL(tmp) == 0) 
					{
						ase_lsp_seterror (lsp, ASE_LSP_EDIVBY0, ASE_NULL, 0);
						return ASE_NULL;
					}
					ival = ival / ASE_LSP_IVAL(tmp);
				}
				else
					rval = rval / ASE_LSP_IVAL(tmp);
			}
		}
		else if (ASE_LSP_TYPE(tmp) == ASE_LSP_OBJ_REAL) 
		{
			if (body == args) 
			{
				ASE_ASSERT (realnum == ASE_FALSE);
				realnum = ASE_TRUE;
				rval = ASE_LSP_RVAL(tmp);
			}
			else 
			{
				if (!realnum) 
				{
					realnum = ASE_TRUE;
					rval = (ase_real_t)ival;
				}
				rval = rval / ASE_LSP_RVAL(tmp);
			}
		}
		else 
		{
			ase_lsp_seterror (lsp, ASE_LSP_EVALBAD, ASE_NULL, 0);
			return ASE_NULL;
		}


		body = ASE_LSP_CDR(body);
	}

	ASE_ASSERT (body == lsp->mem->nil);

	tmp = (realnum)?
		ase_lsp_makerealobj (lsp->mem, rval):
		ase_lsp_makeintobj (lsp->mem, ival);
	if (tmp == ASE_NULL) return ASE_NULL;

	return tmp;
}

ase_lsp_obj_t* ase_lsp_prim_mod (ase_lsp_t* lsp, ase_lsp_obj_t* args)
{
	ase_lsp_obj_t* body, * tmp;
	ase_long_t ival = 0;

	ASE_ASSERT (ASE_LSP_TYPE(args) == ASE_LSP_OBJ_CONS);

	body = args;
	while (ASE_LSP_TYPE(body) == ASE_LSP_OBJ_CONS) 
	{
		tmp = ase_lsp_eval (lsp, ASE_LSP_CAR(body));
		if (tmp == ASE_NULL) return ASE_NULL;

		if (ASE_LSP_TYPE(tmp) == ASE_LSP_OBJ_INT) 
		{
			if (body == args) 
			{
				ival = ASE_LSP_IVAL(tmp);
			}
			else 
			{
				if (ASE_LSP_IVAL(tmp) == 0) 
				{
					ase_lsp_seterror (lsp, ASE_LSP_EDIVBY0, ASE_NULL, 0);
					return ASE_NULL;
				}
				ival = ival % ASE_LSP_IVAL(tmp);
			}
		}
		else if (ASE_LSP_TYPE(tmp) == ASE_LSP_OBJ_REAL) 
		{
			if (body == args) 
			{
				ival = (ase_long_t)ASE_LSP_RVAL(tmp);
			}
			else 
			{
				ase_long_t tmpi = (ase_long_t)ASE_LSP_RVAL(tmp);
				if (tmpi == 0) 
				{
					ase_lsp_seterror (lsp, ASE_LSP_EDIVBY0, ASE_NULL, 0);
					return ASE_NULL;
				}
				ival = ival % tmpi;
			}
		}
		else 
		{
			ase_lsp_seterror (lsp, ASE_LSP_EVALBAD, ASE_NULL, 0);
			return ASE_NULL;
		}


		body = ASE_LSP_CDR(body);
	}

	ASE_ASSERT (body == lsp->mem->nil);

	tmp = ase_lsp_makeintobj (lsp->mem, ival);
	if (tmp == ASE_NULL) return ASE_NULL;

	return tmp;
}
