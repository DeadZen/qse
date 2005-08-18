/*
 * $Id: stx.h,v 1.42 2005-08-18 15:16:39 bacon Exp $
 */

#ifndef _XP_STX_STX_H_
#define _XP_STX_STX_H_

#include <xp/types.h>
#include <xp/macros.h>

typedef struct xp_stx_objhdr_t xp_stx_objhdr_t;
typedef struct xp_stx_object_t xp_stx_object_t;
typedef struct xp_stx_word_object_t xp_stx_word_object_t;
typedef struct xp_stx_byte_object_t xp_stx_byte_object_t;
typedef struct xp_stx_char_object_t xp_stx_char_object_t;
typedef struct xp_stx_t xp_stx_t;

/* common object structure */
struct xp_stx_objhdr_t
{
	/* access - type: 2; size: rest;
	 * type - word indexed: 00 byte indexed: 01 char indexed: 10
	 */
	xp_word_t access; 
	xp_word_t class;
};

struct xp_stx_object_t
{
	xp_stx_objhdr_t header;
};

struct xp_stx_word_object_t
{
	xp_stx_objhdr_t header;
	xp_word_t data[1];
};

struct xp_stx_byte_object_t
{
	xp_stx_objhdr_t header;
	xp_byte_t data[1];
};

struct xp_stx_char_object_t
{
	xp_stx_objhdr_t header;
	xp_char_t data[1];
};

#define XP_STX_IS_SMALLINT(x)   (((x) & 0x01) == 0x01)
#define XP_STX_TO_SMALLINT(x)   (((x) << 1) | 0x01)
#define XP_STX_FROM_SMALLINT(x) ((x) >> 1)

#define XP_STX_IS_OINDEX(x)     (((x) & 0x01) == 0x00)
#define XP_STX_TO_OINDEX(x)     (((x) << 1) | 0x00)
#define XP_STX_FROM_OINDEX(x)   ((x) >> 1)

#define XP_STX_NIL   XP_STX_TO_OINDEX(0)
#define XP_STX_TRUE  XP_STX_TO_OINDEX(1)
#define XP_STX_FALSE XP_STX_TO_OINDEX(2)

#define XP_STX_OBJECT(stx,idx) (((stx)->memory).slots[XP_STX_FROM_OINDEX(idx)])
#define XP_STX_CLASS(stx,idx)  (XP_STX_OBJECT(stx,(idx))->header.class)
#define XP_STX_ACCESS(stx,idx) (XP_STX_OBJECT(stx,(idx))->header.access)
#define XP_STX_DATA(stx,idx)   ((void*)(XP_STX_OBJECT(stx,idx) + 1))

#define XP_STX_TYPE(stx,idx) (XP_STX_ACCESS(stx,idx) & 0x03)
#define XP_STX_SIZE(stx,idx) (XP_STX_ACCESS(stx,idx) >> 0x02)

#define XP_STX_WORD_INDEXED  (0x00)
#define XP_STX_BYTE_INDEXED  (0x01)
#define XP_STX_CHAR_INDEXED  (0x02)

#define XP_STX_IS_WORD_OBJECT(stx,idx) \
	(XP_STX_TYPE(stx,idx) == XP_STX_WORD_INDEXED)
#define XP_STX_IS_BYTE_OBJECT(stx,idx) \
	(XP_STX_TYPE(stx,idx) == XP_STX_BYTE_INDEXED)
#define XP_STX_IS_CHAR_OBJECT(stx,idx) \
	(XP_STX_TYPE(stx,idx) == XP_STX_CHAR_INDEXED)

#define XP_STX_WORD_OBJECT(stx,idx) \
	((xp_stx_word_object_t*)XP_STX_OBJECT(stx,idx))
#define XP_STX_BYTE_OBJECT(stx,idx) \
	((xp_stx_byte_object_t*)XP_STX_OBJECT(stx,idx))
#define XP_STX_CHAR_OBJECT(stx,idx) \
	((xp_stx_char_object_t*)XP_STX_OBJECT(stx,idx))

#define XP_STX_WORD_AT(stx,idx,n) \
	(((xp_word_t*)(XP_STX_OBJECT(stx,idx) + 1))[n])
#define XP_STX_BYTE_AT(stx,idx,n) \
	(((xp_byte_t*)(XP_STX_OBJECT(stx,idx) + 1))[n])
#define XP_STX_CHAR_AT(stx,idx,n) \
	(((xp_char_t*)(XP_STX_OBJECT(stx,idx) + 1))[n])

#ifdef __cplusplus
extern "C" {
#endif

xp_stx_t* xp_stx_open (xp_stx_t* stx, xp_word_t capacity);
void xp_stx_close (xp_stx_t* stx);

#ifdef __cplusplus
}
#endif

#endif
