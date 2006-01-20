/*
 * $Id: sa.h,v 1.1 2006-01-20 07:32:38 bacon Exp $
 */

#ifndef _XP_AWK_SA_H_
#define _XP_AWK_SA_H_

#ifdef __STAND_ALONE 

#include <stdio.h>
#include <wchar.h>
#include <wctype.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define xp_malloc malloc
#define xp_realloc realloc
#define xp_free free
#define xp_assert assert
#define xp_printf wprintf

#define xp_isdigit iswdigit
#define xp_isalpha iswalpha
#define xp_isalnum iswalnum
#define xp_isspace iswspace

#define xp_strcmp wcscmp
#define xp_strlen wcslen
#ifdef _WIN32
#define xp_strdup _wcsdup
#else
#define xp_strdup wcsdup
#endif

#define xp_main  main

#define XP_CHAR(c) L##c
#define XP_TEXT(c) L##c
#define XP_CHAR_EOF WEOF

#define XP_NULL NULL

#define xp_sizeof(n)   (sizeof(n))
#define xp_countof(n)  (sizeof(n) / sizeof(n[0]))

#define xp_true (0 == 0)
#define xp_false (0 != 0)

typedef wchar_t xp_char_t;
typedef wint_t  xp_cint_t;
typedef size_t  xp_size_t;
typedef int	xp_bool_t;

#ifdef _WIN32
typedef long	xp_ssize_t;
#else
typedef ssize_t xp_ssize_t;
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
