/*
 * $Id: sa.h,v 1.16 2006-03-31 12:04:14 bacon Exp $
 */

#ifndef _XP_AWK_SA_H_
#define _XP_AWK_SA_H_

#ifndef _XP_AWK_AWK_H_
#error Never include this file directly. Include <xp/awk/awk.h> instead
#endif

#ifdef __STAND_ALONE 

#if !defined(XP_CHAR_IS_MCHAR) && !defined(XP_CHAR_IS_WCHAR)
#error Neither XP_CHAR_IS_MCHAR nor XP_CHAR_IS_WCHAR is defined.
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#ifdef XP_CHAR_IS_MCHAR
#include <ctype.h>
#else
#include <wchar.h>
#if !defined(__BEOS__)
#include <wctype.h>
#endif
#endif

#define xp_malloc malloc
#define xp_realloc realloc
#define xp_free free
#define xp_assert assert

#ifdef XP_CHAR_IS_MCHAR
#define xp_isdigit isdigit
#define xp_isalpha isalpha
#define xp_isalnum isalnum
#define xp_isspace isspace
#define xp_toupper toupper
#define xp_tolower tolower
#else
#define xp_isdigit iswdigit
#define xp_isalpha iswalpha
#define xp_isalnum iswalnum
#define xp_isspace iswspace
#define xp_toupper towupper
#define xp_tolower towlower
#endif

#define xp_memcpy memcpy
#define xp_memcmp memcmp

#define xp_va_start(pvar,param) va_start(pvar,param)
#define xp_va_list va_list
#define xp_va_end(pvar)      va_end(pvar)
#define xp_va_arg(pvar,type) va_arg(pvar,type)

#define xp_main  main

#define XP_NULL NULL

#define xp_sizeof(n)   (sizeof(n))
#define xp_countof(n)  (sizeof(n) / sizeof(n[0]))

#define xp_true (0 == 0)
#define xp_false (0 != 0)

typedef unsigned char xp_byte_t;
typedef int xp_bool_t;
typedef size_t xp_size_t;

#ifdef XP_CHAR_IS_MCHAR
	typedef char xp_char_t;
	typedef int xp_cint_t;
#else
	typedef wchar_t xp_char_t;
	typedef wint_t xp_cint_t;
#endif

#if defined(_WIN32) || defined(vms) || defined(__vms)
	typedef long xp_ssize_t;
#else
	typedef ssize_t xp_ssize_t;
#endif

#if defined(_WIN32)
	typedef __int64 xp_long_t;
	typedef long double xp_real_t;
#elif defined(vax) || defined(__vax)
	typedef long xp_long_t;
	typedef long double xp_real_t;
#else
	typedef long long xp_long_t;
	typedef long double xp_real_t;
#endif

#ifdef XP_CHAR_IS_MCHAR
	#define XP_CHAR(c) c
	#define XP_TEXT(c) c
	#define XP_CHAR_EOF EOF
#else
	#define XP_CHAR(c) L##c
	#define XP_TEXT(c) L##c
	#ifdef WEOF
	#define XP_CHAR_EOF WEOF
	#else
	#define XP_CHAR_EOF ((xp_char_t)-1)
	#endif
#endif

#define XP_STR_LEN(x)  ((x)->size)
#define XP_STR_SIZE(x) ((x)->size + 1)
#define XP_STR_CAPA(x) ((x)->capa)
#define XP_STR_BUF(x)  ((x)->buf)

typedef struct xp_str_t xp_str_t;

struct xp_str_t
{
	xp_char_t* buf;
	xp_size_t size;
	xp_size_t capa;
	xp_bool_t __dynamic;
};

#ifdef __cplusplus
extern "C" {
#endif

xp_size_t xp_strlen (const xp_char_t* str);
xp_char_t* xp_strdup (const xp_char_t* str);
xp_char_t* xp_strxdup (const xp_char_t* str, xp_size_t len);

xp_size_t xp_strcpy (xp_char_t* buf, const xp_char_t* str);
xp_size_t xp_strxncpy (
	xp_char_t* buf, xp_size_t bsz, const xp_char_t* str, xp_size_t len);

int xp_strcmp (const xp_char_t* s1, const xp_char_t* s2);
xp_long_t xp_strtolong (xp_char_t* str);
xp_real_t xp_strtoreal (xp_char_t* str);

int xp_printf (const xp_char_t* fmt, ...);
int xp_vprintf (const xp_char_t* fmt, xp_va_list ap);
int xp_sprintf (xp_char_t* buf, xp_size_t size, const xp_char_t* fmt, ...);
int xp_vsprintf (
	xp_char_t* buf, xp_size_t size, const xp_char_t* fmt, xp_va_list ap);

xp_str_t* xp_str_open (xp_str_t* str, xp_size_t capa);
void xp_str_close (xp_str_t* str);
xp_size_t xp_str_cat (xp_str_t* str, const xp_char_t* s);
xp_size_t xp_str_ncat (xp_str_t* str, const xp_char_t* s, xp_size_t len);
xp_size_t xp_str_ccat (xp_str_t* str, xp_char_t c);
void xp_str_clear (xp_str_t* str);

#ifdef __cplusplus
}
#endif

#endif

#endif
