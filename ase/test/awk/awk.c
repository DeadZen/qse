#include <xp/awk/awk.h>
#include <xp/bas/stdio.h>
#include <xp/bas/sio.h>

static xp_ssize_t process_source (int cmd, void* arg, xp_char_t* data, xp_size_t size)
{
	xp_ssize_t n;

	switch (cmd) {
	case XP_AWK_IO_OPEN:
	case XP_AWK_IO_CLOSE:
		return 0;

	case XP_AWK_IO_DATA:
		if (size < 0) return -1;
		n = xp_sio_getc (xp_sio_in, data);
		if (n == 0) return 0;
		if (n != 1) return -1;
		return n;
	}

	return -1;
}

int xp_main (int argc, xp_char_t* argv[])
{
	xp_awk_t awk;

#if 0
	if (argc != 2) {
		xp_fprintf (xp_stderr, XP_TEXT("Usage: %s file\n"), argv[0]);
		return -1;
	}
#endif

	if (xp_awk_open(&awk) == XP_NULL) {
		xp_fprintf (xp_stderr, XP_TEXT("Error: cannot open awk\n"));
		return -1;
	}

	if (xp_awk_attach_source(&awk, process_source, XP_NULL) == -1) {
		xp_awk_close (&awk);
		xp_fprintf (xp_stderr, XP_TEXT("error: cannot attach source\n"));
		return -1;
	}

	if (xp_awk_parse(&awk) == -1) {
		xp_awk_close (&awk);
		xp_fprintf (xp_stderr, XP_TEXT("error: cannot parse program\n"));
		return -1;
	}

	xp_awk_close (&awk);
	return 0;
}
