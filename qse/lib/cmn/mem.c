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

#include <qse/cmn/mem.h>

#if defined(_WIN32)
#	include <windows.h>
/*
#elif defined(__OS2__)
#	define INCL_DOSMEMMGR
#	include <os2.h>
#	include <bsememf.h>
*/
#else
#	include <stdlib.h>
#endif

#if defined(__SPU__)
#include <spu_intrinsics.h>
#define SPU_VUC_SIZE QSE_SIZEOF(vector unsigned char)
#endif

/*#define IS_UNALIGNED(ptr) (((qse_size_t)ptr)%QSE_SIZEOF(qse_size_t))*/
#define IS_UNALIGNED(ptr) (((qse_size_t)ptr)&(QSE_SIZEOF(qse_size_t)-1))
#define IS_ALIGNED(ptr) (!IS_UNALIGNED(ptr))

#define IS_EITHER_UNALIGNED(ptr1,ptr2) \
	(((qse_size_t)ptr1|(qse_size_t)ptr2)&(QSE_SIZEOF(qse_size_t)-1))
#define IS_BOTH_ALIGNED(ptr1,ptr2) (!IS_EITHER_UNALIGNED(ptr1,ptr2))

void* qse_memcpy (void* dst, const void* src, qse_size_t n)
{
#if defined(QSE_BUILD_FOR_SIZE)

	qse_byte_t* d = (qse_byte_t*)dst;
	qse_byte_t* s = (qse_byte_t*)src;
	while (n-- > 0) *d++ = *s++;
	return dst;

#elif defined(__SPU__)

	qse_byte_t* d;
	qse_byte_t* s;

	if (n >= SPU_VUC_SIZE &&
	    (((qse_size_t)dst) & (SPU_VUC_SIZE-1)) == 0 &&
	    (((qse_size_t)src) & (SPU_VUC_SIZE-1)) == 0)
	{
		vector unsigned char* du = (vector unsigned char*)dst;
		vector unsigned char* su = (vector unsigned char*)src;

		do
		{
			*du++ = *su++;
			n -= SPU_VUC_SIZE;
		}
		while (n >= SPU_VUC_SIZE);

		d = (qse_byte_t*)du;
		s = (qse_byte_t*)su;
	}
	else
	{
		d = (qse_byte_t*)dst;
		s = (qse_byte_t*)src;
	}

	while (n-- > 0) *d++ = *s++;
	return dst;

#elif defined(__GNUC__) && (defined(__x86_64) || defined(__amd64))

	/* i don't really care about alignments for x86-64 at this moment. fix it later */

	__asm__ volatile (
		"cld\n\t"
		"rep movsq\n"
		: /* no output */
		:"D" (dst), "S" (src), "c" (n >> 3)  /* input: %rdi = d, %rsi = src, %rcx = n / 8 */
		:"memory"
	);

	__asm__ volatile (
		"rep movsb\n" 
		: /* no output */
		:"c" (n & 7)  /* %rcx = n % 8, use existing %rdi and %rsi */
		:"memory", "%rdi", "%rsi"
	);

	return dst;

	#if 0
	qse_byte_t* d = dst;

	__asm__ volatile (
		"cld\n\t"
		"rep movsq\n"
		: "=D" (d), "=S" (src)  /* output: d = %rdi, src = %rsi */
		:"0" (d), "1" (src), "c" (n >> 3)  /* input: %rdi = d, %rsi = src, %rcx = n / 8 */
		:"memory"
	);

	__asm__ volatile (
		"rep movsb"
		: /* no output */
		:"D" (d), "S" (src), "c" (n & 7)  /* input: %rdi = d, %rsi = src, %rcx = n % 8 */
		:"memory"
	);

	return dst;
	#endif

#elif defined(__GNUC__) && (defined(__i386) || defined(i386))

	/* i don't really care about alignments for x86 at this moment. fix it later */

	__asm__ volatile (
		"cld\n\t"
		"rep\n\tmovsl\n"
		: /* no output */
		:"D" (dst), "S" (src), "c" (n >> 2)  /* input: %edi = d, %esi = src, %ecx = n / 4 */
		:"memory"
	);

	__asm__ volatile (
		"rep\n\tmovsb\n" 
		: /* no output */
		:"c" (n & 3)  /* %rcx = n % 4, use existing %edi and %esi */
		:"memory", "%edi", "%esi"
	);

	return dst;

	#if 0
	qse_byte_t* d = dst;

	__asm__ volatile (
		"cld\n\t"
		"rep movsl\n"
		:"=D" (d), "=S" (src) /* output: d = %edi, src = %esi */
		:"0" (d), "1" (src), "c" (n >> 2)  /* input: %edi = d, %esi = src, %ecx = n / 4 */
		:"memory"
	);

	__asm__ volatile (
		"rep movsb\n"
		:
		:"D" (d), "S" (src), "c" (n & 3)  /* input: %edi = d, %esi = src, %ecx = n % 4 */
		:"memory"
	);

	return dst;
	#endif

#else
	qse_byte_t* d;
	qse_byte_t* s;

	if (n < 8)
	{
		d = (qse_byte_t*)dst;
		s = (qse_byte_t*)src;

		switch (n)
		{
			case 7: *d++ = *s++;
			case 6: *d++ = *s++;
			case 5: *d++ = *s++;
			case 4: *d++ = *s++;
			case 3: *d++ = *s++;
			case 2: *d++ = *s++;
			case 1: *d++ = *s++;
		}

		return dst;
	}

	if (n >= QSE_SIZEOF(qse_size_t) && IS_BOTH_ALIGNED(dst,src))
	{
		qse_size_t* du = (qse_size_t*)dst;
		qse_size_t* su = (qse_size_t*)src;

		do
		{
			*du++ = *su++;
			n -= QSE_SIZEOF(qse_size_t);
		}
		while (n >= QSE_SIZEOF(qse_size_t));

		d = (qse_byte_t*)du;
		s = (qse_byte_t*)su;
	}
	else
	{
		d = (qse_byte_t*)dst;
		s = (qse_byte_t*)src;
	}

	while (n-- > 0) *d++ = *s++;
	return dst;

#endif
}

void* qse_memmove (void* dst, const void* src, qse_size_t n)
{
	const qse_byte_t* sre = (const qse_byte_t*)src + n;

	if (dst <= src || dst >= (const void*)sre) 
	{
		qse_byte_t* d = (qse_byte_t*)dst;
		const qse_byte_t* s = (const qse_byte_t*)src;
		while (n-- > 0) *d++ = *s++;
	}
	else 
	{
		qse_byte_t* dse = (qse_byte_t*)dst + n;
		while (n-- > 0) *--dse = *--sre;
	}

	return dst;
}

void* qse_memset (void* dst, int val, qse_size_t n)
{
#if defined(QSE_BUILD_FOR_SIZE)

	qse_byte_t* d = (qse_byte_t*)dst;
	while (n-- > 0) *d++ = (qse_byte_t)val;
	return dst;

#elif defined(__SPU__)

	qse_byte_t* d;
	qse_size_t rem;

	if (n <= 0) return dst;

	d = (qse_byte_t*)dst;

	/* spu SIMD instructions require 16-byte alignment */
	rem = ((qse_size_t)dst) & (SPU_VUC_SIZE-1);
	if (rem > 0)
	{
		/* handle leading unaligned part */
		do { *d++ = (qse_byte_t)val; } 
		while (n-- > 0 && ++rem < SPU_VUC_SIZE);
	}
	
	/* do the vector copy */
	if (n >= SPU_VUC_SIZE)
	{
		/* a vector of 16 unsigned char cells */
		vector unsigned char v16;
		/* a pointer to such a vector */
		vector unsigned char* vd = (vector unsigned char*)d;

		/* fills all 16 unsigned char cells with the same value 
		 * no need to use shift and bitwise-or owing to splats */
		v16 = spu_splats((qse_byte_t)val);

		do
		{
			*vd++ = v16;
			n -= SPU_VUC_SIZE;
		}
		while (n >= SPU_VUC_SIZE);

		d = (qse_byte_t*)vd;
	}

	/* handle the trailing part */
	while (n-- > 0) *d++ = (qse_byte_t)val;
	return dst;
	
#elif defined(__GNUC__) && (defined(__x86_64) || defined(__amd64))

	/* i don't really care about alignments for x86-64 at this moment. fix it later */

	qse_byte_t* d = dst;

	__asm__ volatile ("cld\n");

	if (n >= 8)
	{
		qse_size_t qw = (qse_byte_t)val;
		if (qw)
		{
			qw = (qw << 8) | (qse_byte_t)val;
			qw = (qw << 8) | (qse_byte_t)val;
			qw = (qw << 8) | (qse_byte_t)val;
			qw = (qw << 8) | (qse_byte_t)val;
			qw = (qw << 8) | (qse_byte_t)val;
			qw = (qw << 8) | (qse_byte_t)val;
			qw = (qw << 8) | (qse_byte_t)val;
		}

		__asm__ volatile (
			"rep stosq\n"
			:"=D" (d) /* output: d = %rdi */
			:"0" (d), "a" (qw), "c" (n >> 3)  /* input: %rdi = d, %rax = qw, %rcx = n / 8 */
			:"memory"
		);
	}

	__asm__ volatile (
		"rep stosb\n"
		: /* no output */
		:"D" (d), "a" (val), "c" (n & 7)  /* input: %rdi = d, %rax = src, %rcx = n % 8 */
		:"memory"
	);

	return dst;

#elif defined(__GNUC__) && (defined(__i386) || defined(i386))

	/* i don't really care about alignments for x86 at this moment. fix it later */

	qse_byte_t* d = dst;

	__asm__ volatile ("cld\n");

	if (n >= 4)
	{
		qse_size_t dw = (qse_byte_t)val;
		if (dw)
		{
			dw = (dw << 8) | (qse_byte_t)val;
			dw = (dw << 8) | (qse_byte_t)val;
			dw = (dw << 8) | (qse_byte_t)val;
		}

		__asm__ volatile (
			"rep\n\tstosl\n"
			:"=D" (d) /* output: d = %edi */
			:"0" (d), "a" (dw), "c" (n >> 2)  /* input: %edi = d, %eax = dw, %ecx = n / 4 */
			:"memory"
		);
	}

	__asm__ volatile (
		"rep\n\tstosb\n"
		: /* no output */ 
		:"D" (d), "a" (val), "c" (n & 3)  /* input: %edi = d, %eax = src, %ecx = n % 4 */
		:"memory"
	);

	return dst;

#else
	qse_byte_t* d;
	qse_size_t rem;

	if (n <= 0) return dst;

	d = (qse_byte_t*)dst;

	rem = IS_UNALIGNED(dst);
	if (rem > 0)
	{
		do { *d++ = (qse_byte_t)val; } 
		while (n-- > 0 && ++rem < QSE_SIZEOF(qse_size_t));
	}

	if (n >= QSE_SIZEOF(qse_size_t))
	{
		qse_size_t* u = (qse_size_t*)d;
		qse_size_t uv = 0;
		int i;

		if (val != 0) 
		{
			for (i = 0; i < QSE_SIZEOF(qse_size_t); i++)
				uv = (uv << 8) | (qse_byte_t)val;
		}

		QSE_ASSERT (IS_ALIGNED(u));
		do
		{
			*u++ = uv;
			n -= QSE_SIZEOF(qse_size_t);
		}
		while (n >= QSE_SIZEOF(qse_size_t));

		d = (qse_byte_t*)u;
	}

	while (n-- > 0) *d++ = (qse_byte_t)val;
	return dst;

#endif
}

int qse_memcmp (const void* s1, const void* s2, qse_size_t n)
{
#if defined(QSE_BUILD_FOR_SIZE)

	const qse_byte_t* b1 = (const qse_byte_t*)s1;
	const qse_byte_t* b2 = (const qse_byte_t*)s2;

	while (n-- > 0)
	{
		if (*b1 != *b2) return *b1 - *b2;
		b1++; b2++;
	}

	return 0;

#elif defined(__SPU__)

	const qse_byte_t* b1;
	const qse_byte_t* b2;

	if (n >= SPU_VUC_SIZE &&
	    (((qse_size_t)s1) & (SPU_VUC_SIZE-1)) == 0 &&
	    (((qse_size_t)s2) & (SPU_VUC_SIZE-1)) == 0)
	{
		vector unsigned char* v1 = (vector unsigned char*)s1;
		vector unsigned char* v2 = (vector unsigned char*)s2;

		vector unsigned int tmp;

		do
		{
			unsigned int cnt;
			unsigned int pat;

			/* compare 16 chars at one time */
			tmp = spu_gather(spu_cmpeq(*v1,*v2));
			/* extract the bit pattern */
			pat = spu_extract(tmp, 0);
			/* invert the bit patterns */
			pat = 0xFFFF & ~pat;

			/* put it back to the vector */
			tmp = spu_insert (pat, tmp, 0);
			/* count the leading zeros */
			cnt = spu_extract(spu_cntlz(tmp),0);
			/* 32 leading zeros mean that 
			 * all characters are the same */
			if (cnt != 32) 
			{
				/* otherwise, calculate the 
				 * unmatching pointer address */
				b1 = (const qse_byte_t*)v1 + (cnt - 16);
				b2 = (const qse_byte_t*)v2 + (cnt - 16);
				break;
			}

			v1++; v2++;
			n -= SPU_VUC_SIZE;

			if (n < SPU_VUC_SIZE)
			{
				b1 = (const qse_byte_t*)v1;
				b2 = (const qse_byte_t*)v2;
				break;
			}
		}
		while (1);
	}
	else
	{
		b1 = (const qse_byte_t*)s1;
		b2 = (const qse_byte_t*)s2;
	}

	while (n-- > 0)
	{
		if (*b1 != *b2) return *b1 - *b2;
		b1++; b2++;
	}

	return 0;

#else
	const qse_byte_t* b1;
	const qse_byte_t* b2;

	if (n >= QSE_SIZEOF(qse_size_t) && IS_BOTH_ALIGNED(s1,s2))
	{
		const qse_size_t* u1 = (const qse_size_t*)s1;
		const qse_size_t* u2 = (const qse_size_t*)s2;

		do
		{
			if (*u1 != *u2) break;
			u1++; u2++;
			n -= QSE_SIZEOF(qse_size_t);
		}
		while (n >= QSE_SIZEOF(qse_size_t));

		b1 = (const qse_byte_t*)u1;
		b2 = (const qse_byte_t*)u2;
	}
	else
	{
		b1 = (const qse_byte_t*)s1;
		b2 = (const qse_byte_t*)s2;
	}

	while (n-- > 0)
	{
		if (*b1 != *b2) return *b1 - *b2;
		b1++; b2++;
	}

	return 0;
#endif
}

void* qse_membyte (const void* s, int val, qse_size_t n)
{
	const qse_byte_t* x = (const qse_byte_t*)s;

	while (n-- > 0)
	{
		if (*x == (qse_byte_t)val) return (void*)x;
		x++;
	}

	return QSE_NULL;
}

void* qse_memrbyte (const void* s, int val, qse_size_t n)
{
	const qse_byte_t* x = (qse_byte_t*)s + n - 1;

	while (n-- > 0)
	{
		if (*x == (qse_byte_t)val) return (void*)x;
		x--;
	}

	return QSE_NULL;
}

void* qse_memmem (const void* hs, qse_size_t hl, const void* nd, qse_size_t nl)
{
	if (nl <= hl) 
	{
		qse_size_t i;
		const qse_byte_t* h = (const qse_byte_t*)hs;

		for (i = hl - nl + 1; i > 0; i--)  
		{
			if (qse_memcmp(h, nd, nl) == 0) return (void*)h;
			h++;
		}
	}

	return QSE_NULL;
}

void* qse_memrmem (const void* hs, qse_size_t hl, const void* nd, qse_size_t nl)
{
	if (nl <= hl) 
	{
		qse_size_t i;
		const qse_byte_t* h;

		/* things are slightly more complacated 
		 * when searching backward */
		if (nl == 0) 
		{
			/* when the needle is empty, it returns
			 * the pointer to the last byte of the haystack.
			 * this is because qse_memmem returns the pointer
			 * to the first byte of the haystack when the
			 * needle is empty. but I'm not so sure if this
			 * is really desirable behavior */
			h = (const qse_byte_t*)hs + hl - 1;
			return (void*)h;
		}

		h = (const qse_byte_t*)hs + hl - nl;
		for (i = hl - nl + 1; i > 0; i--)  
		{
			if (qse_memcmp(h, nd, nl) == 0) return (void*)h;
			h--;
		}
	}

	return QSE_NULL;
}

static void* mmgr_alloc (qse_mmgr_t* mmgr, qse_size_t n)
{
#if defined(_WIN32)
	HANDLE heap;
	heap = GetProcessHeap ();
	if (!heap) return QSE_NULL;
	return HeapAlloc (heap, 0, n);	
#else
/* TODO: need to rewrite this for __OS2__ using DosAllocMem()? */
	return malloc (n);
#endif
}

static void* mmgr_realloc (qse_mmgr_t* mmgr, void* ptr, qse_size_t n)
{
#if defined(_WIN32)
	HANDLE heap;
	heap = GetProcessHeap ();
	if (!heap) return QSE_NULL;

	return ptr? HeapReAlloc (heap, 0, ptr, n): 
	            HeapAlloc (heap, 0, n);
#else
	/* realloc() on some old systems doesn't work like 
	 * modern realloc() when ptr is NULL. let's divert
	 * it to malloc() explicitly in such a case */

	/*return realloc (ptr, n);*/
	return ptr? realloc (ptr, n): malloc (n);
#endif
}

static void mmgr_free (qse_mmgr_t* mmgr, void* ptr)
{
#if defined(_WIN32)
	HANDLE heap;
	heap = GetProcessHeap ();
	if (heap) HeapFree (heap, 0, ptr);
#else
	free (ptr);
#endif
}

static qse_mmgr_t builtin_mmgr =
{
	mmgr_alloc,
	mmgr_realloc,
	mmgr_free,
	QSE_NULL
};

static qse_mmgr_t* dfl_mmgr = &builtin_mmgr;

qse_mmgr_t* qse_getdflmmgr (void)
{
	return dfl_mmgr;
}

void qse_setdflmmgr (qse_mmgr_t* mmgr)
{
	dfl_mmgr = (mmgr? mmgr: &builtin_mmgr);
}
