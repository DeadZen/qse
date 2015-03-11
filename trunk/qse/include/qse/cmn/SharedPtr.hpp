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

#ifndef _QSE_CMN_SHAREDPTR_HPP_
#define _QSE_CMN_SHAREDPTR_HPP_

#include <qse/cmn/Mmged.hpp>

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

template <typename T>
struct SharedPtrDeleter
{
	void operator() (T* ptr, void* arg)
	{
		delete ptr;
	}
};

template <typename T>
struct SharedPtrArrayDeleter
{
	void operator() (T* ptr, void* arg)
	{
		delete[] ptr;
	}
};

template <typename T>
struct SharedPtrMmgrDeleter
{
	void operator() (T* ptr, void* arg)
	{
		ptr->~T ();
		::operator delete (ptr, (QSE::Mmgr*)arg);
	}
};

///
/// The SharedPtr class provides a smart pointer that can be shared
/// using reference counting.
///
template<typename T, typename DELETER = SharedPtrDeleter<T> >
class QSE_EXPORT SharedPtr: public Mmged
{
public:
	typedef SharedPtr<T,DELETER> SelfType;

	typedef SharedPtrDeleter<T> DefaultDeleter;

	SharedPtr (T* ptr = (T*)QSE_NULL, void* darg = (void*)QSE_NULL): Mmged(QSE_NULL)
	{
		this->item = new (this->getMmgr()) item_t;
		this->item->ref = 1;
		this->item->ptr = ptr;
		this->item->darg = darg;
	}

	SharedPtr (Mmgr* mmgr, T* ptr = (T*)QSE_NULL, void* darg = (void*)QSE_NULL): Mmged(mmgr)
	{
		this->item = new (this->getMmgr()) item_t;
		this->item->ref = 1;
		this->item->ptr = ptr;
		this->item->darg = darg;
	}

	SharedPtr (const SelfType& sp): Mmged(sp), item (sp.item) 
	{
		this->item->ref++;
	}

	~SharedPtr () 
	{
		this->item->ref--;
		if (this->item->ref <= 0)
		{
			if (this->item->ptr) this->item->deleter (this->item->ptr, this->item->darg);
			// no destructor as *this->_ref is a plain type.
			::operator delete (this->item, this->getMmgr());
		}
	}

	SelfType& operator= (const SelfType& sp)
	{
		if (this != &sp)
		{
			this->item->ref--;
			if (this->item->ref <= 0)
			{
				if (this->item->ptr) this->item->deleter (this->item->ptr, this->item->darg);
				// no destructor as *this->_ref is a plain type.
				::operator delete (this->item, this->getMmgr());
			}

			// must copy the memory manager pointer as the item
			// to be copied is allocated using the memory manager of sp.
			this->setMmgr (sp.getMmgr());

			this->item = sp.item;
			this->item->ref++;
		}

		return *this;
	}

	T& operator* ()
	{
		QSE_ASSERT (this->item->ptr != (T*)QSE_NULL);
		return *this->item->ptr;
	}

	const T& operator* () const 
	{
		QSE_ASSERT (this->item->ptr != (T*)QSE_NULL);
		return *this->item->ptr;
	}

	T* operator-> () 
	{
		QSE_ASSERT (this->item->ptr != (T*)QSE_NULL);
		return this->item->ptr;
	}

	const T* operator-> () const 
	{
		QSE_ASSERT (this->item->ptr != (T*)QSE_NULL);
		return this->item->ptr;
	}

	bool operator! () const 
	{
		return this->item->ptr == (T*)QSE_NULL;
	}

	T& operator[] (qse_size_t idx) 
	{
		QSE_ASSERT (this->item->ptr != (T*)QSE_NULL);
		return this->item->ptr[idx];
	}

	T* getPtr () 
	{
		return this->item->ptr;
	}

	const T* getPtr () const
	{
		return this->item->ptr;
	}

protected:
	struct item_t
	{
		qse_size_t ref;
		T*         ptr;
		void*      darg;
		DELETER    deleter;
	};

	item_t* item;
}; 

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif
