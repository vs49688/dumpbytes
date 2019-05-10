all: dumpbytes

clean:
	rm -f dumpbytes

dumpbytes: dumpbytes.c parg.c parg.h
	gcc -o $@ -Os -Wall -Werror $^
	strip -s $@

.PHONY: all clean

