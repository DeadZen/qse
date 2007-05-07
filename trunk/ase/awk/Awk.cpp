/*
 * $Id: Awk.cpp,v 1.9 2007/05/06 06:55:05 bacon Exp $
 */

#include <ase/awk/Awk.hpp>
#include <ase/cmn/str.h>
#include <ase/cmn/mem.h>

namespace ASE
{

	Awk::Source::Source (Mode mode): mode (mode)
	{
	}

	Awk::Source::Mode Awk::Source::getMode () const
	{
		return this->mode;
	}

	const void* Awk::Source::getHandle () const
	{
		return this->handle;
	}

	void Awk::Source::setHandle (void* handle)
	{
		this->handle = handle;
	}

	Awk::Extio::Extio (const char_t* name): name(name), handle(ASE_NULL)
	{
	}

	const Awk::char_t* Awk::Extio::getName () const
	{
		return this->name;
	}

	const void* Awk::Extio::getHandle () const
	{
		return this->handle;
	}

	void Awk::Extio::setHandle (void* handle)
	{
		this->handle = handle;
	}

	Awk::Pipe::Pipe (char_t* name, Mode mode): Extio(name), mode(mode)
	{
	}

	Awk::File::File (char_t* name, Mode mode): Extio(name), mode(mode)
	{
	}
	
	Awk::Console::Console (char_t* name, Mode mode): Extio(name), mode(mode)
	{
	}

	Awk::Pipe::Mode Awk::Pipe::getMode () const
	{
		return this->mode;
	}

	Awk::File::Mode Awk::File::getMode () const
	{
		return this->mode;
	}

	Awk::Console::Mode Awk::Console::getMode () const
	{
		return this->mode;
	}

	Awk::Awk (): awk (ASE_NULL), functionMap (ASE_NULL), 
		sourceIn (Source::READ), sourceOut (Source::WRITE)
	{
	}

	Awk::~Awk ()
	{
		close ();
	}

	int Awk::parse ()
	{
		ASE_ASSERT (awk != ASE_NULL);

		ase_awk_srcios_t srcios;

		srcios.in = sourceReader;
		srcios.out = sourceWriter;
		srcios.custom_data = this;

		return ase_awk_parse (awk, &srcios);
	}

	int Awk::run (const char_t* main, const char_t** args)
	{
		ASE_ASSERT (awk != ASE_NULL);

		ase_awk_runios_t runios;

		runios.pipe = pipeHandler;
		runios.coproc = ASE_NULL;
		/*
		runios.file = fileHandler;
		runios.console = consoleHandler;
		*/
		runios.custom_data = this;

		return ase_awk_run (
			awk, main, &runios, ASE_NULL, ASE_NULL, this);
	}

	int Awk::open ()
	{
		ASE_ASSERT (awk == ASE_NULL && functionMap == ASE_NULL);

		ase_awk_prmfns_t prmfns;

		prmfns.mmgr.malloc      = malloc;
		prmfns.mmgr.realloc     = realloc;
		prmfns.mmgr.free        = free;
		prmfns.mmgr.custom_data = this;

		prmfns.ccls.is_upper    = isUpper;
		prmfns.ccls.is_lower    = isLower;
		prmfns.ccls.is_alpha    = isAlpha;
		prmfns.ccls.is_digit    = isDigit;
		prmfns.ccls.is_xdigit   = isXdigit;
		prmfns.ccls.is_alnum    = isAlnum;
		prmfns.ccls.is_space    = isSpace;
		prmfns.ccls.is_print    = isPrint;
		prmfns.ccls.is_graph    = isGraph;
		prmfns.ccls.is_cntrl    = isCntrl;
		prmfns.ccls.is_punct    = isPunct;
		prmfns.ccls.to_upper    = toUpper;
		prmfns.ccls.to_lower    = toLower;
		prmfns.ccls.custom_data = this;

		/*
		int (Awk::*ptr) (void*, ase_char_t*, ase_size_t, const ase_char_t*, ...) = &Awk::sprintf;
		(this->*ptr) (ASE_NULL, ASE_NULL, 0, ASE_NULL);
		*/

		prmfns.misc.pow         = pow;
		prmfns.misc.sprintf     = sprintf;
		prmfns.misc.dprintf     = dprintf;
		prmfns.misc.custom_data = this;

		awk = ase_awk_open (&prmfns, this);
		if (awk == ASE_NULL)
		{
			// TODO: SET ERROR INFO
			return -1;
		}

		functionMap = ase_awk_map_open (
			this, 512, freeFunctionMapValue, awk);
		if (functionMap == ASE_NULL)
		{
			// TODO: set ERROR INFO -> ENOMEM...
			ase_awk_close (awk);
			awk = ASE_NULL;
			return -1;
		}

		return 0;
	}

	void Awk::close ()
	{
		if (functionMap != ASE_NULL)
		{
			ase_awk_map_close (functionMap);
			functionMap = ASE_NULL;
		}

		if (awk != ASE_NULL) 
		{
			ase_awk_close (awk);
			awk = ASE_NULL;
		}
	}

	int Awk::dispatchFunction (const char_t* name, size_t len)
	{
		ase_awk_pair_t* pair;

		pair = ase_awk_map_get (functionMap, name, len);
		if (pair == ASE_NULL) return -1;

		FunctionHandler handler;
	       	handler = *(FunctionHandler*)ASE_AWK_PAIR_VAL(pair);	

		return (this->*handler) ();
	}

	int Awk::addFunction (
		const char_t* name, size_t minArgs, size_t maxArgs, 
		FunctionHandler handler)
	{
		ASE_ASSERT (awk != ASE_NULL);

		FunctionHandler* tmp;
		tmp = (FunctionHandler*)this->malloc (ASE_SIZEOF(handler));
		if (tmp == ASE_NULL)
		{
			// TODO: SET ERROR INFO -> ENOMEM
			return -1;
		}

		//ase_memcpy (tmp, &handler, ASE_SIZEOF(handler));
		*tmp = handler;
		
		size_t nameLen = ase_strlen(name);

		void* p = ase_awk_addbfn (awk, name, nameLen,
		                          0, minArgs, maxArgs, ASE_NULL, 
		                          functionHandler);
		if (p == ASE_NULL) 
		{
			this->free (tmp);
			return -1;
		}

		ase_awk_pair_t* pair;
		pair = ase_awk_map_put (functionMap, name, nameLen, tmp);
		if (pair == ASE_NULL)
		{
			// TODO: SET ERROR INFO
			ase_awk_delbfn (awk, name, nameLen);
			this->free (tmp);
			return -1;
		}

		return 0;
	}

	int Awk::deleteFunction (const char_t* name)
	{
		ASE_ASSERT (awk != ASE_NULL);

		size_t nameLen = ase_strlen(name);

		int n = ase_awk_delbfn (awk, name, nameLen);
		if (n == 0) ase_awk_map_remove (functionMap, name, nameLen);

		return n;
	}

	Awk::ssize_t Awk::sourceReader (
		int cmd, void* arg, char_t* data, size_t count)
	{
		Awk* awk = (Awk*)arg;
	
		switch (cmd)
		{
			case ASE_AWK_IO_OPEN:
				return awk->openSource (awk->sourceIn);
			case ASE_AWK_IO_CLOSE:
				return awk->closeSource (awk->sourceIn);
			case ASE_AWK_IO_READ:
				return awk->readSource (awk->sourceIn, data, count);
		}
	
		return -1;
	}

	Awk::ssize_t Awk::sourceWriter (
		int cmd, void* arg, char_t* data, size_t count)
	{
		Awk* awk = (Awk*)arg;
	
		switch (cmd)
		{
			case ASE_AWK_IO_OPEN:
				return awk->openSource (awk->sourceOut);
			case ASE_AWK_IO_CLOSE:
				return awk->closeSource (awk->sourceOut);
			case ASE_AWK_IO_WRITE:
				return awk->writeSource (awk->sourceOut, data, count);
		}
	
		return -1;
	}

	Awk::ssize_t Awk::pipeHandler (
		int cmd, void* arg, char_t* data, size_t count)
	{
		ase_awk_extio_t* epa = (ase_awk_extio_t*)arg;
		Awk* awk = (Awk*)epa->custom_data;

		ASE_ASSERT ((epa->type & 0xFF) == ASE_AWK_EXTIO_PIPE);

		Pipe pipe (epa->name, (Pipe::Mode)epa->mode);

		switch (cmd)
		{
			case ASE_AWK_IO_OPEN:
				return awk->openPipe (pipe);
			case ASE_AWK_IO_CLOSE:
				return awk->closePipe (pipe);

			case ASE_AWK_IO_READ:
				return awk->readPipe (pipe, data, count);
			case ASE_AWK_IO_WRITE:
				return awk->writePipe (pipe, data, count);

			case ASE_AWK_IO_FLUSH:
				return awk->flushPipe (pipe);
			case ASE_AWK_IO_NEXT:
				return awk->nextPipe (pipe);
		}

		return -1;
	}

	Awk::ssize_t Awk::fileHandler (
		int cmd, void* arg, char_t* data, size_t count)
	{
		ase_awk_extio_t* epa = (ase_awk_extio_t*)arg;
		Awk* awk = (Awk*)epa->custom_data;

		ASE_ASSERT ((epa->type & 0xFF) == ASE_AWK_EXTIO_FILE);

		File file (epa->name, (File::Mode)epa->mode);

		switch (cmd)
		{
			case ASE_AWK_IO_OPEN:
				return awk->openFile (file);
			case ASE_AWK_IO_CLOSE:
				return awk->closeFile (file);

			case ASE_AWK_IO_READ:
				return awk->readFile (file, data, count);
			case ASE_AWK_IO_WRITE:
				return awk->writeFile (file, data, count);

			case ASE_AWK_IO_FLUSH:
				return awk->flushFile (file);
			case ASE_AWK_IO_NEXT:
				return awk->nextFile (file);
		}

		return -1;
	}

	Awk::ssize_t Awk::consoleHandler (
		int cmd, void* arg, char_t* data, size_t count)
	{
		ase_awk_extio_t* epa = (ase_awk_extio_t*)arg;
		Awk* awk = (Awk*)epa->custom_data;

		ASE_ASSERT ((epa->type & 0xFF) == ASE_AWK_EXTIO_CONSOLE);

		Console console (epa->name, (Console::Mode)epa->mode);

		switch (cmd)
		{
			case ASE_AWK_IO_OPEN:
				return awk->openConsole (console);
			case ASE_AWK_IO_CLOSE:
				return awk->closeConsole (console);

			case ASE_AWK_IO_READ:
				return awk->readConsole (console, data, count);
			case ASE_AWK_IO_WRITE:
				return awk->writeConsole (console, data, count);

			case ASE_AWK_IO_FLUSH:
				return awk->flushConsole (console);
			case ASE_AWK_IO_NEXT:
				return awk->nextConsole (console);
		}

		return -1;
	}

	int Awk::functionHandler (
		ase_awk_run_t* run, const char_t* name, size_t len)
	{
		Awk* awk = (Awk*) ase_awk_getruncustomdata (run);
		return awk->dispatchFunction (name, len);
	}	

	void Awk::freeFunctionMapValue (void* owner, void* value)
	{
		Awk* awk = (Awk*)owner;
		awk->free (value);
	}

	void* Awk::malloc (void* custom, size_t n)
	{
		return ((Awk*)custom)->malloc (n);
	}

	void* Awk::realloc (void* custom, void* ptr, size_t n)
	{
		return ((Awk*)custom)->realloc (ptr, n);
	}

	void Awk::free (void* custom, void* ptr)
	{
		((Awk*)custom)->free (ptr);
	}

	Awk::bool_t Awk::isUpper (void* custom, cint_t c)  
	{ 
		return ((Awk*)custom)->isUpper (c);
	}
	
	Awk::bool_t Awk::isLower (void* custom, cint_t c)  
	{ 
		return ((Awk*)custom)->isLower (c);
	}
	
	Awk::bool_t Awk::isAlpha (void* custom, cint_t c)  
	{ 
		return ((Awk*)custom)->isAlpha (c);
	}
	
	Awk::bool_t Awk::isDigit (void* custom, cint_t c)  
	{ 
		return ((Awk*)custom)->isDigit (c);
	}
	
	Awk::bool_t Awk::isXdigit (void* custom, cint_t c) 
	{ 
		return ((Awk*)custom)->isXdigit (c);
	}
	
	Awk::bool_t Awk::isAlnum (void* custom, cint_t c)
	{ 
		return ((Awk*)custom)->isAlnum (c);
	}
	
	Awk::bool_t Awk::isSpace (void* custom, cint_t c)
	{ 
		return ((Awk*)custom)->isSpace (c);
	}
	
	Awk::bool_t Awk::isPrint (void* custom, cint_t c)
	{ 
		return ((Awk*)custom)->isPrint (c);
	}
	
	Awk::bool_t Awk::isGraph (void* custom, cint_t c)
	{
		return ((Awk*)custom)->isGraph (c);
	}
	
	Awk::bool_t Awk::isCntrl (void* custom, cint_t c)
	{
		return ((Awk*)custom)->isCntrl (c);
	}
	
	Awk::bool_t Awk::isPunct (void* custom, cint_t c)
	{
		return ((Awk*)custom)->isPunct (c);
	}
	
	Awk::cint_t Awk::toUpper (void* custom, cint_t c)
	{
		return ((Awk*)custom)->toUpper (c);
	}
	
	Awk::cint_t Awk::toLower (void* custom, cint_t c)
	{
		return ((Awk*)custom)->toLower (c);
	}

	Awk::real_t Awk::pow (void* custom, real_t x, real_t y)
	{
		return ((Awk*)custom)->pow (x, y);
	}
		
	int Awk::sprintf (void* custom, char_t* buf, size_t size,
	                  const char_t* fmt, ...)
	{
		va_list ap;
		va_start (ap, fmt);
		int n = ((Awk*)custom)->vsprintf (buf, size, fmt, ap);
		va_end (ap);
		return n;
	}

	void Awk::dprintf (void* custom, const char_t* fmt, ...)
	{
		va_list ap;
		va_start (ap, fmt);
		((Awk*)custom)->vdprintf (fmt, ap);
		va_end (ap);
	}

}
