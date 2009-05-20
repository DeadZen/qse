CC = cl
CFLAGS = /nologo /MT /GX /W3 /GR- /D_WIN32_WINNT=0x0400 -I..\..\..
LDFLAGS = /libpath:..\..\bas /libpath:..\..\stx
LIBS = xpbas.lib xpstx.lib

all: stx parser

stx: stx.obj
	link /nologo /out:stx.exe $(LDFLAGS) $(LIBS) stx.obj

parser: parser.obj
	link /nologo /out:parser.exe $(LDFLAGS) $(LIBS) parser.obj

clean:
	del $(OBJS) *.obj stx.exe parser.exe

.SUFFIXES: .c .obj
.c.obj:
	$(CC) /c $(CFLAGS) $< 
