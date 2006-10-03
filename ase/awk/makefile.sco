SRCS = awk.c err.c tree.c str.c tab.c map.c parse.c \
	run.c rec.c val.c func.c misc.c extio.c
OBJS = $(SRCS:.c=.o)
OUT = libxpawk.a

CC = cc
CFLAGS = -Xc -a ansi -w3 -O2 -I../.. -DXP_AWK_STAND_ALONE

all: $(OBJS)
	ar cr $(OUT) $(OBJS)

clean:
	rm -rf $(OBJS) $(OUT) *.obj

.SUFFIXES: .c .o
.c.o:
	$(CC) $(CFLAGS) -c $<



