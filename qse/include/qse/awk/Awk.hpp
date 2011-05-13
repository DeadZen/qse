/*
 * $Id: Awk.hpp 458 2011-05-13 04:06:55Z hyunghwan.chung $
 *
    Copyright 2006-2011 Chung, Hyung-Hwan.
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

#ifndef _QSE_AWK_AWK_HPP_
#define _QSE_AWK_AWK_HPP_

#include <qse/awk/awk.h>
#include <qse/cmn/htb.h>
#include <qse/cmn/chr.h>
#include <qse/cmn/Mmged.hpp>
#include <stdarg.h>

/// @file
/// AWK Interpreter
///

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

/// 
/// The Awk class implements an AWK interpreter by wrapping around 
/// #qse_awk_t and #qse_awk_rtx_t.
///
class Awk: public Mmged
{
public:
	typedef qse_htb_t htb_t;
	typedef qse_htb_pair_t pair_t;

	/** Defines a primitive handle */
	typedef qse_awk_t awk_t;

	typedef qse_awk_loc_t loc_t;
	typedef qse_awk_errnum_t errnum_t;
	typedef qse_awk_errstr_t errstr_t;
	typedef qse_awk_errinf_t errinf_t;

	/// The depth_t type redefines #qse_awk_depth_t.
	typedef qse_awk_depth_t depth_t;

	/// The option_t type redefines #qse_awk_option_t.
	typedef qse_awk_option_t option_t;

	/// The gbl_id_t type redefines #qse_awk_gbl_id_t.
	typedef qse_awk_gbl_id_t gbl_id_t;

	/** Represents an internal awk value */
	typedef qse_awk_val_t val_t;

	/** Represents a runtime context */
	typedef qse_awk_rtx_t rtx_t;

	/** Represents an runtime I/O data */
	typedef qse_awk_rio_arg_t rio_arg_t;

	typedef qse_awk_rio_cmd_t rio_cmd_t;

	typedef qse_awk_sio_arg_t sio_arg_t;

	typedef qse_awk_sio_cmd_t sio_cmd_t;

	class Run;
	friend class Run;


protected:
	///
	/// @name Error Handling
	///
	/// @{

	///
	/// The getErrorString() function returns a formatting string
	/// for an error code @a num. You can override this function
	/// to customize an error message. You must include the same numbers
	/// of ${X}'s as the orginal formatting string. Their order may be
	/// different. The example below changes the formatting string for
	/// #QSE_AWK_ENOENT.
	/// @code
	/// const MyAwk::char_t* MyAwk::getErrorString (errnum_t num) const 
	/// {
	///    if (num == QSE_AWK_ENOENT) return QSE_T("cannot find '${0}'");
	///    return Awk::getErrorString (num);
	/// }
	/// @endcode
	///
	virtual const char_t* getErrorString (
		errnum_t num
	) const;

public:
	///
	/// The getErrorNumber() function returns the number of the last
	/// error occurred.
	///
	errnum_t getErrorNumber () const;

	///
	/// The getErrorLocation() function returns the location of the 
	/// last error occurred.
	///
	loc_t getErrorLocation () const;

	///
	/// The Awk::getErrorMessage() function returns a message describing
	/// the last error occurred.
	///
	const char_t* getErrorMessage () const;

	///
	/// The setError() function sets error information.
	///
	void setError (
		errnum_t      code, ///< error code
		const cstr_t* args  = QSE_NULL, ///< message formatting 
		                                ///  argument array
		const loc_t*  loc   = QSE_NULL  ///< error location
	);

	///
	/// The setErrorWithMessage() functions sets error information
	/// with a customized error message.
	///
	void setErrorWithMessage (
		errnum_t      code, ///< error code
		const char_t* msg,  ///< error message
		const loc_t*  loc   ///< error location
	);

	///
	/// The clearError() function clears error information 
	///
	void clearError ();

protected:
	void retrieveError ();
	void retrieveError (Run* run);
	/// @}

protected:
	class NoSource;

public:
	/// 
	/// The Source class is an abstract class to encapsulate
	/// source script I/O. The Awk::parse function requires a concrete
	/// object instantiated from its child class.
	///
	class Source
	{
	public:
		///
		/// The Mode type defines opening mode.
		///
		enum Mode
		{	
			READ,   ///< open for read
			WRITE   ///< open for write
		};

		///
		/// The Data class encapsulates information passed in and out
		/// for source script I/O. 
		///
		class Data
		{
		public:
			friend class Awk;

		protected:
			Data (Awk* awk, Mode mode, sio_arg_t* arg): 
				awk (awk), mode (mode), arg (arg)
			{
			}

		public:
			Mode getMode() const
			{
				return mode;
			}

			const char_t* getName() const
			{
				return arg->name;
			}

			void* getHandle () const
			{
				return arg->handle;
			}

			void setHandle (void* handle)
			{
				arg->handle = handle;
			}

			operator Awk* () const
			{
				return awk;
			}

			operator awk_t* () const
			{
				return awk->awk;
			}

		protected:
			Awk* awk;
			Mode  mode;
			sio_arg_t* arg;
		};

		Source () {}
		virtual ~Source () {}

		virtual int open (Data& io) = 0;
		virtual int close (Data& io) = 0;
		virtual ssize_t read (Data& io, char_t* buf, size_t len) = 0;
		virtual ssize_t write (Data& io, const char_t* buf, size_t len) = 0;

		///
		/// The NONE object indicates no source.
		///
		static NoSource NONE;

	private:
		Source (const Source&);
		Source& operator= (const Source&);
	};

protected:
	class NoSource: public Source
	{
	public:
		int open (Data& io) { return -1; }
		int close (Data& io) { return 0; }
		ssize_t read (Data& io, char_t* buf, size_t len) { return 0; }
		ssize_t write (Data& io, const char_t* buf, size_t len) { return 0; }
	};

public:
	///
	/// The RIOBase class is a base class to represent runtime I/O 
	/// operations. The Console, File, Pipe classes implement more specific
	/// I/O operations by inheriting this class.
	///
	class RIOBase
	{
	protected:
		RIOBase (Run* run, rio_arg_t* riod);

	public:
		const char_t* getName() const;
		const void* getHandle () const;
		void  setHandle (void* handle);

		operator Awk* () const;
		operator awk_t* () const;
		operator rio_arg_t* () const;
		operator Run* () const;
		operator rtx_t* () const;

	protected:
		Run* run;
		rio_arg_t* riod;

	private:
		RIOBase (const RIOBase&);
		RIOBase& operator= (const RIOBase&);
	};

	///
	/// The Pipe class encapsulates the pipe operations indicated by
	/// the | and || operators.
	///
	class Pipe: public RIOBase
	{
	public:
		friend class Awk;

		/// The Mode type defines the opening mode.
		enum Mode
		{
			/// open for read-only access
			READ = QSE_AWK_RIO_PIPE_READ,
			/// open for write-only access
			WRITE = QSE_AWK_RIO_PIPE_WRITE,
			/// open for read and write
			RW = QSE_AWK_RIO_PIPE_RW
		};

		/// The CloseMode type defines the closing mode for a pipe
		/// opened in the #RW mode.
		enum CloseMode
		{
			/// close both read and write ends
			CLOSE_FULL = QSE_AWK_RIO_CLOSE_FULL, 
			/// close the read end only
			CLOSE_READ = QSE_AWK_RIO_CLOSE_READ,
			/// close the write end only
			CLOSE_WRITE = QSE_AWK_RIO_CLOSE_WRITE
		};

	protected:
		Pipe (Run* run, rio_arg_t* riod);

	public:
		/// The getMode() function returns the opening mode requested.
		/// You can inspect the opening mode, typically in the 
		/// openPipe() function, to create a pipe with proper 
		/// access mode. It is harmless to call this function from
		/// other pipe handling functions.
		Mode getMode () const;

		/// The getCloseMode() function returns the closing mode 
		/// requested. The returned value is valid if getMode() 
		/// returns #RW.
		CloseMode getCloseMode () const;
	};

	///
	/// The File class encapsulates file operations by inheriting RIOBase.
	///
	class File: public RIOBase
	{
	public:
		friend class Awk;

		enum Mode
		{
			READ = QSE_AWK_RIO_FILE_READ,
			WRITE = QSE_AWK_RIO_FILE_WRITE,
			APPEND = QSE_AWK_RIO_FILE_APPEND
		};

	protected:
		File (Run* run, rio_arg_t* riod);

	public:
		Mode getMode () const;
	};

	///
	/// The Console class encapsulates the console operations by 
	/// inheriting RIOBase.
	///
	class Console: public RIOBase
	{
	public:
		friend class Awk;

		enum Mode
		{
			READ = QSE_AWK_RIO_CONSOLE_READ,
			WRITE = QSE_AWK_RIO_CONSOLE_WRITE
		};

	protected:
		Console (Run* run, rio_arg_t* riod);
		~Console ();

	public:
		Mode getMode () const;
		int setFileName (const char_t* name);
		int setFNR (long_t fnr);

	protected:
		char_t* filename;
	};

	///
	/// The Value class wraps around #qse_awk_val_t to provide a more 
	/// comprehensive interface.
	///
	class Value
	{
	public:
		friend class Awk;

		// initialization
		void* operator new (size_t n, Run* run) throw ();
		void* operator new[] (size_t n, Run* run) throw ();

	#if !defined(__BORLANDC__) 
		// deletion when initialization fails
		void operator delete (void* p, Run* run);
		void operator delete[] (void* p, Run* run);
	#endif

		// normal deletion
		void operator delete (void* p);
		void operator delete[] (void* p);

		///
		/// The Index class encapsulates an index of an arrayed value.
		///
		class Index
		{
		public:
			friend class Value;

			/// The Index() function creates an empty array index.
			Index (): ptr (EMPTY_STRING), len (0) {}

			/// The Index() function creates a string array index.
			Index (const char_t* ptr, size_t len):
				ptr (ptr), len (len) {}

			const char_t* ptr;
			size_t        len;
		};

		///
		/// Represents a numeric index of an arrayed value
		///
		class IntIndex: public Index
		{
		public:
			IntIndex (long_t num);

		protected:
			// 2^32: 4294967296
			// 2^64: 18446744073709551616
			// 2^128: 340282366920938463463374607431768211456 
			// -(2^32/2): -2147483648
			// -(2^64/2): -9223372036854775808
			// -(2^128/2): -170141183460469231731687303715884105728
		#if QSE_SIZEOF_LONG_T > 16
		#	error SIZEOF(qse_long_t) TOO LARGE. 
		#	error INCREASE THE BUFFER SIZE TO SUPPORT IT.
		#elif QSE_SIZEOF_LONG_T == 16
			char_t buf[41];
		#elif QSE_SIZEOF_LONG_T == 8
			char_t buf[21];
		#else
			char_t buf[12];
		#endif
		};

		///
		/// The IndexIterator class is a helper class to make simple
		/// iteration over array elements.
		///
		class IndexIterator: public qse_awk_val_map_itr_t
		{
		public:
			friend class Value;

			///
			/// The END variable is a special variable to 
			/// represent the end of iteration.
			///
			static IndexIterator END;

			///
			/// The IndexIterator() function creates an iterator 
			/// for an arrayed value.
			///
			IndexIterator ()
			{
				this->pair = QSE_NULL;
				this->buckno = 0;
			}

		protected:
			IndexIterator (pair_t* pair, size_t buckno)
			{
				this->pair = pair;
				this->buckno = buckno;
			}

		public:
			bool operator==  (const IndexIterator& ii) const
			{
				return this->pair == ii.pair && this->buckno == ii.buckno;
			}

			bool operator!=  (const IndexIterator& ii) const
			{
				return !operator== (ii);
			}
		};

		///
		/// The Value() function creates an empty value associated
		/// with no runtime context. To set an actual inner value, 
		/// you must specify a context when calling setXXX() functions.
		/// i.e., use setInt(run,10) instead of setInt(10).
		/// 
		Value ();

		///
		/// The Value() function creates an empty value associated
		/// with a runtime context.
		///
		Value (Run& run);

		///
		/// The Value() function creates an empty value associated
		/// with a runtime context.
		///
		Value (Run* run);

		Value (const Value& v);
		~Value ();

		Value& operator= (const Value& v);

		void clear ();

		operator val_t* () const { return val; }
		operator long_t () const;
		operator real_t () const;
		operator const char_t* () const;

		val_t* toVal () const
		{
			return operator val_t* ();
		}

		long_t toInt () const
		{
			return operator long_t ();
		}

		real_t toReal () const
		{
			return operator real_t ();
		}

		const char_t* toStr (size_t* len) const
		{
			const char_t* p;
			size_t l;

			if (getStr (&p, &l) == -1) 
			{
				p = EMPTY_STRING;
				l = 0;
			}
			
			if (len != QSE_NULL) *len = l;
			return p;
		}

		int getInt (long_t* v) const;
		int getReal (real_t* v) const;
		int getStr (const char_t** str, size_t* len) const;

		int setVal (val_t* v);
		int setVal (Run* r, val_t* v);

		int setInt (long_t v);
		int setInt (Run* r, long_t v);
		int setReal (real_t v);
		int setReal (Run* r, real_t v);
		int setStr (const char_t* str, size_t len);
		int setStr (Run* r, const char_t* str, size_t len);
		int setStr (const char_t* str);
		int setStr (Run* r, const char_t* str);

		int setIndexedVal (
			const Index& idx,
			val_t*       v
		);

		int setIndexedVal (
			Run*         r, 
			const Index& idx, 
			val_t*       v
		);

		int setIndexedInt (
			const Index& idx,
			long_t       v
		);

		int setIndexedInt (
			Run* r,
			const Index& idx,
			long_t v);

		int setIndexedReal (
			const Index&  idx,
			real_t        v
		);

		int setIndexedReal (
			Run*          r,
			const Index&  idx,
			real_t        v
		);

		int setIndexedStr (
			const Index&  idx,
			const char_t* str,
			size_t        len
		);

		int setIndexedStr (
			Run*          r,
			const Index&  idx,
			const char_t* str,
			size_t        len
		);

		int setIndexedStr (
			const Index&  idx,
			const char_t* str
		);

		int setIndexedStr (
			Run*          r,
			const Index&  idx,
			const char_t* str
		);

		///
		/// The isIndexed() function determines if a value is arrayed.
		/// @return true if indexed, false if not.
		///
		bool isIndexed () const;

		/// 
		/// The getIndexed() function gets a value at the given 
		/// index @a idx and sets it to @a val.
		/// @return 0 on success, -1 on failure
		///
		int getIndexed (
			const Index&  idx, ///< array index
			Value*        val  ///< value holder
		) const;

		///
		/// The getFirstIndex() function stores the first index of
		/// an arrayed value into @a idx. 
		/// @return IndexIterator::END if the arrayed value is empty,
		///         iterator that can be passed to getNextIndex() if not
		///
		IndexIterator getFirstIndex (
			Index* idx ///< index holder
		) const;

		///
		/// The getNextIndex() function stores into @a idx the next 
		/// index of an array value from the position indicated by 
		/// @a iter.
		/// @return IndexIterator::END if the arrayed value is empty,
		///         iterator that can be passed to getNextIndex() if not
		///
		IndexIterator getNextIndex (
			Index* idx,                 ///< index holder
			const IndexIterator& curitr ///< current position
		) const;

	protected:
		Run* run;
		val_t* val;

		mutable struct
		{	
			qse_xstr_t str;
		} cached;

		static const char_t* EMPTY_STRING;
	};

public:
	///
	/// The Run class wraps around #qse_awk_rtx_t to represent the
	/// runtime context.
	///
	class Run
	{
	protected:
		friend class Awk;
		friend class Value; 
		friend class RIOBase;
		friend class Console;

		Run (Awk* awk);
		Run (Awk* awk, rtx_t* run);
		~Run ();

	public:
		operator Awk* () const;
		operator rtx_t* () const;

		void stop () const;
		bool pendingStop () const;

		errnum_t getErrorNumber () const;
		loc_t getErrorLocation () const;
		const char_t* getErrorMessage () const;

		void setError (
			errnum_t      code, 
			const cstr_t* args = QSE_NULL,
			const loc_t*  loc  = QSE_NULL
		);

		void setErrorWithMessage (
			errnum_t      code, 
			const char_t* msg,
			const loc_t*  loc
		);

		/// 
		/// The setGlobal() function sets the value of a global 
		/// variable identified by @a id
		/// to @a v.
		/// @return 0 on success, -1 on failure
		///
		int setGlobal (int id, long_t v);

		/// 
		/// The setGlobal() function sets the value of a global 
		/// variable identified by @a id
		/// to @a v.
		/// @return 0 on success, -1 on failure
		///
		int setGlobal (int id, real_t v); 

		/// 
		/// The setGlobal() function sets the value of a global 
		/// variable identified by @a id
		/// to a string as long as @a len characters pointed to by 
		/// @a ptr.
		/// @return 0 on success, -1 on failure
		///
		int setGlobal (int id, const char_t* ptr, size_t len);

		/// 
		/// The setGlobal() function sets a global variable 
		/// identified by @a id to a value @a v.
		/// @return 0 on success, -1 on failure
		///	
		int setGlobal (int id, const Value& v);

		///
		/// The getGlobal() function gets the value of a global 
		/// variable identified by @a id and stores it in @a v.
		/// @return 0 on success, -1 on failure
		///	
		int getGlobal (int id, Value& v) const;

	protected:
		Awk*   awk;
		rtx_t* rtx;
	};

	///
	/// Returns the primitive handle 
	///
	operator awk_t* () const;

	///
	/// @name Basic Functions
	/// @{
	///

	/// The Awk() function creates an interpreter without fully 
	/// initializing it. You must call open() for full initialization
	/// before calling other functions. 
	Awk (Mmgr* mmgr);

	/// The ~Awk() function destroys an interpreter. Make sure to have
	/// called close() for finalization before the destructor is executed.
	virtual ~Awk () {}

	///
	/// The open() function initializes an interpreter. 
	/// You must call this function before doing anything meaningful.
	/// @return 0 on success, -1 on failure
	///
	int open ();

	///
	/// The close() function closes the interpreter. 
	///
	void close ();

	///
	/// The parse() function parses the source code read from the input
	/// stream @a in and writes the parse tree to the output stream @a out.
	/// To disable deparsing, you may set @a out to Awk::Source::NONE. 
	/// However, it is not allowed to specify Awk::Source::NONE for @a in.
	///
	/// @return Run object on success, #QSE_NULL on failure
	///
	Awk::Run* parse (
		Source& in,  ///< script to parse 
		Source& out  ///< deparsing target 
	);

	///
	/// The getRunContext() funciton returns the execution context 
	/// returned by the parse() function. The returned context
	/// is valid if parse() has been called. You may call this 
	/// function to get the context if you forgot to store it
	/// in a call to parse().
	///
	const Awk::Run* getRunContext () const
	{
		return &runctx;
	}

	///
	/// The getRunContext() funciton returns the execution context 
	/// returned by the parse() function. The returned context
	/// is valid if parse() has been called. You may call this 
	/// function to get the context if you forgot to store it
	/// in a call to parse().
	///
	Awk::Run* getRunContext () 
	{
		return &runctx;
	}

	///
	/// The loop() function executes the BEGIN block, pattern-action blocks,
	/// and the END block. The return value is stored into @a ret.
	/// @return 0 on succes, -1 on failure
	///
	int loop (
		Value* ret  ///< return value holder
	);

	///
	/// The call() function invokes a function named @a name.
	///
	int call (
		const char_t* name,  ///< function name
		Value*        ret,   ///< return value holder
		const Value*  args,  ///< argument array
		size_t        nargs  ///< number of arguments
	);

	///
	/// The stop() function makes request to abort execution
	///
	void stop ();
	/// @}

	///
	/// @name Configuration
	/// @{
	///

	///
	/// The getOption() function gets the current options.
	/// @return 0 or current options ORed of #option_t enumerators.
	///
	int getOption () const;

	///
	/// The setOption() function changes the current options.
	///
	void setOption (
		int opt ///< options ORed of #option_t enumerators.
	);

	/// 
	/// The setMaxDepth() function sets the maximum processing depth
	/// for operations identified by @a ids.
	///
	void setMaxDepth (
		int ids,     ///< number ORed of #depth_t enumerators
		size_t depth ///< new depth
	);

	///
	/// The getMaxDepth() function gets the maximum depth for an operation
	/// type identified by @a id.
	///
	size_t getMaxDepth (
		depth_t id   ///< operation identifier
	) const;

	///
	/// The addArgument() function adds an ARGV string as long as @a len 
	/// characters pointed to 
	/// by @a arg. loop() and call() make a string added available 
	/// to a script through ARGV. 
	/// @return 0 on success, -1 on failure
	///
	int addArgument (
		const char_t* arg,  ///< string pointer
		size_t        len   ///< string length
	);

	///
	/// The addArgument() function adds a null-terminated string @a arg. 
	/// loop() and call() make a string added available to a script 
	/// through ARGV. 
	/// @return 0 on success, -1 on failure
	///
	int addArgument (
		const char_t* arg ///< string pointer
	);

	///
	/// The clearArguments() function deletes all ARGV strings.
	///
	void clearArguments ();

	///
	/// The addGlobal() function registers an intrinsic global variable. 
	/// @return integer >= 0 on success, -1 on failure.
	///
	int addGlobal (
		const char_t* name ///< variable name
	);

	///
	/// The deleteGlobal() function unregisters an intrinsic global 
	/// variable by name.
	/// @return 0 on success, -1 on failure.
	///
	int deleteGlobal (
		const char_t* name ///< variable name
	);

	///
	/// The addGlobal() function returns the numeric ID of an intrinsic 
	//  global variable. 
	/// @return integer >= 0 on success, -1 on failure.
	///
	int findGlobal (
		const char_t* name ///> variable name
	);

	///
	/// The setGlobal() function sets the value of a global variable 
	/// identified by @a id. The @a id is either a value returned by 
	/// addGlobal() or one of the #gbl_id_t enumerators. It is not allowed
	/// to call this function prior to parse().
	/// @return 0 on success, -1 on failure
	///
	int setGlobal (
		int          id,  ///< numeric identifier
		const Value& v    ///< value
	);

	///
	/// The getGlobal() function gets the value of a global variable 
	/// identified by @a id. The @a id is either a value returned by 
	/// addGlobal() or one of the #gbl_id_t enumerators. It is not allowed
	/// to call this function before parse().
	/// @return 0 on success, -1 on failure
	///
	int getGlobal (
		int    id, ///< numeric identifier 
		Value& v   ///< value store 
	);

	///
	/// The FunctionHandler type defines a intrinsic function handler.
	///
	typedef int (Awk::*FunctionHandler) (
		Run&          run,
		Value&        ret,
		const Value*  args,
		size_t        nargs, 
		const cstr_t* name
	);

	/// 
	/// The addFunction() function adds a new user-defined intrinsic 
	/// function.
	///
	int addFunction (
		const char_t* name,     ///< function name
		size_t minArgs,         ///< minimum numbers of arguments
		size_t maxArgs,         ///< maximum numbers of arguments
		FunctionHandler handler ///< function handler
	);

	///
	/// The deleteFunction() function deletes a user-defined intrinsic 
	/// function by name.
	///
	int deleteFunction (
		const char_t* name ///< function name
	);
	/// @}

	///
	/// @name Word Substitution
	/// @{
	///
	int getWord (
		const cstr_t* ow,
		cstr_t*       nw
	);

	int setWord (
		const cstr_t* ow,
		const cstr_t* nw
	);

	int unsetWord (
		const cstr_t* ow
	);

	void unsetAllWords ();
	/// @}

protected:
	/// 
	/// @name Pipe I/O handlers
	/// Pipe operations are achieved through the following functions.
	/// @{

	/// The openPipe() function is a pure virtual function that must be
	/// overridden by a child class to open a pipe. It must return 1
	/// on success, 0 on end of a pipe, and -1 on failure.
	virtual int     openPipe  (Pipe& io) = 0;

	/// The closePipe() function is a pure virtual function that must be
	/// overridden by a child class to close a pipe. It must return 0
	/// on success and -1 on failure.
	virtual int     closePipe (Pipe& io) = 0;

	virtual ssize_t readPipe  (Pipe& io, char_t* buf, size_t len) = 0;
	virtual ssize_t writePipe (Pipe& io, const char_t* buf, size_t len) = 0;
	virtual int     flushPipe (Pipe& io) = 0;
	/// @}

	/// 
	/// @name File I/O handlers
	/// File operations are achieved through the following functions.
	/// @{
	///
	virtual int     openFile  (File& io) = 0;
	virtual int     closeFile (File& io) = 0;
	virtual ssize_t readFile  (File& io, char_t* buf, size_t len) = 0;
	virtual ssize_t writeFile (File& io, const char_t* buf, size_t len) = 0;
	virtual int     flushFile (File& io) = 0;
	/// @}

	/// 
	/// @name Console I/O handlers
	/// Console operations are achieved through the following functions.
	/// @{
	///
	virtual int     openConsole  (Console& io) = 0;
	virtual int     closeConsole (Console& io) = 0;
	virtual ssize_t readConsole  (Console& io, char_t* buf, size_t len) = 0;
	virtual ssize_t writeConsole (Console& io, const char_t* buf, size_t len) = 0;
	virtual int     flushConsole (Console& io) = 0;
	virtual int     nextConsole  (Console& io) = 0;
	/// @}

	// primitive handlers 
	virtual real_t pow (real_t x, real_t y) = 0;
	virtual int    vsprintf (char_t* buf, size_t size,
	                         const char_t* fmt, va_list arg) = 0;

	// static glue members for various handlers
	static ssize_t readSource (
		awk_t* awk, sio_cmd_t cmd, sio_arg_t* arg,
		char_t* data, size_t count);
	static ssize_t writeSource (
		awk_t* awk, sio_cmd_t cmd, sio_arg_t* arg,
		char_t* data, size_t count);

	static ssize_t pipeHandler (
		rtx_t* rtx, rio_cmd_t cmd, rio_arg_t* riod,
		char_t* data, size_t count);
	static ssize_t fileHandler (
		rtx_t* rtx, rio_cmd_t cmd, rio_arg_t* riod,
		char_t* data, size_t count);
	static ssize_t consoleHandler (
		rtx_t* rtx, rio_cmd_t cmd, rio_arg_t* riod,
		char_t* data, size_t count);

	static int functionHandler (rtx_t* rtx, const cstr_t* name);

	static real_t pow     (awk_t* data, real_t x, real_t y);
	static int    sprintf (awk_t* data, char_t* buf, size_t size,
	                       const char_t* fmt, ...);

protected:
	awk_t* awk;

	errstr_t dflerrstr;
	errinf_t errinf;

	htb_t* functionMap;

	Source* sourceReader;
	Source* sourceWriter;

	struct xstrs_t
	{
		xstrs_t (): ptr (QSE_NULL), len (0), capa (0) {}

		int add (awk_t* awk, const char_t* arg, size_t len);
		void clear (awk_t* awk);

		qse_xstr_t* ptr;
		size_t      len;
		size_t      capa;
	};

	xstrs_t runarg;

private:
	Run runctx;

	int init_runctx ();
	void fini_runctx ();

	int dispatch_function (Run* run, const cstr_t* name);

	static const char_t* xerrstr (awk_t* a, errnum_t num);

private:
	Awk (const Awk&);
	Awk& operator= (const Awk&);
};

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif
