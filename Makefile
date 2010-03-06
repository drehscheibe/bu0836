FLAGS=-mtune=native -pipe -g -O0 -Wall

all: bu0836a Makefile
	./bu0836a

check: bu0836a
	cppcheck -f --enable=all .

bu0836a: options.o bu0836a.o
	g++ -g -o bu0836a options.o bu0836a.o -lusb

bu0836a.o: bu0836a.cpp options.h bu0836a.h
	g++ ${FLAGS} -I/usr/include/libusb-1.0 -c bu0836a.cpp

options.o: options.c options.h
	g++ ${FLAGS} -c options.c

clean:
	rm -f bu0836a.o options.o bu0836a core.bu0836a.*

