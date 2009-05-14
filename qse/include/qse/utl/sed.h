/*
 * $Id$
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

#ifndef _QSE_UTL_SED_H_
#define _QSE_UTL_SED_H_

#include <qse/types.h>
#include <qse/macros.h>
#include <qse/cmn/str.h>
#include <qse/cmn/lda.h>
#include <qse/cmn/map.h>

enum qse_sed_errnum_t
{
	QSE_SED_ENOERR,  /* no error */
	QSE_SED_ENOMEM,  /* no memory */
	QSE_SED_ETMTXT,  /* too much text */
	QSE_SED_ECMDNR,  /* command not recognized */
	QSE_SED_ECMDMS,  /* command missing */
	QSE_SED_ECMDGB,  /* command garbled */
	QSE_SED_EREXBL,  /* regular expression build error */
	QSE_SED_EREXMA,  /* regular expression match error */
	QSE_SED_EA1PHB,  /* address 1 prohibited */
	QSE_SED_EA2PHB,  /* address 2 prohibited */
	QSE_SED_ENEWLN,  /* a new line is expected */
	QSE_SED_EBSEXP,  /* \ is expected */
	QSE_SED_EBSDEL,  /* \ used a delimiter */
	QSE_SED_EGBABS,  /* garbage after \ */
	QSE_SED_ESCEXP,  /* ; is expected */
	QSE_SED_ELABTL,  /* label too long */
	QSE_SED_ELABEM,  /* label name is empty */
	QSE_SED_ELABDU,  /* duplicate label name */
	QSE_SED_ELABNF,  /* label not found */
	QSE_SED_EFILEM,  /* file name is empty */
	QSE_SED_EFILIL,  /* illegal file name */
	QSE_SED_ENOTRM,  /* not terminated properly */
	QSE_SED_ETSNSL,  /* translation set not the same length*/
	QSE_SED_EGRNBA,  /* group brackets not balanced */
	QSE_SED_EGRNTD,  /* group nested too deeply */
	QSE_SED_EOCSDU,  /* multiple occurrence specifiers */
	QSE_SED_EOCSZE,  /* occurrence specifier to s is zero */
	QSE_SED_EOCSTL,  /* occurrence specifier too large */
	QSE_SED_EIOUSR   /* user io error */
};

enum qse_sed_option_t
{
	QSE_SED_STRIPLS  = (1 << 0),  /* strip leading spaces from text*/
	QSE_SED_KEEPTBS  = (1 << 1),  /* keep an trailing backslash */
	QSE_SED_ENSURENL = (1 << 2),  /* ensure NL at the text end */
	QSE_SED_QUIET    = (1 << 3),  /* do not print pattern space */
	QSE_SED_CLASSIC  = (1 << 4)
};

/****e* AWK/qse_sed_io_cmd_t
 * NAME
 *  qse_sed_io_cmd_t - define IO commands
 * SYNOPSIS
 */
enum qse_sed_io_cmd_t
{
	QSE_SED_IO_OPEN  = 0,
	QSE_SED_IO_CLOSE = 1,
	QSE_SED_IO_READ  = 2,
	QSE_SED_IO_WRITE = 3
};
typedef enum qse_sed_io_cmd_t qse_sed_io_cmd_t;
/******/

union qse_sed_io_arg_t
{
	struct
	{
		void*             handle; /* out */
		const qse_char_t* path;   /* in */
	} open;

	struct
	{
		void*             handle; /* in */
		qse_char_t*       buf;    /* out */
		qse_size_t        len;    /* in */
	} read;

	struct
	{
		void*             handle;  /* in */
		const qse_char_t* data;    /* in */
		qse_size_t        len;     /* in */
	} write;

	struct
	{
		void*             handle;  /* in */
	} close;
};
typedef union qse_sed_io_arg_t qse_sed_io_arg_t;

typedef struct qse_sed_t qse_sed_t;

typedef qse_ssize_t (*qse_sed_iof_t) (
        qse_sed_t*        sed,
        qse_sed_io_cmd_t  cmd,
	qse_sed_io_arg_t* arg
);

typedef struct qse_sed_cmd_t qse_sed_cmd_t; /* command */
typedef enum qse_sed_errnum_t qse_sed_errnum_t;

struct qse_sed_t
{
	QSE_DEFINE_COMMON_FIELDS (sed)
	qse_sed_errnum_t errnum;
	int option;

	/* source code pointers */
	struct
	{
		const qse_char_t* ptr;
		const qse_char_t* end;
		const qse_char_t* cur;
	} src;

	void* lastrex;
	qse_str_t rexbuf; /* temporary regular expression buffer */

	/* command array */
	/*qse_lda_t cmds;*/
	struct
	{
		qse_sed_cmd_t* buf;
		qse_sed_cmd_t* end;
		qse_sed_cmd_t* cur;
	} cmd;

	qse_map_t labs; /* label map */

	/* current level of command group nesting */
	int grplvl;
	/* temporary storage to keep track of the begining of a command group */
	qse_sed_cmd_t* grpcmd[128];

	/* io data for execution */
	struct
	{
		struct
		{
			qse_sed_iof_t f;
			qse_sed_io_arg_t arg;

			qse_char_t buf[2048];
			qse_size_t len;
			int        eof;

			qse_map_t files;
		} out;

		struct
		{
			qse_sed_iof_t f;
			qse_sed_io_arg_t arg;

			qse_char_t xbuf[1];
			int xbuf_len;

			qse_char_t buf[2048];
			qse_size_t len;
			qse_size_t pos;
			int        eof;

			qse_str_t line;
			qse_size_t num;
		} in;

	} eio;

	struct
	{
		qse_lda_t appended;
		qse_str_t held;
	} text;
};


#ifdef __cplusplus
extern "C" {
#endif

QSE_DEFINE_COMMON_FUNCTIONS (sed)

/****f* Text Processor/qse_sed_open
 * NAME
 *  qse_sed_open - create a stream editor
 * SYNOPSIS
 */
qse_sed_t* qse_sed_open (
	qse_mmgr_t*    mmgr,
	qse_size_t     xtn
);
/******/

/****f* Text Processor/qse_sed_close
 * NAME
 *  qse_sed_close - destroy a stream editor
 * SYNOPSIS
 */
void qse_sed_close (
	qse_sed_t* sed
);
/******/

/****f* Text Processor/qse_sed_init
 * NAME
 *  qse_sed_init - initialize a stream editor
 * SYNOPSIS
 */
qse_sed_t* qse_sed_init (
	qse_sed_t*     sed,
	qse_mmgr_t*    mmgr
);
/******/

/****f* Text Processor/qse_sed_fini
 * NAME
 *  qse_sed_fini - finalize a stream editor
 * SYNOPSIS
 */
void qse_sed_fini (
	qse_sed_t* sed
);
/******/

/****f* Text Processor/qse_sed_getoption
 * NAME
 *  qse_sed_getoption - get option
 * SYNOPSIS
 */
int qse_sed_getoption (
	qse_sed_t* sed
);
/******/

/****f* Text Processor/qse_sed_setoption
 * NAME
 *  qse_sed_setoption - set option
 * SYNOPSIS
 */
void qse_sed_setoption (
	qse_sed_t* sed,
	int        option
);
/*****/

/****f* Text Processor/qse_sed_geterrmsg
 * NAME
 *  qse_sed_geterrmsg - get an error message
 * SYNOPSIS
 */
const qse_char_t* qse_sed_geterrmsg (
	qse_sed_t* sed
);
/******/

int qse_sed_compile (
	qse_sed_t*        sed,
	const qse_char_t* sptr,
	qse_size_t        slen
);

int qse_sed_execute (
	qse_sed_t*    sed,
	qse_sed_iof_t inf,
	qse_sed_iof_t outf
);

#ifdef __cplusplus
}
#endif

#endif
