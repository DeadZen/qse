/*
 * $Id: StdAwk.cpp,v 1.14 2007/05/16 14:44:13 bacon Exp $
 */

#include <ase/awk/StdAwk.hpp>
#include <ase/cmn/str.h>
#include <ase/utl/stdio.h>
#include <ase/utl/ctype.h>

#include <stdlib.h>
#include <math.h>
#include <time.h>

#ifdef _WIN32
#include <tchar.h>
#else
#include <wchar.h>
#endif

namespace ASE
{

	StdAwk::StdAwk ()
	{
		seed = ::time(NULL);
		::srand (seed);
	}

	int StdAwk::open ()
	{
		int n = Awk::open ();
		if (n == 0)
		{
			int opt = 
				ASE_AWK_IMPLICIT |
				ASE_AWK_EXPLICIT | 
				ASE_AWK_UNIQUEFN | 
				ASE_AWK_IDIV |
				ASE_AWK_SHADING | 
				ASE_AWK_SHIFT | 
				ASE_AWK_EXTIO | 
				ASE_AWK_BLOCKLESS | 
				ASE_AWK_STRBASEONE | 
				ASE_AWK_STRIPSPACES | 
				ASE_AWK_NEXTOFILE |
				ASE_AWK_ARGSTOMAIN;
			ase_awk_setoption (awk, opt);


			addFunction (ASE_T("sin"), 1, 1, (FunctionHandler)&StdAwk::sin);
			addFunction (ASE_T("cos"), 1, 1, (FunctionHandler)&StdAwk::cos);
			addFunction (ASE_T("tan"), 1, 1, (FunctionHandler)&StdAwk::tan);
			addFunction (ASE_T("atan2"), 2, 2, (FunctionHandler)&StdAwk::atan2);
			addFunction (ASE_T("log"), 1, 1, (FunctionHandler)&StdAwk::log);
			addFunction (ASE_T("exp"), 1, 1, (FunctionHandler)&StdAwk::exp);
			addFunction (ASE_T("sqrt"), 1, 1, (FunctionHandler)&StdAwk::sqrt);
			addFunction (ASE_T("int"), 1, 1, (FunctionHandler)&StdAwk::fnint);
			addFunction (ASE_T("rand"), 0, 0, (FunctionHandler)&StdAwk::rand);
			addFunction (ASE_T("srand"), 1, 1, (FunctionHandler)&StdAwk::srand);
			addFunction (ASE_T("systime"), 0, 0, (FunctionHandler)&StdAwk::systime);
			addFunction (ASE_T("strftime"), 0, 2, (FunctionHandler)&StdAwk::strftime);
			addFunction (ASE_T("strfgmtime"), 0, 2, (FunctionHandler)&StdAwk::strfgmtime);
			addFunction (ASE_T("system"), 1, 1, (FunctionHandler)&StdAwk::system);
		}

		return n;
	}

	int StdAwk::sin (Return* ret, const Argument* args, size_t nargs)
	{
		return ret->set ((real_t)::sin(args[0].toReal()));
	}

	int StdAwk::cos (Return* ret, const Argument* args, size_t nargs)
	{
		return ret->set ((real_t)::cos(args[0].toReal()));
	}

	int StdAwk::tan (Return* ret, const Argument* args, size_t nargs)
	{
		return ret->set ((real_t)::tan(args[0].toReal()));
	}

	int StdAwk::atan2 (Return* ret, const Argument* args, size_t nargs)
	{
		return ret->set ((real_t)::atan2(args[0].toReal(), args[1].toReal()));
	}

	int StdAwk::log (Return* ret, const Argument* args, size_t nargs)
	{
		return ret->set ((real_t)::log(args[0].toReal()));
	}

	int StdAwk::exp (Return* ret, const Argument* args, size_t nargs)
	{
		return ret->set ((real_t)::exp(args[0].toReal()));
	}

	int StdAwk::sqrt (Return* ret, const Argument* args, size_t nargs)
	{
		return ret->set ((real_t)::sqrt(args[0].toReal()));
	}

	int StdAwk::fnint (Return* ret, const Argument* args, size_t nargs)
	{
		return ret->set (args[0].toInt());
	}

	int StdAwk::rand (Return* ret, const Argument* args, size_t nargs)
	{
		return ret->set ((long_t)::rand());
	}

	int StdAwk::srand (Return* ret, const Argument* args, size_t nargs)
	{
		unsigned int prevSeed = seed;
		seed = (unsigned int)args[0].toInt();
		::srand (seed);
		return ret->set ((long_t)prevSeed);
	}

	int StdAwk::systime (Return* ret, const Argument* args, size_t nargs)
	{
		return ret->set ((long_t)::time(NULL));
	}

	int StdAwk::strftime (Return* ret, const Argument* args, size_t nargs)
	{
		const char_t* fmt;
		size_t fln;
	       
		fmt = (nargs < 1)? ASE_T("%c"): args[0].toStr(&fln);
		time_t t = (nargs < 2)? ::time(NULL): (time_t)args[1].toInt();

		char_t buf[128]; 
		struct tm* tm;
	#ifdef _WIN32
		tm = localtime (&t);
	#else
		struct tm tmb;
		tm = localtime_r (&t, &tmb);
	#endif

	#ifdef ASE_CHAR_IS_MCHAR
		size_t len = strftime (buf, ASE_COUNTOF(buf), fmt, tm);
	#else
		size_t len = wcsftime (buf, ASE_COUNTOF(buf), fmt, tm);
	#endif

		return ret->set (buf, len);	
	}

	int StdAwk::strfgmtime (Return* ret, const Argument* args, size_t nargs)
	{
		const char_t* fmt;
		size_t fln;
	       
		fmt = (nargs < 1)? ASE_T("%c"): args[0].toStr(&fln);
		time_t t = (nargs < 2)? ::time(NULL): (time_t)args[1].toInt();

		char_t buf[128]; 
		struct tm* tm;
	#ifdef _WIN32
		tm = gmtime (&t);
	#else
		struct tm tmb;
		tm = gmtime_r (&t, &tmb);
	#endif

	#ifdef ASE_CHAR_IS_MCHAR
		size_t len = strftime (buf, ASE_COUNTOF(buf), fmt, tm);
	#else
		size_t len = wcsftime (buf, ASE_COUNTOF(buf), fmt, tm);
	#endif

		return ret->set (buf, len);	
	}

	int StdAwk::system (Return* ret, const Argument* args, size_t nargs)
	{
		size_t len;
		const char_t* ptr = args[0].toStr(&len);

	#ifdef _WIN32
		return ret->set ((long_t)::_tsystem(ptr));
	#elif defined(ASE_CHAR_IS_MCHAR)
		return ret->set ((long_t)::system(ptr));
	#else
		char* mbs = (char*)ase_awk_malloc (ret->getAwk(), len*5+1);
		if (mbs == ASE_NULL) return -1;

		::size_t mbl = ::wcstombs (mbs, ptr, len*5);
		if (mbl == (::size_t)-1) return -1;
		mbs[mbl] = '\0';
		int n =  ret->set ((long_t)::system(mbs));

		ase_awk_free (ret->getAwk(), mbs);
		return n;
	#endif
	}

	int StdAwk::openPipe (Pipe& io) 
	{ 
		Awk::Pipe::Mode mode = io.getMode();
		FILE* fp = NULL;

		switch (mode)
		{
			case Awk::Pipe::READ:
				fp = ase_popen (io.getName(), ASE_T("r"));
				break;
			case Awk::Pipe::WRITE:
				fp = ase_popen (io.getName(), ASE_T("w"));
				break;
		}

		if (fp == NULL) return -1;

		io.setHandle (fp);
		return 1;
	}

	int StdAwk::closePipe (Pipe& io) 
	{
		fclose ((FILE*)io.getHandle());
		return 0; 
	}

	StdAwk::ssize_t StdAwk::readPipe (Pipe& io, char_t* buf, size_t len) 
	{ 
		// TODO: change this implementation...
		FILE* fp = (FILE*)io.getHandle();
		if (ase_fgets (buf, len, fp) == ASE_NULL)
		{
			if (ferror(fp)) return -1;
			return 0;
		}

		return ase_strlen(buf);
	}

	
	StdAwk::ssize_t StdAwk::writePipe (Pipe& io, char_t* buf, size_t len) 
	{ 
		FILE* fp = (FILE*)io.getHandle();
		size_t left = len;

		while (left > 0)
		{
			if (*buf == ASE_T('\0')) 
			{
			#if defined(ASE_CHAR_IS_WCHAR) && defined(__linux)
				if (fputc ('\0', fp) == EOF)
			#else
				if (ase_fputc (*buf, fp) == ASE_CHAR_EOF) 
			#endif
				{
					return -1;
				}
				left -= 1; buf += 1;
			}
			else
			{
			#if defined(ASE_CHAR_IS_WCHAR) && defined(__linux)
			// fwprintf seems to return an error with the file
			// pointer opened by popen, as of this writing. 
			// anyway, hopefully the following replacement 
			// will work all the way.
				int n = fprintf (fp, "%.*ls", left, buf);
				if (n >= 0)
				{
					size_t x;
					for (x = 0; x < left; x++)
					{
						if (buf[x] == ASE_T('\0')) break;
					}
					n = x;
				}
			#else
				int n = ase_fprintf (fp, ASE_T("%.*s"), left, buf);
			#endif

				if (n < 0 || n > left) return -1;
				left -= n; buf += n;
			}
		}

		return len;
	}

	int StdAwk::flushPipe (Pipe& io) 
	{ 
		return ::fflush ((FILE*)io.getHandle()); 
	}

	// file io handlers 
	int StdAwk::openFile (File& io) 
	{ 
		Awk::File::Mode mode = io.getMode();
		FILE* fp = NULL;

		switch (mode)
		{
			case Awk::File::READ:
				fp = ase_fopen (io.getName(), ASE_T("r"));
				break;
			case Awk::File::WRITE:
				fp = ase_fopen (io.getName(), ASE_T("w"));
				break;
			case Awk::File::APPEND:
				fp = ase_fopen (io.getName(), ASE_T("a"));
				break;
		}

		if (fp == NULL) return -1;

		io.setHandle (fp);
		return 1;
	}

	int StdAwk::closeFile (File& io) 
	{ 
		fclose ((FILE*)io.getHandle());
		return 0; 
	}

	StdAwk::ssize_t StdAwk::readFile (File& io, char_t* buf, size_t len) 
	{
		// TODO: replace ase_fgets by something else
		FILE* fp = (FILE*)io.getHandle();
		if (ase_fgets (buf, len, fp) == ASE_NULL)
		{
			if (ferror(fp)) return -1;
			return 0;
		}

		return ase_strlen(buf);
	}

	StdAwk::ssize_t StdAwk::writeFile (File& io, char_t* buf, size_t len)
	{
		FILE* fp = (FILE*)io.getHandle();
		size_t left = len;

		while (left > 0)
		{
			if (*buf == ASE_T('\0')) 
			{
				if (ase_fputc (*buf, fp) == ASE_CHAR_EOF) return -1;
				left -= 1; buf += 1;
			}
			else
			{
				int n = ase_fprintf (fp, ASE_T("%.*s"), left, buf);
				if (n < 0 || n > left) return -1;
				left -= n; buf += n;
			}
		}

		return len;
	}

	int StdAwk::flushFile (File& io) 
	{ 
		return ::fflush ((FILE*)io.getHandle()); 
	}

	// memory allocation primitives
	void* StdAwk::allocMem (size_t n) 
	{ 
		return ::malloc (n); 
	}

	void* StdAwk::reallocMem (void* ptr, size_t n) 
	{ 
		return ::realloc (ptr, n); 
	}

	void  StdAwk::freeMem (void* ptr) 
	{ 
		::free (ptr); 
	}

	// character class primitives
	StdAwk::bool_t StdAwk::isUpper (cint_t c)
	{ 
		return ase_isupper (c); 
	}

	StdAwk::bool_t StdAwk::isLower (cint_t c) 
	{
		return ase_islower (c); 
	}

	StdAwk::bool_t StdAwk::isAlpha (cint_t c) 
	{ 
		return ase_isalpha (c); 
	}

	StdAwk::bool_t StdAwk::isDigit (cint_t c) 
	{
		return ase_isdigit (c); 
	}

	StdAwk::bool_t StdAwk::isXdigit (cint_t c)
	{
		return ase_isxdigit (c); 
	}

	StdAwk::bool_t StdAwk::isAlnum (cint_t c) 
	{
		return ase_isalnum (c); 
	}

	StdAwk::bool_t StdAwk::isSpace (cint_t c) 
	{
		return ase_isspace (c); 
	}

	StdAwk::bool_t StdAwk::isPrint (cint_t c) 
	{
		return ase_isprint (c); 
	}

	StdAwk::bool_t StdAwk::isGraph (cint_t c) 
	{ 
		return ase_isgraph (c); 
	}

	StdAwk::bool_t StdAwk::isCntrl (cint_t c) 
	{ 
		return ase_iscntrl (c); 
	}

	StdAwk::bool_t StdAwk::isPunct (cint_t c) 
	{ 
		return ase_ispunct (c); 
	}

	StdAwk::cint_t StdAwk::toUpper (cint_t c) 
	{ 
		return ase_toupper (c); 
	}

	StdAwk::cint_t StdAwk::toLower (cint_t c) 
	{ 
		return ase_tolower (c); 
	}

	// miscellaneous primitive
	StdAwk::real_t StdAwk::pow (real_t x, real_t y) 
	{ 
		return ::pow (x, y); 
	}

	int StdAwk::vsprintf (
		char_t* buf, size_t size, const char_t* fmt, va_list arg) 
	{
		return ase_vsprintf (buf, size, fmt, arg);
	}

	void StdAwk::vdprintf (const char_t* fmt, va_list arg) 
	{
		ase_vfprintf (stderr, fmt, arg);
	}

}
