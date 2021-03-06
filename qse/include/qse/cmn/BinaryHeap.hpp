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

#ifndef _QSE_CMN_BINARYHEAP_HPP_
#define _QSE_CMN_BINARYHEAP_HPP_

/// \file
/// Provides a binary heap implementation.
///
/// \includelineno bh01.cpp
/// 

#include <qse/cmn/Array.hpp>

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

// greater-than comparator
template <typename T>
struct BinaryHeapComparator
{
	// this can be used to build a max heap
	bool operator() (const T& v1, const T& v2) const
	{
		return v1 > v2;
	}
};

template <typename T>
struct BinaryHeapPositioner
{
	void operator() (T& v, qse_size_t index) const
	{
		// do nothing
	}
};

typedef ArrayResizer BinaryHeapResizer;

#define QSE_BINARY_HEAP_UP(x)     (((x) - 1) / 2)
#define QSE_BINARY_HEAP_LEFT(x)   ((x) * 2 + 1)
#define QSE_BINARY_HEAP_RIGHT(x)  ((x) * 2 + 2)

///
/// The BinaryHeap class is a template class that implements a binary heap.
///
/// \code
/// #include <qse/cmn/BinaryHeap.hpp>
/// #include <stdio.h>
///
/// struct IntComparator
/// {
///         bool operator() (int v1, int v2) const
///         {
///                 //return !(v1 > v2);
///                 return v1 > v2;
///         }
/// };
/// 
/// int main (int argc, char* argv[])
/// {
///         QSE::BinaryHeap<int,IntComparator> heap;
/// 
///         heap.insert (70);
///         heap.insert (90);
///         heap.insert (10);
///         heap.insert (5);
///         heap.insert (88);
///         heap.insert (87);
///         heap.insert (300);
///         heap.insert (91);
///         heap.insert (100);
///         heap.insert (200);
/// 
///         while (heap.getSize() > 0)
///         {
///                 printf ("%d\n", heap.getValueAt(0));
///                 heap.remove (0);
///         }
/// 
///         return 0;
/// }
/// \endcode
///
template <typename T, typename COMPARATOR = BinaryHeapComparator<T>, typename POSITIONER = BinaryHeapPositioner<T>, typename RESIZER = BinaryHeapResizer >
class BinaryHeap: protected Array<T,POSITIONER,RESIZER>
{
public:
	typedef BinaryHeap<T,COMPARATOR,POSITIONER,RESIZER> SelfType;
	typedef Array<T,POSITIONER,RESIZER> ParentType;

	typedef BinaryHeapComparator<T> DefaultComparator;
	typedef BinaryHeapPositioner<T> DefaultPositioner;
	typedef BinaryHeapResizer DefaultResizer;

	enum
	{
		DEFAULT_CAPACITY = ParentType::DEFAULT_CAPACITY,
		INVALID_INDEX = ParentType::INVALID_INDEX
	};

	BinaryHeap (qse_size_t capacity = DEFAULT_CAPACITY): ParentType (QSE_NULL, capacity)
	{
	}

	BinaryHeap (Mmgr* mmgr, qse_size_t capacity = DEFAULT_CAPACITY): ParentType (mmgr, capacity)
	{
	}

	BinaryHeap (const SelfType& heap): ParentType (heap)
	{
	}

#if defined(QSE_CPP_ENABLE_CPP11_MOVE)
	BinaryHeap (SelfType& heap): ParentType (heap)
	{
	}
#endif

	~BinaryHeap ()
	{
	}

	SelfType& operator= (const SelfType& heap)
	{
		if (this != &heap)
		{
			ParentType::operator= (heap);
		}
		return *this;
	}

#if defined(QSE_CPP_ENABLE_CPP11_MOVE)
	SelfType& operator= (SelfType&& heap)
	{
		if (this != &heap)
		{
			ParentType::operator= (heap);
		}
		return *this;
	}
#endif

	/// The isEmpty() function returns true if the binary heap contains no
	/// item and false otherwise.
	bool isEmpty() const { return ParentType::isEmpty(); }

	/// The getSize() function returns the number of items in the binary heap.
	qse_size_t getSize() const { return ParentType::getSize(); }

	/// The getCapacity() function returns the capacity of the internal buffer
	/// of the binary heap.
	qse_size_t getCapacity() const { return ParentType::getCapacity(); }

	/// The getIndex() function returns the index of an item reference \a v.
	qse_size_t getIndex (const T& v) const { return ParentType::getIndex(v); }

	/// The clear() function returns all items.
	void clear (bool purge_buffer = false) { ParentType::clear (purge_buffer); }

	/// The compact() function restructures the internal buffer to the size
	/// that is just large enough to hold the existing items.
	void compact() { ParentType::compact (); }

	/// The operator[] function returns the constant reference to the item 
	/// at the specified \a index.
	const T& operator[] (qse_size_t index) const
	{
		return ParentType::getValueAt (index);
	}

	/// The getValueAt() function returns the constant reference to the item
	/// at the specified \a index.
	const T& getValueAt (qse_size_t index) const
	{
		return ParentType::getValueAt (index);
	}

	/// The insert() function adds a new item \a value to the binary heap.
	qse_size_t insert (const T& value)
	{
		qse_size_t index = this->count;

		// add the item at the back of the array
		ParentType::insert (index, value);

		// move the item up to the top if it's greater than the up item
		return this->sift_up(index);
	}

#if defined(QSE_CPP_ENABLE_CPP11_MOVE)
	qse_size_t insert (T&& value)
	{
		qse_size_t index = this->count;

		// add the item at the back of the array
		ParentType::insert (index, QSE_CPP_RVREF(value));

		// move the item up to the top if it's greater than the up item
		return this->sift_up(index);
	}
#endif

	/// The update() function changes the item at the specified \a index
	/// to a new item \a value.
	qse_size_t update (qse_size_t index, const T& value)
	{
		T old = this->buffer[index];
		ParentType::update (index, value);
		return (this->greater_than(value, old))? this->sift_up(index): this->sift_down(index);
	}

#if defined(QSE_CPP_ENABLE_CPP11_MOVE)
	qse_size_t update (qse_size_t index, T&& value)
	{
		T old = QSE_CPP_RVREF(this->buffer[index]);
		ParentType::update (index, QSE_CPP_RVREF(value));
		return (this->greater_than(value, old))? this->sift_up(index): this->sift_down(index);
	}
#endif

	/// The remove() function removes an item at the specified \a index.
	void remove (qse_size_t index)
	{
		QSE_ASSERT (index < this->count);

		if (this->count == 1)
		{
			QSE_ASSERT (index == 0);
			ParentType::remove (this->count - 1);
		}
		else if (this->count > 1)
		{
			// store the item to remove temporarily
			T old = QSE_CPP_RVREF(this->buffer[index]);

			// copy the last item to the position to remove 
			ParentType::update (index, QSE_CPP_RVREF(this->buffer[this->count - 1]));

			// delete the last item
			ParentType::remove (this->count - 1);
			
			// relocate the item
			(this->greater_than (this->buffer[index], old))? this->sift_up(index): this->sift_down(index);
		}
	}

protected:
	qse_size_t sift_up (qse_size_t index)
	{
		qse_size_t up;

		up = QSE_BINARY_HEAP_UP(index);
		if (index > 0 && this->greater_than(this->buffer[index], this->buffer[up]))
		{
			T item = QSE_CPP_RVREF(this->buffer[index]);

			do 
			{
				ParentType::setValueAt (index, QSE_CPP_RVREF(this->buffer[up]));

				index = up;
				up = QSE_BINARY_HEAP_UP(up);
			}
			while (index > 0 && this->greater_than(item, this->buffer[up]));

			ParentType::setValueAt (index, QSE_CPP_RVREF(item));
		}

		return index;
	}

	qse_size_t sift_down (qse_size_t index)
	{
		qse_size_t half_data_count = this->count / 2;
		
		if (index < half_data_count)
		{
			// if at least 1 child is under the 'index' position
			// perform sifting

			T item = QSE_CPP_RVREF(this->buffer[index]);

			do
			{
				qse_size_t left, right, greater;

				left = QSE_BINARY_HEAP_LEFT(index);
				right = QSE_BINARY_HEAP_RIGHT(index);

				// choose the larger one between 2 BinaryHeap 
				if (right < this->count && 
				    this->greater_than(this->buffer[right], this->buffer[left]))
				{
					// if the right child exists and 
					// the right item is greater than the left item
					greater = right;
				}
				else
				{
					greater = left;
				}

				if (this->greater_than(item, this->buffer[greater])) break;

				ParentType::setValueAt (index, QSE_CPP_RVREF(this->buffer[greater]));
				index = greater;
			}
			while (index < half_data_count);

			ParentType::setValueAt (index, QSE_CPP_RVREF(item));
		}

		return index;
	}

protected:
	COMPARATOR greater_than;
};


/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif
