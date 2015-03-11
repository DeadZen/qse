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

#ifndef _QSE_CMN_ARRAY_HPP_
#define _QSE_CMN_ARRAY_HPP_

#include <qse/Types.hpp>
#include <qse/cmn/Mmged.hpp>

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

template <typename T>
struct ArrayPositioner
{
	void operator() (T& v, qse_size_t index) const
	{
		// do nothing
	}
};

struct ArrayResizer
{
	qse_size_t operator() (qse_size_t current) const
	{
		if (current <= 0) current = 1;

		return (current < 5000)?   (current + current):
		       (current < 50000)?  (current + (current / 2)):
		       (current < 100000)? (current + (current / 4)):
		       (current < 150000)? (current + (current / 8)):
		                           (current + (current / 16));
	}
};

///
/// The Array class provides a dynamically resized array.
/// 
template <typename T, typename POSITIONER = ArrayPositioner<T>, typename RESIZER = ArrayResizer >
class Array: public Mmged
{
public:
	typedef Array<T,POSITIONER,RESIZER> SelfType;

	typedef ArrayPositioner<T> DefaultPositioner;
	typedef ArrayResizer DefaultResizer;

	enum 
	{
		DEFAULT_CAPACITY = 128,
		INVALID_INDEX = ~(qse_size_t)0
	};

private:
	void init_array (int capacity)
	{
		if (capacity <= 0) 
		{
			this->buffer = QSE_NULL;
			this->capacity = 0;
		}
		else 
		{
			//this->buffer = new T[capacity];
			this->buffer = (T*)::operator new (capacity * QSE_SIZEOF(*this->buffer), this->getMmgr());
			this->capacity = capacity;
		}

		this->count  = 0;
	}

public:
	Array (qse_size_t capacity = DEFAULT_CAPACITY): Mmged (QSE_NULL)
	{
		this->init_array (capacity);
	}

	Array (Mmgr* mmgr, qse_size_t capacity = DEFAULT_CAPACITY): Mmged (mmgr)
	{
		this->init_array (capacity);
	}

	Array (const SelfType& array): 
		Mmged (array.getMmgr()),
		count (0), capacity (0), buffer (QSE_NULL)
	{
		if (array.buffer)
		{
			this->buffer = this->clone_buffer (array.buffer, array.capacity, array.count);
			this->count = array.count;
			this->capacity = array.capacity;
		}
	}

	~Array ()
	{
		this->clear (true);
	}

	SelfType& operator= (const SelfType& array)
	{
		if (this != &array)
		{
			this->clear (true);
			if (array.buffer)
			{
				this->buffer = this->clone_buffer (array, array.capacity, array.count);
				this->count = array.count;
				this->capacity = array.capacity;
			}
		}
		return *this;
	}

protected:
	T* clone_buffer (const T* srcbuf, qse_size_t capa, qse_size_t cnt)
	{
		QSE_ASSERT (capa > 0);
		QSE_ASSERT (cnt <= capa);

		qse_size_t index;

		//T* tmp = new T[capa];
		T* tmp = (T*)::operator new (capa * QSE_SIZEOF(*tmp), this->getMmgr());

		try 
		{
			for (index = 0; index < cnt; index++) 
			{
				// copy-construct each element.
				new((QSE::Mmgr*)QSE_NULL, &tmp[index]) T(srcbuf[index]);
				this->positioner (tmp[index], index);
			}
		}
		catch (...) 
		{
			// in case copy-constructor raises an exception.
			QSE_ASSERT (tmp != QSE_NULL);
			while (index > 0)
			{
				--index;
				this->positioner (tmp[index], INVALID_INDEX);
				tmp[index].~T ();
			}
			::operator delete (tmp, this->getMmgr());
			throw;
		}

		return tmp;
	}

	void put_item (qse_size_t index, const T& value)
	{
		if (index >= this->count)
		{
			// no value exists in the given position.
			// i can copy-construct the value.
			new((QSE::Mmgr*)QSE_NULL, &this->buffer[index]) T(value);
			this->positioner (this->buffer[index], index);
		}
		else
		{
			// there is an old value in the position.
			this->buffer[index] = value;
			this->positioner (this->buffer[index], index);
		}
	}

	void clear_all_items  ()
	{
		QSE_ASSERT (this->count <= 0 || (this->count >= 1 && this->buffer));

		for (qse_size_t i = this->count; i > 0; )
		{
			--i;
			this->positioner (this->buffer[i], INVALID_INDEX);
			this->buffer[i].~T ();
		}
	
		this->count = 0;
	}

public:
	bool isEmpty () const
	{
		return this->count == 0;
	}

	qse_size_t getSize () const
	{
		return this->count;
	}

	qse_size_t getCapacity () const
	{
		return this->capacity;
	}
	
	operator T* ()
	{
		return this->buffer;
	}

	operator const T* () const
	{
		return this->buffer;
	}

	T* getBuffer ()
	{
		return this->buffer;
	}

	const T* getBuffer () const
	{
		return this->buffer;
	}

	/// The getIndex() function returns the index of the given value \a v 
	/// if it belongs to the array. It returns #INVALID_INDEX if not. 
	/// Note that this is not a search function.
	///
	/// \code
	///  QSE::Array<int> a;
	///  a.insert (0, 10);
	///  a.insert (0, 20);
	///  a.insert (0, 30);
	///  const int& t = a[2];
	///  printf ("%lu\n", (unsigned long int)a.getIndex(t)); // print 2
	/// \endcode
	qse_size_t getIndex (const T& v)
	{
		if (&v >= &this->buffer[0] && &v < &this->buffer[this->count])
		{
			return &v - &this->buffer[0];
		}

		return INVALID_INDEX;
	}

	// i don't want expose a non-const accessor as i don't like
	// a proper update procesure to be skipped. 
	// use setValueAt() or update() to modify the existing element.
	//T& operator[] (qse_size_t index)
	//{
	//	QSE_ASSERT (index < this->count);
	//	return this->buffer[index];
	//}

	const T& operator[] (qse_size_t index) const
	{
		QSE_ASSERT (index < this->count);
		return this->buffer[index];
	}

	// i don't want expose a non-const accessor as i don't like
	// a proper update procesure to be skipped. 
	// use setValueAt() or update() to modify the existing element.
	//T& getValueAt (qse_size_t index)
	//{
	//	QSE_ASSERT (index < this->count);
	//	return this->buffer[index];
	//}

	const T& getValueAt (qse_size_t index) const
	{
		QSE_ASSERT (index < this->count);
		return this->buffer[index];
	}

	void setValueAt (qse_size_t index, const T& value)
	{
		this->update (index, value);
	}

	qse_size_t insert (qse_size_t index, const T& value)
	{
		// Unlike insert() in RedBlackTree and HashList,
		// it inserts an item when index exists in the existing array.
		// It is because array allows duplicate items.
		// RedBlckTree::insert() and HashList::insert() return failure
		// if existing item exists.

		if (index >= this->capacity) 
		{
			// the position to add the element is beyond the
			// capacity. resize the buffer.
			qse_size_t new_capa = this->resizer (this->capacity);

			if (index < new_capa)
				this->setCapacity (new_capa);
			else
				this->setCapacity (index + 1);
		}
		else if (this->count >= this->capacity) 
		{
			// the array is already full.
			// insertion requires at least one more slot
			qse_size_t new_capa = this->resizer (this->capacity);
			this->setCapacity (new_capa);
		}

		if (index < this->count)
		{
			// shift the existing elements to the back by one slot.
			for (qse_size_t i = this->count; i > index; i--) 
			{
				//this->buffer[i] = this->buffer[i - 1];
				this->put_item (i, this->buffer[i - 1]);
			}
		}
		else if (index > this->count)
		{
			// the insertion position leaves some gaps in between.
			// fill the gap with a default value.
			for (qse_size_t i = this->count; i < index; i++)
			{
				new((QSE::Mmgr*)QSE_NULL, &this->buffer[i]) T();
				this->positioner (this->buffer[i], i);
			}
		}

		//this->buffer[index] = value;
		this->put_item (index, value);
		if (index > this->count) this->count = index + 1;
		else this->count++;

		return index;
	}

	qse_size_t update (qse_size_t index, const T& value)
	{
		QSE_ASSERT (index < this->count);
		this->buffer[index] = value;
		this->positioner (this->buffer[index], index);
		return index;
	}

	qse_size_t upsert (qse_size_t index, const T& value)
	{
		if (index < this->count)
			return this->update (index, value);
		else
			return this->insert (index, value);
	}

	qse_size_t ensert (qse_size_t index, const T& value)
	{
		if (index < this->count)
			return index; // no update
		else
			return this->insert (index, value);
	}

	void remove (qse_size_t index)
	{
		this->remove (index, index);
	}

	void remove (qse_size_t from_index, qse_size_t to_index)
	{
		QSE_ASSERT (from_index < this->count);
		QSE_ASSERT (to_index < this->count);

		qse_size_t j = from_index;
		qse_size_t i = to_index + 1;

		// replace deleted elements by surviving elements at the back
		while (i < this->count) 
		{
			// which is better? 
			//  1. destruct followed by copy construct
			//  2. operator assignment.

			// 1. destruct followed by copy construct
			//this->positioner (this->buffer[j], INVALID_INDEX);
			//this->buffer[j].~T();
			//new((QSE::Mmgr*)QSE_NULL, &this->buffer[j]) T(this->buffer[i]);
			//this->positioner (this->buffer[j], j);

			// 2. operator assignment
			this->buffer[j] = this->buffer[i];
			this->positioner (this->buffer[j], j);

			j++; i++;
		}

		// call the destructor of deleted elements.
		while (j < this->count)
		{
			this->positioner (this->buffer[j], INVALID_INDEX);
			this->buffer[j].~T ();
			j++;
		}

		// recalculate the number of elements 
		this->count -= to_index - from_index + 1;
	}

#if 0
	/// \return the number of items deleted.
	int removeByValue (const T& value)
	{
		qse_size_t index = this->findFirstIndex (value);
		if (index == INVALID_INDEX) return 0;
		this->remove (index);
		return 1; 
	}

	qse_size_t removeAllByValue (const T& value)
	{
		qse_size_t cnt = 0;
		while (this->removeByValue(value) > 0) cnt++;
		return cnt;
	}
#endif

public:
	void clear (bool purge_buffer = false)
	{
		this->clear_all_items ();

		if (purge_buffer && this->buffer)
		{
			QSE_ASSERT (this->count <= 0);
			QSE_ASSERT (this->capacity > 0);

			::operator delete (this->buffer, this->getMmgr());
			this->capacity = 0;
			this->buffer = QSE_NULL;

			QSE_ASSERT (this->capacity == 0);
		}
	}

	void setSize (qse_size_t size)
	{
		if (size < this->count)
		{
			for (qse_size_t i = size; i < this->count; ++i)
			{
				// call the destructor of the items 
				this->positioner (this->buffer[i], INVALID_INDEX);
				this->buffer[i].~T ();
			}

			this->count = size;
		}
		else if (size > this->count)
		{
			if (size > this->capacity) this->setCapacity (size);
			for (qse_size_t i = this->count; i < size; ++i)
			{
				// use the default contructor to set the value.
				new((QSE::Mmgr*)QSE_NULL, &this->buffer[i]) T();
				this->positioner (this->buffer[i], i);
			}

			this->count = size;
		}
	}

	void setCapacity (qse_size_t capa)
	{
		if (capa == this->capacity) return;

		if (capa <= 0) 
		{
			this->clear (true);
		}
		else if (this->buffer)
		{
			QSE_ASSERT (this->capacity > 0);

			qse_size_t cnt = this->count;
			if (cnt > capa) cnt = capa;

			T* tmp = this->clone_buffer (this->buffer, capa, cnt);
			this->clear (true);
			this->buffer = tmp;
			this->capacity = capa;
			this->count = cnt;
		}
		else
		{
			QSE_ASSERT (this->capacity <= 0);
			QSE_ASSERT (this->count <= 0);

			this->buffer = (T*)::operator new (capa * QSE_SIZEOF(*this->buffer), this->getMmgr());
			this->capacity = capa;
		}
	}

	// The compact() function 
	void compact ()
	{
		this->setCapacity (this->size);
	}

#if 0
	qse_size_t findFirstIndex (const T& value) const
	{
		for (qse_size_t i = 0; i < this->count; i++) 
		{
			if (this->is_equal (this->buffer[i], value)) return i;
		}
		return INVALID_INDEX;
	}
	
	qse_size_t findFirstIndex (const T& value, qse_size_t index) const
	{
		for (qse_size_t i = index; i < this->count; i++) 
		{
			if (this->is_equal (this->buffer[i], value)) return i;
		}
		return INVALID_INDEX;
	}

	qse_size_t findLastIndex (const T& value) const
	{
		for (qse_size_t i = this->count; i > 0; ) 
		{
			if (this->is_equal (this->buffer[--i], value)) return i;
		}
		return INVALID_INDEX;
	}
	
	qse_size_t findLastIndex (const T& value, qse_size_t index) const
	{
		for (qse_size_t i = index + 1; i > 0; ) 
		{
			if (this->is_equal (this->buffer[--i], value)) return i;
		}
		return INVALID_INDEX;
	}
#endif

	
	void rotate (int dir, qse_size_t n)
	{
		qse_size_t first, last, cnt, index, nk;
		T c;

		if (dir == 0) return;
		if ((n %= this->count) == 0) return;

		if (dir > 0) n = this->count - n;
		first = 0; nk = this->count - n; cnt = 0; 

		while (cnt < n) 
		{
			last = first + nk;
			index = first;
			c = this->buffer[first];
			while (1) 
			{
				cnt++;
				while (index < nk) 
				{
					this->buffer[index] = this->buffer[index + n];
					this->positioner (this->buffer[index], index);
					index += n;
				}
				if (index == last) break;

				this->buffer[index] = this->buffer[index - nk];
				this->positioner (this->buffer[index], index);
				index -= nk;
			}

			this->buffer[last] = c;
			this->positioner (this->buffer[last], last);
			first++;
		}
	}

protected:
	POSITIONER positioner;
	RESIZER    resizer;

	qse_size_t count;
	qse_size_t capacity;
	T*         buffer;
};

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif

