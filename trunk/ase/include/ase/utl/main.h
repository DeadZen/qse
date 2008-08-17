/*
 * $Id: main.h 223 2008-06-26 06:44:41Z baconevi $
 * 
 * {License}
 */

#ifndef _ASE_UTL_MAIN_H_
#define _ASE_UTL_MAIN_H_

#include <ase/types.h>
#include <ase/macros.h>

#if defined(_WIN32)
	#include <tchar.h>
	#define ase_main _tmain
	typedef ase_char_t ase_achar_t;
#else
	#define ase_main main
	typedef ase_mchar_t ase_achar_t;
#endif


#ifdef __cplusplus
extern "C" {
#endif

int ase_runmain (int argc, ase_achar_t* argv[], int(*mf) (int,ase_char_t*[]));

#ifdef __cplusplus
}
#endif

#endif