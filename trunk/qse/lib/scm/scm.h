/*
 * $Id$
 *
    Copyright 2006-2009 Chung, Hyung-Hwan.
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

#ifndef _QSE_LIB_SCM_SCM_H_
#define _QSE_LIB_SCM_SCM_H_

#include "../cmn/mem.h"
#include <qse/cmn/chr.h>
#include <qse/cmn/str.h>
#include <qse/scm/scm.h>

/* Note that not all these values can be ORed with each other.
 * each value represents its own type except the following combinations.
 *
 *   QSE_SCM_ENT_T
 *   QSE_SCM_ENT_F
 *   QSE_SCM_ENT_SYM 
 */
enum qse_scm_ent_type_t
{
	QSE_SCM_ENT_NIL     = (1 << 0),
	QSE_SCM_ENT_T       = (1 << 1),
	QSE_SCM_ENT_F       = (1 << 2),
	QSE_SCM_ENT_NUM     = (1 << 3),
	QSE_SCM_ENT_REAL    = (1 << 4),
	QSE_SCM_ENT_STR     = (1 << 5), 
	QSE_SCM_ENT_NAM     = (1 << 6),
	QSE_SCM_ENT_SYM     = (1 << 7),
	QSE_SCM_ENT_PAIR    = (1 << 8),
	QSE_SCM_ENT_PROC    = (1 << 9),
	QSE_SCM_ENT_CLOS    = (1 << 10)

};

/**
 * The qse_scm_ent_t type defines an entity that represents an individual
 * value in scheme.
 */
struct qse_scm_ent_t
{
	qse_uint32_t dswcount: 2;
	qse_uint32_t mark:     1;
	qse_uint32_t atom:     1;
	qse_uint32_t synt:     1; /* can be set to 1 if type is QSE_SCM_ENT_SYM */
	qse_uint32_t type:     27;

	union
	{
		struct
		{
			qse_long_t val;
		} num; /* number */

		struct
		{
			qse_real_t val;
		} real;

		struct
		{
			/* a string doesn't need to be null-terminated 
			 * as the length is remembered */
			qse_char_t* ptr; 
			qse_size_t  len;
		} str; /* string */

		struct
		{
			qse_char_t* ptr;  /* null-terminated string */
			void*       uptr; /* used for syntax entities only */
		} lab; /* label */

		struct
		{
			int code;
		} proc;
		
		struct
		{
			qse_scm_ent_t* ent[2];
		} ref; 
	} u;
};

#define DSWCOUNT(v)       ((v)->dswcount)
#define MARK(v)           ((v)->mark)
#define ATOM(v)           ((v)->atom)
#define SYNT(v)           ((v)->synt)
#define TYPE(v)           ((v)->type)

#define NUM_VALUE(v)      ((v)->u.num.val)
#define REAL_VALUE(v)     ((v)->u.real.val)
#define STR_PTR(v)        ((v)->u.str.ptr)
#define STR_LEN(v)        ((v)->u.str.len)
#define LAB_PTR(v)        ((v)->u.lab.ptr)
#define LAB_UPTR(v)       ((v)->u.lab.uptr)
#define SYM_NAME(v)       ((v)->u.ref.ent[0])
#define SYM_PROP(v)       ((v)->u.ref.ent[1])
#define SYNT_UPTR(v)      LAB_UPTR(SYM_NAME(v))
#define PAIR_CAR(v)       ((v)->u.ref.ent[0])
#define PAIR_CDR(v)       ((v)->u.ref.ent[1])
#define PROC_CODE(v)      ((v)->u.proc.code)
#define CLOS_CODE(v)      ((v)->u.ref.ent[0])
#define CLOS_ENV(v)       ((v)->u.ref.ent[1])

/**
 * The qse_scm_enb_t type defines a value block. A value block is allocated
 * when more memory is requested and is chained to existing value blocks.
 */
typedef struct qse_scm_enb_t qse_scm_enb_t;
struct qse_scm_enb_t
{
	qse_scm_ent_t* ptr;
	qse_size_t     len;
	qse_scm_enb_t* next;	
};

struct qse_scm_t 
{
	QSE_DEFINE_COMMON_FIELDS (scm)

	/** error information */
	struct 
	{
		qse_scm_errstr_t str;      /**< error string getter */
		qse_scm_errnum_t num;      /**< stores an error number */
		qse_char_t       msg[128]; /**< error message holder */
		qse_scm_loc_t    loc;      /**< location of the last error */
	} err; 

	/** I/O functions */
	struct
	{
		qse_scm_io_t fns;

		struct
		{
			qse_scm_io_arg_t in;
			qse_scm_io_arg_t out;
		} arg;
	} io;

	/** data for reading */
	struct
	{
		qse_cint_t curc; 
		qse_scm_loc_t curloc;

		/** token */
		struct
		{
			int           type;
			qse_scm_loc_t loc;
			qse_long_t    ival;
			qse_real_t    rval;
			qse_str_t     name;
		} t;

		qse_scm_ent_t* s; /* stack for reading */
		qse_scm_ent_t* e; /* last entity read */
	} r; 

	/** data for printing */
	struct
	{
		qse_scm_ent_t* s; /* stack for printing */
		qse_scm_ent_t* e; /* top entity being printed */
	} p; 

	/* data for evaluation */
	struct
	{
		int (*op) (qse_scm_t*);

		qse_scm_ent_t* in;
		qse_scm_ent_t* out;

		qse_scm_ent_t* arg; /* function arguments */
		qse_scm_ent_t* env; /* current environment */
		qse_scm_ent_t* cod; /* current code */
		qse_scm_ent_t* dmp; /* stack register for next evaluation */
	} e; 

	/* common values */
	qse_scm_ent_t* nil;
	qse_scm_ent_t* t;
	qse_scm_ent_t* f;
	qse_scm_ent_t* lambda;
	qse_scm_ent_t* quote;

	qse_scm_ent_t* gloenv; /* global environment */
	qse_scm_ent_t* symtab; /* symbol table */

	/* fields for entity allocation */
	struct
	{
		qse_scm_enb_t* ebl;  /* entity block list */
		qse_scm_ent_t* free;
	} mem;
};


#define IS_NIL(scm,ent)          QSE_SCM_ENT_ISNIL(scm,ent)
#define IS_SMALLINT(scm,ent)     QSE_SCM_ENT_ISSMALLINT(scm,ent)
#define FROM_SMALLINT(scm,ent)   QSE_SCM_ENT_FROMSMALLINT(scm,ent)
#define TO_SMALLINT(scm,num)     QSE_SCM_ENT_TOSMALLINT(scm,num)
#define CAN_BE_SMALLINT(scm,num) QSE_SCM_ENT_CANBESMALLINT(scm,num)

#ifdef __cplusplus
extern "C" {
#endif


/* eval.c */
int qse_scm_dolambda (qse_scm_t* scm);
int qse_scm_doquote  (qse_scm_t* scm);
int qse_scm_dodefine (qse_scm_t* scm);
int qse_scm_dobegin  (qse_scm_t* scm);
int qse_scm_doif     (qse_scm_t* scm);

/* err.c */
const qse_char_t* qse_scm_dflerrstr (qse_scm_t* scm, qse_scm_errnum_t errnum);

#ifdef __cplusplus
}
#endif

#endif