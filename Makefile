all: bu0836a Makefile
	./bu0836a; read key

bu0836a: bu0836a.o
	g++ -g -o bu0836a bu0836a.o -lusb

bu0836a.o: bu0836a.cpp
	g++ -g -O0 -Wall -I/usr/include/libusb-1.0 -c bu0836a.cpp

clean:
	rm bu0836a.o bu0836a

