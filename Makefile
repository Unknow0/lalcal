all:
	$(CC) lalcal.c -O2 -o lalcal $(shell pkg-config x11 xft --libs --cflags)

debug:
	$(CC) -Wall -g3 -O0 -ggdb lalcal.c -o lalcal $(shell pkg-config x11 xft --libs --cflags)

