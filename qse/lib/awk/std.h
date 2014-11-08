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

#ifndef _QSE_LIB_AWK_STD_H
#define _QSE_LIB_AWK_STD_H_

#include "awk.h"

#ifdef __cplusplus
extern "C" {
#endif

qse_awk_flt_t qse_awk_stdmathpow (qse_awk_t* awk, qse_awk_flt_t x, qse_awk_flt_t y);
qse_awk_flt_t qse_awk_stdmathmod (qse_awk_t* awk, qse_awk_flt_t x, qse_awk_flt_t y);

#ifdef __cplusplus
}
#endif


#endif
