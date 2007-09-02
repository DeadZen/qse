/*
 * $Id: Awk.cpp,v 1.32 2007/08/26 14:33:38 bacon Exp $
 */

#include <ase/awk/StdAwk.hpp>
#include <ase/cmn/str.h>
#include <ase/utl/stdio.h>
#include <ase/utl/main.h>

#include <stdlib.h>
#include <math.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

class TestAwk: public ASE::StdAwk
{
public:
	TestAwk (): srcInName(ASE_NULL), srcOutName(ASE_NULL), 
	            numConInFiles(0), numConOutFiles(0)
	{
	#ifdef _WIN32
		heap = ASE_NULL;
	#endif
	}

	~TestAwk ()
	{
		close ();
	}

	int open ()
	{
	#ifdef _WIN32
		ASE_ASSERT (heap == ASE_NULL);
		heap = ::HeapCreate (0, 1000000, 1000000);
		if (heap == ASE_NULL) return -1;
	#endif

	#if defined(_MSC_VER) && (_MSC_VER<1400)
		int n = StdAwk::open ();
	#else
		int n = ASE::StdAwk::open ();
	#endif

		if (addFunction (ASE_T("sleep"), 1, 1,
			(FunctionHandler)&TestAwk::sleep) == -1)
		{
		#if defined(_MSC_VER) && (_MSC_VER<1400)
			StdAwk::close ();
		#else
			ASE::StdAwk::close ();
		#endif

		#ifdef _WIN32
			HeapDestroy (heap); 
			heap = ASE_NULL;
		#endif
			return -1;
		}

		return n;
	}

	void close ()
	{
	#if defined(_MSC_VER) && (_MSC_VER<1400)
		StdAwk::close ();
	#else
		ASE::StdAwk::close ();
	#endif

		numConInFiles = 0;
		numConOutFiles = 0;

	#ifdef _WIN32
		if (heap != ASE_NULL)
		{
			HeapDestroy (heap); 
			heap = ASE_NULL;
		}
	#endif
	}

	int sleep (Return* ret, const Argument* args, size_t nargs, 
		const char_t* name, size_t len)
	{
	#ifdef _WIN32
		::Sleep (args[0].toInt() * 1000);
		return ret->set ((long_t)0);
	#else
		return ret->set ((long_t)::sleep (args[0].toInt()));
	#endif
	}

	int addConsoleInput (const char_t* file)
	{
		if (numConInFiles < ASE_COUNTOF(conInFile))
		{
			conInFile[numConInFiles++] = file;
			return 0;
		}

		return -1;
	}

	int addConsoleOutput (const char_t* file)
	{
		if (numConOutFiles < ASE_COUNTOF(conOutFile))
		{
			conOutFile[numConOutFiles++] = file;
			return 0;
		}

		return -1;
	}

	int parse (const char_t* in, const char_t* out)
	{
		srcInName = in;
		srcOutName = out;
	#if defined(_MSC_VER) && (_MSC_VER<1400)
		return StdAwk::parse ();
	#else
		return ASE::StdAwk::parse ();
	#endif
	}

protected:

	void onRunStart (const Run& run)
	{
		ase_printf (ASE_T("*** awk run started ***\n"));
	}

	void onRunEnd (const Run& run)
	{
		ErrorCode err = run.getErrorCode();

		if (err != ERR_NOERR)
		{
			ase_fprintf (stderr, ASE_T("cannot run: LINE[%d] %s\n"), 
				run.getErrorLine(), run.getErrorMessage());
		}

		ase_printf (ASE_T("*** awk run ended ***\n"));
	}

	void onRunReturn (const Run& run, const Argument& ret)
	{
		size_t len;
		const char_t* ptr = ret.toStr (&len);
		ase_printf (ASE_T("*** return [%.*s] ***\n"), len, ptr);
	}

	int openSource (Source& io)
	{
		Source::Mode mode = io.getMode();
		FILE* fp = ASE_NULL;

		if (mode == Source::READ)
		{
			if (srcInName == ASE_NULL) 
			{
				io.setHandle (stdin);
				return 0;
			}

			if (srcInName[0] == ASE_T('\0')) fp = stdin;
			else fp = ase_fopen (srcInName, ASE_T("r"));
		}
		else if (mode == Source::WRITE)
		{
			if (srcOutName == ASE_NULL)
			{
				io.setHandle (stdout);
				return 0;
			}

			if (srcOutName[0] == ASE_T('\0')) fp = stdout;
			else fp = ase_fopen (srcOutName, ASE_T("w"));
		}

		if (fp == ASE_NULL) return -1;
		io.setHandle (fp);
		return 1;
	}

	int closeSource (Source& io)
	{
		Source::Mode mode = io.getMode();
		FILE* fp = (FILE*)io.getHandle();
		if (fp == stdout || fp == stderr) fflush (fp);
		if (fp != stdin && fp != stdout && fp != stderr) fclose (fp);
		io.setHandle (ASE_NULL);
		return 0;
	}

	ssize_t readSource (Source& io, char_t* buf, size_t len)
	{
		FILE* fp = (FILE*)io.getHandle();
		ssize_t n = 0;

		while (n < (ssize_t)len)
		{
			ase_cint_t c = ase_fgetc (fp);
			if (c == ASE_CHAR_EOF) break;

			buf[n++] = c;
			if (c == ASE_T('\n')) break;
		}

		return n;
	}

	ssize_t writeSource (Source& io, char_t* buf, size_t len)
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
				if (n < 0) return -1;
				left -= n; buf += n;
			}
		}

		return len;
	}

	// console io handlers 
	int openConsole (Console& io) 
	{ 
	#if defined(_MSC_VER) && (_MSC_VER<1400)
		StdAwk::Console::Mode mode = io.getMode();
	#else
		ASE::StdAwk::Console::Mode mode = io.getMode();
	#endif
		FILE* fp = ASE_NULL;
		const char_t* fn = ASE_NULL;

		switch (mode)
		{
		#if defined(_MSC_VER) && (_MSC_VER<1400)
			case StdAwk::Console::READ:
		#else
			case ASE::StdAwk::Console::READ:
		#endif
				if (numConInFiles == 0) fp = stdin;
				else
				{
					fn = conInFile[0];
					fp = ase_fopen (fn, ASE_T("r"));
				}
				break;

		#if defined(_MSC_VER) && (_MSC_VER<1400)
			case StdAwk::Console::WRITE:
		#else
			case ASE::StdAwk::Console::WRITE:
		#endif
				if (numConOutFiles == 0) fp = stdout;
				else
				{
					fn = conOutFile[0];
					fp = ase_fopen (fn, ASE_T("w"));
				}
				break;
		}

		if (fp == NULL) return -1;

		ConTrack* t = (ConTrack*) 
			ase_awk_malloc (awk, ASE_SIZEOF(ConTrack));
		if (t == ASE_NULL)
		{
			if (fp != stdin && fp != stdout) fclose (fp);
			return -1;
		}

		t->handle = fp;
		t->nextConIdx = 1;

		if (fn != ASE_NULL) 
		{
			if (io.setFileName (fn) == -1)
			{
				if (fp != stdin && fp != stdout) fclose (fp);
				ase_awk_free (awk, t);
				return -1;
			}
		}

		io.setHandle (t);
		return 1;
	}

	int closeConsole (Console& io) 
	{ 
		ConTrack* t = (ConTrack*)io.getHandle();
		FILE* fp = t->handle;

		if (fp == stdout || fp == stderr) fflush (fp);
		if (fp != stdin && fp != stdout && fp != stderr) fclose (fp);

		ase_awk_free (awk, t);
		return 0;
	}

	ssize_t readConsole (Console& io, char_t* buf, size_t len) 
	{
		ConTrack* t = (ConTrack*)io.getHandle();
		FILE* fp = t->handle;
		ssize_t n = 0;

		while (n < (ssize_t)len)
		{
			ase_cint_t c = ase_fgetc (fp);
			if (c == ASE_CHAR_EOF) break;

			buf[n++] = c;
			if (c == ASE_T('\n')) break;
		}

		return n;
	}

	ssize_t writeConsole (Console& io, char_t* buf, size_t len) 
	{
		ConTrack* t = (ConTrack*)io.getHandle();
		FILE* fp = t->handle;
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
				if (n < 0) return -1;
				left -= n; buf += n;
			}
		}

		return len;
	}

	int flushConsole (Console& io) 
	{ 
		ConTrack* t = (ConTrack*)io.getHandle();
		FILE* fp = t->handle;
		return ::fflush (fp);
	}

	int nextConsole (Console& io) 
	{ 
	#if defined(_MSC_VER) && (_MSC_VER<1400)
		StdAwk::Console::Mode mode = io.getMode();
	#else
		ASE::StdAwk::Console::Mode mode = io.getMode();
	#endif
		ConTrack* t = (ConTrack*)io.getHandle();
		FILE* ofp = t->handle;
		FILE* nfp = ASE_NULL;
		const char_t* fn = ASE_NULL;

		switch (mode)
		{
		#if defined(_MSC_VER) && (_MSC_VER<1400)
			case StdAwk::Console::READ:
		#else
			case ASE::StdAwk::Console::READ:
		#endif
				if (t->nextConIdx >= numConInFiles) return 0;
				fn = conInFile[t->nextConIdx];
				nfp = ase_fopen (fn, ASE_T("r"));
				break;

		#if defined(_MSC_VER) && (_MSC_VER<1400)
			case StdAwk::Console::WRITE:
		#else
			case ASE::StdAwk::Console::WRITE:
		#endif
				if (t->nextConIdx >= numConOutFiles) return 0;
				fn = conOutFile[t->nextConIdx];
				nfp = ase_fopen (fn, ASE_T("w"));
				break;
		}

		if (nfp == ASE_NULL) return -1;

		if (fn != ASE_NULL)
		{
			if (io.setFileName (fn) == -1)
			{
				fclose (nfp);
				return -1;
			}
		}

		fclose (ofp);

		t->nextConIdx++;
		t->handle = nfp;

		return 1;
	}

	void* allocMem (size_t n) 
	{ 
	#ifdef _WIN32
		return ::HeapAlloc (heap, 0, n);
	#else
		return ::malloc (n);
	#endif
	}

	void* reallocMem (void* ptr, size_t n) 
	{ 
	#ifdef _WIN32
		if (ptr == NULL)
			return ::HeapAlloc (heap, 0, n);
		else
			return ::HeapReAlloc (heap, 0, ptr, n);
	#else
		return ::realloc (ptr, n);
	#endif
	}

	void freeMem (void* ptr) 
	{ 
	#ifdef _WIN32
		::HeapFree (heap, 0, ptr);
	#else
		::free (ptr);
	#endif
	}

private:
	const char_t* srcInName;
	const char_t* srcOutName;
	
	struct ConTrack
	{
		FILE* handle;
		size_t nextConIdx;
	};

	size_t        numConInFiles;
	const char_t* conInFile[128];

	size_t        numConOutFiles;
	const char_t* conOutFile[128];

#ifdef _WIN32
	void* heap;
#endif
};

#ifndef NDEBUG
void ase_assert_abort (void)
{
	abort ();
}

void ase_assert_printf (const ase_char_t* fmt, ...)
{
	va_list ap;
#ifdef _WIN32
	int n;
	ase_char_t buf[1024];
#endif

	va_start (ap, fmt);
#if defined(_WIN32)
	n = _vsntprintf (buf, ASE_COUNTOF(buf), fmt, ap);
	if (n < 0) buf[ASE_COUNTOF(buf)-1] = ASE_T('\0');

	#if defined(_MSC_VER) && (_MSC_VER<1400)
	MessageBox (NULL, buf, 
		ASE_T("Assertion Failure"), MB_OK|MB_ICONERROR);
	#else
	MessageBox (NULL, buf, 
		ASE_T("\uB2DD\uAE30\uB9AC \uC870\uB610"), MB_OK|MB_ICONERROR);
	#endif
#else
	ase_vprintf (fmt, ap);
#endif
	va_end (ap);
}
#endif

static void print_error (const ase_char_t* msg)
{
	ase_printf (ASE_T("Error: %s\n"), msg);
}

static void print_usage (const ase_char_t* argv0)
{
	const ase_char_t* base;
	
	base = ase_strrchr(argv0, ASE_T('/'));
	if (base == ASE_NULL) base = ase_strrchr(argv0, ASE_T('\\'));
	if (base == ASE_NULL) base = argv0; else base++;

	ase_printf (ASE_T("Usage: %s [-m main] [-si file]? [-so file]? [-ci file]* [-co file]* [-a arg]* [-w o:n]* \n"), base);
	ase_printf (ASE_T("    -m  main  Specify the main function name\n"));
	ase_printf (ASE_T("    -si file  Specify the input source file\n"));
	ase_printf (ASE_T("              The source code is read from stdin when it is not specified\n"));
	ase_printf (ASE_T("    -so file  Specify the output source file\n"));
	ase_printf (ASE_T("              The deparsed code is not output when is it not specified\n"));
	ase_printf (ASE_T("    -ci file  Specify the input console file\n"));
	ase_printf (ASE_T("    -co file  Specify the output console file\n"));
	ase_printf (ASE_T("    -a  str   Specify an argument\n"));
	ase_printf (ASE_T("    -w  o:n   Specify an old and new word pair\n"));
	ase_printf (ASE_T("              o - an original word\n"));
	ase_printf (ASE_T("              n - the new word to replace the original\n"));
	ase_printf (ASE_T("    -ns       Don't strip whitespaces\n"));
	ase_printf (ASE_T("              The STRIPSPACES option is truned off\n"));
}

int awk_main (int argc, ase_char_t* argv[])
{
	TestAwk awk;
	int mode = 0;
	const ase_char_t* mainfn = NULL;
	const ase_char_t* srcin = ASE_T("");
	const ase_char_t* srcout = NULL;
	const ase_char_t* args[256];
	ase_size_t nargs = 0;
	ase_size_t nsrcins = 0;
	ase_size_t nsrcouts = 0;

	if (awk.open() == -1)
	{
		ase_fprintf (stderr, ASE_T("cannot open awk\n"));
		return -1;
	}

	for (int i = 1; i < argc; i++)
	{
		if (mode == 0)
		{
			if (ase_strcmp(argv[i], ASE_T("-si")) == 0) mode = 1;
			else if (ase_strcmp(argv[i], ASE_T("-so")) == 0) mode = 2;
			else if (ase_strcmp(argv[i], ASE_T("-ci")) == 0) mode = 3;
			else if (ase_strcmp(argv[i], ASE_T("-co")) == 0) mode = 4;
			else if (ase_strcmp(argv[i], ASE_T("-a")) == 0) mode = 5;
			else if (ase_strcmp(argv[i], ASE_T("-m")) == 0) mode = 6;
			else if (ase_strcmp(argv[i], ASE_T("-w")) == 0) mode = 7;
			else if (ase_strcmp(argv[i], ASE_T("-ns")) == 0) 
			{
				awk.setOption (awk.getOption () & ~TestAwk::OPT_STRIPSPACES);
			}
			else 
			{
				print_usage (argv[0]);
				return -1;
			}
		}
		else
		{
			if (argv[i][0] == ASE_T('-'))
			{
				print_usage (argv[0]);
				return -1;
			}

			if (mode == 1) // source input 
			{
				if (nsrcins != 0) 
				{
					print_usage (argv[0]);
					return -1;
				}
	
				srcin = argv[i];
				nsrcins++;
				mode = 0;
			}
			else if (mode == 2) // source output 
			{
				if (nsrcouts != 0) 
				{
					print_usage (argv[0]);
					return -1;
				}
	
				srcout = argv[i];
				nsrcouts++;
				mode = 0;
			}
			else if (mode == 3) // console input
			{
				if (awk.addConsoleInput (argv[i]) == -1)
				{
					print_error (ASE_T("too many console inputs"));
					return -1;
				}

				mode = 0;
			}
			else if (mode == 4) // console output
			{
				if (awk.addConsoleOutput (argv[i]) == -1)
				{
					print_error (ASE_T("too many console outputs"));
					return -1;
				}

				mode = 0;
			}
			else if (mode == 5) // argument mode
			{
				if (nargs >= ASE_COUNTOF(args))
				{
					print_usage (argv[0]);
					return -1;
				}

				args[nargs++] = argv[i];
				mode = 0;
			}
			else if (mode == 6) // entry point
			{
				if (mainfn != NULL) 
				{
					print_usage (argv[0]);
					return -1;
				}

				mainfn = argv[i];
				mode = 0;
			}
			else if (mode == 7) // word replacement
			{
				const ase_char_t* p;
				ase_size_t l;

				p = ase_strchr(argv[i], ASE_T(':'));
				if (p == ASE_NULL)
				{
					print_usage (argv[0]);
					return -1;
				}

				l = ase_strlen (argv[i]);

				awk.setWord (
					argv[i], p - argv[i], 
					p + 1, l - (p - argv[i] + 1));

				mode = 0;
			}
		}
	}

	if (mode != 0)
	{
		print_usage (argv[0]);
		awk.close ();
		return -1;
	}


	if (awk.parse (srcin, srcout) == -1)
	{
		ase_fprintf (stderr, ASE_T("cannot parse: LINE[%d] %s\n"), 
			awk.getErrorLine(), awk.getErrorMessage());
		awk.close ();
		return -1;
	}

	awk.enableRunCallback ();

	if (awk.run (mainfn, args, nargs) == -1)
	{
		ase_fprintf (stderr, ASE_T("cannot run: LINE[%d] %s\n"), 
			awk.getErrorLine(), awk.getErrorMessage());
		awk.close ();
		return -1;
	}

	awk.close ();
	return 0;
}

extern "C" int ase_main (int argc, ase_achar_t* argv[])
{

	int n;

#if defined(__linux) && defined(_DEBUG)
	mtrace ();
#endif
#if defined(_WIN32) && defined(_DEBUG) && defined(_MSC_VER)
	_CrtSetDbgFlag (_CRTDBG_LEAK_CHECK_DF | _CRTDBG_ALLOC_MEM_DF);
#endif


	n = ase_runmain (argc,argv,awk_main);

#if defined(__linux) && defined(_DEBUG)
	muntrace ();
#endif
#if defined(_WIN32) && defined(_DEBUG)
	/* #if defined(_MSC_VER)
	_CrtDumpMemoryLeaks ();
	#endif */
	_tprintf (_T("Press ENTER to quit\n"));
	getchar ();
#endif

	return n;
}
