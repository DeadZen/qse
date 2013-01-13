/*
 * $Id$
 *
    Copyright 2006-2012 Chung, Hyung-Hwan.
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

#ifndef _QSE_SED_STDSED_HPP_
#define _QSE_SED_STDSED_HPP_

#include <qse/sed/Sed.hpp>
#include <qse/cmn/StdMmgr.hpp>
#include <qse/cmn/str.h>

/** @file
 * This file defines easier-to-use stream editor classes providing standard
 * memory management and I/O handling.
 */

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

///
/// The StdSed class inherits the Sed class, implements a standard
/// I/O stream class, and sets the default memory manager.
///
class QSE_EXPORT StdSed: public Sed
{
public:
	StdSed (Mmgr* mmgr = StdMmgr::getDFL()): Sed (mmgr) {}

	///
	/// The FileStream class implements a stream over input
	/// and output files.
	///
	class QSE_EXPORT FileStream: public Stream
	{
	public:
		FileStream (const char_t* infile = QSE_NULL,
		            const char_t* outfile = QSE_NULL,
		            qse_cmgr_t* cmgr = QSE_NULL): 
			infile(infile), outfile(outfile), cmgr(cmgr) 
		{
		}

		int open (Data& io);
		int close (Data& io);
		ssize_t read (Data& io, char_t* buf, size_t len);
		ssize_t write (Data& io, const char_t* buf, size_t len);

	protected:
		const char_t* infile;
		const char_t* outfile;
		qse_cmgr_t*   cmgr;
	};

	///
	/// The StringStream class implements a stream over a string
	///
	class QSE_EXPORT StringStream: public Stream
	{
	public:
		StringStream (const char_t* in);
		StringStream (const char_t* in, size_t len);
		~StringStream ();

		int open (Data& io);
		int close (Data& io);
		ssize_t read (Data& io, char_t* buf, size_t len);
		ssize_t write (Data& io, const char_t* buf, size_t len);

		const char_t* getInput (size_t* len = QSE_NULL) const;
		const char_t* getOutput (size_t* len = QSE_NULL) const;

	protected:
		struct
		{
			const char_t* ptr; 
			const char_t* end; 
			const char_t* cur;
		} in;

		struct
		{
			bool inited;
			qse_str_t buf;
		} out;
	};
};

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif
