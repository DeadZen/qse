/* 
 * $Id: awk.h,v 1.62 2006-06-16 07:35:06 bacon Exp $
 */

#ifndef _XP_AWK_AWK_H_
#define _XP_AWK_AWK_H_

#include <xp/types.h>
#include <xp/macros.h>

typedef struct xp_awk_t xp_awk_t;

typedef xp_ssize_t (*xp_awk_io_t) (
	int cmd, void* arg, xp_char_t* data, xp_size_t count);

/* io function commands */
enum 
{
	XP_AWK_INPUT_OPEN   = 0,
	XP_AWK_INPUT_CLOSE  = 1,
	XP_AWK_INPUT_NEXT   = 2,
	XP_AWK_INPUT_DATA   = 3,

	XP_AWK_OUTPUT_OPEN  = 4,
	XP_AWK_OUTPUT_CLOSE = 5,
	XP_AWK_OUTPUT_NEXT  = 6,
	XP_AWK_OUTPUT_DATA  = 7
};

/* parse options */
enum
{
	XP_AWK_IMPLICIT   = (1 << 0), /* allow undeclared variables */
	XP_AWK_EXPLICIT   = (1 << 1), /* variable requires explicit declaration */
	XP_AWK_UNIQUE     = (1 << 2), /* a function name should not coincide to be a variable name */
	XP_AWK_SHADING    = (1 << 3), /* allow variable shading */
	XP_AWK_SHIFT      = (1 << 4), /* support shift operators */
	XP_AWK_HASHSIGN   = (1 << 5), /* support comments by a hash sign */
	XP_AWK_DBLSLASHES = (1 << 6)  /* support comments by double slashes */
};

/* run options */
enum
{
	XP_AWK_RUNMAIN  = (1 << 0)  /* execution starts from main */
};

/* error code */
enum
{
	XP_AWK_ENOERR,         /* no error */
	XP_AWK_ENOMEM,         /* out of memory */

	XP_AWK_ENOSRCIO,       /* no source io handler set */
	XP_AWK_ESRCINOPEN,
	XP_AWK_ESRCINCLOSE,
	XP_AWK_ESRCINNEXT,
	XP_AWK_ESRCINDATA,     /* error in reading source */

	XP_AWK_ETXTINOPEN,
	XP_AWK_ETXTINCLOSE,
	XP_AWK_ETXTINNEXT,
	XP_AWK_ETXTINDATA,     /* error in reading text */

	XP_AWK_ELXCHR,         /* lexer came accross an wrong character */
	XP_AWK_ELXUNG,         /* lexer failed to unget a character */

	XP_AWK_EENDSRC,        /* unexpected end of source */
	XP_AWK_EENDSTR,        /* unexpected end of a string */
	XP_AWK_EENDREX,        /* unexpected end of a regular expression */
	XP_AWK_ELBRACE,        /* left brace expected */
	XP_AWK_ELPAREN,        /* left parenthesis expected */
	XP_AWK_ERPAREN,        /* right parenthesis expected */
	XP_AWK_ERBRACK,        /* right bracket expected */
	XP_AWK_ECOMMA,         /* comma expected */
	XP_AWK_ESEMICOLON,     /* semicolon expected */
	XP_AWK_ECOLON,         /* colon expected */
	XP_AWK_EIN,            /* keyword 'in' is expected */
	XP_AWK_ENOTVAR,        /* not a variable name after 'in' */
	XP_AWK_EEXPRESSION,    /* expression expected */

	XP_AWK_EWHILE,         /* keyword 'while' is expected */
	XP_AWK_EASSIGNMENT,    /* assignment statement expected */
	XP_AWK_EIDENT,         /* identifier expected */
	XP_AWK_EDUPBEGIN,      /* duplicate BEGIN */
	XP_AWK_EDUPEND,        /* duplicate END */
	XP_AWK_EDUPFUNC,       /* duplicate function name */
	XP_AWK_EDUPPARAM,      /* duplicate parameter name */
	XP_AWK_EDUPVAR,        /* duplicate variable name */
	XP_AWK_EDUPNAME,       /* duplicate name - function, variable, etc */
	XP_AWK_EUNDEF,         /* undefined identifier */
	XP_AWK_ELVALUE,        /* l-value required */
	XP_AWK_ETOOMANYARGS,   /* too many arguments */

	/* run time error */
	XP_AWK_EDIVBYZERO,     /* divide by zero */
	XP_AWK_EOPERAND,       /* invalid operand */
	XP_AWK_ENOSUCHFUNC,    /* no such function */
	XP_AWK_ENOTASSIGNABLE, /* value not assignable */
	XP_AWK_ENOTINDEXABLE,  /* not indexable value */
	XP_AWK_EWRONGINDEX,    /* wrong index */
	XP_AWK_EPIPE,          /* pipe operation error */
	XP_AWK_EINTERNAL       /* internal error */
};

#ifdef __cplusplus
extern "C" {
#endif

xp_awk_t* xp_awk_open (void);
int xp_awk_close (xp_awk_t* awk);

int xp_awk_geterrnum (xp_awk_t* awk);
const xp_char_t* xp_awk_geterrstr (xp_awk_t* awk);

void xp_awk_clear (xp_awk_t* awk);
void xp_awk_setparseopt (xp_awk_t* awk, int opt);
void xp_awk_setrunopt (xp_awk_t* awk, int opt);

int xp_awk_attsrc (xp_awk_t* awk, xp_awk_io_t src, void* arg);
int xp_awk_detsrc (xp_awk_t* awk);

xp_size_t xp_awk_getsrcline (xp_awk_t* awk);
/* TODO: xp_awk_parse (xp_awk_t* awk, xp_awk_io_t src, void* arg)??? */
int xp_awk_parse (xp_awk_t* awk);
int xp_awk_run (xp_awk_t* awk, xp_awk_io_t txtio, void* txtio_arg);

/* utility functions exported by awk.h */
xp_long_t xp_awk_strtolong (
	const xp_char_t* str, int base, const xp_char_t** endptr);
xp_real_t xp_awk_strtoreal (const xp_char_t* str);

#ifdef __cplusplus
}
#endif

#endif
