LDLIBS=-lm
CFLAGS=-g
braillify.o:braillify.c braillify.h

main.o:main.c braillify.h

braillify:braillify.o main.o
