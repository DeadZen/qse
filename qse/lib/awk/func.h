/*
 * $Id: func.h 363 2008-09-04 10:58:08Z baconevi $
 *
   Copyright 2006-2008 Chung, Hyung-Hwan.

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

#ifndef _QSE_LIB_AWK_FUNC_H_
#define _QSE_LIB_AWK_FUNC_H_

typedef struct qse_awk_bfn_t qse_awk_bfn_t;

struct qse_awk_bfn_t
{
	struct
	{
		qse_char_t* ptr;
		qse_size_t  len;
	} name;

	int valid; /* the entry is valid when this option is set */

	struct
	{
		qse_size_t min;
		qse_size_t max;
		qse_char_t* spec;
	} arg;

	int (*handler) (qse_awk_run_t*, const qse_char_t*, qse_size_t);

	/*qse_awk_bfn_t* next;*/
};

#ifdef __cplusplus
extern "C" {
#endif

qse_awk_bfn_t* qse_awk_getbfn (
	qse_awk_t* awk, const qse_char_t* name, qse_size_t len);

#ifdef __cplusplus
}
#endif

#endif
