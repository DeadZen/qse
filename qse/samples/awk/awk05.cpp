/*
 * $Id: Awk.cpp 341 2008-08-20 10:58:19Z baconevi $
 *
   Copyright 2006-2009 Chung, Hyung-Hwan.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#include <qse/awk/StdAwk.hpp>
#include <qse/cmn/str.h>
#include <qse/cmn/stdio.h>
#include <qse/cmn/main.h>

#include <stdlib.h>
#include <math.h>

#if defined(_WIN32)
#	include <windows.h>
#else
#	include <unistd.h>
#	include <signal.h>
#	include <errno.h>
#endif

class TestAwk;
#ifdef _WIN32
static BOOL WINAPI stop_run (DWORD ctrl_type);
#else
static void stop_run (int sig);
#endif

static void set_intr_run (void);
static void unset_intr_run (void);

TestAwk* app_awk = QSE_NULL;
static bool verbose = false;

class TestAwk: public QSE::StdAwk
{
public:
	TestAwk (): srcInName(QSE_NULL), srcOutName(QSE_NULL), 
	            numConInFiles(0), numConOutFiles(0)
	{
	#ifdef _WIN32
		heap = QSE_NULL;
	#endif
	}

	~TestAwk ()
	{
		close ();
	}

	int open ()
	{
	#ifdef _WIN32
		QSE_ASSERT (heap == QSE_NULL);
		heap = ::HeapCreate (0, 1000000, 1000000);
		if (heap == QSE_NULL) return -1;
	#endif

		int n = StdAwk::open ();
		if (n <= -1)
		{
	#ifdef _WIN32
			HeapDestroy (heap); 
			heap = QSE_NULL;
	#endif
			return -1;
		}

		idLastSleep = addGlobal (QSE_T("LAST_SLEEP"));
		if (idLastSleep <= -1) goto failure;

		if (addFunction (QSE_T("sleep"), 1, 1,
		    	(FunctionHandler)&TestAwk::sleep) <= -1) goto failure;

		if (addFunction (QSE_T("sumintarray"), 1, 1,
		    	(FunctionHandler)&TestAwk::sumintarray) <= -1) goto failure;

		if (addFunction (QSE_T("arrayindices"), 1, 1,
		    	(FunctionHandler)&TestAwk::arrayindices) <= -1) goto failure;
		return 0;

	failure:
		StdAwk::close ();

	#ifdef _WIN32
		HeapDestroy (heap); 
		heap = QSE_NULL;
	#endif
		return -1;
	}

	void close ()
	{
		StdAwk::close ();

		numConInFiles = 0;
		numConOutFiles = 0;

	#ifdef _WIN32
		if (heap != QSE_NULL)
		{
			HeapDestroy (heap); 
			heap = QSE_NULL;
		}
	#endif
	}

	int sleep (Run& run, Return& ret, const Argument* args, size_t nargs, 
		const char_t* name, size_t len)
	{
		if (args[0].isIndexed()) 
		{
			run.setError (ERR_INVAL);
			return -1;
		}

		long_t x = args[0].toInt();

		/*Argument arg;
		if (run.getGlobal(idLastSleep, arg) == 0)
			qse_printf (QSE_T("GOOD: [%d]\n"), (int)arg.toInt());
		else { qse_printf (QSE_T("BAD:\n")); }
		*/

		if (run.setGlobal (idLastSleep, x) <= -1) return -1;

	#ifdef _WIN32
		::Sleep ((DWORD)(x * 1000));
		return ret.set ((long_t)0);
	#else
		return ret.set ((long_t)::sleep (x));
	#endif
	}

	int sumintarray (Run& run, Return& ret, const Argument* args, size_t nargs, 
		const char_t* name, size_t len)
	{
		long_t x = 0;

		if (args[0].isIndexed()) 
		{
			Argument idx(run), val(run);

			int n = args[0].getFirstIndex (idx);
			while (n > 0)
			{
				size_t len;
				const char_t* ptr = idx.toStr(&len);

				if (args[0].getIndexed(ptr, len, val) <= -1) return -1;
				x += val.toInt ();

				n = args[0].getNextIndex (idx);
			}
			if (n != 0) return -1;
		}
		else x += args[0].toInt();

		return ret.set (x);
	}

	int arrayindices (Run& run, Return& ret, const Argument* args, size_t nargs, 
		const char_t* name, size_t len)
	{
		if (!args[0].isIndexed()) return 0;

		Argument idx (run);
		long_t i;

		int n = args[0].getFirstIndex (idx);
		for (i = 0; n > 0; i++)
		{
			size_t len;
			const char_t* ptr = idx.toStr(&len);
			n = args[0].getNextIndex (idx);
			if (ret.setIndexed (i, ptr, len) <= -1) return -1;
		}
		if (n != 0) return -1;
	
		return 0;
	}
	
	int addConsoleInput (const char_t* file)
	{
		if (numConInFiles < QSE_COUNTOF(conInFile))
		{
			conInFile[numConInFiles++] = file;
			return 0;
		}

		return -1;
	}

	int addConsoleOutput (const char_t* file)
	{
		if (numConOutFiles < QSE_COUNTOF(conOutFile))
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
		return StdAwk::parse ();
	}

protected:

	bool onRunEnter (Run& run)
	{
		set_intr_run ();
		return true;
	}

	void onRunExit (Run& run, const Argument& ret)
	{
		unset_intr_run ();

		if (verbose)
		{
			size_t len;
			const char_t* ptr = ret.toStr (&len);
			qse_printf (QSE_T("*** return [%.*s] ***\n"), (int)len, ptr);
		}
	}
	

	int openSource (Source& io)
	{
		Source::Mode mode = io.getMode();
		FILE* fp = QSE_NULL;

		// TODO: use sio instead of stdio
		if (mode == Source::READ)
		{
			if (srcInName == QSE_NULL) 
			{
				io.setHandle (stdin);
				return 0;
			}

			if (srcInName[0] == QSE_T('\0')) fp = stdin;
			else fp = qse_fopen (srcInName, QSE_T("r"));
		}
		else if (mode == Source::WRITE)
		{
			if (srcOutName == QSE_NULL)
			{
				io.setHandle (stdout);
				return 0;
			}

			if (srcOutName[0] == QSE_T('\0')) fp = stdout;
			else fp = qse_fopen (srcOutName, QSE_T("w"));
		}

		if (fp == QSE_NULL) return -1;
		io.setHandle (fp);
		return 1;
	}

	int closeSource (Source& io)
	{
		FILE* fp = (FILE*)io.getHandle();
		if (fp == stdout || fp == stderr) fflush (fp);
		if (fp != stdin && fp != stdout && fp != stderr) fclose (fp);
		io.setHandle (QSE_NULL);
		return 0;
	}

	ssize_t readSource (Source& io, char_t* buf, size_t len)
	{
		FILE* fp = (FILE*)io.getHandle();
		ssize_t n = 0;

		while (n < (ssize_t)len)
		{
			qse_cint_t c = qse_fgetc (fp);
			if (c == QSE_CHAR_EOF) 
			{
				if (qse_ferror(fp)) n = -1;
				break;
			}

			buf[n++] = c;
			if (c == QSE_T('\n')) break;
		}

		return n;
	}

	ssize_t writeSource (Source& io, char_t* buf, size_t len)
	{
		FILE* fp = (FILE*)io.getHandle();
		size_t left = len;

		while (left > 0)
		{
			if (*buf == QSE_T('\0')) 
			{
				if (qse_fputc(*buf,fp) == QSE_CHAR_EOF) return -1;
				left -= 1; buf += 1;
			}
			else
			{
				int chunk = (left > QSE_TYPE_MAX(int))? QSE_TYPE_MAX(int): (int)left;
				int n = qse_fprintf (fp, QSE_T("%.*s"), chunk, buf);
				if (n < 0 || n > chunk) return -1;
				left -= n; buf += n;
			}
		}

		return len;
	}

	// console io handlers 
	int openConsole (Console& io) 
	{ 
		StdAwk::Console::Mode mode = io.getMode();

		FILE* fp = QSE_NULL;
		const char_t* fn = QSE_NULL;

		switch (mode)
		{
			case StdAwk::Console::READ:

				if (numConInFiles == 0) fp = stdin;
				else
				{
					fn = conInFile[0];
					fp = qse_fopen (fn, QSE_T("r"));
				}
				break;

			case StdAwk::Console::WRITE:

				if (numConOutFiles == 0) fp = stdout;
				else
				{
					fn = conOutFile[0];
					fp = qse_fopen (fn, QSE_T("w"));
				}
				break;
		}

		if (fp == NULL) return -1;

		ConTrack* t = (ConTrack*) 
			qse_awk_alloc (awk, QSE_SIZEOF(ConTrack));
		if (t == QSE_NULL)
		{
			if (fp != stdin && fp != stdout) fclose (fp);
			return -1;
		}

		t->handle = fp;
		t->nextConIdx = 1;

		if (fn != QSE_NULL) 
		{
			if (io.setFileName(fn) <= -1)
			{
				if (fp != stdin && fp != stdout) fclose (fp);
				qse_awk_free (awk, t);
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

		qse_awk_free (awk, t);
		return 0;
	}

	ssize_t readConsole (Console& io, char_t* buf, size_t len) 
	{
		ConTrack* t = (ConTrack*)io.getHandle();
		FILE* fp = t->handle;
		ssize_t n = 0;

		while (n < (ssize_t)len)
		{
			qse_cint_t c = qse_fgetc (fp);
			if (c == QSE_CHAR_EOF) 
			{
				if (qse_ferror(fp)) return -1;
				if (t->nextConIdx >= numConInFiles) break;

				const char_t* fn = conInFile[t->nextConIdx];
				FILE* nfp = qse_fopen (fn, QSE_T("r"));
				if (nfp == QSE_NULL) return -1;

				if (io.setFileName(fn) <= -1 || io.setFNR(0) <= -1)
				{
					fclose (nfp);
					return -1;
				}

				fclose (fp);
				fp = nfp;
				t->nextConIdx++;
				t->handle = fp;

				if (n == 0) continue;
				else break;
			}

			buf[n++] = c;
			if (c == QSE_T('\n')) break;
		}

		return n;
	}

	ssize_t writeConsole (Console& io, const char_t* buf, size_t len) 
	{
		ConTrack* t = (ConTrack*)io.getHandle();
		FILE* fp = t->handle;
		size_t left = len;

		while (left > 0)
		{
			if (*buf == QSE_T('\0')) 
			{
				if (qse_fputc(*buf,fp) == QSE_CHAR_EOF) return -1;
				left -= 1; buf += 1;
			}
			else
			{
				int chunk = (left > QSE_TYPE_MAX(int))? QSE_TYPE_MAX(int): (int)left;
				int n = qse_fprintf (fp, QSE_T("%.*s"), chunk, buf);
				if (n < 0 || n > chunk) return -1;
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
		StdAwk::Console::Mode mode = io.getMode();

		ConTrack* t = (ConTrack*)io.getHandle();
		FILE* ofp = t->handle;
		FILE* nfp = QSE_NULL;
		const char_t* fn = QSE_NULL;

		switch (mode)
		{
			case StdAwk::Console::READ:

				if (t->nextConIdx >= numConInFiles) return 0;
				fn = conInFile[t->nextConIdx];
				nfp = qse_fopen (fn, QSE_T("r"));
				break;

			case StdAwk::Console::WRITE:

				if (t->nextConIdx >= numConOutFiles) return 0;
				fn = conOutFile[t->nextConIdx];
				nfp = qse_fopen (fn, QSE_T("w"));
				break;
		}

		if (nfp == QSE_NULL) return -1;

		if (fn != QSE_NULL)
		{
			if (io.setFileName (fn) <= -1)
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

	void* allocMem (size_t n) throw ()
	{ 
	#ifdef _WIN32
		return ::HeapAlloc (heap, 0, n);
	#else
		return ::malloc (n);
	#endif
	}

	void* reallocMem (void* ptr, size_t n) throw ()
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

	void freeMem (void* ptr) throw ()
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

	int idLastSleep;

#ifdef _WIN32
	void* heap;
#endif
};

#ifdef _WIN32
static BOOL WINAPI stop_run (DWORD ctrl_type)
{
	if (ctrl_type == CTRL_C_EVENT ||
	    ctrl_type == CTRL_CLOSE_EVENT)
	{
		if (app_awk) app_awk->stop ();
		return TRUE;
	}

	return FALSE;
}
#else

static int setsignal (int sig, void(*handler)(int), int restart)
{
	struct sigaction sa_int;

	sa_int.sa_handler = handler;
	sigemptyset (&sa_int.sa_mask);
	
	sa_int.sa_flags = 0;

	if (restart)
	{
	#ifdef SA_RESTART
		sa_int.sa_flags |= SA_RESTART;
	#endif
	}
	else
	{
	#ifdef SA_INTERRUPT
		sa_int.sa_flags |= SA_INTERRUPT;
	#endif
	}
	return sigaction (sig, &sa_int, NULL);
}

static void stop_run (int sig)
{
	int e = errno;
	if (app_awk) app_awk->stop ();
	errno = e;
}
#endif

static void set_intr_run (void)
{
#ifdef _WIN32
	SetConsoleCtrlHandler (stop_run, TRUE);
#else
	/*setsignal (SIGINT, stop_run, 1); TO BE MORE COMPATIBLE WITH WIN32*/
	setsignal (SIGINT, stop_run, 0);
#endif
}

static void unset_intr_run (void)
{
#ifdef _WIN32
	SetConsoleCtrlHandler (stop_run, FALSE);
#else
	setsignal (SIGINT, SIG_DFL, 1);
#endif
}

static void print_error (const qse_char_t* msg)
{
	qse_printf (QSE_T("Error: %s\n"), msg);
}

static struct
{
	const qse_char_t* name;
	TestAwk::Option   opt;
} otab[] =
{
	{ QSE_T("implicit"),    TestAwk::OPT_IMPLICIT },
	{ QSE_T("explicit"),    TestAwk::OPT_EXPLICIT },
	{ QSE_T("bxor"),        TestAwk::OPT_BXOR },
	{ QSE_T("shift"),       TestAwk::OPT_SHIFT },
	{ QSE_T("idiv"),        TestAwk::OPT_IDIV },
	{ QSE_T("rio"),         TestAwk::OPT_RIO },
	{ QSE_T("rwpipe"),      TestAwk::OPT_RWPIPE },
	{ QSE_T("newline"),     TestAwk::OPT_NEWLINE },
	{ QSE_T("stripspaces"), TestAwk::OPT_STRIPSPACES },
	{ QSE_T("nextofile"),   TestAwk::OPT_NEXTOFILE },
	{ QSE_T("crlf"),        TestAwk::OPT_CRLF },
	{ QSE_T("reset"),       TestAwk::OPT_RESET },
	{ QSE_T("maptovar"),    TestAwk::OPT_MAPTOVAR },
	{ QSE_T("pablock"),     TestAwk::OPT_PABLOCK }
};

static void print_usage (const qse_char_t* argv0)
{
	const qse_char_t* base;
	int j;
	
	base = qse_strrchr(argv0, QSE_T('/'));
	if (base == QSE_NULL) base = qse_strrchr(argv0, QSE_T('\\'));
	if (base == QSE_NULL) base = argv0; else base++;

	qse_printf (QSE_T("Usage: %s [-si file]? [-so file]? [-ci file]* [-co file]* [-a arg]* [-w o:n]* \n"), base);
	qse_printf (QSE_T("    -si file  Specify the input source file\n"));
	qse_printf (QSE_T("              The source code is read from stdin when it is not specified\n"));
	qse_printf (QSE_T("    -so file  Specify the output source file\n"));
	qse_printf (QSE_T("              The deparsed code is not output when is it not specified\n"));
	qse_printf (QSE_T("    -ci file  Specify the input console file\n"));
	qse_printf (QSE_T("    -co file  Specify the output console file\n"));
	qse_printf (QSE_T("    -a  str   Specify an argument\n"));
	qse_printf (QSE_T("    -w  o:n   Specify an old and new word pair\n"));
	qse_printf (QSE_T("              o - an original word\n"));
	qse_printf (QSE_T("              n - the new word to replace the original\n"));
	qse_printf (QSE_T("    -v        Print extra messages\n"));


	qse_printf (QSE_T("\nYou may specify the following options to change the behavior of the interpreter.\n"));
	for (j = 0; j < (int)QSE_COUNTOF(otab); j++)
	{
		qse_printf (QSE_T("    -%-20s -no%-20s\n"), otab[j].name, otab[j].name);
	}
}

static int awk_main (int argc, qse_char_t* argv[])
{
	TestAwk awk;

	int mode = 0;
	const qse_char_t* srcin = QSE_T("");
	const qse_char_t* srcout = NULL;
	const qse_char_t* args[256];
	qse_size_t nargs = 0;
	qse_size_t nsrcins = 0;
	qse_size_t nsrcouts = 0;

	if (awk.open() <= -1)
	{
		qse_fprintf (stderr, QSE_T("cannot open awk\n"));
		return -1;
	}

	for (int i = 1; i < argc; i++)
	{
		if (mode == 0)
		{
			if (qse_strcmp(argv[i], QSE_T("-si")) == 0) mode = 1;
			else if (qse_strcmp(argv[i], QSE_T("-so")) == 0) mode = 2;
			else if (qse_strcmp(argv[i], QSE_T("-ci")) == 0) mode = 3;
			else if (qse_strcmp(argv[i], QSE_T("-co")) == 0) mode = 4;
			else if (qse_strcmp(argv[i], QSE_T("-a")) == 0) mode = 5;
			else if (qse_strcmp(argv[i], QSE_T("-w")) == 0) mode = 6;
			else if (qse_strcmp(argv[i], QSE_T("-v")) == 0)
			{
				verbose = true;
			}
			else 
			{
				if (argv[i][0] == QSE_T('-'))
				{
					int j;

					if (argv[i][1] == QSE_T('n') && argv[i][2] == QSE_T('o'))
					{
						for (j = 0; j < (int)QSE_COUNTOF(otab); j++)
						{
							if (qse_strcmp(&argv[i][3], otab[j].name) == 0)
							{
								awk.setOption (awk.getOption() & ~otab[j].opt);
								goto ok_valid;
							}
						}
					}
					else
					{
						for (j = 0; j < (int)QSE_COUNTOF(otab); j++)
						{
							if (qse_strcmp(&argv[i][1], otab[j].name) == 0)
							{
								awk.setOption (awk.getOption() | otab[j].opt);
								goto ok_valid;
							}
						}
					}
				}

				print_usage (argv[0]);
				return -1;

			ok_valid:
				;
			}
		}
		else
		{
			if (argv[i][0] == QSE_T('-'))
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
				if (awk.addConsoleInput (argv[i]) <= -1)
				{
					print_error (QSE_T("too many console inputs"));
					return -1;
				}

				mode = 0;
			}
			else if (mode == 4) // console output
			{
				if (awk.addConsoleOutput (argv[i]) <= -1)
				{
					print_error (QSE_T("too many console outputs"));
					return -1;
				}

				mode = 0;
			}
			else if (mode == 5) // argument mode
			{
				if (nargs >= QSE_COUNTOF(args))
				{
					print_usage (argv[0]);
					return -1;
				}

				args[nargs++] = argv[i];
				mode = 0;
			}
			else if (mode == 6) // word replacement
			{
				const qse_char_t* p;
				qse_size_t l;

				p = qse_strchr(argv[i], QSE_T(':'));
				if (p == QSE_NULL)
				{
					print_usage (argv[0]);
					return -1;
				}

				l = qse_strlen (argv[i]);

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

	if (awk.parse (srcin, srcout) <= -1)
	{
		qse_fprintf (stderr, QSE_T("cannot parse: LINE[%d] %s\n"), 
			awk.getErrorLine(), awk.getErrorMessage());
		awk.close ();
		return -1;
	}

	awk.enableRunCallback ();
	app_awk = &awk;

	for (qse_size_t i = 0; i < nargs; i++)
	{
		if (awk.addArgument (args[i]) <= -1)
		{
			qse_fprintf (stderr, 
				QSE_T("ERROR: %s\n"), 
				awk.getErrorMessage()
			);
			awk.close ();
			return -1;
		}
	}

	if (awk.loop () <= -1)
	{
		qse_fprintf (stderr, QSE_T("cannot run: LINE[%d] %s\n"), 
			awk.getErrorLine(), awk.getErrorMessage());
		awk.close ();
		return -1;
	}

	app_awk = QSE_NULL;
	awk.close ();

	return 0;
}

int qse_main (int argc, qse_achar_t* argv[])
{
	return qse_runmain (argc,argv,awk_main);
}
