/*
 * $Id: str.h 496 2008-12-15 09:56:48Z baconevi $
 *
 * {License}
 */

#ifndef _ASE_CMN_STR_H_
#define _ASE_CMN_STR_H_

#include <ase/types.h>
#include <ase/macros.h>

/****o* ase.cmn.str/string
 * DESCRIPTION
 *  <ase/cmn/str.h> defines various functions, types, macros to manipulate
 *  strings.
 *
 *  The ase_cstr_t type and the ase_xstr_t defined in ase/types.h helps you
 *  dealing with a string pointer and length.
 *
 *  #include <ase/cmn/str.h>
 *
 * EXAMPLES
 *  void f (void)
 *  {
 *  }
 ******
 */


#define ASE_STR_LEN(s)       ((s)->len)
#define ASE_STR_PTR(s)       ((s)->ptr)
#define ASE_STR_CAPA(s)      ((s)->capa)
#define ASE_STR_CHAR(s,idx)  ((s)->ptr[idx])

#define ASE_STR_MMGR(s)      ((s)->mmgr)
#define ASE_STR_XTN(s)       ((void*)(((ase_str_t*)s) + 1))
#define ASE_STR_SIZER(s)     ((s)->sizer)

typedef struct ase_str_t ase_str_t;
typedef ase_size_t (*ase_str_sizer_t) (ase_str_t* data, ase_size_t hint);

struct ase_str_t
{
	ase_mmgr_t* mmgr;
	ase_str_sizer_t sizer;

	ase_char_t* ptr;
	ase_size_t  len;
	ase_size_t  capa;
};

/* int ase_chartonum (ase_char_t c, int base) */
#define ASE_CHARTONUM(c,base) \
	((c>=ASE_T('0') && c<=ASE_T('9'))? ((c-ASE_T('0')<base)? (c-ASE_T('0')): base): \
	 (c>=ASE_T('A') && c<=ASE_T('Z'))? ((c-ASE_T('A')+10<base)? (c-ASE_T('A')+10): base): \
	 (c>=ASE_T('a') && c<=ASE_T('z'))? ((c-ASE_T('a')+10<base)? (c-ASE_T('a')+10): base): base)

/* ase_strtonum (const ase_char_t* nptr, ase_char_t** endptr, int base) */
#define ASE_STRTONUM(value,nptr,endptr,base) \
{ \
	int __ston_f = 0, __ston_v; \
	const ase_char_t* __ston_ptr = nptr; \
	for (;;) { \
		ase_char_t __ston_c = *__ston_ptr; \
		if (__ston_c == ASE_T(' ') || \
		    __ston_c == ASE_T('\t')) { __ston_ptr++; continue; } \
		if (__ston_c == ASE_T('-')) { __ston_f++; __ston_ptr++; } \
		if (__ston_c == ASE_T('+')) { __ston_ptr++; } \
		break; \
	} \
	for (value = 0; (__ston_v = ASE_CHARTONUM(*__ston_ptr, base)) < base; __ston_ptr++) { \
		value = value * base + __ston_v; \
	} \
	if (endptr != ASE_NULL) *((const ase_char_t**)endptr) = __ston_ptr; \
	if (__ston_f > 0) value *= -1; \
}

/* ase_strxtonum (const ase_char_t* nptr, ase_size_t len, ase_char_char** endptr, int base) */
#define ASE_STRXTONUM(value,nptr,len,endptr,base) \
{ \
	int __ston_f = 0, __ston_v; \
	const ase_char_t* __ston_ptr = nptr; \
	const ase_char_t* __ston_end = __ston_ptr + len; \
	value = 0; \
	while (__ston_ptr < __ston_end) { \
		ase_char_t __ston_c = *__ston_ptr; \
		if (__ston_c == ASE_T(' ') || __ston_c == ASE_T('\t')) { \
			__ston_ptr++; continue; \
		} \
		if (__ston_c == ASE_T('-')) { __ston_f++; __ston_ptr++; } \
		if (__ston_c == ASE_T('+')) { __ston_ptr++; } \
		break; \
	} \
	for (value = 0; __ston_ptr < __ston_end && \
	               (__ston_v = ASE_CHARTONUM(*__ston_ptr, base)) != base; __ston_ptr++) { \
		value = value * base + __ston_v; \
	} \
	if (endptr != ASE_NULL) *((const ase_char_t**)endptr) = __ston_ptr; \
	if (__ston_f > 0) value *= -1; \
}

#ifdef __cplusplus
extern "C" {
#endif

/* 
 * basic string functions
 */
ase_size_t ase_strlen (const ase_char_t* str);
ase_size_t ase_strbytes (const ase_char_t* str);

ase_size_t ase_strcpy (
	ase_char_t* buf, const ase_char_t* str);
ase_size_t ase_strxcpy (
	ase_char_t* buf, ase_size_t bsz, const ase_char_t* str);
ase_size_t ase_strncpy (
	ase_char_t* buf, const ase_char_t* str, ase_size_t len);
ase_size_t ase_strxncpy (
    ase_char_t* buf, ase_size_t bsz, const ase_char_t* str, ase_size_t len);

ase_size_t ase_strxcat (
    ase_char_t* buf, ase_size_t bsz, const ase_char_t* str);
ase_size_t ase_strxncat (
    ase_char_t* buf, ase_size_t bsz, const ase_char_t* str, ase_size_t len);

int ase_strcmp (const ase_char_t* s1, const ase_char_t* s2);
int ase_strxcmp (const ase_char_t* s1, ase_size_t len1, const ase_char_t* s2);
int ase_strxncmp (
	const ase_char_t* s1, ase_size_t len1, 
	const ase_char_t* s2, ase_size_t len2);

int ase_strcasecmp (
	const ase_char_t* s1, const ase_char_t* s2, ase_ccls_t* ccls);

/****f* ase.cmn.str/ase_strxncasecmp
 * NAME
 *  ase_strxncasecmp - compare strings ignoring case
 *
 * DESCRIPTION
 *  The ase_strxncasecmp() function compares characters at the same position 
 *  in each string after converting them to the same case temporarily. 
 *  It accepts two strings and a character class handler. A string is 
 *  represented by its beginning pointer and length. You can write your own
 *  character class handler or use ASE_CCLS_GETDFL() to get the default 
 *  character class handler.
 *
 *  For two strings to be equal, they need to have the same length and all
 *  characters in the first string should be equal to their counterpart in the
 *  second string.
 *
 * RETURN
 *  The ase_strxncasecmp() returns 0 if two strings are equal, a positive
 *  number if the first string is larger, -1 if the second string is larger.
 *  
 * EXAMPLES
 *   ase_strxncasecmp (ASE_T("foo"), 3, ASE_T("FoO"), 3, ASE_CCLS_GETDFL());
 *
 * SYNOPSIS
 */
int ase_strxncasecmp (
	const ase_char_t* s1  /* the pointer to the first string */,
	ase_size_t len1       /* the length of the first string */, 
	const ase_char_t* s2  /* the pointer to the second string */,
	ase_size_t len2       /* the length of the second string */,
	ase_ccls_t* ccls      /* character class handler */
);
/******/

ase_char_t* ase_strdup (const ase_char_t* str, ase_mmgr_t* mmgr);
ase_char_t* ase_strxdup (
	const ase_char_t* str, ase_size_t len, ase_mmgr_t* mmgr);
ase_char_t* ase_strxdup2 (
	const ase_char_t* str1, ase_size_t len1,
	const ase_char_t* str2, ase_size_t len2, ase_mmgr_t* mmgr);

ase_char_t* ase_strstr (const ase_char_t* str, const ase_char_t* sub);
ase_char_t* ase_strxstr (
	const ase_char_t* str, ase_size_t size, const ase_char_t* sub);
ase_char_t* ase_strxnstr (
	const ase_char_t* str, ase_size_t strsz, 
	const ase_char_t* sub, ase_size_t subsz);

ase_char_t* ase_strchr (const ase_char_t* str, ase_cint_t c);
ase_char_t* ase_strxchr (const ase_char_t* str, ase_size_t len, ase_cint_t c);
ase_char_t* ase_strrchr (const ase_char_t* str, ase_cint_t c);
ase_char_t* ase_strxrchr (const ase_char_t* str, ase_size_t len, ase_cint_t c);

/* Checks if a string begins with a substring */
ase_char_t* ase_strbeg (const ase_char_t* str, const ase_char_t* sub);
ase_char_t* ase_strxbeg (
	const ase_char_t* str, ase_size_t len, const ase_char_t* sub);
ase_char_t* ase_strnbeg (
	const ase_char_t* str, const ase_char_t* sub, ase_size_t len);
ase_char_t* ase_strxnbeg (
	const ase_char_t* str, ase_size_t len1, 
	const ase_char_t* sub, ase_size_t len2);

/* Checks if a string ends with a substring */
ase_char_t* ase_strend (const ase_char_t* str, const ase_char_t* sub);
ase_char_t* ase_strxend (
	const ase_char_t* str, ase_size_t len, const ase_char_t* sub);
ase_char_t* ase_strnend (
	const ase_char_t* str, const ase_char_t* sub, ase_size_t len);
ase_char_t* ase_strxnend (
	const ase_char_t* str, ase_size_t len1, 
	const ase_char_t* sub, ase_size_t len2);

/* 
 * string conversion
 */
int ase_strtoi (const ase_char_t* str);
long ase_strtol (const ase_char_t* str);
unsigned int ase_strtoui (const ase_char_t* str);
unsigned long ase_strtoul (const ase_char_t* str);

int ase_strxtoi (const ase_char_t* str, ase_size_t len);
long ase_strxtol (const ase_char_t* str, ase_size_t len);
unsigned int ase_strxtoui (const ase_char_t* str, ase_size_t len);
unsigned long ase_strxtoul (const ase_char_t* str, ase_size_t len);

ase_int_t ase_strtoint (const ase_char_t* str);
ase_long_t ase_strtolong (const ase_char_t* str);
ase_uint_t ase_strtouint (const ase_char_t* str);
ase_ulong_t ase_strtoulong (const ase_char_t* str);

ase_int_t ase_strxtoint (const ase_char_t* str, ase_size_t len);
ase_long_t ase_strxtolong (const ase_char_t* str, ase_size_t len);
ase_uint_t ase_strxtouint (const ase_char_t* str, ase_size_t len);
ase_ulong_t ase_strxtoulong (const ase_char_t* str, ase_size_t len);

ase_str_t* ase_str_open (
	ase_mmgr_t* mmgr,
	ase_size_t ext,
	ase_size_t capa
);

void ase_str_close (
	ase_str_t* str
);

/* 
 * If capa is 0, it doesn't allocate the buffer in advance. 
 */
ase_str_t* ase_str_init (
	ase_str_t* str,
	ase_mmgr_t* mmgr,
	ase_size_t capa
);

void ase_str_fini (
	ase_str_t* str
);

/****f* ase.cmn.str/ase_str_yield
 * NAME 
 *  ase_str_yield - yield the buffer 
 * 
 * DESCRIPTION
 *  The ase_str_yield() function assigns the buffer to an variable of the
 *  ase_xstr_t type and recreate a new buffer of the new_capa capacity.
 *  The function fails if it fails to allocate a new buffer.
 *
 * RETURN
 *  The ase_str_yield() function returns 0 on success, and -1 on failure.
 *
 * SYNOPSIS
 */
int ase_str_yield (
	ase_str_t* str  /* a dynamic string */,
	ase_xstr_t* buf /* the pointer to a ase_xstr_t variable */,
	int new_capa    /* new capacity in number of characters */
);
/******/

void* ase_str_getxtn (
	ase_str_t* str
);

ase_mmgr_t* ase_str_getmmgr (
	ase_str_t* str
);

void ase_str_setmmgr (
	ase_str_t* str,
	ase_mmgr_t* mmgr
);

/*
 * NAME: get the sizer
 *
 * DESCRIPTION:
 *  The ase_str_getsizer() function returns the sizer specified.
 *
 * RETURNS: a sizer function set or ASE_NULL if no sizer is set.
 */
ase_str_sizer_t ase_str_getsizer (
	ase_str_t* str  /* a dynamic string */
);

/*
 * NAME: specify a sizer
 *
 * DESCRIPTION:
 *  The ase_str_setsizer() function specify a new sizer for a dynamic string.
 *  With no sizer specified, the dynamic string doubles the current buffer
 *  when it needs to increase its size. The sizer function is passed a dynamic
 *  string and the minimum capacity required to hold data after resizing.
 *  The string is truncated if the sizer function returns a smaller number
 *  than the hint passed.
 */
void ase_str_setsizer (
	ase_str_t* str         /* a dynamic string */,
	ase_str_sizer_t sizer /* a sizer function */
);

/*
 * NAME: get capacity
 *
 * DESCRIPTION:
 *  The ase_str_getcapa() function returns the current capacity.
 *  You may use ASE_STR_CAPA(str) macro for performance sake.
 *
 * RETURNS: the current capacity in number of characters.
 */
ase_size_t ase_str_getcapa (
	ase_str_t* str /* a dynamic string */
);

/*
 * NAME: set new capacity
 *
 * DESCRIPTION:
 *  The ase_str_setcapa() function set new capacity. If the new capacity
 *  is smaller than the old, the overflowing characters are removed from
 *  from the buffer.
 * 
 * RETURNS: -1 on failure, a new capacity on success 
 */
ase_size_t ase_str_setcapa (
	ase_str_t* str  /* a dynamic string */,
	ase_size_t capa /* a new capacity */
);

void ase_str_clear (
	ase_str_t* str
);

void ase_str_swap (
	ase_str_t* str,
	ase_str_t* str2
);

ase_size_t ase_str_cpy (
	ase_str_t*        str,
	const ase_char_t* s
);

ase_size_t ase_str_ncpy (
	ase_str_t*        str,
	const ase_char_t* s,
	ase_size_t        len
);

ase_size_t ase_str_cat (
	ase_str_t*        str,
	const ase_char_t* s
);

ase_size_t ase_str_ncat (
	ase_str_t*        str,
	const ase_char_t* s,
	ase_size_t        len
);

ase_size_t ase_str_ccat (
	ase_str_t* str,
	ase_char_t c
);

ase_size_t ase_str_nccat (
	ase_str_t* str,
	ase_char_t c,
	ase_size_t len
);


ase_size_t ase_mbstowcs (
        const ase_mchar_t* mbs,
        ase_wchar_t*       wcs,
	ase_size_t*        wcslen
);

/****f* ase.cmn.str/ase_mbsntowcsn
 * NAME
 *  ase_mbsntowcsn - convert a multibyte string to a wide character string
 * 
 * RETURN
 *  The ase_mbstowcs() function returns the number of bytes handled.
 *
 * SYNOPSIS
 */
ase_size_t ase_mbsntowcsn (
        const ase_mchar_t* mbs,
	ase_size_t         mbslen,
        ase_wchar_t*       wcs,
	ase_size_t*        wcslen
);
/******/

/****f* ase.cmn.str/ase_wcstombs
 * NAME
 *  ase_wcstombs - convert a wide character string to a multibyte string.
 *
 * DESCRIPTION
 *  The ase_wcstombs() function converts a null-terminated wide character 
 *  string to a multibyte string and stores it into the buffer pointed to
 *  by mbs. The pointer to a variable holding the buffer length should be
 *  passed to the function as the third parameter. After conversion, it holds 
 *  the length of the multibyte string excluding the terminating-null.
 *  It may not null-terminate the resulting multibyte string if the buffer
 *  is not large enough.
 *
 * RETURN
 *  The ase_wcstombs() function returns the number of wide characters handled.
 *
 * SYNOPSIS
 */
ase_size_t ase_wcstombs (
        const ase_wchar_t* wcs,
        ase_mchar_t*       mbs,
	ase_size_t*        mbslen
);

/****f* ase.cmn.str/ase_wcsntombsn
 * NAME
 *  ase_wcstombs - convert a wide character string to a multibyte string
 *
 * RETURN
 *  The ase_wcstombs() function returns the number of wide characters handled.
 *
 * SYNOPSIS
 */
ase_size_t ase_wcsntombsn (
        const ase_wchar_t* wcs,
	ase_size_t         wcslen,
        ase_mchar_t*       mbs,
	ase_size_t*        mbslen
);
/******/

/****f* ase.cmn.str/ase_wcstombs_strict
 * NAME
 *  ase_wcstombs_strict - convert a wide character string to a multibyte string.
 *
 * DESCRIPTION
 *  The ase_wcstombs_strict() function performs the same as the ase_wcsmbs() 
 *  function except that it returns an error if it can't fully convert the
 *  input string and/or the buffer is not large enough.
 *
 * RETURN
 *  The ase_wcstombs_strict() function returns 0 on success and -1 on failure.
 * SYNOPSIS
 */
int ase_wcstombs_strict (
        const ase_wchar_t* wcs,
        ase_mchar_t*       mbs,
	ase_size_t         mbslen
);
/******/

#ifdef __cplusplus
}
#endif

#endif
