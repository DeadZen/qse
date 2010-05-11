/*
 * $Id: map.h 327 2010-05-10 13:15:55Z hyunghwan.chung $
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

#ifndef _QSE_CMN_MAP_H_
#define _QSE_CMN_MAP_H_

#include <qse/types.h>
#include <qse/macros.h>

/**@file
 * A hash map maintains buckets for key/value pairs with the same key hash
 * chained under the same bucket.
 */

/* values that can be returned by qse_map_walker_t */
enum qse_map_walk_t
{
        QSE_MAP_WALK_STOP = 0,
        QSE_MAP_WALK_FORWARD = 1
};

enum qse_map_id_t
{
	QSE_MAP_KEY = 0,
	QSE_MAP_VAL = 1
};

typedef struct qse_map_t qse_map_t;
typedef struct qse_map_pair_t qse_map_pair_t;
typedef enum qse_map_walk_t qse_map_walk_t;
typedef enum qse_map_id_t   qse_map_id_t;

/**
 * The qse_map_copier_t type defines a pair contruction callback
 */
typedef void* (*qse_map_copier_t) (
	qse_map_t* map  /* a map */,
	void*      dptr /* the pointer to a key or a value */, 
	qse_size_t dlen /* the length of a key or a value */
);

/**
 * The qse_map_freeer_t defines a key/value destruction callback
 */
typedef void (*qse_map_freeer_t) (
	qse_map_t* map  /* a map */,
	void*      dptr /* the pointer to a key or a value */, 
	qse_size_t dlen /* the length of a key or a value */
);

/* key hasher */
typedef qse_size_t (*qse_map_hasher_t) (
	qse_map_t* map   /* a map */, 
	const void* kptr /* the pointer to a key */, 
	qse_size_t klen  /* the length of a key in bytes */
);

/**
 * The qse_map_comper_t type defines a key comparator that is called when
 * the map needs to compare keys. A map is created with a default comparator
 * which performs bitwise comparison between two keys.
 *
 * The comparator should return 0 if the keys are the same and a non-zero
 * integer otherwise.
 */
typedef int (*qse_map_comper_t) (
	qse_map_t*  map,    /**< a map */ 
	const void* kptr1,  /**< the pointer to a key */
	qse_size_t  klen1,  /**< the length of a key */ 
	const void* kptr2,  /**< the pointer to a key */ 
	qse_size_t  klen2   /**< the length of a key */
);

/**
 * The qse_map_keeper_t type defines a value keeper that is called when 
 * a value is retained in the context that it should be destroyed because
 * it is identical to a new value. Two values are identical if their beginning
 * pointers and their lengths are equal.
 */
typedef void (*qse_map_keeper_t) (
	qse_map_t* map,    /**< a map */
	void*      vptr,   /**< value pointer */
	qse_size_t vlen    /**< value length */
);

/**
 * The qse_map_sizer_T type defines a bucket size claculator that is called
 * when a map should resize the bucket. The current bucket size +1 is passed
 * as the hint.
 */
typedef qse_size_t (*qse_map_sizer_t) (
	qse_map_t* map,  /**< a map */
	qse_size_t hint  /**< a sizing hint */
);

/**
 * The qse_map_walker_t defines a pair visitor.
 */
typedef qse_map_walk_t (*qse_map_walker_t) (
	qse_map_t*      map,   /**< a map */
	qse_map_pair_t* pair,  /**< the pointer to a key/value pair */
	void*           arg    /**< the pointer to user-defined data */
);

/**
 * The qse_map_pair_t type defines a map pair. A pair is composed of a key
 * and a value. It maintains pointers to the beginning of a key and a value
 * plus their length. The length is scaled down with the scale factor 
 * specified in an owning map. Use macros defined in the 
 */
struct qse_map_pair_t
{
	void*           kptr;  /**< the pointer to a key */
	qse_size_t      klen;  /**< the length of a key */
	void*           vptr;  /**< the pointer to a value */
	qse_size_t      vlen;  /**< the length of a value */
	qse_map_pair_t* next;  /**< the next pair under the same slot */
};

/**
 * The qse_map_t type defines a hash map.
 */
struct qse_map_t
{
	QSE_DEFINE_COMMON_FIELDS (map)

        qse_map_copier_t copier[2];
        qse_map_freeer_t freeer[2];
	qse_map_hasher_t hasher;   /* key hasher */
	qse_map_comper_t comper;   /* key comparator */
	qse_map_keeper_t keeper;   /* value keeper */
	qse_map_sizer_t  sizer;    /* bucket capacity recalculator */
	qse_byte_t       scale[2]; /* length scale */
	qse_byte_t       factor;   /* load factor */
	qse_byte_t       filler0;
	qse_size_t       size;
	qse_size_t       capa;
	qse_size_t       threshold;
	qse_map_pair_t** bucket;
};

#define QSE_MAP_COPIER_SIMPLE ((qse_map_copier_t)1)
#define QSE_MAP_COPIER_INLINE ((qse_map_copier_t)2)

/****m* Common/QSE_MAP_SIZE
 * NAME
 *  QSE_MAP_SIZE - get the number of pairs
 * DESCRIPTION
 *  The QSE_MAP_SIZE() macro returns the number of pairs in a map.
 * SYNOPSIS
 */
#define QSE_MAP_SIZE(m) ((m)->size)
/*****/

/****m* Common/QSE_MAP_CAPA
 * NAME
 *  QSE_MAP_CAPA - get the capacity of a map
 *
 * DESCRIPTION
 *  The QSE_MAP_CAPA() macro returns the maximum number of pairs a map can hold.
 *
 * SYNOPSIS
 */
#define QSE_MAP_CAPA(m) ((m)->capa)
/*****/

#define QSE_MAP_KCOPIER(m)   ((m)->copier[QSE_MAP_KEY])
#define QSE_MAP_VCOPIER(m)   ((m)->copier[QSE_MAP_VAL])
#define QSE_MAP_KFREEER(m)   ((m)->freeer[QSE_MAP_KEY])
#define QSE_MAP_VFREEER(m)   ((m)->freeer[QSE_MAP_VAL])
#define QSE_MAP_HASHER(m)    ((m)->hasher)
#define QSE_MAP_COMPER(m)    ((m)->comper)
#define QSE_MAP_KEEPER(m)    ((m)->keeper)
#define QSE_MAP_SIZER(m)     ((m)->sizer)

#define QSE_MAP_FACTOR(m)    ((m)->factor)
#define QSE_MAP_KSCALE(m)    ((m)->scale[QSE_MAP_KEY])
#define QSE_MAP_VSCALE(m)    ((m)->scale[QSE_MAP_VAL])

#define QSE_MAP_KPTR(p) ((p)->kptr)
#define QSE_MAP_KLEN(p) ((p)->klen)
#define QSE_MAP_VPTR(p) ((p)->vptr)
#define QSE_MAP_VLEN(p) ((p)->vlen)
#define QSE_MAP_NEXT(p) ((p)->next)

#ifdef __cplusplus
extern "C" {
#endif

QSE_DEFINE_COMMON_FUNCTIONS (map)

/****f* Common/qse_map_open
 * NAME
 *  qse_map_open - creates a hash map
 * DESCRIPTION 
 *  The qse_map_open() function creates a hash map with a dynamic array 
 *  bucket and a list of values chained. The initial capacity should be larger
 *  than 0. The load factor should be between 0 and 100 inclusive and the load
 *  factor of 0 disables bucket resizing. If you need extra space associated
 *  with a map, you may pass a non-zero value as the second parameter. 
 *  The QSE_MAP_XTN() macro and the qse_map_getxtn() function return the 
 *  pointer to the beginning of the extension.
 * RETURN
 *  The qse_map_open() function returns an qse_map_t pointer on success and 
 *  QSE_NULL on failure.
 * SEE ALSO 
 *  QSE_MAP_XTN, qse_map_getxtn
 * SYNOPSIS
 */
qse_map_t* qse_map_open (
	qse_mmgr_t* mmgr   /* a memory manager */,
	qse_size_t  ext    /* extension size in bytes */,
	qse_size_t  capa   /* initial capacity */,
	int         factor /* load factor */
);
/******/


/****f* Common/qse_map_close 
 * NAME
 *  qse_map_close - destroy a hash map
 * DESCRIPTION 
 *  The qse_map_close() function destroys a hash map.
 * SYNOPSIS
 */
void qse_map_close (
	qse_map_t* map /* a map */
);
/******/

/****f* Common/qse_map_init
 * NAME
 *  qse_map_init - initialize a hash map
 * SYNOPSIS
 */
qse_map_t* qse_map_init (
	qse_map_t*  map,
	qse_mmgr_t* mmgr,
	qse_size_t  capa,
	int         factor
);
/******/

/****f* Common/qse_map_fini
 * NAME
 *  qse_map_fini - finalize a hash map
 * SYNOPSIS
 */
void qse_map_fini (
	qse_map_t* map
);
/******/

/****f* Common/qse_map_getsize
 * NAME
 *  qse_map_getsize  - get the number of pairs in a map
 * SYNOPSIS
 */
qse_size_t qse_map_getsize (
	qse_map_t* map
);
/******/

/****f* Common/qse_map_getcapa
 * NAME
 *  qse_map_getcapa  - get the number of slots in a hash bucket
 * SYNOPSIS
 */
qse_size_t qse_map_getcapa (
	qse_map_t* map
);
/******/

/****f* Common/qse_map_getscale
 * NAME
 *  qse_map_getscale - get the scale factor
 * PARAMETERS
 *  * id - QSE_MAP_KEY or QSE_MAP_VAL
 * SYNOPSIS
 */
int qse_map_getscale (
	qse_map_t*   map,
	qse_map_id_t id  /* QSE_MAP_KEY or QSE_MAP_VAL */
);
/******/

/****f* Common/qse_map_setscale
 * NAME
 *  qse_map_setscale - set the scale factor
 * DESCRIPTION 
 *  The qse_map_setscale() function sets the scale factor of the length
 *  of a key and a value. A scale factor determines the actual length of
 *  a key and a value in bytes. A map is created with a scale factor of 1.
 *  The scale factor should be larger than 0 and less than 256.
 * PARAMETERS
 *  * id - QSE_MAP_KEY or QSE_MAP_VAL
 *  * scale - a scale factor in bytes
 * NOTES
 *  It is a bad idea to change the scale factor when a map is not empty.
 * SYNOPSIS
 */
void qse_map_setscale (
	qse_map_t*   map,
	qse_map_id_t id,
	int          scale
);
/******/

/**
 * The qse_map_getcopier() function gets a data copier.
 */
qse_map_copier_t qse_map_getcopier (
	qse_map_t*   map,
	qse_map_id_t id /**< QSE_MAP_KEY or QSE_MAP_VAL */
);

/**
 * The qse_map_setcopier() function specifies how to clone an element.
 *  A special copier QSE_MAP_COPIER_INLINE is provided. This copier enables
 *  you to copy the data inline to the internal node. No freeer is invoked
 *  when the node is freeed.
 *
 *  You may set the copier to QSE_NULL to perform no special operation 
 *  when the data pointer is rememebered.
 */
void qse_map_setcopier (
	qse_map_t* map,          /**< a map */
	qse_map_id_t id,         /**< QSE_MAP_KEY or QSE_MAP_VAL */
	qse_map_copier_t copier  /**< element copier */
);

qse_map_freeer_t qse_map_getfreeer (
	qse_map_t*   map, /**< map */
	qse_map_id_t id   /**< QSE_MAP_KEY or QSE_MAP_VAL */
);

/**
 * The qse_map_setfreeer() function specifies how to destroy an element.
 * The @a freeer is called when a node containing the element is destroyed.
 */
void qse_map_setfreeer (
	qse_map_t* map,          /**< a map */
	qse_map_id_t id,         /**< QSE_MAP_KEY or QSE_MAP_VAL */
	qse_map_freeer_t freeer  /**< an element freeer */
);


qse_map_hasher_t qse_map_gethasher (
	qse_map_t* map
);

void qse_map_sethasher (
	qse_map_t* map,
	qse_map_hasher_t hasher
);

qse_map_comper_t qse_map_getcomper (
	qse_map_t* map
);

void qse_map_setcomper (
	qse_map_t* map,
	qse_map_comper_t comper
);

qse_map_keeper_t qse_map_getkeeper (
	qse_map_t* map
);

void qse_map_setkeeper (
	qse_map_t* map,
	qse_map_keeper_t keeper
);

qse_map_sizer_t qse_map_getsizer (
	qse_map_t* map
);

/* the sizer function is passed a map object and map->capa + 1 */
void qse_map_setsizer (
	qse_map_t* map,
	qse_map_sizer_t sizer
);

/**
 * The qse_map_search() function searches a map to find a pair with a 
 * matching key. It returns the pointer to the pair found. If it fails
 * to find one, it returns QSE_NULL.
 * @return pointer to the pair with a maching key, 
 *         or QSE_NULL if no match is found.
 */
qse_map_pair_t* qse_map_search (
	qse_map_t*  map,   /**< a map */
	const void* kptr,  /**< the pointer to a key */
	qse_size_t  klen   /**< the size of the key */
);

/**
 * The qse_map_upsert() function searches a map for the pair with a matching
 * key. If one is found, it updates the pair. Otherwise, it inserts a new
 * pair with a key and a value. It returns the pointer to the pair updated 
 * or inserted.
 * @return a pointer to the updated or inserted pair on success, 
 *         QSE_NULL on failure. 
 * SYNOPSIS
 */
qse_map_pair_t* qse_map_upsert (
	qse_map_t* map,   /**< a map */
	void*      kptr,  /**< the pointer to a key */
	qse_size_t klen,  /**< the length of the key */
	void*      vptr,  /**< the pointer to a value */
	qse_size_t vlen   /**< the length of the value */
);

/**
 * The qse_map_ensert() function inserts a new pair with the key and the value
 * given. If there exists a pair with the key given, the function returns 
 * the pair containing the key.
 * @return pointer to a pair on success, QSE_NULL on failure. 
 */
qse_map_pair_t* qse_map_ensert (
	qse_map_t* map,   /**< a map */
	void*      kptr,  /**< the pointer to a key */
	qse_size_t klen,  /**< the length of the key */
	void*      vptr,  /**< the pointer to a value */
	qse_size_t vlen   /**< the length of the value */
);

/**
 * The qse_map_insert() function inserts a new pair with the key and the value
 * given. If there exists a pair with the key given, the function returns 
 * QSE_NULL without channging the value.
 * @return pointer to the pair created on success, QSE_NULL on failure. 
 */
qse_map_pair_t* qse_map_insert (
	qse_map_t* map,   /**< a map */
	void*      kptr,  /**< the pointer to a key */
	qse_size_t klen,  /**< the length of the key */
	void*      vptr,  /**< the pointer to a value */
	qse_size_t vlen   /**< the length of the value */
);

/**
 * The qse_map_update() function updates the value of an existing pair
 * with a matching key.
 * @return pointer to the pair on success, QSE_NULL on no matching pair
 */
qse_map_pair_t* qse_map_update (
	qse_map_t* map,   /**< a map */
	void*      kptr,  /**< the pointer to a key */
	qse_size_t klen,  /**< the length of the key */
	void*      vptr,  /**< the pointer to a value */
	qse_size_t vlen   /**< the length of the value */
);

/* delete a pair with a matching key */
int qse_map_delete (
	qse_map_t* map   /* a map */,
	const void* kptr /* the pointer to a key */,
	qse_size_t klen  /* the size of the key in bytes */
);

/* clear a map */
void qse_map_clear (
	qse_map_t* map /* a map */
);

/* traverse a map */
qse_size_t qse_map_walk (
	qse_map_t* map          /* a map */,
	qse_map_walker_t walker /* the pointer to the function for each pair */,
	void* ctx               /* a pointer to user-specific data */
);

/* get the pointer to the first pair in the map. */
qse_map_pair_t* qse_map_getfirstpair (
	qse_map_t* map /* a map */, 
	qse_size_t* buckno
);

/* get the pointer to the next pair in the map. */
qse_map_pair_t* qse_map_getnextpair (
	qse_map_t* map /* a map */,
	qse_map_pair_t* pair,
	qse_size_t* buckno
);

#ifdef __cplusplus
}
#endif

#endif
