AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include \
	$(LTDLINCL)

if WIN32
# you must adjust the value of DEFAULT_MODPOSTFIX according to
# -version-info in ../../lib/awk/Makefile.am
AM_CPPFLAGS += -DDEFAULT_MODPREFIX=\"libqseawk-\" -DDEFAULT_MODPOSTFIX=\"-1\"
else
AM_CPPFLAGS += -DDEFAULT_MODPREFIX=\"$(libdir)/libqseawk-\" -DDEFAULT_MODPOSTFIX=\"\"
endif

#####################################################################3

bin_PROGRAMS = qseawk

qseawk_SOURCES = awk.c
qseawk_LDFLAGS = -L../../lib/awk -L../../lib/cmn 
qseawk_LDADD = -lqseawk -lqsecmn $(LIBM) $(LIBLTDL) 

if WIN32
if WCHAR
qseawk_LDADD += $(UNICOWS_LIBS)
endif
endif

if ENABLE_STATIC_MODULE
qseawk_LDFLAGS += -L../../lib/sed
qseawk_LDADD += -lqsesed
endif
