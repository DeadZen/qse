/*
 * $Id: stx.c,v 1.12 2005-05-12 15:33:38 bacon Exp $
 */

#include <xp/stx/stx.h>
#include <xp/stx/memory.h>
#include <xp/stx/object.h>
#include <xp/stx/hash.h>
#include <xp/bas/memory.h>
#include <xp/bas/assert.h>

xp_stx_t* xp_stx_open (xp_stx_t* stx, xp_stx_word_t capacity)
{
	if (stx == XP_NULL) {
		stx = (xp_stx_t*) xp_malloc (xp_sizeof(stx));
		if (stx == XP_NULL) return XP_NULL;
		stx->__malloced = xp_true;
	}
	else stx->__malloced = xp_false;

	if (xp_stx_memory_open (&stx->memory, capacity) == XP_NULL) {
		if (stx->__malloced) xp_free (stx);
		return XP_NULL;
	}

	stx->nil = XP_STX_NIL;
	stx->true = XP_STX_TRUE;
	stx->false = XP_STX_FALSE;

	stx->symbol_table = XP_STX_NIL;
	stx->class_symbol = XP_STX_NIL;
	stx->class_metaclass = XP_STX_NIL;
	stx->class_symbol_link = XP_STX_NIL;

	return stx;
}

void xp_stx_close (xp_stx_t* stx)
{
	xp_stx_memory_close (&stx->memory);
	if (stx->__malloced) xp_free (stx);
}

static void __reset_symbol_link_class (xp_stx_t* stx, xp_stx_word_t idx)
{
	XP_STX_CLASS(stx,idx) = stx->class_symbol_link;
}

int xp_stx_bootstrap (xp_stx_t* stx)
{
	xp_stx_word_t symtab, symbol_Smalltalk;
	xp_stx_word_t symbol_nil, symbol_true, symbol_false;
	xp_stx_word_t symbol_Symbol, symbol_SymbolMeta;
	xp_stx_word_t symbol_Metaclass, symbol_MetaclassMeta;
	xp_stx_word_t class_Symbol, class_SymbolMeta;
	xp_stx_word_t class_Metaclass, class_MetaclassMeta;
	xp_stx_word_t class_Object, class_Class;
	xp_stx_word_t tmp;

	/* allocate three keyword objects */
	stx->nil = xp_stx_alloc_object (stx, 0);
	stx->true = xp_stx_alloc_object (stx, 0);
	stx->false = xp_stx_alloc_object (stx, 0);

	xp_assert (stx->nil == XP_STX_NIL);
	xp_assert (stx->true == XP_STX_TRUE);
	xp_assert (stx->false == XP_STX_FALSE);

	/* build a symbol table */   // TODO: symbol table size
	symtab = xp_stx_alloc_object (stx, 1000); 

	/* tweak the initial object structure */
	symbol_Symbol = 
		xp_stx_alloc_string_object(stx, XP_STX_TEXT("Symbol"));
	symbol_SymbolMeta = 
		xp_stx_alloc_string_object(stx,XP_STX_TEXT("Symbol class"));
	symbol_Metaclass = 
		xp_stx_alloc_string_object(stx, XP_STX_TEXT("Metaclass"));
	symbol_MetaclassMeta = 
		xp_stx_alloc_string_object(stx, XP_STX_TEXT("Metaclass class"));

	class_Metaclass = xp_stx_alloc_object(stx, XP_STX_CLASS_SIZE);
	class_MetaclassMeta = xp_stx_alloc_object(stx, XP_STX_CLASS_SIZE);
	class_Symbol = xp_stx_alloc_object(stx, XP_STX_CLASS_SIZE);
	class_SymbolMeta = xp_stx_alloc_object(stx, XP_STX_CLASS_SIZE);

	XP_STX_CLASS(stx,symbol_SymbolMeta) = class_Symbol;
	XP_STX_CLASS(stx,symbol_Metaclass) = class_Symbol;
	XP_STX_CLASS(stx,symbol_MetaclassMeta) = class_Symbol;

	XP_STX_CLASS(stx,class_Symbol) = class_SymbolMeta;
	XP_STX_CLASS(stx,class_SymbolMeta) = class_Metaclass;
	XP_STX_CLASS(stx,class_Metaclass) = class_MetaclassMeta;
	XP_STX_CLASS(stx,class_MetaclassMeta) = class_Metaclass;

	xp_stx_hash_insert (stx, symtab,
		xp_stx_hash_string_object(stx, symbol_Symbol),
		symbol_Symbol, class_Symbol);
	xp_stx_hash_insert (stx, symtab,
		xp_stx_hash_string_object(stx, symbol_SymbolMeta),
		symbol_SymbolMeta, class_SymbolMeta);
	xp_stx_hash_insert (stx, symtab,
		xp_stx_hash_string_object(stx, symbol_Metaclass),
		symbol_Metaclass, class_Metaclass);
	xp_stx_hash_insert (stx, symtab,
		xp_stx_hash_string_object(stx, symbol_MetaclassMeta),
		symbol_MetaclassMeta, class_MetaclassMeta);

	/* now ready to use new_symbol & new_class */
	stx->symbol_table = symtab;
	stx->class_symbol = class_Symbol;
	stx->class_metaclass = class_Metaclass;

	/* more initialization for symbol table */
	stx->class_symbol_link = 
		xp_stx_new_class (stx, XP_STX_TEXT("SymbolLink"));

	xp_stx_hash_traverse (stx, symtab, __reset_symbol_link_class);
	XP_STX_CLASS(stx,symtab) = 
		xp_stx_new_class (stx, XP_STX_TEXT("Array"));
	symbol_Smalltalk = 
		xp_stx_new_symbol (stx, XP_STX_TEXT("Smalltalk"));
	xp_stx_hash_insert (stx, symtab,
		xp_stx_hash_string_object(stx, symbol_Smalltalk),
		symbol_Smalltalk, symtab);	

	/* more initialization for nil, true, false */
	symbol_nil = xp_stx_new_symbol (stx, XP_STX_TEXT("nil"));
	symbol_true = xp_stx_new_symbol (stx, XP_STX_TEXT("true"));
	symbol_false = xp_stx_new_symbol (stx, XP_STX_TEXT("false"));

	xp_stx_hash_insert (stx, symtab,
		xp_stx_hash_string_object(stx, symbol_nil),
		symbol_nil, stx->nil);
	xp_stx_hash_insert (stx, symtab,
		xp_stx_hash_string_object(stx, symbol_true),
		symbol_true, stx->true);
	xp_stx_hash_insert (stx, symtab,
		xp_stx_hash_string_object(stx, symbol_false),
		symbol_false, stx->false);

	XP_STX_CLASS(stx,stx->nil) =
		xp_stx_new_class (stx, XP_STX_TEXT("UndefinedObject"));
	XP_STX_CLASS(stx,stx->true) =
		xp_stx_new_class (stx, XP_STX_TEXT("True"));
	XP_STX_CLASS(stx,stx->false) = 
		xp_stx_new_class (stx, XP_STX_TEXT("False"));

	/* weave the class-metaclass chain */
	class_Object = xp_stx_new_class (stx, XP_STX_TEXT("Object"));
	class_Class = xp_stx_new_class (stx, XP_STX_TEXT("Class"));
	tmp = XP_STX_CLASS(stx,class_Object);
	XP_STX_AT(stx,tmp,XP_STX_CLASS_SUPERCLASS) = class_Class;

	return 0;
}

