all:
	$(CC) lalcal.c calendar.c -O2 -o lalcal $(shell pkg-config x11 xft --libs --cflags)

debug:
	$(CC) -Wall -g3 -O0 -ggdb lalcal.c calendar.c -o lalcal $(shell pkg-config x11 xft --libs --cflags)

