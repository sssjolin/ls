LIBS=-lm -lbsd
OBJS=ls.o
CFLAGS=-Wall
ls:${OBJS}
	gcc -o ls ${OBJS} ${LIBS}
clean:
	rm -f ls ${OBJS}
