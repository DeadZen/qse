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

#include <qse/cmn/Mmgr.hpp>
#include <qse/cmn/StdMmgr.hpp>
#include "mem.h"

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

void* Mmgr::alloc_mem (mmgr_t* mmgr, qse_size_t n) 
{
	return ((Mmgr*)mmgr->ctx)->allocMem (n);
}

void* Mmgr::realloc_mem (mmgr_t* mmgr, void* ptr, qse_size_t n) 
{
	return ((Mmgr*)mmgr->ctx)->reallocMem (ptr, n);
}

void Mmgr::free_mem (mmgr_t* mmgr, void* ptr) 
{
	((Mmgr*)mmgr->ctx)->freeMem (ptr);
}

void* Mmgr::callocate (qse_size_t n, bool raise_exception)
{
	void* ptr = this->allocate (n, raise_exception);
	QSE_MEMSET (ptr, 0, n);
	return ptr;
}

Mmgr* Mmgr::dfl_mmgr = StdMmgr::getInstance();

Mmgr* Mmgr::getDFL ()
{
	return Mmgr::dfl_mmgr;
}

void Mmgr::setDFL (Mmgr* mmgr)
{
	Mmgr::dfl_mmgr = mmgr;
}

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

void* operator new (qse_size_t size, QSE::Mmgr* mmgr)
{
	return mmgr->allocate (size);
}

#if defined(QSE_CPP_NO_OPERATOR_DELETE_OVERLOADING)
void qse_operator_delete (void* ptr, QSE::Mmgr* mmgr)
#else
void operator delete (void* ptr, QSE::Mmgr* mmgr)
#endif
{
	mmgr->dispose (ptr);
}

void* operator new (qse_size_t size, QSE::Mmgr* mmgr, void* existing_ptr)
{
	// mmgr unused. i put it in the parameter list to make this function
	// less conflicting with the stock ::operator new() that doesn't allocate.
	return existing_ptr;
}

#if 0
void* operator new[] (qse_size_t size, QSE::Mmgr* mmgr)
{
	return mmgr->allocate (size);
}

void operator delete[] (void* ptr, QSE::Mmgr* mmgr)
{
	mmgr->dispose (ptr);
}
#endif
