CC=${HOME}/x-tools/x86_64-pc-linux-musl/bin/x86_64-pc-linux-musl-gcc
STRIP=${HOME}/x-tools/x86_64-pc-linux-musl/bin/x86_64-pc-linux-musl-strip

MINGCC=x86_64-w64-mingw32-gcc
MINGSTRIP=x86_64-w64-mingw32-strip

CFLAGS=-static -Os -Wall -Werror -pedantic

all: dumpbytes dumpbytes.exe

clean:
	rm -f dumpbytes dumpbytes.exe

dumpbytes: dumpbytes.c parg.c parg.h
	${CC} -o $@ ${CFLAGS} $^
	${STRIP} -s $@

dumpbytes.exe: dumpbytes.c parg.c parg.h
	${MINGCC} -o $@ -D__USE_MINGW_ANSI_STDIO ${CFLAGS} $^
	${MINGSTRIP} -s $@

.PHONY: all clean

