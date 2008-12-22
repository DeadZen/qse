/*
 * $Id$
 */

#ifndef _QSE_CMN_TIME_H_
#define _QSE_CMN_TIME_H_

#include <qse/types.h>
#include <qse/macros.h>

#define QSE_EPOCH_YEAR  (1970)
#define QSE_EPOCH_MON   (1)
#define QSE_EPOCH_DAY   (1)
#define QSE_EPOCH_WDAY  (4)

#define QSE_BTIME_YEAR_BASE (1900)

#define QSE_DAYS_PER_WEEK  (7)
#define QSE_MONS_PER_YEAR  (12)
#define QSE_HOURS_PER_DAY  (24)
#define QSE_MINS_PER_HOUR  (60)
#define QSE_MINS_PER_DAY   (QSE_MINS_PER_HOUR*QSE_HOURS_PER_DAY)
#define QSE_SECS_PER_MIN   (60)
#define QSE_SECS_PER_HOUR  (QSE_SECS_PER_MIN*QSE_MINS_PER_HOUR)
#define QSE_SECS_PER_DAY   (QSE_SECS_PER_MIN*QSE_MINS_PER_DAY)
#define QSE_MSECS_PER_SEC  (1000)
#define QSE_MSECS_PER_MIN  (QSE_MSECS_PER_SEC*QSE_SECS_PER_MIN)
#define QSE_MSECS_PER_HOUR (QSE_MSECS_PER_SEC*QSE_SECS_PER_HOUR)
#define QSE_MSECS_PER_DAY  (QSE_MSECS_PER_SEC*QSE_SECS_PER_DAY)

#define QSE_USECS_PER_MSEC (1000)
#define QSE_NSECS_PER_USEC (1000)
#define QSE_USECS_PER_SEC  (QSE_USECS_PER_MSEC*QSE_MSECS_PER_SEC)

#define QSE_IS_LEAPYEAR(year) (!((year)%4) && (((year)%100) || !((year)%400)))
#define QSE_DAYS_PER_YEAR(year) (QSE_IS_LEAPYEAR(year)? 366: 365)

/* number of milliseconds since the Epoch (00:00:00 UTC, Jan 1, 1970) */
typedef qse_long_t qse_ntime_t;
typedef struct qse_btime_t qse_btime_t;

struct qse_btime_t
{
	int msec;
	int sec;  /* 0-61 */
	int min;  /* 0-59 */
	int hour; /* 0-23 */
	int mday; /* 1-31 */
	int mon;  /* 0(jan)-11(dec) */
	int year; /* the number of years since 1900 */
	int wday; /* 0(sun)-6(sat) */
	int yday; /* 0(jan 1) to 365 */
	int isdst;
	int offset;
};

#ifdef __cplusplus
extern "C" {
#endif

/****f* qse.cmn/qse_gettime
 * NAME
 *  qse_gettime - get the current time
 *
 * SYNPOSIS
 */
int qse_gettime (
	qse_ntime_t* nt
);
/******/

/****f* qse.cmn/qse_settime
 * NAME
 *  qse_settime - set the current time
 *
 * SYNOPSIS
 */
int qse_settime (
	qse_ntime_t nt
);
/******/


/****f* qse.cmn/qse_gmtime
 * NAME
 *  qse_gmtime - convert numeric time to broken-down time 
 *
 * SYNOPSIS
 */
void qse_gmtime (
	qse_ntime_t  nt, 
	qse_btime_t* bt
);
/******/

/****f* qse.cmn/qse_localtime
 * NAME
 *  qse_localtime - convert numeric time to broken-down time 
 *
 * SYNOPSIS
 */
int qse_localtime (
	qse_ntime_t  nt, 
	qse_btime_t* bt
); 
/******/

/****f* qse.cmn/qse_mktime
 * NAME
 *  qse_mktime - convert broken-down time to numeric time
 *
 * SYNOPSIS
 */
int qse_mktime (
	const qse_btime_t* bt,
	qse_ntime_t*       nt
);
/******/

/****f* qse.cmn/qse_strftime
 * NAME
 *  qse_strftime - format time
 *
 * SYNOPSIS
 */
qse_size_t qse_strftime (
        qse_char_t*       buf, 
	qse_size_t        size, 
	const qse_char_t* fmt,
	qse_btime_t*      bt
);
/******/

#ifdef __cplusplus
}
#endif

#endif
