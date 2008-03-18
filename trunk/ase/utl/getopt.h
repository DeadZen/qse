
#ifndef _ASE_UTL_GETOPT_H_
#define _ASE_UTL_GETOPT_H_

#include <ase/cmn/types.h>
#include <ase/cmn/macros.h>

typedef struct ase_opt_t ase_opt_t;

struct ase_opt_t
{
	/* input */
	const ase_char_t* str;

	/* output */
	ase_cint_t opt;  /* character checked for validity */
	ase_char_t* arg; /* argument associated with an option */
	int err;

	/* input + output */
	int ind;         /* index into parent argv vector */
};

#ifdef __cplusplus
extern "C" {
#endif

ase_cint_t ase_getopt (int argc, ase_char_t* argv[], ase_opt_t* opt);

#ifdef __cplusplus
}
#endif

#endif
