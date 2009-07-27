/*
 * $Id: class.c,v 1.27 2005-10-02 15:45:09 bacon Exp $
 */

#include <xp/stx/class.h>
#include <xp/stx/symbol.h>
#include <xp/stx/object.h>
#include <xp/stx/dict.h>
#include <xp/stx/misc.h>

xp_word_t xp_stx_new_class (xp_stx_t* stx, const xp_char_t* name)
{
	xp_word_t meta, class;
	xp_word_t class_name;

	meta = xp_stx_alloc_word_object (
		stx, XP_NULL, XP_STX_METACLASS_SIZE, XP_NULL, 0);
	XP_STX_CLASS(stx,meta) = stx->class_metaclass;
	/* the spec of the metaclass must be the spec of its
	 * instance. so the XP_STX_CLASS_SIZE is set */
	XP_STX_WORD_AT(stx,meta,XP_STX_METACLASS_SPEC) = 
		XP_STX_TO_SMALLINT((XP_STX_CLASS_SIZE << XP_STX_SPEC_INDEXABLE_BITS) | XP_STX_SPEC_NOT_INDEXABLE);
	
	/* the spec of the class is set later in __create_builtin_classes */
	class = xp_stx_alloc_word_object (
		stx, XP_NULL, XP_STX_CLASS_SIZE, XP_NULL, 0);
	XP_STX_CLASS(stx,class) = meta;
	class_name = xp_stx_new_symbol (stx, name);
	XP_STX_WORD_AT(stx,class,XP_STX_CLASS_NAME) = class_name;

	xp_stx_dict_put (stx, stx->smalltalk, class_name, class);
	return class;
}

xp_word_t xp_stx_lookup_class (xp_stx_t* stx, const xp_char_t* name)
{
	xp_word_t assoc, meta, value;

	assoc = xp_stx_dict_lookup (stx, stx->smalltalk, name);
	if (assoc == stx->nil) {
		return stx->nil;
	}

	value = XP_STX_WORD_AT(stx,assoc,XP_STX_ASSOCIATION_VALUE);
	meta = XP_STX_CLASS(stx,value);
	if (XP_STX_CLASS(stx,meta) != stx->class_metaclass) return stx->nil;

	return value;
}

int xp_stx_get_instance_variable_index (
	xp_stx_t* stx, xp_word_t class_index, 
	const xp_char_t* name, xp_word_t* index)
{
	xp_word_t index_super = 0;
	xp_stx_class_t* class_obj;
	xp_stx_char_object_t* string;

	class_obj = (xp_stx_class_t*)XP_STX_OBJECT(stx, class_index);
	xp_assert (class_obj != XP_NULL);

	if (class_obj->superclass != stx->nil) {
		if (xp_stx_get_instance_variable_index (
			stx, class_obj->superclass, name, &index_super) == 0) {
			*index = index_super;
			return 0;
		}
	}

	if (class_obj->header.class == stx->class_metaclass) {
		/* metaclass */
		/* TODO: can a metaclas have instance variables? */	
		*index = index_super;
	}
	else {
		if (class_obj->variables == stx->nil) *index = 0;
		else {
			string = XP_STX_CHAR_OBJECT(stx, class_obj->variables);
			if (xp_stx_strword(string->data, name, index) != XP_NULL) {
				*index += index_super;
				return 0;
			}
		}

		*index += index_super;
	}

	return -1;
}

xp_word_t xp_stx_lookup_class_variable (
	xp_stx_t* stx, xp_word_t class_index, const xp_char_t* name)
{
	xp_stx_class_t* class_obj;

	class_obj = (xp_stx_class_t*)XP_STX_OBJECT(stx, class_index);
	xp_assert (class_obj != XP_NULL);

	if (class_obj->superclass != stx->nil) {
		xp_word_t tmp;
		tmp = xp_stx_lookup_class_variable (
			stx, class_obj->superclass, name);
		if (tmp != stx->nil) return tmp;
	}

	/* TODO: can a metaclas have class variables? */	
	if (class_obj->header.class != stx->class_metaclass &&
	    class_obj->class_variables != stx->nil) {
		if (xp_stx_dict_lookup(stx,
			class_obj->class_variables,name) != stx->nil) return class_index;
	}

	return stx->nil;
}

xp_word_t xp_stx_lookup_method (xp_stx_t* stx, 
	xp_word_t class_index, const xp_char_t* name, xp_bool_t from_super)
{
	xp_stx_class_t* class_obj;

	class_obj = (xp_stx_class_t*)XP_STX_OBJECT(stx, class_index);
	xp_assert (class_obj != XP_NULL);

#if 0
	if (class_obj->header.class != stx->class_metaclass &&
	    class_obj->methods != stx->nil) {
		xp_word_t assoc;
		assoc = xp_stx_dict_lookup(stx, class_obj->methods, name);
		if (assoc != stx->nil) {
			xp_assert (XP_STX_CLASS(stx,assoc) == stx->class_association);
			return XP_STX_WORD_AT(stx, assoc, XP_STX_ASSOCIATION_VALUE);
		}
	}

	if (class_obj->superclass != stx->nil) {
		xp_word_t tmp;
		tmp = xp_stx_lookup_method (
			stx, class_obj->superclass, name);
		if (tmp != stx->nil) return tmp;
	}
#endif

	while (class_index != stx->nil) {
		class_obj = (xp_stx_class_t*)XP_STX_OBJECT(stx, class_index);

		xp_assert (class_obj != XP_NULL);
		xp_assert (
			class_obj->header.class == stx->class_metaclass ||
			XP_STX_CLASS(stx,class_obj->header.class) == stx->class_metaclass);

		if (from_super) {	
			from_super = xp_false;
		}
		else if (class_obj->methods != stx->nil) {
			xp_word_t assoc;
			assoc = xp_stx_dict_lookup(stx, class_obj->methods, name);
			if (assoc != stx->nil) {
				xp_assert (XP_STX_CLASS(stx,assoc) == stx->class_association);
				return XP_STX_WORD_AT(stx, assoc, XP_STX_ASSOCIATION_VALUE);
			}
		}

		class_index = class_obj->superclass;
	}

	return stx->nil;
}
