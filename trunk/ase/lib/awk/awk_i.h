/*
 * $Id: awk_i.h 271 2008-07-20 12:42:39Z baconevi $
 *
 * {License}
 */

#ifndef _ASE_AWK_AWKI_H_
#define _ASE_AWK_AWKI_H_

#include <ase/cmn/mem.h>
#include <ase/cmn/str.h>
#include <ase/cmn/map.h>
#include <ase/cmn/rex.h>

typedef struct ase_awk_chain_t ase_awk_chain_t;
typedef struct ase_awk_tree_t ase_awk_tree_t;

#include <ase/awk/awk.h>
#include "tree.h"
#include "func.h"
#include "tab.h"
#include "parse.h"
#include "run.h"
#include "extio.h"
#include "misc.h"

#ifdef _MSC_VER
#pragma warning (disable: 4996)
#pragma warning (disable: 4296)
#endif

#define ASE_AWK_MAX_GLOBALS 9999
#define ASE_AWK_MAX_LOCALS  9999
#define ASE_AWK_MAX_PARAMS  9999

#define ASE_AWK_MALLOC(awk,size)      ASE_MALLOC((awk)->mmgr,size)
#define ASE_AWK_REALLOC(awk,ptr,size) ASE_REALLOC((awk)->mmgr,ptr,size)
#define ASE_AWK_FREE(awk,ptr)         ASE_FREE((awk)->mmgr,ptr)

#define ASE_AWK_ISUPPER(awk,c)  ASE_ISUPPER((awk)->ccls,c)
#define ASE_AWK_ISLOWER(awk,c)  ASE_ISLOWER((awk)->ccls,c)
#define ASE_AWK_ISALPHA(awk,c)  ASE_ISALPHA((awk)->ccls,c)
#define ASE_AWK_ISDIGIT(awk,c)  ASE_ISDIGIT((awk)->ccls,c)
#define ASE_AWK_ISXDIGIT(awk,c) ASE_ISXDIGIT((awk)->ccls,c)
#define ASE_AWK_ISALNUM(awk,c)  ASE_ISALNUM((awk)->ccls,c)
#define ASE_AWK_ISSPACE(awk,c)  ASE_ISSPACE((awk)->ccls,c)
#define ASE_AWK_ISPRINT(awk,c)  ASE_ISPRINT((awk)->ccls,c)
#define ASE_AWK_ISGRAPH(awk,c)  ASE_ISGRAPH((awk)->ccls,c)
#define ASE_AWK_ISCNTRL(awk,c)  ASE_ISCNTRL((awk)->ccls,c)
#define ASE_AWK_ISPUNCT(awk,c)  ASE_ISPUNCT((awk)->ccls,c)
#define ASE_AWK_TOUPPER(awk,c)  ASE_TOUPPER((awk)->ccls,c)
#define ASE_AWK_TOLOWER(awk,c)  ASE_TOLOWER((awk)->ccls,c)

#define ASE_AWK_STRDUP(awk,str) (ase_strdup(str,(awk)->mmgr))
#define ASE_AWK_STRXDUP(awk,str,len) (ase_strxdup(str,len,(awk)->mmgr))

struct ase_awk_tree_t
{
	ase_size_t nglobals; /* total number of globals */
	ase_size_t nbglobals; /* number of intrinsic globals */
	ase_cstr_t cur_afn;
	ase_map_t* afns; /* awk function map */

	ase_awk_nde_t* begin;
	ase_awk_nde_t* begin_tail;

	ase_awk_nde_t* end;
	ase_awk_nde_t* end_tail;

	ase_awk_chain_t* chain;
	ase_awk_chain_t* chain_tail;
	ase_size_t chain_size; /* number of nodes in the chain */

	int ok;
};

struct ase_awk_t
{
	ase_mmgr_t* mmgr;
	ase_ccls_t* ccls;
	ase_awk_prmfns_t* prmfns;

	void* assoc_data;

	/* options */
	int option;

	/* word table */
	ase_map_t* wtab;
	/* reverse word table */
	ase_map_t* rwtab;

	/* regular expression processing routines */
	ase_awk_rexfns_t* rexfns;

	/* parse tree */
	ase_awk_tree_t tree;

	/* temporary information that the parser needs */
	struct
	{
		struct
		{
			int block;
			int loop;
			int stmnt; /* statement */
		} id;

		struct
		{
			struct
			{
				ase_size_t block;
				ase_size_t loop;
				ase_size_t expr; /* expression */
			} cur;

			struct
			{
				ase_size_t block;
				ase_size_t expr;
			} max;
		} depth;

		/* function calls */
		ase_map_t* afns;

		/* named variables */
		ase_map_t* named;

		/* global variables */
		ase_awk_tab_t globals;

		/* local variables */
		ase_awk_tab_t locals;

		/* parameters to a function */
		ase_awk_tab_t params;

		/* maximum number of local variables */
		ase_size_t nlocals_max;

		ase_awk_nde_t* (*parse_block) (
			ase_awk_t*,ase_size_t,ase_bool_t);

	} parse;

	/* source code management */
	struct
	{
		ase_awk_srcios_t ios;

		struct
		{
			ase_cint_t curc;
			ase_cint_t ungotc[5];
			ase_size_t ungotc_line[5];
			ase_size_t ungotc_column[5];
			ase_size_t ungotc_count;

			ase_size_t line;
			ase_size_t column;
		} lex;

		struct
		{
			ase_char_t buf[512];
			ase_size_t buf_pos;
			ase_size_t buf_len;
		} shared;	
	} src;

	/* token */
	struct 
	{
		struct
		{
			int type;
			ase_size_t line;
			ase_size_t column;
		} prev;

		int        type;
		ase_str_t  name;
		ase_size_t line;
		ase_size_t column;
	} token;

	/* intrinsic functions */
	struct
	{
		ase_awk_bfn_t* sys;
		ase_map_t* user;
	} bfn;

	struct
	{
		struct
		{
			struct
			{
				ase_size_t block;
				ase_size_t expr;
			} max;
		} depth;
	} run;

	struct
	{
		struct
		{
			struct
			{
				ase_size_t build;
				ase_size_t match;
			} max;
		} depth;
	} rex;

	struct
	{
		ase_char_t fmt[1024];
	} tmp;

	/* housekeeping */
	int errnum;
	ase_size_t errlin;
	ase_char_t errmsg[256];
	ase_char_t* errstr[ASE_AWK_NUMERRNUM];

	ase_bool_t stopall;
};

struct ase_awk_chain_t
{
	ase_awk_nde_t* pattern;
	ase_awk_nde_t* action;
	ase_awk_chain_t* next;	
};

struct ase_awk_run_t
{
	int id;
	ase_map_t* named;

	void** stack;
	ase_size_t stack_top;
	ase_size_t stack_base;
	ase_size_t stack_limit;
	int exit_level;

	ase_awk_val_ref_t* fcache[128];
	/*ase_awk_val_str_t* scache32[128];
	ase_awk_val_str_t* scache64[128];*/
	ase_size_t fcache_count;
	/*ase_size_t scache32_count;
	ase_size_t scache64_count;*/

	struct
	{
		ase_awk_val_int_t* ifree;
		ase_awk_val_chunk_t* ichunk;
		ase_awk_val_real_t* rfree;
		ase_awk_val_chunk_t* rchunk;
	} vmgr;

	ase_awk_nde_blk_t* active_block;
	ase_byte_t* pattern_range_state;

	struct
	{
		ase_char_t buf[1024];
		ase_size_t buf_pos;
		ase_size_t buf_len;
		ase_bool_t eof;

		ase_str_t line;
		ase_awk_val_t* d0; /* $0 */

		ase_size_t maxflds;
		ase_size_t nflds; /* NF */
		struct
		{
			ase_char_t*    ptr;
			ase_size_t     len;
			ase_awk_val_t* val; /* $1 .. $NF */
		}* flds;

	} inrec;

	struct
	{
		void* rs;
		void* fs;
		int ignorecase;

		ase_long_t nr;
		ase_long_t fnr;

		struct 
		{
			ase_char_t* ptr;
			ase_size_t len;
		} convfmt;
		struct
		{
			ase_char_t* ptr;
			ase_size_t len;
		} ofmt;
		struct
		{
			ase_char_t* ptr;
			ase_size_t len;
		} ofs;
		struct
		{
			ase_char_t* ptr;
			ase_size_t len;
		} ors;
		struct
		{
			ase_char_t* ptr;
			ase_size_t len;
		} subsep;
	} global;

	/* extio chain */
	struct
	{
		ase_awk_io_t handler[ASE_AWK_EXTIO_NUM];
		void* custom_data;
		ase_awk_extio_t* chain;
	} extio;

	struct
	{
		ase_str_t fmt;
		ase_str_t out;

		struct
		{
			ase_char_t* ptr;
			ase_size_t len;	/* length */
			ase_size_t inc; /* increment */
		} tmp;
	} format;

	struct
	{
		struct
		{
			ase_size_t block;
			ase_size_t expr; /* expression */
		} cur;

		struct
		{
			ase_size_t block;
			ase_size_t expr;
		} max;
	} depth;

	int errnum;
	ase_size_t errlin;
	ase_char_t errmsg[256];

	void* custom_data;

	ase_awk_t* awk;
	ase_awk_runcbs_t* cbs;
};


#define ASE_AWK_FREEREX(awk,code) ase_freerex((awk)->mmgr,code)
#define ASE_AWK_ISEMPTYREX(awk,code) ase_isemptyrex(code)
#define ASE_AWK_BUILDREX(awk,ptn,len,errnum) \
	ase_awk_buildrex(awk,ptn,len,errnum)
#define ASE_AWK_MATCHREX(awk,code,option,str,len,match_ptr,match_len,errnum) \
	ase_awk_matchrex(awk,code,option,str,len,match_ptr,match_len,errnum)

#endif