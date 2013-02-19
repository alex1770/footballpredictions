all:: s

CFLAGS:= -Wall -funroll-loops -O9
LFLAGS:= -lm

s: Makefile s.c
	gcc -o s s.c $(CFLAGS) $(LFLAGS)

.PHONY : clean
clean:
	rm -f s
