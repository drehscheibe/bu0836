FLAGS=-mtune=native -pipe -g -O0 -Wall

all: bu0836a Makefile
	./bu0836a; read key

bu0836a: options.o bu0836a.o
	g++ -g -o bu0836a options.o bu0836a.o -lusb

bu0836a.o: bu0836a.cpp
	g++ ${FLAGS} -I/usr/include/libusb-1.0 -c bu0836a.cpp

options.o: options.c
	g++ ${FLAGS} -c options.c

clean:
	rm -f bu0836a.o bu0836a

