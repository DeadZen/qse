/*
 * $Id$
 *
    Copyright 2006-2014 Chung, Hyung-Hwan.
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

#ifndef _QSE_AWK_STDAWK_HPP_
#define _QSE_AWK_STDAWK_HPP_

#include <qse/awk/Awk.hpp>
#include <qse/cmn/StdMmgr.hpp>
#include <qse/cmn/time.h>

/// @file
/// Standard AWK Interpreter

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
////////////////////////////////

///
/// The StdAwk class provides an easier-to-use interface by overriding 
/// primitive methods, and implementing the file handler, the pipe handler, 
/// and common intrinsic functions.
///
class QSE_EXPORT StdAwk: public Awk
{
public:
	///
	/// The SourceFile class implements script I/O from and to a file.
	///
	class QSE_EXPORT SourceFile: public Source 
	{
	public:
		SourceFile (const char_t* name, qse_cmgr_t* cmgr = QSE_NULL): 
			name (name), cmgr (cmgr)
		{
			dir.ptr = QSE_NULL; dir.len = 0; 
		}

		int open (Data& io);
		int close (Data& io);
		ssize_t read (Data& io, char_t* buf, size_t len);
		ssize_t write (Data& io, const char_t* buf, size_t len);

	protected:
		const char_t* name;
		qse_cstr_t dir;
		qse_cmgr_t* cmgr;
	};

	///
	/// The SourceString class implements script input from a string. 
	/// Deparsing is not supported.
	///
	class QSE_EXPORT SourceString: public Source
	{
	public:
		SourceString (const char_t* str): str (str) {}

		int open (Data& io);
		int close (Data& io);
		ssize_t read (Data& io, char_t* buf, size_t len);
		ssize_t write (Data& io, const char_t* buf, size_t len);

	protected:
		const char_t* str;
		const char_t* ptr;
	};
        
	StdAwk (Mmgr* mmgr = StdMmgr::getDFL()):
		Awk (mmgr), console_cmgr (QSE_NULL) 
	{
	}

	int open ();
	void close ();
	Run* parse (Source& in, Source& out);

	/// The setConsoleCmgr() function sets the encoding type of 
	/// the console streams. They include both the input and the output
	/// streams. It provides no way to specify a different encoding
	/// type for the input and the output stream.
	void setConsoleCmgr (const qse_cmgr_t* cmgr);

	/// The getConsoleCmgr() function returns the current encoding
	/// type set for the console streams.
	const qse_cmgr_t* getConsoleCmgr () const;

	/// The addConsoleOutput() function adds a file to form an
	/// output console stream.
	int addConsoleOutput (const char_t* arg, size_t len);
	int addConsoleOutput (const char_t* arg);

	void clearConsoleOutputs ();

protected:
	int make_additional_globals (Run* run);
	int build_argcv (Run* run);
	int build_environ (Run* run);
	int __build_environ (Run* run, void* envptr);

	// intrinsic functions 
	qse_cmgr_t* getcmgr (const char_t* ioname);

	int setioattr (Run& run, Value& ret, Value* args, size_t nargs,
		const char_t* name, size_t len);
	int getioattr (Run& run, Value& ret, Value* args, size_t nargs,
		const char_t* name, size_t len);

	// pipe io handlers 
	int openPipe (Pipe& io);
	int closePipe (Pipe& io);
	ssize_t readPipe  (Pipe& io, char_t* buf, size_t len);
	ssize_t writePipe (Pipe& io, const char_t* buf, size_t len);
	int flushPipe (Pipe& io);

	// file io handlers 
	int openFile (File& io);
	int closeFile (File& io);
	ssize_t readFile (File& io, char_t* buf, size_t len);
	ssize_t writeFile (File& io, const char_t* buf, size_t len);
	int flushFile (File& io);

	// console io handlers 
	int openConsole (Console& io);
	int closeConsole (Console& io);
	ssize_t readConsole (Console& io, char_t* buf, size_t len);
	ssize_t writeConsole (Console& io, const char_t* buf, size_t len);
	int flushConsole (Console& io);
	int nextConsole (Console& io);

	// primitive handlers 
	void* allocMem   (size_t n);
	void* reallocMem (void* ptr, size_t n);
	void  freeMem    (void* ptr);

	flt_t pow (flt_t x, flt_t y);
	flt_t mod (flt_t x, flt_t y);

	void* modopen (const mod_spec_t* spec);
	void  modclose (void* handle);
	void* modsym (void* handle, const char_t* name);

protected:
	qse_htb_t cmgrtab;
	bool cmgrtab_inited;

	qse_cmgr_t* console_cmgr;

	// global variables 
	int gbl_argc;
	int gbl_argv;
	int gbl_environ;

	// standard input console - reuse runarg 
	size_t runarg_index;
	size_t runarg_count;

	// standard output console 
	xstrs_t ofile;
	size_t ofile_index;
	size_t ofile_count;

public:
	struct ioattr_t
	{
		qse_cmgr_t* cmgr;
		char_t cmgr_name[64]; // i assume that the cmgr name never exceeds this length.
		qse_ntime_t tmout[4];

		ioattr_t (): cmgr (QSE_NULL)
		{
			this->cmgr_name[0] = QSE_T('\0');
			for (size_t i = 0; i < QSE_COUNTOF(this->tmout); i++)
			{
				this->tmout[i].sec = -999;
				this->tmout[i].nsec = 0;
			}
		}
	};

	static ioattr_t default_ioattr;

protected:
	ioattr_t* get_ioattr (const char_t* ptr, size_t len);
	ioattr_t* find_or_make_ioattr (const char_t* ptr, size_t len);


private:
	int open_console_in (Console& io);
	int open_console_out (Console& io);

	int open_pio (Pipe& io);
	int open_nwio (Pipe& io, int flags, void* nwad);
};

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif


