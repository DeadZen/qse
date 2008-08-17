SRCS = \
	stx.c memory.c object.c symbol.c class.c array.c \
	dict.c misc.c name.c token.c parser.c bootstrp.c \
	bytecode.c interp.c 
OBJS = $(SRCS:.c=.obj)
OUT = xpstx.lib

CC = cl
CFLAGS = /nologo /MT /GX /W3 /GR- -I../..

all: $(OBJS)
	link -lib @<<
/nologo /out:$(OUT) $(OBJS)
<<


clean:
	del $(OBJS) $(OUT) *.obj

.SUFFIXES: .c .obj
.c.obj:
	$(CC) $(CFLAGS) /c $<
