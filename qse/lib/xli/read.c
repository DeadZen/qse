/*
 * $Id$
 *
    Copyright (c) 2006-2014 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "xli.h"
#include <qse/cmn/chr.h>

static int get_char (qse_xli_t* xli);
static int get_token (qse_xli_t* xli);
static int read_list (qse_xli_t* xli, qse_xli_list_t* list, const qse_xli_scm_t* override);

enum
{
	TOK_STATUS_ENABLE_NSTR = (1 << 0)
};

static qse_xli_scm_t scm_val_iffy = { QSE_XLI_SCM_VALSTR | QSE_XLI_SCM_KEYNODUP, 1, 1 };

static int close_current_stream (qse_xli_t* xli)
{
	qse_ssize_t n;

	n = xli->rio.impl (xli, QSE_XLI_IO_CLOSE, xli->rio.inp, QSE_NULL, 0);
	if (n <= -1)
	{
		if (xli->errnum == QSE_XLI_ENOERR) 
			qse_xli_seterrnum (xli, QSE_XLI_EIOUSR, QSE_NULL);
		return -1;
	}

	return 0;
}

enum tok_t
{
	TOK_EOF,
	TOK_XINCLUDE,
	TOK_SEMICOLON,
	TOK_LBRACE,
	TOK_RBRACE,
	TOK_EQ,
	TOK_COMMA,
	TOK_DQSTR,
	TOK_SQSTR,
	TOK_NSTR,
	TOK_IDENT,
	TOK_TEXT,
	TOK_TAG,

	__TOKEN_COUNT__
};

#define GET_CHAR(xli) \
	do { if (get_char(xli) <= -1) return -1; } while(0)

#define GET_CHAR_TO(xli,c) \
	do { \
		if (get_char(xli) <= -1) return -1; \
		c = (xli)->rio.last.c; \
	} while(0)

#define ADD_TOKEN_CHAR(xli,tok,c) \
	do { \
		if (qse_str_ccat((tok)->name,(c)) == (qse_size_t)-1) \
		{ \
			qse_xli_seterrnum (xli, QSE_XLI_ENOMEM, QSE_NULL); \
			return -1; \
		} \
	} while (0)

#define ADD_TOKEN_STR(xli,tok,s,l) \
	do { \
		if (qse_str_ncat((tok)->name,(s),(l)) == (qse_size_t)-1) \
		{ \
			qse_xli_seterrnum (xli, QSE_XLI_ENOMEM, QSE_NULL); \
			return -1; \
		} \
	} while (0)

#define SET_TOKEN_TYPE(xli,tok,code) \
	do { (tok)->type = (code); } while (0)

#define MATCH(xli,tok_type) ((xli)->tok.type == (tok_type))

typedef struct kwent_t kwent_t;

struct kwent_t 
{ 
	qse_cstr_t name;
	int type; 
};

/* note that the keyword must start with @. */
static kwent_t kwtab[] = 
{
	/* keep it sorted by the first field for binary search */
	{ { QSE_T("@include"),     8 }, TOK_XINCLUDE }
};

static int get_char (qse_xli_t* xli)
{
	qse_ssize_t n;

	if (xli->rio.inp->b.pos >= xli->rio.inp->b.len)
	{
		n = xli->rio.impl (
			xli, QSE_XLI_IO_READ, xli->rio.inp,
			xli->rio.inp->b.buf, QSE_COUNTOF(xli->rio.inp->b.buf)
		);
		if (n <= -1)
		{
			if (xli->errnum == QSE_XLI_ENOERR)
				qse_xli_seterrnum (xli, QSE_XLI_EIOUSR, QSE_NULL); 
			return -1;
		}

		if (n == 0)
		{
			xli->rio.inp->last.c = QSE_CHAR_EOF;
			xli->rio.inp->last.line = xli->rio.inp->line;
			xli->rio.inp->last.colm = xli->rio.inp->colm;
			xli->rio.inp->last.file = xli->rio.inp->name;
			xli->rio.last = xli->rio.inp->last;
			return 0;
		}

		xli->rio.inp->b.pos = 0;
		xli->rio.inp->b.len = n;
	}

	if (xli->rio.inp->last.c == QSE_T('\n'))
	{
		/* if the previous charater was a newline,
		 * increment the line counter and reset column to 1.
		 * incrementing it line number here instead of
		 * updating inp->last causes the line number for
		 * TOK_EOF to be the same line as the last newline. */
		xli->rio.inp->line++;
		xli->rio.inp->colm = 1;
	}
	
	xli->rio.inp->last.c = xli->rio.inp->b.buf[xli->rio.inp->b.pos++];
	xli->rio.inp->last.line = xli->rio.inp->line;
	xli->rio.inp->last.colm = xli->rio.inp->colm++;
	xli->rio.inp->last.file = xli->rio.inp->name;
	xli->rio.last = xli->rio.inp->last;
	return 0;
}

static int skip_spaces (qse_xli_t* xli)
{
	qse_cint_t c = xli->rio.last.c;
	while (QSE_ISSPACE(c)) GET_CHAR_TO (xli, c);
	return 0;
}

static int skip_comment (qse_xli_t* xli, qse_xli_tok_t* tok)
{
	qse_cint_t c = xli->rio.last.c;

	if (c == QSE_T('#'))
	{
		/* skip up to \n */
		/* TODO: support a different line terminator */
		qse_str_clear (tok->name);

		do
		{ 
			GET_CHAR_TO (xli, c); 
			if (c == QSE_T('\n') || c == QSE_CHAR_EOF) break;

			if  (xli->opt.trait & QSE_XLI_KEEPTEXT) ADD_TOKEN_CHAR (xli, tok, c);
		}
		while (1);

		if ((xli->opt.trait & QSE_XLI_KEEPTEXT) && 
		    qse_xli_inserttext (xli, xli->parlink->list, QSE_NULL, QSE_STR_PTR(tok->name)) == QSE_NULL) return -1;

		GET_CHAR (xli); /* eat the new line letter */
		return 1; /* comment by # */
	}

	return 0; 
}

static int classify_ident (qse_xli_t* xli, const qse_cstr_t* name)
{
	/* perform binary search */

	/* declaring left, right, mid to be the int type is ok
	 * because we know kwtab is small enough. */
	int left = 0, right = QSE_COUNTOF(kwtab) - 1, mid;

	while (left <= right)
	{
		int n;
		kwent_t* kwp;

		mid = (left + right) / 2;
		kwp = &kwtab[mid];

		n = qse_strxncmp (kwp->name.ptr, kwp->name.len, name->ptr, name->len);
		if (n > 0) 
		{
			/* if left, right, mid were of qse_size_t,
			 * you would need the following line. 
			if (mid == 0) break;
			 */
			right = mid - 1;
		}
		else if (n < 0) left = mid + 1;
		else return kwp->type;
	}

	return TOK_IDENT;
}

static int get_symbols (qse_xli_t* xli, qse_cint_t c, qse_xli_tok_t* tok)
{
	struct ops_t
	{
		const qse_char_t* str;
		qse_size_t len;
		int tid;
	};

	static struct ops_t ops[] = 
	{
		{ QSE_T("="),   1, TOK_EQ          },
		{ QSE_T(","),   1, TOK_COMMA       },
		{ QSE_T(";"),   1, TOK_SEMICOLON   },
		{ QSE_T("{"),   1, TOK_LBRACE      },
		{ QSE_T("}"),   1, TOK_RBRACE      },
		{ QSE_NULL,     0, 0,              }
	};

	struct ops_t* p;
	int idx = 0;

	/* note that the loop below is not generaic enough.
	 * you must keep the operators strings in a particular order */


	for (p = ops; p->str != QSE_NULL; )
	{
		if (p->str[idx] == QSE_T('\0'))
		{
			ADD_TOKEN_STR (xli, tok, p->str, p->len);
			SET_TOKEN_TYPE (xli, tok, p->tid);
			return 1;
		}

		if (c == p->str[idx])
		{
			idx++;
			GET_CHAR_TO (xli, c);
			continue;
		}

		p++;
	}

	return 0;
}

static int end_include (qse_xli_t* xli, int noeof)
{
	int x;
	qse_xli_io_arg_t* cur;

	if (xli->rio.inp == &xli->rio.top) return 0; /* no include */


	/* if it is an included file, close it and
	 * retry to read a character from an outer file */

	x = xli->rio.impl (
		xli, QSE_XLI_IO_CLOSE, 
		xli->rio.inp, QSE_NULL, 0);

	/* if closing has failed, still destroy the
	 * sio structure first as normal and return
	 * the failure below. this way, the caller 
	 * does not call QSE_XLI_SIO_CLOSE on 
	 * xli->rio.inp again. */

	cur = xli->rio.inp;
	xli->rio.inp = xli->rio.inp->prev;

	QSE_ASSERT (cur->name != QSE_NULL);
	QSE_MMGR_FREE (xli->mmgr, cur);
	/* xli->parse.depth.incl--; */

	if ((xli->opt.trait & QSE_XLI_KEEPFILE) && !noeof && 
	    qse_xli_inserteof (xli, xli->parlink->list, QSE_NULL) == QSE_NULL) return -1;

	if (x != 0)
	{
		/* the failure mentioned above is returned here */
		if (xli->errnum == QSE_XLI_ENOERR)
			qse_xli_seterrnum (xli, QSE_XLI_EIOUSR, QSE_NULL); 
		return -1;
	}

	xli->rio.last = xli->rio.inp->last;
	return 1; /* ended the included file successfully */
}

static int begin_include (qse_xli_t* xli)
{
	qse_link_t* link;
	qse_xli_io_arg_t* arg = QSE_NULL;

	link = (qse_link_t*) qse_xli_callocmem (xli, 
		QSE_SIZEOF(*link) + QSE_SIZEOF(qse_char_t) * (QSE_STR_LEN(xli->tok.name) + 1));
	if (link == QSE_NULL) goto oops;

	qse_strncpy ((qse_char_t*)(link + 1), QSE_STR_PTR(xli->tok.name), QSE_STR_LEN(xli->tok.name));
	link->link = xli->rio_names;
	xli->rio_names = link;

	arg = (qse_xli_io_arg_t*) qse_xli_callocmem (xli, QSE_SIZEOF(*arg));
	if (arg == QSE_NULL) goto oops;

	arg->name = (const qse_char_t*)(link + 1);
	arg->line = 1;
	arg->colm = 1;

	/* let the argument's prev point field to the current */
	arg->prev = xli->rio.inp; 

	if (xli->rio.impl (xli, QSE_XLI_IO_OPEN, arg, QSE_NULL, 0) <= -1)
	{
		if (xli->errnum == QSE_XLI_ENOERR)
			qse_xli_seterrnum (xli, QSE_XLI_EIOUSR, QSE_NULL); 
		goto oops;
	}

	/* i update the current pointer after opening is successful */
	xli->rio.inp = arg;
	/* xli->parse.depth.incl++; */

	/* read in the first character in the included file. 
	 * so the next call to get_token() sees the character read
	 * from this file. */
	if (get_char (xli) <= -1 || get_token (xli) <= -1) 
	{
		end_include (xli, 1); 
		/* i don't jump to oops since i've called 
		 * end_include() where xli->rio.inp/arg is freed. */
		return -1;
	}

	if ((xli->opt.trait & QSE_XLI_KEEPFILE) &&
	    qse_xli_insertfile (xli, xli->parlink->list, QSE_NULL, arg->name) == QSE_NULL) 
	{
		end_include (xli, 1);
		return -1;
	}

	return 0;

oops:
	/* i don't need to free 'link' since it's linked to
	 * xli->rio_names that's freed at the beginning of qse_xli_read()
	 * or by qse_xli_fini() */
	if (arg) QSE_MMGR_FREE (xli->mmgr, arg);
	return -1;
}


static int get_token_into (qse_xli_t* xli, qse_xli_tok_t* tok)
{
	qse_cint_t c;
	int n;
	int skip_semicolon_after_include = 0;

retry:
	do 
	{
		if (skip_spaces (xli) <= -1) return -1;
		if ((n = skip_comment (xli, tok)) <= -1) return -1; 
	} 
	while (n >= 1);

	qse_str_clear (tok->name);
	tok->loc.file = xli->rio.last.file;
	tok->loc.line = xli->rio.last.line;
	tok->loc.colm = xli->rio.last.colm;

	c = xli->rio.last.c;

	if (c == QSE_CHAR_EOF) 
	{
		n = end_include (xli, 0);
		if (n <= -1) return -1;
		if (n >= 1) 
		{
			/*xli->rio.last = xli->rio.inp->last;*/
			/* mark that i'm retrying after end of an included file */
			skip_semicolon_after_include = 1; 
			goto retry;
		}

		ADD_TOKEN_STR (xli, tok, QSE_T("<EOF>"), 5);
		SET_TOKEN_TYPE (xli, tok, TOK_EOF);
	}	
	else if (c == QSE_T('@'))
	{
		/* keyword/directive - start with @ */

		int type;

		ADD_TOKEN_CHAR (xli, tok, c);
		GET_CHAR_TO (xli, c);

		if (!QSE_ISALPHA (c))
		{
			/* this directive is empty, not followed by a valid word */
			qse_xli_seterror (xli, QSE_XLI_EXKWEM, QSE_NULL, &tok->loc);
			return -1;
		}

		/* expect an identifier starting with an alphabet. the identifier 
		 * forming a keyword/directory is composed of alphabets. */
		do 
		{
			ADD_TOKEN_CHAR (xli, tok, c);
			GET_CHAR_TO (xli, c);
		} 
		while (QSE_ISALPHA (c));

		type = classify_ident (xli, QSE_STR_XSTR(tok->name));
		if (type == TOK_IDENT)
		{
			/* this keyword/directive is not recognized */
			qse_xli_seterror (xli, QSE_XLI_EXKWNR, QSE_STR_XSTR(tok->name), &tok->loc);
			return -1;
		}
		SET_TOKEN_TYPE (xli, tok, type);
	}
	else if (c == QSE_T('_') || QSE_ISALPHA (c) || 
	         (!(xli->tok_status & TOK_STATUS_ENABLE_NSTR) &&
	          (xli->opt.trait & QSE_XLI_LEADDIGIT) && 
	          QSE_ISDIGIT(c)))
	{
		int lead_digit = QSE_ISDIGIT(c);
		int all_digits = 1;

		/* a normal identifier can be composed of wider varieties of 
		 * characters than a keyword/directive */
		while (1)
		{
			ADD_TOKEN_CHAR (xli, tok, c);
			GET_CHAR_TO (xli, c);

			if (c == QSE_T('_') || c == QSE_T('-') || 
			    c == QSE_T(':') || c == QSE_T('*') ||
			    c == QSE_T('/') || QSE_ISALPHA (c)) 
			{
				all_digits = 0;
			}
			else if (QSE_ISDIGIT(c)) 
			{
				/* nothing to do */
			}
			else break;
		} 

		if (lead_digit && all_digits)
		{
			/* if an identifier begins with a digit, it must contain a non-digits character */
			qse_xli_seterror (xli, QSE_XLI_EIDENT, QSE_STR_XSTR(tok->name), &tok->loc);
			return -1;
		}

		SET_TOKEN_TYPE (xli, tok, TOK_IDENT);
	}
	else if ((xli->tok_status & TOK_STATUS_ENABLE_NSTR) && QSE_ISDIGIT(c))
	{
		SET_TOKEN_TYPE (xli, tok, TOK_NSTR);
		do
		{
			ADD_TOKEN_CHAR (xli, tok, c);
			GET_CHAR_TO (xli, c);
		}
		while (QSE_ISDIGIT(c));
	}
	else if (c == QSE_T('\''))
	{
		/* single-quoted string - no escaping */
		SET_TOKEN_TYPE (xli, tok, TOK_SQSTR);

		while (1)
		{
			GET_CHAR_TO (xli, c);

			if (c == QSE_CHAR_EOF)
			{
				/* the string is not closed */
				qse_xli_seterror (xli, QSE_XLI_ESTRNC, QSE_NULL, &tok->loc);
				return -1;
			}

			if (c == QSE_T('\''))
			{
				/* terminating quote */
				GET_CHAR (xli);
				break;
			}

			ADD_TOKEN_CHAR (xli, tok, c);
		}
	}
	else if (c == QSE_T('\"'))
	{
		/* double-quoted string - support escaping */
		int escaped = 0;

		SET_TOKEN_TYPE (xli, tok, TOK_DQSTR);

		while (1)
		{
			GET_CHAR_TO (xli, c);

			if (c == QSE_CHAR_EOF)
			{
				/* the string is not closed */
				qse_xli_seterror (xli, QSE_XLI_ESTRNC, QSE_NULL, &tok->loc);
				return -1;
			}

			if (!escaped)
			{
				if (c == QSE_T('\\')) 
				{
					escaped = 1;
					continue;
				}

				if (c == QSE_T('\"'))
				{
					/* terminating quote */
					GET_CHAR (xli);
					break;
				}

				ADD_TOKEN_CHAR (xli, tok, c);
			}
			else
			{
				ADD_TOKEN_CHAR (xli, tok, c);
				escaped = 0;
			}
		}
	}
	else if ((xli->opt.trait & (QSE_XLI_KEYTAG | QSE_XLI_STRTAG)) && c == QSE_T('['))
	{
		/* a string tag is a bracketed word placed in front of a string value.
		 *   A = [tg] "abc"; 
		 * "tg" is stored into the tag field of qse_xli_str_t. 
		 */

		SET_TOKEN_TYPE (xli, tok, TOK_TAG);
		
		while (1)
		{
			GET_CHAR_TO (xli, c);

			if (c == QSE_CHAR_EOF)
			{
				/* the string tag is not closed */
				qse_xli_seterror (xli, QSE_XLI_ETAGNC, QSE_NULL, &xli->tok.loc);
				return -1;
			}

			if (c == QSE_T(']'))
			{
				/* terminating quote */
				GET_CHAR (xli);
				break;
			}

			if (!QSE_ISALNUM(c) && c != QSE_T('-') && c != QSE_T('_') && 
			    c != QSE_T(':') && c != QSE_T('*') && c != QSE_T('/') && 
			    c != QSE_T(',') && c != QSE_T('.') && c != QSE_T('|'))
			{
				qse_char_t cc = (qse_char_t)c;
				qse_cstr_t ea;
				ea.ptr = &cc;
				ea.len = 1;
				qse_xli_seterror (xli, QSE_XLI_ETAGCHR, &ea, &tok->loc);
				return -1;
			}

			ADD_TOKEN_CHAR (xli, tok, c);
		}
	}
	else
	{
		n = get_symbols (xli, c, tok);
		if (n <= -1) return -1;
		if (n == 0)
		{
			/* not handled yet */
			if (c == QSE_T('\0'))
			{
				qse_cstr_t ea;
				ea.ptr = QSE_T("<NUL>");
				ea.len = 5;
				qse_xli_seterror (xli, QSE_XLI_ELXCHR, &ea, &tok->loc);
			}
			else
			{
				qse_char_t cc = (qse_char_t)c;
				qse_cstr_t ea;
				ea.ptr = &cc;
				ea.len = 1;
				qse_xli_seterror (xli, QSE_XLI_ELXCHR, &ea, &tok->loc);
			}
			return -1;
		}

		if (skip_semicolon_after_include && tok->type == TOK_SEMICOLON)
		{
			/* this handles the optional semicolon after the 
			 * included file named as in @include "file-name"; */
			skip_semicolon_after_include = 0;
			goto retry;
		}
	}

	if (skip_semicolon_after_include)
	{
		/* semiclon has not been skipped yet */
		qse_xli_seterror (xli, QSE_XLI_ESCOLON, QSE_STR_XSTR(tok->name), &tok->loc);
		return -1;
	}

	return 0;
}

static int get_token (qse_xli_t* xli)
{
	return get_token_into (xli, &xli->tok);
}

static int read_pair (qse_xli_t* xli, const qse_char_t* keytag, const qse_xli_scm_t* override)
{
	qse_cstr_t key;
	qse_xli_loc_t kloc;
	qse_char_t* name;
	qse_xli_pair_t* pair;
	qse_xli_list_t* parlist;
	qse_size_t dotted_curkey_len;
	qse_char_t* strtag;

	const qse_xli_scm_t* scm = QSE_NULL;
	int key_nodup = 0, key_alias = 0, val_iffy = 0;

	key.ptr = QSE_NULL;
	name = QSE_NULL;
	strtag = QSE_NULL;
	dotted_curkey_len = (qse_size_t)-1;

	parlist = xli->parlink->list;

	if (xli->opt.trait & QSE_XLI_KEYNODUP) key_nodup = 1;
	if (xli->opt.trait & QSE_XLI_KEYALIAS) key_alias = 1;

	kloc = xli->tok.loc;
	key.len = QSE_STR_LEN(xli->tok.name);
	key.ptr = qse_strdup (QSE_STR_PTR(xli->tok.name), xli->mmgr);
	if (key.ptr == QSE_NULL) 
	{
		qse_xli_seterrnum (xli, QSE_XLI_ENOMEM, QSE_NULL); 
		goto oops;
	}

	dotted_curkey_len = QSE_STR_LEN (xli->dotted_curkey);
	if ((dotted_curkey_len > 0 && qse_str_cat (xli->dotted_curkey, QSE_T(".")) == (qse_size_t)-1) ||
	    qse_str_cat (xli->dotted_curkey, key.ptr) == (qse_size_t)-1)
	{
		qse_xli_seterrnum (xli, QSE_XLI_ENOMEM, QSE_NULL); 
		goto oops;
	}

	if (xli->opt.trait & QSE_XLI_VALIDATE) 
	{
		if (override) scm = override;
		else
		{
			qse_rbt_pair_t* pair;

			pair = qse_rbt_search (xli->schema, QSE_STR_PTR(xli->dotted_curkey), QSE_STR_LEN(xli->dotted_curkey));
			if (pair == QSE_NULL)
			{
				qse_xli_seterror (xli, QSE_XLI_EUDKEY, (const qse_cstr_t*)&key, &kloc);
				goto oops;
			}

			scm = (qse_xli_scm_t*)QSE_RBT_VPTR(pair);
		}

		if (scm->flags & QSE_XLI_SCM_KEYNODUP) key_nodup = 2;
		if (scm->flags & QSE_XLI_SCM_KEYALIAS) key_alias = 2;
		if (scm->flags & QSE_XLI_SCM_VALIFFY) val_iffy = 1;
	}

	if (key_nodup)
	{
		qse_xli_atom_t* atom;

		/* find any key conflicts in the current scope */
		/* TODO: optimization. no sequential search */
		atom = parlist->tail;
		while (atom)
		{
			if (atom->type == QSE_XLI_PAIR &&
			    qse_strcmp (((qse_xli_pair_t*)atom)->key, QSE_STR_PTR(xli->tok.name)) == 0)
			{
				qse_xli_seterror (xli, QSE_XLI_EEXIST, QSE_STR_XSTR(xli->tok.name), &xli->tok.loc);
				goto oops;
			}

			atom = atom->prev;
		}
	}

	/* once the key name is read, enable the numeric string for a value */
	xli->tok_status |= TOK_STATUS_ENABLE_NSTR;

	if (get_token (xli) <= -1) goto oops;

	if (key_alias)
	{
		/* the alias part must be unique for the same key(s) */
		if (MATCH (xli, TOK_IDENT) || MATCH (xli, TOK_DQSTR) || MATCH (xli, TOK_SQSTR) || MATCH(xli, TOK_NSTR))
		{
			qse_xli_atom_t* atom;

			atom = parlist->tail;
			while (atom)
			{
				if (atom->type == QSE_XLI_PAIR &&
				    ((qse_xli_pair_t*)atom)->alias && 
				    qse_strcmp (((qse_xli_pair_t*)atom)->key, key.ptr) == 0 &&
				    qse_strcmp (((qse_xli_pair_t*)atom)->alias, QSE_STR_PTR(xli->tok.name)) == 0)
				{
					qse_xli_seterror (xli, QSE_XLI_EEXIST, QSE_STR_XSTR(xli->tok.name), &xli->tok.loc);
					goto oops;
				}
				atom = atom->prev;
			}

			name = qse_strdup (QSE_STR_PTR(xli->tok.name), xli->mmgr);
			if (name == QSE_NULL) 
			{
				qse_xli_seterrnum (xli, QSE_XLI_ENOMEM, QSE_NULL); 
				goto oops;
			}

			if (get_token (xli) <= -1) goto oops;
		}
		else if (key_alias == 2)
		{
			/* SCM_KEYALIAS is specified for this particular item. Let the alias be required. 
			 * If KEYALIAS is globally specified with the specific one, it's optional. */
			qse_xli_seterrnum (xli, QSE_XLI_ENOALI, (const qse_cstr_t*)&key); 
			goto oops;
		}
	}

	if (MATCH (xli, TOK_EQ))
	{
		if (get_token (xli) <= -1) goto oops;

		if ((xli->opt.trait & QSE_XLI_STRTAG) && MATCH (xli, TOK_TAG))
		{
			strtag = qse_strxdup (QSE_STR_PTR(xli->tok.name), QSE_STR_LEN(xli->tok.name), xli->mmgr);
			if (strtag == QSE_NULL)
			{
				qse_xli_seterrnum (xli, QSE_XLI_ENOMEM, QSE_NULL); 
				goto oops;
			}

			if (get_token (xli) <= -1) goto oops;
		}

		if (MATCH (xli, TOK_SQSTR) || MATCH (xli, TOK_DQSTR) || MATCH(xli, TOK_NSTR) || MATCH (xli, TOK_IDENT))
		{
			qse_xli_str_t* curstrseg;
			qse_size_t segcount = 0;

			if (scm && !(scm->flags & QSE_XLI_SCM_VALSTR))
			{
				/* check the value type */
				qse_xli_seterror (xli, QSE_XLI_EILVAL, (const qse_cstr_t*)&key, &kloc);
				goto oops;
			}

			/* add a new pair with the initial string segment */
			pair = qse_xli_insertpairwithstr (xli, parlist, QSE_NULL, key.ptr, name, keytag, QSE_STR_XSTR(xli->tok.name), strtag);
			if (pair == QSE_NULL) goto oops;

			segcount++;
			curstrseg = (qse_xli_str_t*)pair->val;

			if (get_token (xli) <= -1) goto oops;
			if ((xli->opt.trait & QSE_XLI_MULSEGSTR) && MATCH(xli, TOK_COMMA))
			{
				/* multi-segmented string */
				do
				{
					if (get_token (xli) <= -1) goto oops; /* skip the comma */

					if (strtag) 
					{
						QSE_MMGR_FREE (xli->mmgr, strtag);
						strtag = QSE_NULL;
					}

					if ((xli->opt.trait & QSE_XLI_STRTAG) && MATCH (xli, TOK_TAG))
					{
						strtag = qse_strxdup (QSE_STR_PTR(xli->tok.name), QSE_STR_LEN(xli->tok.name), xli->mmgr);
						if (strtag == QSE_NULL)
						{
							qse_xli_seterrnum (xli, QSE_XLI_ENOMEM, QSE_NULL); 
							goto oops;
						}
						
						if (get_token (xli) <= -1) goto oops; 
					}

					if (!MATCH (xli, TOK_SQSTR) && !MATCH (xli, TOK_DQSTR) && !MATCH (xli, TOK_NSTR) && !MATCH (xli, TOK_IDENT))
					{
						qse_xli_seterror (xli, QSE_XLI_ESYNTAX, QSE_NULL, &xli->tok.loc);
						goto oops;
					}

					/* add an additional segment to the string */
					curstrseg = qse_xli_addsegtostr (xli, curstrseg, strtag, QSE_STR_XSTR(xli->tok.name));
					if (curstrseg == QSE_NULL) goto oops;

					segcount++;
					if (get_token (xli) <= -1) goto oops; /* skip the value */
				}
				while (MATCH (xli, TOK_COMMA));
			}

			/* semicolon is mandatory for a string */
			if (!MATCH (xli, TOK_SEMICOLON))
			{
				qse_xli_seterror (xli, QSE_XLI_ESCOLON, QSE_STR_XSTR(xli->tok.name), &xli->tok.loc);
				goto oops;
			}

			if (scm && (segcount < scm->str_minseg || segcount > scm->str_maxseg))
			{
				/* too many string segments for the key */
				qse_xli_seterror (xli, QSE_XLI_ESTRSEG, (const qse_cstr_t*)&key, &kloc);
				goto oops;
			}

			/* semicolon read. turn off NSTR */
			xli->tok_status &= ~TOK_STATUS_ENABLE_NSTR;
			if (get_token (xli) <= -1) goto oops;
		}
		else
		{
			qse_xli_seterror (xli, QSE_XLI_EPAVAL, QSE_STR_XSTR(xli->tok.name), &xli->tok.loc);
			goto oops;
		}


		/* TODO: check against schema */

	}
	else if (!(xli->opt.trait & QSE_XLI_NOLIST) && MATCH (xli, TOK_LBRACE))
	{
		if (scm && !(scm->flags & QSE_XLI_SCM_VALLIST))
		{
			/* check the value type */
			qse_xli_seterror (xli, QSE_XLI_EILVAL, (const qse_cstr_t*)&key, &kloc);
			goto oops;
		}

		xli->tok_status &= ~TOK_STATUS_ENABLE_NSTR;

		/* insert a pair with an empty list */
		pair = qse_xli_insertpairwithemptylist (xli, parlist, QSE_NULL, key.ptr, name, keytag);
		if (pair == QSE_NULL) goto oops;

		/* skip validations of child pairs if the schema for the 
		 * current pair is set with QSE_XLI_SCM_VALIFFY. 
		 * the schema for the child pairs, if specified, must not 
		 * take effect. */
		if (read_list (xli, (qse_xli_list_t*)pair->val, (val_iffy? &scm_val_iffy: QSE_NULL)) <= -1) goto oops;
		
		if (!MATCH (xli, TOK_RBRACE))
		{
			qse_xli_seterror (xli, QSE_XLI_ERBRCE, QSE_STR_XSTR(xli->tok.name), &xli->tok.loc);
			goto oops;
		}

		if (get_token (xli) <= -1) goto oops;

		/* semicolon is optional for a list */
		if (MATCH (xli, TOK_SEMICOLON))
		{
			/* skip the semicolon */
			if (get_token (xli) <= -1) goto oops;
		}

		/* TODO: check against schema */
	}
	else if (MATCH (xli, TOK_SEMICOLON))
	{
		if (xli->opt.trait & QSE_XLI_NONIL) 
		{
			qse_xli_seterror (xli, QSE_XLI_ENOVAL, (const qse_cstr_t*)&key, &kloc);
			goto oops;
		}

		if (scm && !(scm->flags & QSE_XLI_SCM_VALNIL) &&
		           !((scm->flags & QSE_XLI_SCM_VALSTR) && scm->str_minseg <= 0))
		{
			/* check the value type */
			qse_xli_seterror (xli, QSE_XLI_ENOVAL, (const qse_cstr_t*)&key, &kloc);
			goto oops;
		}

		xli->tok_status &= ~TOK_STATUS_ENABLE_NSTR;

		/* no value has been specified for the pair */
		pair = qse_xli_insertpair (xli, parlist, QSE_NULL, key.ptr, name, keytag, (qse_xli_val_t*)&xli->root->xnil);
		if (pair == QSE_NULL) goto oops;

		/* skip the semicolon */
		if (get_token (xli) <= -1) goto oops;

		/* TODO: check against schema */
	}
	else
	{
		qse_xli_seterror (xli, QSE_XLI_ELBREQ, QSE_STR_XSTR(xli->tok.name), &xli->tok.loc);
		goto oops;
	}

	if (strtag) QSE_MMGR_FREE (xli->mmgr, strtag);
	QSE_MMGR_FREE (xli->mmgr, name);
	QSE_MMGR_FREE (xli->mmgr, key.ptr);
	qse_str_setlen (xli->dotted_curkey, dotted_curkey_len);
	return 0;
	
oops:
	xli->tok_status &= ~TOK_STATUS_ENABLE_NSTR;
	if (strtag) QSE_MMGR_FREE (xli->mmgr, strtag);
	if (name) QSE_MMGR_FREE (xli->mmgr, name);
	if (key.ptr) QSE_MMGR_FREE (xli->mmgr, key.ptr);
	if (dotted_curkey_len != (qse_size_t)-1)
		qse_str_setlen (xli->dotted_curkey, dotted_curkey_len);
	return -1;
}

static qse_xli_list_link_t* make_list_link (qse_xli_t* xli, qse_xli_list_t* parlist)
{
	qse_xli_list_link_t* link;

	link = (qse_xli_list_link_t*) qse_xli_callocmem (xli, QSE_SIZEOF(*link));
	if (link == QSE_NULL) return QSE_NULL;

	link->list = parlist;
	link->next = xli->parlink;
	xli->parlink = link;

	return link;
}

static void free_list_link (qse_xli_t* xli, qse_xli_list_link_t* link)
{
	xli->parlink = link->next;
	qse_xli_freemem (xli, link);
}

static int __read_list (qse_xli_t* xli, const qse_xli_scm_t* override)
{
	while (1)
	{
		if (MATCH (xli, TOK_XINCLUDE))
		{
			if (get_token(xli) <= -1) return -1;

			if (!MATCH(xli,TOK_SQSTR) && !MATCH(xli,TOK_DQSTR))
			{
				qse_xli_seterror (xli, QSE_XLI_EINCLSTR, QSE_NULL, &xli->tok.loc);
				return -1;
			}

			if (begin_include (xli) <= -1) return -1;
		}
		else if ((xli->opt.trait & QSE_XLI_KEYTAG) && MATCH (xli, TOK_TAG))
		{
			qse_char_t* keytag;
			int x;

			keytag = qse_strxdup (QSE_STR_PTR(xli->tok.name), QSE_STR_LEN(xli->tok.name), xli->mmgr);
			if (keytag == QSE_NULL)
			{
				qse_xli_seterrnum (xli, QSE_XLI_ENOMEM, QSE_NULL); 
				return -1;
			}

			if (get_token(xli) <= -1) 
			{
				QSE_MMGR_FREE (xli->mmgr, keytag);
				return -1;
			}

			if (!MATCH(xli,TOK_IDENT))
			{
				QSE_MMGR_FREE (xli->mmgr, keytag);
				qse_xli_seterror (xli, QSE_XLI_ENOKEY, QSE_NULL, &xli->tok.loc);
				return -1;
			}

			x = read_pair (xli, keytag, override);
			QSE_MMGR_FREE (xli->mmgr, keytag);
			if (x <= -1) return -1;
		}
		else if (MATCH (xli, TOK_IDENT))
		{
			if (read_pair (xli, QSE_NULL, override) <= -1) return -1;
		}
		else if (MATCH (xli, TOK_TEXT))
		{
			if (get_token(xli) <= -1) return -1;
		}
		else 
		{
			break;
		}
	}

	return 0;
}

static int read_list (qse_xli_t* xli, qse_xli_list_t* parlist, const qse_xli_scm_t* override)
{
	qse_xli_list_link_t* link;

	link = make_list_link (xli, parlist);
	if (link == QSE_NULL) return -1;

	/* get_token() here is to read the token after the left brace.
	 * it must be called after the xli->parlink has been updated
	 * in case there are comments at the beginning of the list */
	if (get_token (xli) <= -1 || __read_list (xli, override) <= -1) 
	{
		free_list_link (xli, link);
		return -1;
	}

	QSE_ASSERT (link == xli->parlink);
	free_list_link (xli, link);

	return 0;
}

static int read_root_list (qse_xli_t* xli)
{
	qse_xli_list_link_t* link;

	link = make_list_link (xli, &xli->root->list);
	if (link == QSE_NULL) return -1;

	if (get_char (xli) <= -1 || get_token (xli) <= -1 || __read_list (xli, QSE_NULL) <= -1)
	{
		free_list_link (xli, link);
		return -1;
	}

	QSE_ASSERT (link == xli->parlink);
	free_list_link (xli, link);

	return 0;
}

void qse_xli_clearrionames (qse_xli_t* xli)
{
	qse_link_t* cur;
	while (xli->rio_names)
	{
		cur = xli->rio_names;
		xli->rio_names = cur->link;
		QSE_MMGR_FREE (xli->mmgr, cur);
	}
}

int qse_xli_read (qse_xli_t* xli, qse_xli_io_impl_t io)
{
	qse_ssize_t n;

	if (io == QSE_NULL)
	{
		qse_xli_seterrnum (xli, QSE_XLI_EINVAL, QSE_NULL); 
		return -1;
	}

	QSE_MEMSET (&xli->rio, 0, QSE_SIZEOF(xli->rio));
	xli->rio.impl = io;
	xli->rio.top.line = 1;
	xli->rio.top.colm = 1;
	xli->rio.inp = &xli->rio.top;

	qse_xli_seterrnum (xli, QSE_XLI_ENOERR, QSE_NULL); 
	qse_xli_clearrionames (xli);

	QSE_ASSERT (QSE_STR_LEN(xli->dotted_curkey) == 0);

	n = xli->rio.impl (xli, QSE_XLI_IO_OPEN, xli->rio.inp, QSE_NULL, 0);
	if (n <= -1)
	{
		if (xli->errnum == QSE_XLI_ENOERR)
			qse_xli_seterrnum (xli, QSE_XLI_EIOUSR, QSE_NULL); 
		return -1;
	}
	/* the input stream is open now */

	if (read_root_list (xli) <= -1) goto oops;

	QSE_ASSERT (xli->parlink == QSE_NULL);

	if (!MATCH (xli, TOK_EOF))
	{
		qse_xli_seterror (xli, QSE_XLI_ESYNTAX, QSE_NULL, &xli->tok.loc);
		goto oops;
	}

	QSE_ASSERT (xli->rio.inp == &xli->rio.top);
	close_current_stream (xli);
	qse_str_clear (xli->tok.name);
	return 0;

oops:
	/* an error occurred and control has reached here
	 * probably, some included files might not have been 
	 * closed. close them */
	while (xli->rio.inp != &xli->rio.top)
	{
		qse_xli_io_arg_t* prev;

		/* nothing much to do about a close error */
		close_current_stream (xli);

		prev = xli->rio.inp->prev;
		QSE_ASSERT (xli->rio.inp->name != QSE_NULL);
		QSE_MMGR_FREE (xli->mmgr, xli->rio.inp);
		xli->rio.inp = prev;
	}
	
	close_current_stream (xli);
	qse_str_clear (xli->tok.name);
	return -1;
}
