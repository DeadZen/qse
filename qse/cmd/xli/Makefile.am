AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include 

bin_PROGRAMS = qsexli

qsexli_SOURCES = xli.c
qsexli_LDFLAGS = -L../../lib/xli -L../../lib/cmn 
qsexli_LDADD = -lqsexli -lqsecmn

if WIN32 
if WCHAR
qsexli_LDADD += $(UNICOWS_LIBS)
endif
endif
